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
* This is an LCD driver for PCD8544 based modules. These are
* typically sold as "Nokia 5110 SPI LCD modules" from Adafruit
* or suchlike. There is also a simulator mode for running
* on a PC, which pushes pixels via a FIFO to a separate
* LCD simulator program.
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
 * Initialise the LCD.
 *
 * This will include GPIO pin directions
 * as required.
 *
 * @param[int] p_filename The path to the device file, if required.
 *                        In simulator mode, this is the FIFO. With
 *                        SPI LCDs, this is the /dev/spidevX.X entry.
 * @return 0 on success, anything else on error
 */
extern int lcd_init(const char* p_filename);

/**
 * De-initialise the LCD.
 *
 * This will reduce power consumption and allows the display
 * to be powered off without back-powering through the IO
 * lines.
 */
extern void lcd_deinit(void);


/**
 * Enables LCD display.
 */
extern void lcd_on(void);


/**
 * Flushes the framebuffer to the LCD.
 */
extern void lcd_flush(void);


/**
 * Paints a solid rectangle to the LCD in the given colour.
 *
 * @param[in] bg the RGB colour for all the pixels
 * @param[in] x1 the starting column
 * @param[in] x2 the end column
 * @param[in] y1 the starting row
 * @param[in] y2 the end row
 */
extern void lcd_paint_fill_rectangle(
    lcd_colour_t bg,
    lcd_col_t x1,
    lcd_col_t x2,
    lcd_row_t y1,
    lcd_row_t y2
);


/**
 * Paint a single pixel.
 *
 * This is a wrapper around a rectangle fill
 * with a 1x1 sized rectangle.
 *
 * @param[in] col the RGB colour for the pixels
 * @param[in] x the column
 * @param[in] y the row
 */
#define lcd_paint_pixel(col, x, y) lcd_paint_fill_rectangle(col, x, x, y, y)


/**
 * Set the entire screen to black.
 *
 * This is a wrapper around a rectangle fill set
 * to the size of the screen.
 *
 */
#define lcd_paint_clear_screen() lcd_paint_fill_rectangle( \
        LCD_BLACK, \
        LCD_FIRST_COLUMN, LCD_LAST_COLUMN, \
        LCD_FIRST_ROW, LCD_LAST_ROW)


/**
 * Paints a mono rectangle to the LCD in the given colours. This is useful for
 * text.
 *
 * @param[in] fg the RGB colour for set pixels
 * @param[in] bg the RGB colour for unset pixels
 * @param[in] x1 the starting column
 * @param[in] x2 the end column
 * @param[in] y1 the starting row
 * @param[in] y2 the end row
 * @param[in] p_pixels 1bpp data for the given rectangle, length (x2-x1+1)*(y2-y1+1) bits
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


#ifdef __cplusplus
}
#endif

#endif /* ndef LCD_H */

/**************************************************
* End of file
***************************************************/

