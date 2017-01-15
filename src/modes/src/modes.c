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
* This is the mode management module. Modes roughly align
* with the various challenges the robot has to undertake.

* These are:
*
*   Remote control challenges (all in one mode):
*   * Skittles (shoving a ball around)
*   * Robot golf (shoving a golf ball around)
*   * Pi Noon (popping another robot's balloon)
*   * Obstacle Course (remote control around a course)
*
*   Autonomous challenges:
*   * Straight Line Speed Test (drive fast in a straight line between two walls)
*   * Line following (following a black line course autonomously)
*   * Minimal maze (drive around a maze with high walls autonomously)
*
* The motor controller takes speed inputs between zero
* and 330 ticks per second. We measured 330 ticks/second
* as the fastest the motor would go (7.2V, no load).
* The maximum speed is definable here as we don't want to
* ask the robot to go faster than it can, otherwise
* we lose closed-loop feedback control.
*****************************************************/

/**************************************************
* Includes
***************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "util/util.h"
#include "dualshock/dualshock.h"
#include "menu/menu.h"
#include "font/font.h"
#include "lcd/lcd.h"
#include "motor/motor.h"
#include "line_follower/server.h"

#include <modes/modes.h>

/**************************************************
* Defines
***************************************************/

/* There are 1000 ticks per three wheel rotations */
/* Wheels are 60mm diameter => ~188mm circumference */
/* => 1000 mm = ~5.3 wheel turns = ~1773 ticks */

#define MM_TO_TICKS(x) (((x) * 1773) / 1000)
/* @todo change this! */
#define TURN_90 (233)

/**************************************************
* Data Types
**************************************************/

typedef void (*mode_function_t)(void);

/**************************************************
* Function Prototypes
**************************************************/

static void mode_menu(void);
static void mode_remote_control(void);
static void mode_test(void);
static void change_mode(mode_function_t new_mode);
static bool select_mode(
    const struct menu_t *p_menu,
    const struct menu_item_t *p_menu_item
);
static bool debounce_button(void);

/**************************************************
* Public Data
**************************************************/

/* None */

/**************************************************
* Private Data
**************************************************/

static mode_function_t current_mode = mode_menu;

static bool mode_first = true;

static const struct menu_item_t top_menu_items[] =
{
    /* http://piwars.org/2017-competition/challenges/slightly-deranged-golf/
       http://piwars.org/2017-competition/challenges/skittles/
       http://piwars.org/2017-competition/challenges/obstacle-course/
       http://piwars.org/2017-competition/challenges/pi-noon-the-robot-vs-robot-duel/ */
    { "Remote", MENU_ITEM_TYPE_ACTION, NULL, select_mode },
    /* http://piwars.org/2017-competition/challenges/straight-line-speed-test/ */
    { "Speed", MENU_ITEM_TYPE_ACTION, NULL, select_mode },
    /* http://piwars.org/2017-competition/challenges/minimal-maze/ */
    { "Maze", MENU_ITEM_TYPE_ACTION, NULL, select_mode },
    /* http://piwars.org/2017-competition/challenges/line-following/ */
    { "Line", MENU_ITEM_TYPE_ACTION, NULL, select_mode },
};

static const struct menu_t top_menu =
{
    .p_title = "P.W.R.S",
    .num_items = NUMELTS(top_menu_items),
    .p_menu_items = top_menu_items,
    .hide_back = true
};

static enum dualshock_button_t last_button = DUALSHOCK_NUM_BUTTONS;

/**************************************************
* Public Functions
***************************************************/

/*
 * Jump to whichever mode we have registered as the current one.
 */
void mode_handle(void)
{
    current_mode();
}

/**************************************************
* Private Functions
***************************************************/

/**
 * Mode selection menu.
 */
static void mode_menu(void)
{

    if (mode_first)
    {
        menu_init(&top_menu);
        menu_redraw(true);
        mode_first = false;
    }

    if (debounce_button())
    {
        return;
    }

    if (dualshock_read_button(DUALSHOCK_BUTTON_UP))
    {
        printf("Up!\r\n");
        menu_keypress(MENU_KEYPRESS_UP);
        last_button = DUALSHOCK_BUTTON_UP;
    }
    else if (dualshock_read_button(DUALSHOCK_BUTTON_DOWN))
    {
        printf("Down!\r\n");
        menu_keypress(MENU_KEYPRESS_DOWN);
        last_button = DUALSHOCK_BUTTON_DOWN;
    }
    else if (dualshock_read_button(DUALSHOCK_BUTTON_CROSS))
    {
        printf("Enter!\r\n");
        menu_keypress(MENU_KEYPRESS_ENTER);
        last_button = DUALSHOCK_BUTTON_CROSS;
    }
    else if (dualshock_read_button(DUALSHOCK_BUTTON_TRIANGLE))
    {
        printf("Reboot!\r\n");
        system("reboot");
    }
    else if (dualshock_read_button(DUALSHOCK_BUTTON_CIRCLE))
    {
        printf("Shutting Down!\r\n");
#ifndef LCD_SIM
        system("shutdown now");
#endif
    }
}

/**
 * Remote control mode.
 */
static void mode_remote_control(void)
{
    char msg[14];
    int stick_left, stick_right, trim;
    int motor_left, motor_right;

    if (debounce_button())
    {
        return;
    }

    stick_left = dualshock_read_axis(DUALSHOCK_AXIS_LY);
    stick_right = dualshock_read_axis(DUALSHOCK_AXIS_RY);
    trim = dualshock_read_axis(DUALSHOCK_AXIS_R2);

    if (trim <= 10)
    {
        /* We allow 0..MOTOR_MAX_SPEED/2 */
        trim = 2;
    }
    else
    {
        /* We allow 0..MOTOR_MAX_SPEED */
        trim = 1;
    }

    /* max input is +/- 32767 */
    motor_left = (stick_left * MOTOR_MAX_SPEED) / DUALSHOCK_MAX_AXIS_VALUE;
    motor_right = (stick_right * MOTOR_MAX_SPEED) / DUALSHOCK_MAX_AXIS_VALUE;

    motor_left = motor_left / trim;
    motor_right = motor_right / trim;

    sprintf(msg, "L%c%03d %04X", (motor_left < 0) ? '-' : '+', abs(motor_left), (uint16_t) stick_left);
    font_draw_text_small(0, 0, msg, LCD_WHITE, LCD_BLACK, FONT_MONOSPACE);
    sprintf(msg, "R%c%03d %04X", (motor_right < 0) ? '-' : '+', abs(motor_right), (uint16_t) stick_right);
    font_draw_text_small(0, 10, msg, LCD_WHITE, LCD_BLACK, FONT_MONOSPACE);
    sprintf(msg, "Trim:%d", trim);
    font_draw_text_small(0, 20, msg, LCD_WHITE, LCD_BLACK, FONT_MONOSPACE);
    lcd_flush();

    enum motor_status_t status;

    status = motor_control(
                 MOTOR_LEFT,
                 motor_left,
                 abs(motor_left)
             );

    if (status != MOTOR_STATUS_OK)
    {
        printf("Error writing to serial port: %u\r\n", status);
    }

    status = motor_control(
                 MOTOR_RIGHT,
                 motor_right,
                 abs(motor_right)
             );

    if (status != MOTOR_STATUS_OK)
    {
        printf("Error writing to serial port: %u\r\n", status);
    }

    if (dualshock_read_button(DUALSHOCK_BUTTON_CROSS))
    {
        last_button = DUALSHOCK_BUTTON_CROSS;
        change_mode(mode_menu);
    }
}

/**
 * Tests the LCD.
 */
static void mode_test(void)
{
    static int x = 1;
    if (debounce_button())
    {
        return;
    }
    lcd_paint_fill_rectangle(LCD_WHITE, LCD_FIRST_COLUMN, LCD_LAST_COLUMN, LCD_FIRST_ROW, LCD_LAST_ROW);
    lcd_paint_fill_rectangle(LCD_BLACK, LCD_FIRST_COLUMN+x, LCD_LAST_COLUMN-x, LCD_FIRST_ROW+x, LCD_LAST_ROW-x);
    if (x == 24) {
        x = 1;
    } else {
        x = x + 1;
    }
    if (dualshock_read_button(DUALSHOCK_BUTTON_CROSS))
    {
        last_button = DUALSHOCK_BUTTON_CROSS;
        change_mode(mode_menu);
    }
}

/*
 * Called when a mode is changed. Clears up the LCD ready
 * for the new mode.
 */
static void change_mode(mode_function_t new_mode)
{
    lcd_paint_clear_screen();
    current_mode = new_mode;
    mode_first = true;
}

/*
 * Called when a menu item is selected.
 */
static bool select_mode(
    const struct menu_t *p_menu,
    const struct menu_item_t *p_menu_item
)
{
    bool retval = false;
    if (p_menu_item == &top_menu_items[0])
    {
        /* Remote mode */
        change_mode(mode_remote_control);
    }
    else if (p_menu_item == &top_menu_items[1])
    {
        /* Test mode (for now) */
        change_mode(mode_test);
    }
    else if (p_menu_item == &top_menu_items[2])
    {
        /* Test mode (for now) */
        change_mode(mode_test);
    }
    else if (p_menu_item == &top_menu_items[3])
    {
        /* Spin mode (tet mode) */
        change_mode(mode_test);
    }
    else
    {
        /* Go back to menu? */
        printf("Unknown pointer %p\r\n", (const void *) p_menu_item);
        change_mode(mode_menu);
        retval = true;
    }
    return retval;
}

/**
 * While button is still held, mode routine should
 * do nothing. Avoids accidentally selecting something
 * immediately when you enter a mode.
 */
static bool debounce_button(void)
{
    bool retval = false;
    if (last_button != DUALSHOCK_NUM_BUTTONS)
    {
        if (dualshock_read_button(last_button))
        {
            /* Button still down .. do nothing */
            retval = true;
        }
        else
        {
            last_button = DUALSHOCK_NUM_BUTTONS;
        }
    }
    return retval;
}

/**************************************************
* End of file
***************************************************/

