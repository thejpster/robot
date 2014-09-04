/*****************************************************
*
* Stellaris Launchpad Example Project
*
* Copyright (c) 2014 theJPster (www.thejpster.org.uk)
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
*****************************************************/

#ifndef TEMPLATE_H
#define TEMPLATE_H

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

enum dualshock_axis_t
{
    DUALSHOCK_AXIS_LX,
    DUALSHOCK_AXIS_LY,
    DUALSHOCK_AXIS_RX,
    DUALSHOCK_AXIS_RY,
    DUALSHOCK_AXIS_L1,
    DUALSHOCK_AXIS_L2,
    DUALSHOCK_AXIS_R1,
    DUALSHOCK_AXIS_R2,
    DUALSHOCK_NUM_AXES
};

enum dualshock_button_t
{
    DUALSHOCK_BUTTON_SQUARE,
    DUALSHOCK_BUTTON_CIRCLE,
    DUALSHOCK_BUTTON_TRIANGLE,
    DUALSHOCK_BUTTON_CROSS,
    DUALSHOCK_BUTTON_PS,
    DUALSHOCK_BUTTON_START,
    DUALSHOCK_BUTTON_SELECT,
    DUALSHOCK_BUTTON_LEFTSTICK,
    DUALSHOCK_BUTTON_RIGHTSTICK,
    DUALSHOCK_BUTTON_UP,
    DUALSHOCK_BUTTON_DOWN,
    DUALSHOCK_BUTTON_LEFT,
    DUALSHOCK_BUTTON_RIGHT,
    DUALSHOCK_BUTTON_L1,
    DUALSHOCK_BUTTON_L2,
    DUALSHOCK_BUTTON_R1,
    DUALSHOCK_BUTTON_R2,
    DUALSHOCK_NUM_BUTTONS
};

/**************************************************
* Public Data
**************************************************/

/* None */

/**************************************************
* Public Function Prototypes
***************************************************/

int dualshock_init(const char* sz_jsdev);

void dualshock_read_or_timeout(struct timeval* p_delay);

int dualshock_read_axis(enum dualshock_axis_t axis);

bool dualshock_read_button(enum dualshock_button_t axis);

#ifdef __cplusplus
}
#endif

#endif /* ndef TEMPLATE_H */

/**************************************************
* End of file
***************************************************/

