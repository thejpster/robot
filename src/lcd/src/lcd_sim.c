/*****************************************************
*
* Stellaris Launchpad LCD Driver Simulator
*
* Copyright (c) 2014 theJPster (www.thejpster.org.uk)
*
* Simulates an LCD by writing primitives to a FIFO.
* There is a Python application (lcd_render.py) which
* picks up these primitives and renders them to a 
* window.
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

static void pixel_fn(int x, int y, uint32_t col);

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
 * Will set up the GPIO for driving the LCD.
 *
 * See header file for pinout.
 *
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

void lcd_deinit(void)
{
    /* Nothing */
}

void lcd_on(void)
{
    /* Nothing */
}

void lcd_off(void)
{
    /* Nothing */
}

void lcd_set_backlight(uint8_t brightness)
{
    /* Nothing */
}

/**
 * Paints a solid rectangle to the LCD in the given colour.
 *
 * @param bg the RGB colour for all the pixels
 * @param x1 the starting column
 * @param x2 the end column
 * @param y1 the starting row
 * @param y2 the end row
 */
void lcd_paint_fill_rectangle(
    uint32_t bg,
    lcd_col_t x1,
    lcd_col_t x2,
    lcd_row_t y1,
    lcd_row_t y2
)
{
    fprintf(f, "box %d %d %d %d 0x%06"PRIu32"\n", x1, x2, y1, y2, bg);
    fflush(f);
}

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
    fprintf(f, "bitmap %d %d %d %d 0x%06"PRIu32" 0x%06"PRIu32" ", x1, x2, y1, y2, fg, bg);
    size_t size = (1 + x2 - x1) * (1 + y2 - y1);
    size_t bytes = (size + 7) / 8;
    for(size_t i = 0; i < bytes; i++)
    {
        fprintf(f, "%02X", p_pixels[i]);
    }        
    fprintf(f, "\n");
    fflush(f);
}

/**
 * Paints a full-colour rectangle to the LCD. This is useful for graphics but
 * you need up to 510KB for a full-screen image.
 *
 * @param x1 the starting column
 * @param x2 the end column
 * @param y1 the starting row
 * @param y2 the end row
 * @param p_pixels Run-length encoded pixel values, where the count is in the top byte
 */
void lcd_paint_colour_rectangle(
    lcd_col_t x1,
    lcd_col_t x2,
    lcd_row_t y1,
    lcd_row_t y2,
    const uint32_t *p_rle_pixels
)
{
    size_t size = (1 + x2 - x1) * (1 + y2 - y1);
    lcd_col_t x = x1;
    lcd_row_t y = y1;
    while (size)
    {
        uint32_t pixel = *p_rle_pixels;
        uint8_t count = (pixel >> 24) & 0xFF;
        size -= count;
        while (count--)
        {
            pixel_fn(x, y, pixel & 0xFFFFFF);
            if (x == x2)
            {
                x = x1;
                y++;
            }
            else
            {
                x++;
            }
        }
        p_rle_pixels++;
    }
}

/**************************************************
* Private Functions
***************************************************/

static void pixel_fn(int x, int y, uint32_t colour)
{
    fprintf(f, "plot %d %d 0x%06"PRIu32"\n", x, y, colour);
    fflush(f);
}


#endif

/**************************************************
* End of file
***************************************************/

