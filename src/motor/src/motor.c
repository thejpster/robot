/*****************************************************
*
* Pi Wars Robot Software (PWRS) LCD Driver
*
* Copyright (c) 2013-2017 theJPster (www.thejpster.org.uk)
*
* PWRS is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* PWRS is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with PWRS.  If not, see <http://www.gnu.org/licenses/>.
*
* This module is the interface to the Realtime Motor Controller.
* as implemented on the Arduino. The motor controller
* performs closed loop feedback to dial out variations
* in motor speed between left and right, as well as variations
* in battery voltage.
*
*****************************************************/

/**************************************************
* Includes
***************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "util/util.h"
#include "../motor.h"

/**************************************************
* Defines
***************************************************/

/*
Messages look like this:

COMMAND DATA_LEN <DATA> CRC

Message are then SLIP encoded for transmission over the UART.

Frame Start/End: MESSAGE_HEADER
MESSAGE_HEADER => MESSAGE_ESC MESSAGE_ESC_HEADER
MESSAGE_ESC    => MESSAGE_ESC MESSAGE_ESC_ESC
*/

#define BAUDRATE B115200

#define MESSAGE_HEADER             0xC0
#define MESSAGE_ESC                0xDB
#define MESSAGE_ESC_HEADER         0xDC
#define MESSAGE_ESC_ESC            0xDD

#define MAX_MESSAGE_LEN 254

#define MICROSECONDS_PER_CM 29.154519

// #define VERBOSE

#ifdef VERBOSE
#define printf_verbose printf
#else
#define printf_verbose(...)
#endif

/**************************************************
* Data Types
**************************************************/

typedef enum motor_command_t
{
    MESSAGE_COMMAND_SPEED_REQ,
    MESSAGE_COMMAND_SPEED_IND,
    MESSAGE_COMMAND_CURRENT_OVERFLOW_IND,
    MESSAGE_COMMAND_CURRENT_IND,
    MESSAGE_COMMAND_RANGE_IND,
    MAX_VALID_COMMAND
} motor_command_t;

typedef struct message_speed_req_t
{
    uint32_t ctx;
    uint8_t side; // 0 = left, 1 = right
    uint8_t clicks; // max clicks to travel
    int16_t speed; // clicks per second
} message_speed_req_t;

typedef struct message_speed_ind_t
{
    uint16_t speed;
    uint8_t motor;
} message_speed_ind_t;

typedef struct message_range_ind_t
{
    uint16_t range;
    uint8_t sensor;
} message_range_ind_t;

typedef struct message_current_ind_t
{
    uint16_t current;
    uint8_t motor;
} message_current_ind_t;

typedef struct motor_settings_t
{
    bool forward;
    unsigned int speed;
    unsigned int tick_count;
    unsigned int last_ticks_remaining;
} motor_settings_t;

typedef struct message_t
{
    motor_command_t command;
    size_t data_len;
    size_t data_read;
    uint8_t data[MAX_MESSAGE_LEN];
} message_t;

typedef enum read_state_t
{
    READ_STATE_IDLE,
    READ_STATE_COMMAND,
    READ_STATE_LEN,
    READ_STATE_DATA,
    READ_STATE_CHECKSUM,
} read_state_t;

/**************************************************
* Function Prototypes
**************************************************/

static uint8_t calc_checksum(const message_t* p_message);
static void process_rx_message(const message_t* p_message);
static void process_rx_byte(uint8_t byte);
static void send_message(motor_command_t command, size_t data_len, const uint8_t* p_data);
static void write_esc(int fd, uint8_t data);

#ifdef VERBOSE
static uint32_t get_ts(void);
#endif

/**************************************************
* Public Data
**************************************************/

/* None */

/**************************************************
* Private Data
**************************************************/

static int fd = -1;

static message_t rx_message = { 0 };

static read_state_t read_state = READ_STATE_IDLE;

static float currents[4] = { 0 };

static double range_cm[3] = { 10, 10, 10 };

/**************************************************
* Public Functions
***************************************************/

/**
 * Initialise the motor controller interface.
 *
 * Opening the USB serial port of an Arduino
 * causes it to reset. This stops the motors
 * and puts us in a known state. It is OK to call
 * this function multiple times without an explcit
 * motor_close() call in between; it is called implicitly.
 *
 * @return An error code
 */
enum motor_status_t motor_init(
    const char *sz_serial_port
)
{
    struct termios newtio;
    fd = open(sz_serial_port, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(sz_serial_port);
        return MOTOR_STATUS_NO_DEVICE;
    }

    memset(&newtio, 0, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
    newtio.c_cc[VTIME] = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN] = 0;    /* don't block */
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &newtio);

    return MOTOR_STATUS_OK;
}

/**
 * Disable the motor controller interface.
 *
 * Closes the serial port. Calling motor_init() when it is
 * already initialised implicity calls this function first.
 *
 */
void motor_close(void)
{
    if (fd >= 0)
    {
        close(fd);
        fd = -1;
    }
}

/**
 * Control the motor(s)
 *
 * A speed of zero stops the motor. Positive speeds move
 * the robot forwards, negative backwards. You must also place
 * an upper bound on the number of steps the motor can rotate
 * though before it will come to a halt. This is
 * useful to a) guard against the Pi crashing (the robot will
 * automatically come to a stop after a short while) or b)
 * dead reckoning your way around a course.
 *
 * A 'step' is one transition on the quadrature encoder. 320
 * steps is around one turn of the robot's wheel, or around
 * 180 mm.
 *
 * A call to this function supercedes any previous commands
 * received for the specified motor(s). i.e. if you ask
 * for 100 steps at maximum speed on both motors, and then
 * full stop on the left motor, the right motor will carry on
 * until the 100 steps are complete.
 *
 * @param[in] motor Which motor to set.
 * @param[in] speed The motor speed. +ve is forwards, -ve is reverse.
 * @param[in] steps The number of steps to take before stopping.
 * @return An error code
 */
enum motor_status_t motor_control(
    motor_t motor,
    motor_speed_t speed
)
{
    static uint32_t last_ctx = 0;
    // printf("In motor_control(motor=%u, speed=%d, steps=%u)\n", motor, speed, step_count);
    if (speed > MOTOR_MAX_SPEED)
    {
        speed = MOTOR_MAX_SPEED;
    }
    if (speed < -MOTOR_MAX_SPEED)
    {
        speed = -MOTOR_MAX_SPEED;
    }
    if (motor == MOTOR_BOTH)
    {
        message_speed_req_t req = {
                .ctx = last_ctx++,
                .side = 0,
                .clicks = 0,
                .speed = speed
        };
        send_message(MESSAGE_COMMAND_SPEED_REQ, sizeof(req), (const uint8_t*) &req);
        req.side = 1;
        req.ctx = last_ctx++;
        send_message(MESSAGE_COMMAND_SPEED_REQ, sizeof(req), (const uint8_t*) &req);
    } else {
        message_speed_req_t req = {
                .ctx = last_ctx++,
                .side = (motor == MOTOR_LEFT) ? 0 : 1,
                .clicks = 0,
                .speed = speed
        };
        send_message(MESSAGE_COMMAND_SPEED_REQ, sizeof(req), (const uint8_t*) &req);
    }
    return MOTOR_STATUS_OK;
}

/**
 * Check the motor controller serial port for incoming messages,
 * dispatching them to the handler when complete.
 *
 * @return An error code
 */
motor_status_t motor_poll(void)
{
    motor_status_t result = MOTOR_STATUS_OK;
    uint8_t message_buffer[256];
    if (fd >= 0)
    {
        ssize_t read_result = read(fd, message_buffer, sizeof(message_buffer));
        if (read_result > 0)
        {
            //printf("Read %zu from serial port\n", read_result);
            bool is_escape = false;
            for (ssize_t index = 0; index < read_result; index++)
            {
                const uint8_t data = message_buffer[index];
                // printf("%02x ", data);
                if (is_escape)
                {
                    if (data == MESSAGE_ESC_HEADER)
                    {
                        // Escaped header => process normally
                        process_rx_byte(MESSAGE_HEADER);
                    }
                    else if (data == MESSAGE_ESC_ESC)
                    {
                        process_rx_byte(MESSAGE_ESC);
                    }
                    else
                    {
                        printf("Bad escape 0x%02x\n", data);
                    }
                    is_escape = false;
                }
                else if (data == MESSAGE_ESC)
                {
                    is_escape = true;
                }
                else if (data == MESSAGE_HEADER)
                {
                    // Unescaped header => start of message
                    read_state = READ_STATE_COMMAND;
                }
                else
                {
                    process_rx_byte(data);
                }
            }
            // printf("\n");
        }
        else if (read_result < 0)
        {
            printf("Error reading serial port! %zd\n", read_result);
            result = MOTOR_STATUS_SERIAL_ERROR;
        }
    }
    else
    {
        result = MOTOR_STATUS_NO_DEVICE;
    }
    return result;
}

/**
 * Find out how much current a motor is using.
 *
 * @param[in] motor Which motor to read
 * @return The amount of current in Amps.
 */
float motor_current(
    uint8_t channel
)
{
    if (channel < NUMELTS(currents)) {
        return currents[channel];
    } else {
        return 0;
    }
}

/**
 * Read the latest ultrasound measurement.
 *
 * @return a distance in cm to the nearest object
 */
double motor_read_distance(
    uint8_t sensor
)
{
    if (sensor < NUMELTS(range_cm)) {
        return range_cm[sensor];
    } else {
        return 0;
    }
}

/**************************************************
* Private Functions
***************************************************/

/**
 * Calculate the message checksum.
 *
 * XORs the all the bytes in the given message.
 *
 * @return the calculated checksum
 */
static uint8_t calc_checksum(const message_t* p_message)
{
    uint8_t result = 0xFF;
    result ^= (uint8_t) p_message->command;
    result ^= (uint8_t) p_message->data_len;
    for (size_t i = 0; i < p_message->data_len; i++)
    {
        result ^= p_message->data[i];
    }
    return result;
}

/**
 * Process the message from the controller. Currently just
 * does some logging. Checksums have already been verified at
 * this stage.
 *
 * @param[in] p_message The received message
 */
static void process_rx_message(const message_t* p_message)
{
    // printf("RX %02x: ", p_message->command);
    // for (size_t i = 0; i < p_message->data_len; i++)
    // {
    //     printf("%02x ", p_message->data[i]);
    // }
    // printf("\n");

    switch (p_message->command) {
    case MESSAGE_COMMAND_SPEED_IND:
        {
            // printf("Speed ind %zu bytes\n", p_message->data_len);
            if (p_message->data_len == 3)
            {
#ifdef VERBOSE
                message_speed_ind_t ind = { 0 };
                ind.speed = p_message->data[0] | (p_message->data[1] << 8);
                ind.motor = p_message->data[2];
                printf_verbose("%u: Speed ind motor %u, speed %u\n", get_ts(), ind.motor, ind.speed);
#endif
            }
        }
        break;
    case MESSAGE_COMMAND_CURRENT_OVERFLOW_IND:
        printf("Overflow ind %zu bytes\n", p_message->data_len);
        abort();
        break;
    case MESSAGE_COMMAND_CURRENT_IND:
        {
            if (p_message->data_len == 3)
            {
                message_current_ind_t ind = { 0 };
                ind.current = p_message->data[0] | (p_message->data[1] << 8);
                ind.motor = p_message->data[2];
                printf_verbose("%u: Current ind motor %u, current %f mA (%u)\n", get_ts(), ind.motor, ind.current * 4.9f, ind.current);
                currents[ind.motor] = (ind.current * 4.9f) / 1000.0f;
            }
        }
        break;
    case MESSAGE_COMMAND_RANGE_IND:
        {
            // printf("Range ind %zu bytes\n", p_message->data_len);
            if (p_message->data_len == 3)
            {
                message_range_ind_t ind = { 0 };
                ind.range = p_message->data[0] | (p_message->data[1] << 8);
                ind.sensor = p_message->data[2];
                double range = ind.range;
                range = range / MICROSECONDS_PER_CM;
                // There and back
                range = range / 2;
                range_cm[ind.sensor] = range;
                printf_verbose("%u: Range ind sensor %u, range %f cm / %u µs\n", get_ts(), ind.sensor, range, ind.range);
            }
        }
        break;
    default:
        printf("Unknown command 0x%02x\n", p_message->command);
    }
}

/**
 * Feed incoming bytes through the state machine.
 * Will call process_rx_message() when a valid message
 * has been received.
 *
 * @param[in] byte The received byte
 */
static void process_rx_byte(uint8_t byte)
{
    switch (read_state)
    {
    case READ_STATE_IDLE:
        break;
    case READ_STATE_COMMAND:
        if (byte < MAX_VALID_COMMAND)
        {
            rx_message.command = (motor_command_t) byte;
            read_state = READ_STATE_LEN;
        } else {
            read_state = READ_STATE_IDLE;
        }
        break;
    case READ_STATE_LEN:
        rx_message.data_read = 0;
        rx_message.data_len = byte;
        read_state = rx_message.data_len ? READ_STATE_DATA : READ_STATE_CHECKSUM;
        break;
    case READ_STATE_DATA:
        rx_message.data[rx_message.data_read++] = byte;
        if (rx_message.data_read == rx_message.data_len)
        {
            read_state = READ_STATE_CHECKSUM;
        }
        break;
    case READ_STATE_CHECKSUM:
        if (byte == calc_checksum(&rx_message))
        {
            process_rx_message(&rx_message);
        }
        else
        {
            printf("Dropping bad packet\n");
        }
        read_state = READ_STATE_IDLE;
        break;
    }
}


/**
 * Write a message to the UART.
 *
 * @param command[in] The command to send
 * @param data_len[in] The number of bytes in p_data
 * @param p_data[in] The data for the command
 */
static void send_message(motor_command_t command, size_t data_len, const uint8_t* p_data)
{
    uint8_t csum = 0xFF;

    if (fd < 0)
    {
        return;
    }

    if (data_len >= 256)
    {
        return;
    }

    //printf("TX %02x: ", command);
    //for (size_t i = 0; i < data_len; i++)
    //{
    //    printf("%02x ", p_data[i]);
    //}
    //printf("\n");

    uint8_t temp = MESSAGE_HEADER;
    write(fd, &temp, 1);

    csum ^= (uint8_t) command;
    write_esc(fd, (uint8_t) command);

    csum ^= (uint8_t) data_len;
    write_esc(fd, (uint8_t) data_len);

    for (size_t i = 0; i < data_len; i++)
    {
        write_esc(fd, p_data[i]);
        csum ^= (uint8_t) p_data[i];
    }

    write_esc(fd, csum);
}

/**
 * Write a SLIP-encoded byte to the given file descriptor.
 *
 * @param fd[in] the open file descriptor
 * @param data[in] the byte to write
 */
static void write_esc(int fd, uint8_t data)
{
    if (data == MESSAGE_ESC)
    {
        uint8_t data[] = { MESSAGE_ESC, MESSAGE_ESC_ESC };
        write(fd, data, sizeof(data));
    }
    else if (data == MESSAGE_HEADER)
    {
        uint8_t data[] = { MESSAGE_ESC, MESSAGE_ESC_HEADER };
        write(fd, data, sizeof(data));
    }
    else
    {
        write(fd, &data, 1);
    }
}

#ifdef VERBOSE
static uint32_t get_ts(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint32_t ms = tv.tv_sec * 1000;
    ms += tv.tv_usec / 1000;
    return ms;
}
#endif

/**************************************************
* End of file
***************************************************/

