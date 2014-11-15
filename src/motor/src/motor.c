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

#define BAUDRATE B9600

#define MESSAGE_HEADER 0x00
#define MESSAGE_HEADER_OFFSET 0
#define MESSAGE_CONTROL_OFFSET 1
#define MESSAGE_LEFT_SPEED_OFFSET 2
#define MESSAGE_RIGHT_SPEED_OFFSET 3
#define MESSAGE_CHECKSUM_OFFSET 4
#define MESSAGE_LEN 5

#define CONTROL_LEFT_DIR  (1 << 0)
#define CONTROL_RIGHT_DIR  (1 << 1)
#define CONTROL_LEFT_STOP (1 << 2)
#define CONTROL_RIGHT_STOP (1 << 3)
#define CONTROL_MODE (1 << 4)

/**************************************************
* Data Types
**************************************************/

struct motor_settings_t
{
    bool forward;
    bool stop;
    uint8_t speed;
};

/**************************************************
* Function Prototypes
**************************************************/

static void build_message(
    const struct motor_settings_t* p_left,
    const struct motor_settings_t* p_right,
    uint8_t* p_message
);
static uint8_t calc_checksum(const uint8_t* p_message);

/**************************************************
* Public Data
**************************************************/

/* None */

/**************************************************
* Private Data
**************************************************/

static int fd = -1;
static char *sz_serial_port = "/dev/ttyACM0";

static struct motor_settings_t left = {
    .forward = true,
    .stop = true,
    .speed = 1
};

static struct motor_settings_t right = {
    .forward = true,
    .stop = true,
    .speed = 1
};

/* @todo need to cache motor speeds so we
 * can provide the old value if only one side is
 * being set. */

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
enum motor_status_t motor_init(void)
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
 * A 'step' is one transition on the quadrature encoder. The
 * amount of distance the robot covers is TBD.
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
    enum motor_t motor,
    motor_speed_t speed,
    motor_step_count_t step_count
)
{
    enum motor_status_t result;
    if (fd >= 0)
    {
        ssize_t written;
        uint8_t message[MESSAGE_LEN];
        bool forward;
        bool stop;
        if (speed >= 0)
        {
            forward = true;
        }
        else
        {
            forward = false;
            speed = abs(speed);                
        }
        speed = (speed * 256) / 32768; 
        if (speed > 255)
        {
            speed = 255;
        }
        else if (speed == 0)
        {
            stop = true;
            speed = 1;
        }

        if ((motor == MOTOR_LEFT) || (motor == MOTOR_BOTH))
        {
            left.stop = stop;
            left.forward = forward;
            left.speed = speed;
            printf("Set L=%c%u %s\r\n",
                left.forward ? '+' : '-',
                left.speed,
                left.stop ? "Stop" : "Run"
                );
        }
        if ((motor == MOTOR_RIGHT) || (motor == MOTOR_BOTH))
        {
            right.stop = stop;
            right.forward = forward;
            right.speed = speed;
            printf("Set L=%c%u %s\r\n",
                right.forward ? '+' : '-',
                right.speed,
                right.stop ? "Stop" : "Run"
                );
        }

        build_message(&left, &right, message);
        written = write(fd, message, sizeof(message));
        if (written == sizeof(message))
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
 * Control the motor(s)
 *
 * See @motor_control
 * 
 * @param[in] lspeed The left motor speed. +ve is forwards, -ve is reverse.
 * @param[in] lsteps The number of steps to take before stopping.
 * @param[in] rspeed The right motor speed. +ve is forwards, -ve is reverse.
 * @param[in] rsteps The number of steps to take before stopping.
 * @return An error code
 */
extern enum motor_status_t motor_control_pair(
    motor_speed_t lspeed,
    motor_step_count_t lstep_count,
    motor_speed_t rspeed,
    motor_step_count_t rstep_count
)
{
    enum motor_status_t result;
    if (fd >= 0)
    {
        ssize_t written;
        uint8_t message[MESSAGE_LEN];

        if (lspeed >= 0)
        {
            left.forward = true;
        }
        else
        {
            left.forward = false;
            lspeed = abs(lspeed);                
        }
        lspeed = (lspeed * 256) / 32768; 
        if (lspeed > 255)
        {
            lspeed = 255;
        }
        left.speed = lspeed;
        if (lspeed == 0)
        {
            left.stop = true;
            left.speed = 1;
        }

        if (rspeed >= 0)
        {
            right.forward = true;
        }
        else
        {
            right.forward = false;
            rspeed = abs(rspeed);                
        }
        rspeed = (rspeed * 256) / 32768; 
        if (rspeed > 255)
        {
            rspeed = 255;
        }
        right.speed = rspeed;
        if (rspeed == 0)
        {
            right.stop = true;
            right.speed = 1;
        }

        printf("Set L=%c%u %s\r\n",
            left.forward ? '+' : '-',
            left.speed,
            left.stop ? "Stop" : "Run"
            );
        printf("Set R=%c%u %s\r\n",
            right.forward ? '+' : '-',
            right.speed,
            right.stop ? "Stop" : "Run"
            );

        build_message(&left, &right, message);
        written = write(fd, message, sizeof(message));
        if (written == sizeof(message))
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
/**************************************************
* Private Functions
***************************************************/

/**
 * Build a message to go over the serial connection.
 *
 * @param[in] p_left Settings for the left motor
 * @param[in] p_right Settings for the right motor
 * @param[out] p_message The constructed message
 */
static void build_message(
    const struct motor_settings_t* p_left,
    const struct motor_settings_t* p_right,
    uint8_t* p_message
)
{
    uint8_t control = 0;
    p_message[MESSAGE_HEADER_OFFSET] = MESSAGE_HEADER;
    if (p_left->stop)
    {
        control |= CONTROL_LEFT_STOP;
    }
    if (p_left->forward)
    {
        control |= CONTROL_LEFT_DIR;
    }
    if (p_right->stop)
    {
        control |= CONTROL_RIGHT_STOP;
    }
    if (p_right->forward)
    {
        control |= CONTROL_RIGHT_DIR;
    }

    p_message[MESSAGE_CONTROL_OFFSET] = control;
    /* @todo add step_count to the message */
    p_message[MESSAGE_CHECKSUM_OFFSET] = calc_checksum(p_message);
}

/**
 * Calculate the message checksum.
 *
 * XORs the all the bytes bar the last one.
 *
 * @return the calculated checksum
 */
static uint8_t calc_checksum(const uint8_t* p_message)
{
    uint8_t result = 0;
    for(size_t i = 0; i < (MESSAGE_LEN-1); i++)
    {
        result = result ^ p_message[i];
    }
    return result;
}

/**************************************************
* End of file
***************************************************/

