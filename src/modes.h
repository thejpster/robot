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
* Extra notes on this module go here.
*
*****************************************************/

#ifndef MODES_H
#define MODES_H

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

/* None */

/**************************************************
* Public Data Types
**************************************************/

typedef void (*mode_function_t)(void);

/**************************************************
* Public Data
**************************************************/

extern mode_function_t mode_current;

/**************************************************
* Public Function Prototypes
***************************************************/

/**
 * Menu mode. Selects another mode.
 *
 */
extern void mode_menu(void);


/**
 * Direct remote control mode.
 *
 * Converts pad inputs into motor drive.
 *
 */
extern void mode_remote_control(void);


/**
 * Does a three point turn.
 *
 */
extern void mode_turn(void);


/**
 * Follows a line.
 *
 */
extern void mode_follow(void);


/**
 * Drives until it (almost) hits something.
 *
 */
extern void mode_hunt(void);

#ifdef __cplusplus
}
#endif

#endif /* ndef MODES_H */

/**************************************************
* End of file
***************************************************/

