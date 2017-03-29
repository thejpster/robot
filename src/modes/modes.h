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

/* None */

/**************************************************
* Public Data
**************************************************/

/* None */

/**************************************************
* Public Function Prototypes
***************************************************/

/*
 * Call this periodically to do whatever needs to be done
 * in this mode. Will exit promptly.
 */
void mode_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* ndef MODES_H */

/**************************************************
* End of file
***************************************************/

