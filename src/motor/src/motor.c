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

#include "util/util.h"
#include "../motor.h"

/**************************************************
* Defines
***************************************************/

/* None */

/**************************************************
* Data Types
**************************************************/

/* None */

/**************************************************
* Function Prototypes
**************************************************/

/* None */

/**************************************************
* Public Data
**************************************************/

/* None */

/**************************************************
* Private Data
**************************************************/

/* None */

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
    return MOTOR_STATUS_OK;
}


/**************************************************
* Private Functions
***************************************************/

/* None */

/**************************************************
* End of file
***************************************************/

