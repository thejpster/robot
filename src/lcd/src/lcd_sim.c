/*****************************************************
*
* Pi Wars Robot Software (PWRS) LCD Driver Simulator
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
* This allows you to develop LCD code on a PC, by pushing
* pixels via a FIFO to a separate rendering program.
*
* The protocol is ASCII, line-based. Co-ordinates are decimal,
* colours are 24-bit RGB hex. The commands available
* are:
*
*   reset
*       - resets the display
*   box <x1> <x2> <y1> <y2> <colour>
*       - draws a box in the specified colour
*   bitmap <x1> <x2> <y1> <y2> <fg-colour> <bg-colour> <bitmap>
*       - draws a bitmap. Each bit in the bitmap is a row-wise pixel.
*         When 1, fg-colour is used, otherwise bg-colour is used.
*   plot <x> <y> <colour>
*       - plots a single pixel in the specified colour
*
*****************************************************/

#ifdef LCD_SIM

/**************************************************
* Includes
***************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <lcd/lcd.h>

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

static FILE* f;

/**************************************************
* Public Functions
***************************************************/

/**
 * Opens the FIFO for writing to the simulated LCD.
 *
 * @param[int] p_filename The path to the FIFO file.
 * @return 0 on success, anything else on error
 */
int lcd_init(const char* p_filename)
{
    printf("LCD device: %s\r\n", p_filename);

    f = fopen(p_filename, "w");

    if (!f)
    {
        perror("Can't open LCD device");
        return -1;
    }

    PRINTF("LCD init...\n");

    fprintf(f, "reset\n");

    lcd_paint_clear_screen();

    lcd_paint_fill_rectangle(LCD_RED,
        LCD_FIRST_COLUMN, LCD_LAST_COLUMN, 0, 20);

    delay_ms(100);

    lcd_paint_fill_rectangle(LCD_BLUE,
        LCD_FIRST_COLUMN, LCD_LAST_COLUMN, 20, 40);

    delay_ms(100);

    lcd_paint_fill_rectangle(LCD_GREEN,
        LCD_FIRST_COLUMN, LCD_LAST_COLUMN, 40, 60);

    delay_ms(100);

    lcd_paint_clear_screen();

    PRINTF("Done!\n");

    return 0;
}

/**
 * Close the fifo.
 */
void lcd_deinit(void)
{
    fclose(f);
    f = NULL;
}

/**
 * Turn the LCD on (actually, does nothing).
 */
void lcd_on(void)
{
    /* Nothing */
}


/**
 * Flushes the framebuffer to the LCD.
 *
 * Actually, this does nothing as this isn't a framebuffer.
 */
void lcd_flush(void)
{
    /* Nothing */
}

/**
 * Paints a solid rectangle to the LCD in the given colour.
 *
 * @param[in] bg the RGB colour for all the pixels
 * @param[in] x1 the starting column
 * @param[in] x2 the end column
 * @param[in] y1 the starting row
 * @param[in] y2 the end row
 */
void lcd_paint_fill_rectangle(
    uint32_t bg,
    lcd_col_t x1,
    lcd_col_t x2,
    lcd_row_t y1,
    lcd_row_t y2
)
{
    fprintf(f, "box %d %d %d %d 0x%06"PRIx32"\n", x1, x2, y1, y2, bg);
    fflush(f);
}

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
void lcd_paint_mono_rectangle(
    uint32_t fg,
    uint32_t bg,
    lcd_col_t x1,
    lcd_col_t x2,
    lcd_row_t y1,
    lcd_row_t y2,
    const uint8_t *p_pixels
)
{
    fprintf(f, "bitmap %d %d %d %d 0x%06"PRIx32" 0x%06"PRIx32" ", x1, x2, y1, y2, fg, bg);
    size_t size = (1 + x2 - x1) * (1 + y2 - y1);
    size_t bytes = (size + 7) / 8;
    for(size_t i = 0; i < bytes; i++)
    {
        fprintf(f, "%02X", p_pixels[i]);
    }
    fprintf(f, "\n");
    fflush(f);
}


/**************************************************
* Private Functions
***************************************************/

/* None */


/**************************************************
* Placeholder
***************************************************/

#else

/* We get warnings if this file is empty. Shut them up. */
extern void lcd_sim_cant_be_empty(void);

#endif

/**************************************************
* End of file
***************************************************/
