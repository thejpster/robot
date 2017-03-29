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
* This is most likely implemented on an 8-bit microcontroller
* as the closed loop feedback is a bit much for the Raspberry Pi.
*
* There is also a simulation mode, where the motor values
* are printed to screen instead of sent over the serial port.
*
*****************************************************/

#ifndef MOTOR_H
#define MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************
* Includes
***************************************************/

/* None */

/**************************************************
* Public Defines
***************************************************/

#define MOTOR_MAX_SPEED 320 

#define MESSAGE_LEN 5

/**************************************************
* Public Data Types
**************************************************/

typedef enum motor_status_t
{
    MOTOR_STATUS_OK,
    MOTOR_STATUS_NO_DEVICE,
    MOTOR_STATUS_SERIAL_ERROR,
    MOTOR_STATUS_NO_RESPONSE,
    MOTOR_STATUS_BAD_MOTOR
} motor_status_t;

typedef enum motor_t
{
    MOTOR_LEFT,
    MOTOR_RIGHT,
    MOTOR_BOTH
} motor_t;

/* Positive is forwards, negative is reverse */
/* Valid range is -32768..32767 */
/* Outside of this is clipped */
typedef int motor_speed_t;

/* Must be greater than zero. */
typedef uint16_t motor_step_count_t;

/**************************************************
* Public Data
**************************************************/

/* None */

/**************************************************
* Public Function Prototypes
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
extern enum motor_status_t motor_init(
    const char* sz_serial_device
);

/**
 * Disable the motor controller interface.
 *
 * Closes the serial port. Calling motor_init() when it is
 * already initialised implicity calls this function first.
 *
 */
extern void motor_close(void);

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
extern enum motor_status_t motor_control(
    enum motor_t motor,
    motor_speed_t speed,
    motor_step_count_t step_count
);

/**
 * Check the motor controller serial port for ACKs and
 * tick count updates. Call this regularly
 * otherwise the serial port buffer will fill up.
 *
 * @return An error code
 */
motor_status_t motor_poll(void);


/**
 * Find out how many ticks are left from the last command.
 *
 * @param[in] motor Which motor to read
 * @return The number of ticks left
 */
motor_step_count_t motor_read_steps(
    enum motor_t motor
);

/**
 * Find out how much current a channel is using.
 *
 * There are four channels.
 *
 * @param[in] motor Which motor to read
 * @return The amount of current in Amps.
 */
float motor_current(
    uint8_t channel
);

/**
 * Read the latest ultrasound measurement.
 *
 * @return a distance in cm to the nearest object
 */
double motor_read_distance(uint8_t sensor);

#ifdef __cplusplus
}
#endif

#endif /* ndef MOTOR_H */

/**************************************************
* End of file
***************************************************/

