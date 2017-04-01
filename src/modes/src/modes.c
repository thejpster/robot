/*****************************************************
*
* Pi Wars Robot Software (PWRS) Operating Modes
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
* and 320 ticks per second. We once measured 320 ticks/second
* as the fastest the motor would go (7.2V, no load).
*
* In an ideal world, the motor controller would perform
* closed-loop motor speed control. Currently, it just
* scales the 0-320 value to 0-255 and writes it to the
* PWM output. Feedback is performed using ultrasonic
* range sensors and line readers instead.
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
#include "gpio/gpio.h"
#include "lcd/lcd.h"
#include "motor/motor.h"

#include "modes/modes.h"

/**************************************************
* Defines
***************************************************/

#define LINE_SENSOR_LEFT GPIO_MAKE_IO_PIN(0, 21)
#define LINE_SENSOR_RIGHT GPIO_MAKE_IO_PIN(0, 20)
#define LINE_SENSOR_POWER GPIO_MAKE_IO_PIN(0, 26)

/**************************************************
* Data Types
**************************************************/

typedef void (*mode_function_t)(void);

struct straight_line_t
{
    bool running;
};

struct line_follow_t
{
    bool running;
};

/**************************************************
* Function Prototypes
**************************************************/

static void mode_menu(void);
static void mode_remote_control(void);
static void mode_straight_line(void);
static void mode_line_follow(void);
static void render_text(int motor_left, int motor_right);
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

static struct straight_line_t straight_line;

static struct line_follow_t line_follow;

static char msg[14] = { 0 };

/**************************************************
* Public Functions
***************************************************/

/*
 * Setup pins as required.
 */
void mode_init(void)
{
    gpio_make_output(LINE_SENSOR_POWER, 0);
    gpio_make_input(LINE_SENSOR_LEFT);
    gpio_make_input(LINE_SENSOR_RIGHT);
}

/*
 * Jump to whichever mode we have registered as the current one.
 *
 * Will be called LOOPS_PER_SECOND times per second.
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
        printf("Backlight toggle!\r\n");
        lcd_toggle_backlight();
        last_button = DUALSHOCK_BUTTON_TRIANGLE;
    }
    else if (dualshock_read_button(DUALSHOCK_BUTTON_CIRCLE))
    {
        printf("Shutting Down!\r\n");
#ifndef LCD_SIM
        system("sudo poweroff");
#endif
    }
}

/**
 * Remote control mode.
 */
static void mode_remote_control(void)
{
    if (debounce_button())
    {
        return;
    }

    const int stick_left = dualshock_read_axis(DUALSHOCK_AXIS_LY);
    const int stick_right = dualshock_read_axis(DUALSHOCK_AXIS_RY);

    /* max input is +/- 32767 */
    const int motor_left = (stick_left * MOTOR_MAX_SPEED) / DUALSHOCK_MAX_AXIS_VALUE;
    const int motor_right = (stick_right * MOTOR_MAX_SPEED) / DUALSHOCK_MAX_AXIS_VALUE;

    render_text(motor_left, motor_right);

    motor_control(MOTOR_LEFT, motor_left);

    motor_control(MOTOR_RIGHT, motor_right);

    if (dualshock_read_button(DUALSHOCK_BUTTON_CROSS))
    {
        last_button = DUALSHOCK_BUTTON_CROSS;
        change_mode(mode_menu);
    }

    if (dualshock_read_button(DUALSHOCK_BUTTON_TRIANGLE))
    {
        printf("Backlight toggle!\r\n");
        lcd_toggle_backlight();
        last_button = DUALSHOCK_BUTTON_TRIANGLE;
    }
}

/**
 * Straight line mode.
 */
static void mode_straight_line(void)
{
    if (debounce_button())
    {
        return;
    }

    // Calculate balance - i.e. how far off centre the robot is
    // 0.5 is dead straight.
    // < 0.5 means robot is closer to left wall and should go right
    // > 0.5 means robot is closer to right wall and should go left
    double range_left = motor_read_distance(0);
    double range_right = motor_read_distance(1);
    double range_front = motor_read_distance(2);
    double total = range_left + range_right;
    double balance = range_left / total;

    if (range_front < 30)
    {
        // Hit the end!
        straight_line.running = false;
    }

    unsigned int motor_left = 0;
    unsigned int motor_right = 0;

    if (balance > 0.5)
    {
        // Need to go left a bit
        motor_right = MOTOR_MAX_SPEED;
        motor_left = (MOTOR_MAX_SPEED / 2) / balance;
    }
    else
    {
        // Need to go right a bit
        motor_left = MOTOR_MAX_SPEED;
        motor_right = (MOTOR_MAX_SPEED / 2) / (1 - balance);
    }

    render_text(motor_left, motor_right);

    if (!straight_line.running)
    {
        // Force speed to be zero
        motor_left = 0;
        motor_right = 0;
    }

    motor_control(MOTOR_LEFT, motor_left);

    motor_control(MOTOR_RIGHT, motor_right);

    if (dualshock_read_button(DUALSHOCK_BUTTON_CROSS))
    {
        gpio_set_output(LINE_SENSOR_POWER, 0);
        last_button = DUALSHOCK_BUTTON_CROSS;
        change_mode(mode_menu);
    }

    if (dualshock_read_button(DUALSHOCK_BUTTON_TRIANGLE))
    {
        lcd_toggle_backlight();
        last_button = DUALSHOCK_BUTTON_TRIANGLE;
    }

    if (dualshock_read_button(DUALSHOCK_BUTTON_START))
    {
        straight_line.running = ! straight_line.running;
        last_button = DUALSHOCK_BUTTON_START;
    }
}

/**
 * Line following mode.
 */
static void mode_line_follow(void)
{
    if (debounce_button())
    {
        return;
    }

    // Read line sensors
    unsigned int motor_left = MOTOR_MAX_SPEED / 4;
    unsigned int motor_right = MOTOR_MAX_SPEED / 4;

    // Black = low, White = high
    if (!gpio_read_input(LINE_SENSOR_LEFT))
    {
        motor_left = -motor_left;
    }

    if (!gpio_read_input(LINE_SENSOR_RIGHT))
    {
        motor_right = -motor_right;
    }

    render_text(motor_left, motor_right);

    if (!line_follow.running)
    {
        // Force speed to be zero
        motor_left = 0;
        motor_right = 0;
    }

    motor_control(MOTOR_LEFT, motor_left);

    motor_control(MOTOR_RIGHT, motor_right);

    if (dualshock_read_button(DUALSHOCK_BUTTON_CROSS))
    {
        last_button = DUALSHOCK_BUTTON_CROSS;
        change_mode(mode_menu);
    }

    if (dualshock_read_button(DUALSHOCK_BUTTON_TRIANGLE))
    {
        lcd_toggle_backlight();
        last_button = DUALSHOCK_BUTTON_TRIANGLE;
    }

    if (dualshock_read_button(DUALSHOCK_BUTTON_START))
    {
        line_follow.running = ! line_follow.running;
        last_button = DUALSHOCK_BUTTON_START;
    }
}

/*
 * Put information on the screen.
 */
static void render_text(int motor_left, int motor_right)
{
    snprintf(msg, sizeof(msg) - 1, "%c%03d %c%03d", (motor_left < 0) ? '-' : '+', abs(motor_left), (motor_right < 0) ? '-' : '+', abs(motor_right));
    font_draw_text_small(0, 0, msg, LCD_WHITE, LCD_BLACK, FONT_MONOSPACE);
    int current_mA[4] = { 0 };
    for (uint8_t i = 0; i < 4; i++)
    {
        current_mA[i] = 1000 * motor_current(i);
        if (current_mA[i] > 999)
        {
            current_mA[i] = 999;
        }
    }
    snprintf(msg, sizeof(msg) - 1, " %03d  %03d", current_mA[2], current_mA[0]);
    font_draw_text_small(0, 10, msg, LCD_WHITE, LCD_BLACK, FONT_MONOSPACE);
    snprintf(msg, sizeof(msg) - 1, " %03d  %03d", current_mA[3], current_mA[1]);
    font_draw_text_small(0, 20, msg, LCD_WHITE, LCD_BLACK, FONT_MONOSPACE);
    int range_cm[3] = { 0 };
    for (int i = 0; i < 3; i++)
    {
        double range = motor_read_distance(i);
        range_cm[i] = range > 999 ? 999 : (int) range;
    }
    snprintf(msg, sizeof(msg) - 1, "   %03d", range_cm[2]);
    font_draw_text_small(0, 30, msg, LCD_WHITE, LCD_BLACK, FONT_MONOSPACE);
    snprintf(msg, sizeof(msg) - 1, " %03d  %03d", range_cm[0], range_cm[1]);
    font_draw_text_small(0, 40, msg, LCD_WHITE, LCD_BLACK, FONT_MONOSPACE);
    lcd_flush();
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
        change_mode(mode_straight_line);
        straight_line.running = false;
    }
    else if (p_menu_item == &top_menu_items[2])
    {

    }
    else if (p_menu_item == &top_menu_items[3])
    {
        gpio_set_output(LINE_SENSOR_POWER, 1);
        change_mode(mode_line_follow);
        line_follow.running = false;
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

