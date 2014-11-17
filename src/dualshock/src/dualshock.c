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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/select.h>
#include <fcntl.h>
#include <assert.h>
#include <inttypes.h>

#include "../dualshock.h"

/**************************************************
* Defines
***************************************************/

/* None */

/**************************************************
* Data Types
**************************************************/

enum event_type_t
{
    EVENT_TYPE_BUTTON = 1,
    EVENT_TYPE_STICK = 2,
    EVENT_TYPE_INIT = 128,
    EVENT_TYPE_INITBUTTON = 129,
    EVENT_TYPE_INITSTICK = 130,
};

/* As read from device */
struct event_data_t
{
    uint32_t timestamp;
    uint16_t value;
    uint8_t type;
    uint8_t idx;
};

/* The ones we're interested in */
enum event_stick_idx_t
{
    EVENT_STICK_IDX_LX = 0,
    EVENT_STICK_IDX_LY = 1,
    EVENT_STICK_IDX_RX = 2,
    EVENT_STICK_IDX_RY = 3,
    EVENT_STICK_IDX_L2 = 12,
    EVENT_STICK_IDX_R2 = 13,
    EVENT_STICK_IDX_L1 = 14,
    EVENT_STICK_IDX_R1 = 15
};

enum event_button_idx_t
{
    EVENT_BUTTON_IDX_SELECT = 0,
    EVENT_BUTTON_IDX_LEFTSTICK = 1,
    EVENT_BUTTON_IDX_RIGHTSTICK = 2,
    EVENT_BUTTON_IDX_START = 3,
    EVENT_BUTTON_IDX_UP = 4,
    EVENT_BUTTON_IDX_RIGHT = 5,
    EVENT_BUTTON_IDX_DOWN = 6,
    EVENT_BUTTON_IDX_LEFT = 7,
    EVENT_BUTTON_IDX_L2 = 8,
    EVENT_BUTTON_IDX_R2 = 9,
    EVENT_BUTTON_IDX_L1 = 10,
    EVENT_BUTTON_IDX_R1 = 11,
    EVENT_BUTTON_IDX_PS = 16,
    EVENT_BUTTON_IDX_TRIANGLE = 12,
    EVENT_BUTTON_IDX_CIRCLE = 13,
    EVENT_BUTTON_IDX_CROSS = 14,
    EVENT_BUTTON_IDX_SQUARE = 15,
};
struct js_state_t
{
    int16_t lx;
    int16_t ly;
    int16_t rx;
    int16_t ry;
    uint16_t l2;
    uint16_t r2;
    uint16_t l1;
    uint16_t r1;
    bool buttons[DUALSHOCK_NUM_BUTTONS];
};

/**************************************************
* Function Prototypes
**************************************************/

static void process_event(const struct event_data_t *p_data);

/**************************************************
* Public Data
**************************************************/

extern int verbose_flag;

/**************************************************
* Private Data
**************************************************/

static int fd;
static struct js_state_t js_state;

/**************************************************
* Public Functions
***************************************************/

int dualshock_init(const char *sz_jsdev)
{
    int retval = 0;
    printf("Opening joystick device %s\n", sz_jsdev);
    fd = open(sz_jsdev, O_RDONLY);
    if (fd > 0)
    {
        printf("Joystick device %s is open\n", sz_jsdev);
    }
    else
    {
        perror("Can't open device");
    }
    return retval;
}

void dualshock_read_or_timeout(struct timeval *p_delay)
{
    fd_set rfds;
    FD_ZERO(&rfds);
    if (fd > 0)
    {
        FD_SET(fd, &rfds);
        select(fd + 1, &rfds, NULL, NULL, p_delay);
    }
    else
    {
        select(0, NULL, NULL, NULL, p_delay);
    }
    if (FD_ISSET(fd, &rfds))
    {
        struct event_data_t data;
        size_t rx = read(fd, &data, sizeof(data));
        if (verbose_flag)
        {
            printf("Read %zu from joystick\n", rx);
        }
        assert(rx == sizeof(data));
        process_event(&data);
    }
}

int dualshock_read_axis(enum dualshock_axis_t axis)
{
    int retval = 0;
    switch (axis)
    {
    case DUALSHOCK_AXIS_LX:
        retval = js_state.lx;
        break;
    case DUALSHOCK_AXIS_LY:
        retval = js_state.ly;
        break;
    case DUALSHOCK_AXIS_RX:
        retval = js_state.rx;
        break;
    case DUALSHOCK_AXIS_RY:
        retval = js_state.ry;
        break;
    case DUALSHOCK_AXIS_L2:
        retval = js_state.l2;
        break;
    case DUALSHOCK_AXIS_R2:
        retval = js_state.r2;
        break;
    case DUALSHOCK_AXIS_L1:
        retval = js_state.l1;
        break;
    case DUALSHOCK_AXIS_R1:
        retval = js_state.r1;
        break;
    default:
        abort();
    }
    return retval;
}

bool dualshock_read_button(enum dualshock_button_t button)
{
    return js_state.buttons[button];
}

/**************************************************
* Private Functions
***************************************************/

static void process_event(const struct event_data_t *p_event)
{
    if (verbose_flag)
    {
        printf("\ttimestamp = %"PRIx32"\n", p_event->timestamp);
        printf("\tvalue = %04"PRIx16"\n", p_event->value);
        printf("\ttype = %02"PRIx8"\n", p_event->type);
        printf("\tidx = %02"PRIx8"\n", p_event->idx);
    }
    if ((p_event->type == EVENT_TYPE_STICK) || (p_event->type == EVENT_TYPE_INITSTICK))
    {
        enum event_stick_idx_t idx = p_event->idx;
        switch (idx)
        {
        case EVENT_STICK_IDX_LX:
            js_state.lx = -p_event->value;
            break;
        case EVENT_STICK_IDX_LY:
            js_state.ly = -p_event->value;
            break;
        case EVENT_STICK_IDX_RX:
            js_state.rx = -p_event->value;
            break;
        case EVENT_STICK_IDX_RY:
            js_state.ry = -p_event->value;
            break;
        case EVENT_STICK_IDX_L2:
            js_state.l2 = p_event->value + 32767;
            break;
        case EVENT_STICK_IDX_R2:
            js_state.r2 = p_event->value + 32767;
            break;
        case EVENT_STICK_IDX_L1:
            js_state.l1 = p_event->value + 32767;
            break;
        case EVENT_STICK_IDX_R1:
            js_state.r1 = p_event->value + 32767;
            break;
        default:
            /* Ignore unwanted events */
            break;
        }
    }
    else if ((p_event->type == EVENT_TYPE_BUTTON) || (p_event->type == EVENT_TYPE_INITBUTTON))
    {
        enum event_button_idx_t idx = p_event->idx;
        switch (idx)
        {
        case EVENT_BUTTON_IDX_SELECT:
            js_state.buttons[DUALSHOCK_BUTTON_SELECT] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_LEFTSTICK:
            js_state.buttons[DUALSHOCK_BUTTON_LEFTSTICK] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_RIGHTSTICK:
            js_state.buttons[DUALSHOCK_BUTTON_RIGHTSTICK] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_START:
            js_state.buttons[DUALSHOCK_BUTTON_START] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_UP:
            js_state.buttons[DUALSHOCK_BUTTON_UP] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_RIGHT:
            js_state.buttons[DUALSHOCK_BUTTON_RIGHT] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_DOWN:
            js_state.buttons[DUALSHOCK_BUTTON_DOWN] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_LEFT:
            js_state.buttons[DUALSHOCK_BUTTON_LEFT] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_L2:
            js_state.buttons[DUALSHOCK_BUTTON_L2] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_R2:
            js_state.buttons[DUALSHOCK_BUTTON_R2] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_L1:
            js_state.buttons[DUALSHOCK_BUTTON_L1] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_R1:
            js_state.buttons[DUALSHOCK_BUTTON_R1] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_PS:
            js_state.buttons[DUALSHOCK_BUTTON_PS] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_TRIANGLE:
            js_state.buttons[DUALSHOCK_BUTTON_TRIANGLE] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_CIRCLE:
            js_state.buttons[DUALSHOCK_BUTTON_CIRCLE] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_CROSS:
            js_state.buttons[DUALSHOCK_BUTTON_CROSS] = p_event->value;
            break;
        case EVENT_BUTTON_IDX_SQUARE:
            js_state.buttons[DUALSHOCK_BUTTON_SQUARE] = p_event->value;
            break;
        default:
            /* Ignore unwanted events */
            break;
        }
    }

}

/**************************************************
* End of file
***************************************************/

