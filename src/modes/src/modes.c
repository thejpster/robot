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
*
* These are:
*
*   Remote control challenges (all in one mode):
*   * Robot golf (moving a ball in to a goal)
*   * Sumo (shoving another robot off the pitch)
*   * Obstacle Course (remote control around a course)
*   * Straight Line Speed Test (drive fast in a straight line)
*
*   Autonomous challenges:
*   * Line following (following a black line course autonomously)
*   * Proxmity Alert (drive up to a wall but don't hit it)
*   * Three Point Turn (back up, turn around and return, autonomously)
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

#include "util/util.h"
#include "dualshock/dualshock.h"
#include "menu/menu.h"
#include "font/font.h"
#include "lcd/lcd.h"
#include "motor/motor.h"

#include <modes/modes.h>

/**************************************************
* Defines
***************************************************/

/* 
The route:
+------+-----------------------+------------+
|      |                       |            |
|      |                       +-----11-----+
|      |                       |            |
|      |                       |      12    |
+------+                       |   10|      |
|      |                       |     |13    |
|START 1  2   3   4  5  6  7  8|  9  |      |
|  25  24  23  22   21   20   19  18 |14    |
+------+                       |     |      |
|      |                       |   17 15    |
|      |                       |            |
|      |                       +----16------+
|      |                       |            |
+------+-----------------------+------------+
*/

/* There are 1000 ticks per three wheel rotations */
/* Wheels are 60mm diameter => ~188mm circumference */
/* => 1000 mm = ~5.3 wheel turns = ~1773 ticks */

#define MM_TO_TICKS(x) (((x) * 1773) / 1000)
/* @todo change this! */
#define TURN_90 (333)

/**************************************************
* Data Types
**************************************************/


typedef struct movement_t
{
    /* Bigger than a motor_step_count_t */
    uint32_t left_ticks;
    uint32_t right_ticks;
    motor_speed_t left_speed;
    motor_speed_t right_speed;
} movement_t;


/**************************************************
* Function Prototypes
**************************************************/

static void change_mode(mode_function_t new_mode);

bool select_mode(
    const struct menu_t *p_menu,
    const struct menu_item_t *p_menu_item
);

static bool debounce_button(void);

/**************************************************
* Public Data
**************************************************/

mode_function_t mode_current = mode_menu;

/**************************************************
* Private Data
**************************************************/

static bool mode_first = true;

static const struct menu_item_t top_menu_items[] =
{
    { "Remote", MENU_ITEM_TYPE_ACTION, NULL, select_mode },
    { "Follow", MENU_ITEM_TYPE_ACTION, NULL, select_mode },
    { "Hunt", MENU_ITEM_TYPE_ACTION, NULL, select_mode },
    { "Turn", MENU_ITEM_TYPE_ACTION, NULL, select_mode },
};

static const struct menu_t top_menu =
{
    .p_title = "P.W.R.S",
    .num_items = NUMELTS(top_menu_items),
    .p_menu_items = top_menu_items,
    .hide_back = true
};

static enum dualshock_button_t last_button = DUALSHOCK_NUM_BUTTONS;

static struct movement_t movements[] = 
{
    { 
        .left_ticks = MM_TO_TICKS(1640),
        .right_ticks = MM_TO_TICKS(1640),
        .left_speed = MOTOR_MAX_SPEED,
        .right_speed = MOTOR_MAX_SPEED,
    },
    { 
        .left_ticks = MM_TO_TICKS(TURN_90),
        .right_ticks = MM_TO_TICKS(TURN_90),
        .left_speed = -MOTOR_MAX_SPEED,
        .right_speed = MOTOR_MAX_SPEED,
    },
    { 
        .left_ticks = MM_TO_TICKS(900),
        .right_ticks = MM_TO_TICKS(900),
        .left_speed = MOTOR_MAX_SPEED,
        .right_speed = MOTOR_MAX_SPEED,
    },
    { 
        .left_ticks = MM_TO_TICKS(1800),
        .right_ticks = MM_TO_TICKS(1800),
        .left_speed = -MOTOR_MAX_SPEED,
        .right_speed = -MOTOR_MAX_SPEED,
    },
    { 
        .left_ticks = MM_TO_TICKS(900),
        .right_ticks = MM_TO_TICKS(900),
        .left_speed = MOTOR_MAX_SPEED,
        .right_speed = MOTOR_MAX_SPEED,
    },
    { 
        .left_ticks = MM_TO_TICKS(TURN_90),
        .right_ticks = MM_TO_TICKS(TURN_90),
        .left_speed = -MOTOR_MAX_SPEED,
        .right_speed = MOTOR_MAX_SPEED,
    },
    { 
        .left_ticks = MM_TO_TICKS(1640),
        .right_ticks = MM_TO_TICKS(1640),
        .left_speed = MOTOR_MAX_SPEED,
        .right_speed = MOTOR_MAX_SPEED,
    }
};

/**************************************************
* Public Functions
***************************************************/

/**
 * Mode selection menu.
 */
void mode_menu(void)
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
}

/**
 * Remote control mode.
 */
void mode_remote_control(void)
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

    if (trim == 0)
    {
        /* We allow 0..MOTOR_MAX_SPEED/4 */
        trim = 4;
    }
    else if ((trim > 10) && (trim <= 65530))
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
    font_draw_text_small(0, 0, msg, LCD_WHITE, LCD_BLACK, true);
    sprintf(msg, "R%c%03d %04X", (motor_right < 0) ? '-' : '+', abs(motor_right), (uint16_t) stick_right);
    font_draw_text_small(0, 10, msg, LCD_WHITE, LCD_BLACK, true);
    sprintf(msg, "Trim:%d", trim);
    font_draw_text_small(0, 20, msg, LCD_WHITE, LCD_BLACK, true);
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
 * Does a three point turn.
 *
 */
void mode_turn(void)
{
    if (debounce_button())
    {
        return;
    }

    font_draw_text_small(0, 10, "Turn!", LCD_WHITE, LCD_BLACK, true);
    lcd_flush();    
    if (dualshock_read_button(DUALSHOCK_BUTTON_CROSS))
    {
        last_button = DUALSHOCK_BUTTON_CROSS;
        change_mode(mode_menu);
    }
}

/**
 * Follows a white line.
 *
 */
void mode_follow(void)
{
    if (debounce_button())
    {
        return;
    }

    font_draw_text_small(0, 10, "Line!", LCD_WHITE, LCD_BLACK, true);
    lcd_flush();    
    if (dualshock_read_button(DUALSHOCK_BUTTON_CROSS))
    {
        last_button = DUALSHOCK_BUTTON_CROSS;
        change_mode(mode_menu);
    }
}

/**
 * Drives until it (almost) hits something.
 *
 */
void mode_hunt(void)
{
    if (debounce_button())
    {
        return;
    }

    font_draw_text_small(0, 10, "Hunt!", LCD_WHITE, LCD_BLACK, true);
    lcd_flush();    
    if (dualshock_read_button(DUALSHOCK_BUTTON_CROSS))
    {
        last_button = DUALSHOCK_BUTTON_CROSS;
        change_mode(mode_menu);
    }
}

/**************************************************
* Private Functions
***************************************************/

static void change_mode(mode_function_t new_mode)
{
    lcd_paint_clear_screen();
    mode_current = new_mode;
    mode_first = true;
}

bool select_mode(
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
        /* Spin mode */
        change_mode(mode_follow);
    }
    else if (p_menu_item == &top_menu_items[2])
    {
        /* Spin mode */
        change_mode(mode_hunt);
    }
    else if (p_menu_item == &top_menu_items[3])
    {
        /* Spin mode */
        change_mode(mode_turn);
    }
    else
    {
        /* Go back to menu? */
        printf("Unknown pointer %p\r\n", (const void*) p_menu_item);
        change_mode(mode_menu);
        retval = true;
    }
    return retval;
}

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

