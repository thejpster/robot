/*****************************************************
*
* Pi Wars Robot Software (PWRS) LCD Driver
*
* Copyright (c) 2013-2014 theJPster (www.thejpster.org.uk)
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
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "util/util.h"
#include "../motor.h"

/**************************************************
* Defines
***************************************************/

#define BAUDRATE B115200

#define MESSAGE_HEADER             0xFF
#define MESSAGE_CONTROL_LHS_FWD    0x00
#define MESSAGE_CONTROL_LHS_BACK   0x01
#define MESSAGE_CONTROL_RHS_FWD    0x02
#define MESSAGE_CONTROL_RHS_BACK   0x03
#define MESSAGE_CONTROL_ACK        0x04
#define MESSAGE_CONTROL_STALL      0x05
#define MESSAGE_CONTROL_RHS_CLICKS 0x06
#define MESSAGE_CONTROL_LHS_CLICKS 0x07

#define MESSAGE_HEADER_OFFSET      0
#define MESSAGE_CONTROL_OFFSET     1
#define MESSAGE_SPEED_OFFSET       2
#define MESSAGE_COUNT_OFFSET       3
#define MESSAGE_CHECKSUM_OFFSET    4
#define MESSAGE_LEN                5

/**************************************************
* Data Types
**************************************************/

typedef struct motor_settings_t
{
    bool forward;
    unsigned int speed;
    unsigned int tick_count;
} motor_settings_t;

typedef struct rx_message_t
{
    uint8_t command;
    unsigned int ticks_remaining;
} rx_message_t;

typedef enum read_state_t
{
    READ_STATE_HEADER,
    READ_STATE_COMMAND,
    READ_STATE_CLICKS,
    READ_STATE_CHECKSUM
} read_state_t;

/**************************************************
* Function Prototypes
**************************************************/

static void build_message(
    motor_t motor,
    const motor_settings_t *p_settings,
    uint8_t *p_message
);
static uint8_t calc_checksum(const uint8_t *p_message);

static void set_motor(int speed, unsigned int tick_count, motor_settings_t *p_motor);

static enum motor_status_t send_message(const uint8_t *p_message);

static void process_rx_message(const rx_message_t* p_message);

static void process_rx_byte(uint8_t byte);

/**************************************************
* Public Data
**************************************************/

/* None */

/**************************************************
* Private Data
**************************************************/

static int fd = -1;

static motor_settings_t left =
{
    .forward = true,
    .speed = 0
};

static motor_settings_t right =
{
    .forward = true,
    .speed = 0
};

static rx_message_t rx_message = { 0 };

static read_state_t read_state = READ_STATE_HEADER;

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
    motor_speed_t speed,
    motor_step_count_t step_count
)
{
    enum motor_status_t result;
    motor_settings_t temp;
    uint8_t message[MESSAGE_LEN];

    printf("In motor_control(motor=%u, speed=%d, steps=%u)\r\n", motor, speed, step_count);

    /* Allow 1 second of running */
    set_motor(speed, abs(speed), &temp);

    if ((motor == MOTOR_LEFT) || (motor == MOTOR_BOTH))
    {
        left = temp;
        printf("Set L=%c%u\r\n",
               left.forward ? '+' : '-',
               left.speed
              );
        build_message(MOTOR_LEFT, &left, message);
        result = send_message(message);
    }
    if ((motor == MOTOR_RIGHT) || (motor == MOTOR_BOTH))
    {
        right = temp;
        printf("Set R=%c%u\r\n",
               right.forward ? '+' : '-',
               right.speed
              );
        build_message(MOTOR_RIGHT, &right, message);
        result = send_message(message);
    }
    else
    {
        printf("Bad motor!\r\n");
    }

    return result;
}

/**
 * Check the motor controller serial port for ACKs and
 * tick count updates.
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
            printf("Read %zu from serial port\r\n", read_result);
            size_t index = 0;
            while(read_result)
            {
                process_rx_byte(message_buffer[index]);
                index++;
                read_result--;
            }
        }
        else if (read_result < 0)
        {
            printf("Error reading serial port! %zd\r\n", read_result);
            result = MOTOR_STATUS_SERIAL_ERROR;
        }
    }
    else
    {
        result = MOTOR_STATUS_NO_DEVICE;
    }
    return result;
}

/**************************************************
* Private Functions
***************************************************/

/**
 * Build a message to go over the serial connection.
 *
 * @param[in] motor MOTOR_LEFT or MOTOR_RIGHT
 * @param[in] p_motor Settings for the motor
 * @param[out] p_message The constructed message
 */
static void build_message(
    motor_t motor,
    const motor_settings_t *p_motor,
    uint8_t *p_message
)
{
    uint8_t control = 0;
    p_message[MESSAGE_HEADER_OFFSET] = MESSAGE_HEADER;
    if (motor == MOTOR_LEFT)
    {
        control = p_motor->forward ? MESSAGE_CONTROL_LHS_FWD : MESSAGE_CONTROL_LHS_BACK;
    }
    else if (motor == MOTOR_RIGHT)
    {
        control = p_motor->forward ? MESSAGE_CONTROL_RHS_FWD : MESSAGE_CONTROL_RHS_BACK;
    }
    else
    {
        fprintf(stderr, "Invalid motor in build_message\r\n");
        abort();
    }

    p_message[MESSAGE_CONTROL_OFFSET] = control;
    /* Speed is 0..320, so we send 0..160 */
    p_message[MESSAGE_SPEED_OFFSET] = (p_motor->speed >> 1);
    /* Tick count is 0..320, so we send 0..160 */
    p_message[MESSAGE_COUNT_OFFSET] = (p_motor->tick_count >> 1);
    p_message[MESSAGE_CHECKSUM_OFFSET] = calc_checksum(p_message);
}

/**
 * Calculate the message checksum.
 *
 * XORs the all the bytes bar the last one.
 *
 * @return the calculated checksum
 */
static uint8_t calc_checksum(const uint8_t *p_message)
{
    uint8_t result = 0;
    for (size_t i = 0; i < (MESSAGE_LEN - 1); i++)
    {
        result = result ^ p_message[i];
    }
    /* Don't allow checksum to match header byte */
    if (result == MESSAGE_HEADER)
    {
        result = ~result;
    }
    return result;
}

/**
 * Converts a signed integer speed
 * into the relevant control bits.
 *
 * @param[in] speed -MOTOR_MAX_SPEED..+MOTOR_MAX_SPEED
 * @param[out] p_motor The motor to change
 */
static void set_motor(int speed, unsigned int tick_count, motor_settings_t *p_motor)
{
    if (speed >= 0)
    {
        p_motor->forward = true;
    }
    else
    {
        p_motor->forward = false;
        speed = abs(speed);
    }

    if (tick_count > speed)
    {
        /* Max one second run time */
        tick_count = speed;
    }

    p_motor->tick_count = tick_count;

    if (speed > MOTOR_MAX_SPEED)
    {
        p_motor->speed = MOTOR_MAX_SPEED;
    }
    else
    {
        p_motor->speed = speed;
    }
}

/**
 * Writes message to serial port.
 *
 * @param[in] p_message MESSAGE_LEN bytes of message
 * @return success or an error
 */
static enum motor_status_t send_message(const uint8_t *p_message)
{
    enum motor_status_t result;
    if (fd >= 0)
    {
        printf("Writing %u bytes\r\nData: ", MESSAGE_LEN);
        for(size_t i = 0; i < MESSAGE_LEN; i++)
        {
            printf("%02x ", p_message[i]);
        }
        printf("\r\n");
        ssize_t written = write(fd, p_message, MESSAGE_LEN);
        printf("TX: ");
        for(size_t i = 0; i < MESSAGE_LEN; i++)
        {
            printf("%02X ", p_message[i]);
        }
        printf("\r\n");
        if (written == MESSAGE_LEN)
        {
            result = MOTOR_STATUS_OK;
        }
        else
        {
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
 * Process the message from the controller. Currently just
 * does some logging. Checksums have already been verified at
 * this stage.
 *
 * @param[in] p_message The received message
 */
static void process_rx_message(const rx_message_t* p_message)
{
    switch(p_message->command)
    {
    case MESSAGE_CONTROL_ACK:
        printf("ACK\r\n");
        break;
    case MESSAGE_CONTROL_LHS_CLICKS:
        printf("LHS clicks = %u\r\n", p_message->ticks_remaining);
        break;
    case MESSAGE_CONTROL_RHS_CLICKS:
        printf("RHS clicks = %u\r\n", p_message->ticks_remaining);
        break;
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
    static uint8_t checksum = 0;
    switch(read_state)
    {
    case READ_STATE_HEADER:
        if (byte == MESSAGE_HEADER)
        {
            read_state = READ_STATE_COMMAND;
            checksum = MESSAGE_HEADER;
        }
        break;
    case READ_STATE_COMMAND:
        switch(byte)
        {
        case MESSAGE_CONTROL_ACK:
            rx_message.command = byte;
            checksum ^= byte;
            read_state = READ_STATE_CHECKSUM;
            break;
        case MESSAGE_CONTROL_LHS_CLICKS:
        case MESSAGE_CONTROL_RHS_CLICKS:
            rx_message.command = byte;
            checksum ^= byte;
            read_state = READ_STATE_CLICKS;
            break;
        default:
            printf("Unknown command %02x\r\n", byte);
            read_state = READ_STATE_HEADER;
            break;
        }
        break;
    case READ_STATE_CLICKS:
        checksum ^= byte;
        rx_message.ticks_remaining = byte << 1;
        read_state = READ_STATE_CHECKSUM;
        break;
    case READ_STATE_CHECKSUM:
        if (byte == checksum)
        {
            process_rx_message(&rx_message);
        }
        else
        {
            printf("Dropping bad packet\r\n");
        }
        read_state = READ_STATE_HEADER;
        break;
    }
}

/**************************************************
* End of file
***************************************************/

