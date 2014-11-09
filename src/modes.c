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

/**************************************************
* Includes
***************************************************/

#include "util/util.h"
#include "dualshock/dualshock.h"
#include "menu/menu.h"
#include "font/font.h"
#include "lcd/lcd.h"
#include "modes.h"

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

    if (debounce_button())
    {
        return;
    }

    sprintf(msg, "L:%05d ", dualshock_read_axis(DUALSHOCK_AXIS_LY));
    font_draw_text_small(0, 0, msg, LCD_WHITE, LCD_BLACK, true);
    sprintf(msg, "R:%05d ", dualshock_read_axis(DUALSHOCK_AXIS_RY));
    font_draw_text_small(0, 10, msg, LCD_WHITE, LCD_BLACK, true);
    sprintf(msg, "B:%05d ", dualshock_read_axis(DUALSHOCK_AXIS_R2));
    font_draw_text_small(0, 20, msg, LCD_WHITE, LCD_BLACK, true);
    lcd_flush();    

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
        printf("Unknown pointer %p\r\n", p_menu_item);
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

