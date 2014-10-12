/*****************************************************
*
* Pi Wars Robot Software (PWRS)
*
* Copyright (c) 2014 theJPster (pwrs@hejpster.org.uk)
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
*****************************************************/

/**************************************************
* Includes
***************************************************/

#include <time.h>
#include "../util.h"

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

/* Delays for specified number of milliseconds */
void delay_ms(uint32_t milliseconds)
{
    struct timespec tv = {
        .tv_sec = milliseconds / 1000,
        .tv_nsec = (milliseconds % 1000) * 1000 * 1000
    };
    nanosleep(&tv, NULL);
}

/**************************************************
* Private Functions
***************************************************/

/* None */

/**************************************************
* End of file
***************************************************/

