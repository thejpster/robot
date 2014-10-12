/*****************************************************
*
* Stellaris Launchpad LCD Driver
*
* Driver suits SSD1961 with self-initialisation, e.g.
* DisplayTech INT043BTFT.
*
* See https://www.displaytech-us.com/4-3-inch-integrated-tft-driver-boards
*
* Copyright (c) 2013-2014 theJPster (www.thejpster.org.uk)
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

#ifndef LCD_H
#define LCD_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************
* Includes
***************************************************/

#include "util/util.h"

/**************************************************
* Public Defines
***************************************************/

/* r, b and b should be 0x00..0xFF */
#define MAKE_COLOUR(r, g, b) ( ((r) << 16) | ((g) << 8) | ((b) << 0) )
/* count should also be 0x00..0xFF */
#define MAKE_RLE_COLOUR(count, r, g, b) ( ((count) << 24) | ((r) << 16) | ((g) << 8) | ((b) << 0) )

/* Colours */
#define LCD_BLACK       MAKE_COLOUR(0x00, 0x00, 0x00)
#define LCD_RED         MAKE_COLOUR(0xFF, 0x00, 0x00)
#define LCD_GREEN       MAKE_COLOUR(0x00, 0xFF, 0x00)
#define LCD_BLUE        MAKE_COLOUR(0x00, 0x00, 0xFF)
#define LCD_YELLOW      MAKE_COLOUR(0xFF, 0xFF, 0x00)
#define LCD_CYAN        MAKE_COLOUR(0x00, 0xFF, 0xFF)
#define LCD_MAGENTA     MAKE_COLOUR(0xFF, 0x00, 0xFF)
#define LCD_WHITE       MAKE_COLOUR(0xFF, 0xFF, 0xFF)

#define LCD_RED_DIM     MAKE_COLOUR(0x80, 0x00, 0x00)
#define LCD_GREEN_DIM   MAKE_COLOUR(0x00, 0x80, 0x00)
#define LCD_BLUE_DIM    MAKE_COLOUR(0x00, 0x00, 0x80)
#define LCD_YELLOW_DIM  MAKE_COLOUR(0x80, 0x80, 0x00)
#define LCD_CYAN_DIM    MAKE_COLOUR(0x00, 0x80, 0x80)
#define LCD_MAGENTA_DIM MAKE_COLOUR(0x80, 0x00, 0x80)
#define LCD_GREY        MAKE_COLOUR(0x80, 0x80, 0x80)

/* LCD sizes */
#define LCD_WIDTH 84
#define LCD_HEIGHT 48

#define LCD_FIRST_COLUMN 0
#define LCD_FIRST_ROW 0
#ifdef LCD_ROTATE_DISPLAY
#define LCD_LAST_COLUMN (LCD_HEIGHT-1)
#define LCD_LAST_ROW (LCD_WIDTH-1)
#else
#define LCD_LAST_COLUMN (LCD_WIDTH-1)
#define LCD_LAST_ROW (LCD_HEIGHT-1)
#endif

/**************************************************
* Public Data Types
**************************************************/

typedef unsigned int lcd_row_t;
typedef unsigned int lcd_col_t;

/* Stores NRGB where N is an RLE number of pixels (optional) */
typedef uint32_t lcd_colour_t;

/**************************************************
* Public Data
**************************************************/

/* None */

/**************************************************
* Public Function Prototypes
***************************************************/

/**
 * Will set up the GPIO for driving the LCD.
 */
extern int lcd_init(const char* p_filename);

/* Make all pins inputs */
extern void lcd_deinit(void);

/**
 * Enables LCD display.
 */
extern void lcd_on(void);

/**
 * Disables LCD display.
 */
extern void lcd_off(void);

/**
 * Sets the LCD backlight brightness.
 * @param brightness 0xFF is maximum brightness, 0x00 is minimum.
 */
extern void lcd_set_backlight(uint8_t brightness);

/**
 * Paints a solid rectangle to the LCD in the given colour.
 *
 * @param bg the RGB colour for all the pixels
 * @param x1 the starting column
 * @param x2 the end column
 * @param y1 the starting row
 * @param y2 the end row
 */
extern void lcd_paint_fill_rectangle(
    lcd_colour_t bg,
    lcd_col_t x1,
    lcd_col_t x2,
    lcd_row_t y1,
    lcd_row_t y2
);

#define lcd_paint_pixel(col, x, y) lcd_paint_fill_rectangle(col, x, x, y, y)

#define lcd_paint_clear_screen() lcd_paint_fill_rectangle( \
        LCD_BLACK, \
        LCD_FIRST_COLUMN, LCD_LAST_COLUMN, \
        LCD_FIRST_ROW, LCD_LAST_ROW)

/**
 * Paints a mono rectangle to the LCD in the given colours. This is useful for
 * text.
 *
 * @param fg the RGB colour for set pixels
 * @param bg the RGB colour for unset pixels
 * @param x1 the starting column
 * @param x2 the end column
 * @param y1 the starting row
 * @param y2 the end row
 * @param p_pixels 1bpp data for the given rectangle, length (x2-x1+1)*(y2-y1+1) bits
 */
extern void lcd_paint_mono_rectangle(
    lcd_colour_t fg,
    lcd_colour_t bg,
    lcd_col_t x1,
    lcd_col_t x2,
    lcd_row_t y1,
    lcd_row_t y2,
    const uint8_t *p_pixels
);

/**
 * Paints a full-colour rectangle to the LCD. This is useful for graphics but
 * you need 510KB for a full-screen image.
 *
 * @param x1 the starting column
 * @param x2 the end column
 * @param y1 the starting row
 * @param y2 the end row
 * @param p_pixels Run-length encoded pixel values, where the count is in the top byte
 */
extern void lcd_paint_colour_rectangle(
    lcd_col_t x1,
    lcd_col_t x2,
    lcd_row_t y1,
    lcd_row_t y2,
    const lcd_colour_t *p_rle_pixels
);

extern void lcd_read_color_rectangle(
    lcd_col_t x1,
    lcd_col_t x2,
    lcd_row_t y1,
    lcd_row_t y2,
    lcd_colour_t *p_rle_pixels,
    size_t pixel_len
);

#ifdef __cplusplus
}
#endif

#endif /* ndef LCD_H */

/**************************************************
* End of file
***************************************************/

