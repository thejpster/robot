/*****************************************************
*
* Stellaris Launchpad LCD Driver
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
* This is an LCD driver for PCD8544 based modules. These are
* typically sold as "Nokia 5110 SPI LCD modules" from Adafruit
* or suchlike.
*
*****************************************************/

#ifndef LCD_SIM

/**************************************************
* Includes
***************************************************/

#include <fcntl.h>
#include <inttypes.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <gpio/gpio.h>
#include <lcd/lcd.h>

/**************************************************
* Defines
***************************************************/

#define LCD_DC_PIN GPIO_MAKE_IO_PIN(0, 23)
#define LCD_RST_PIN GPIO_MAKE_IO_PIN(0, 24)

#define STRIPE_SIZE                 8
#define NUM_STRIPES                 6

/* These commands work in either mode */
#define PCD8544_NOP                 0x00
#define PCD8544_FUNCTIONSET         0x20

/* Options for FUNCTIONSET command */
#define PCD8544_POWERDOWN           0x04 /* Power-down screen */
#define PCD8544_VERTICALMODE        0x02 /* Use vertical addressing */
#define PCD8544_EXTENDEDINSTRUCTION 0x01 /* Enable extended mode */

/* These are normal mode commands */
#define PCD8544_SETYADDR            0x40 /* Add Y = 0..5 */
#define PCD8544_SETXADDR            0x80 /* Add X = 0..83 */
#define PCD8544_DISPLAYCONTROL      0x08 /* Add one of the following... */

/* Options for DISPLAYCONTROL */
#define PCD8544_DISPLAYBLANK        0x0
#define PCD8544_DISPLAYNORMAL       0x4
#define PCD8544_DISPLAYALLON        0x1
#define PCD8544_DISPLAYINVERTED     0x5

/* These are extended mode commands */
#define PCD8544_SETTEMP             0x04 /* Add 0..3 to set temp co-efficient */
#define PCD8544_SETBIAS             0x10 /* Add 0..7 to set bias */
#define PCD8544_SETVOP              0x80 /* Add 0..0x7F to set contrast */

#define COMMAND true
#define DATA false

/* Calculates which byte in the framebuffer holds the given pixel */
#define CALC_OFFSET(x, y) ((x) + (((y)/8)*LCD_WIDTH))

#define SET_PIXEL(x, y) do { \
        size_t offset = CALC_OFFSET(x,y); \
        /* printf("Setting %u,%u @ %zu\r\n", x, y, offset);*/ \
        if (offset > sizeof(frame_buffer)) \
        { \
            abort(); \
        } \
        frame_buffer[offset] |= 1 << (((y)&7)); \
    } while(0)
#define CLR_PIXEL(x, y) do { \
        size_t offset = CALC_OFFSET(x,y); \
        /*printf("Clearing %u,%u @ %zu\r\n", x, y, offset); */ \
        if (offset > sizeof(frame_buffer)) \
        { \
            abort(); \
        } \
        frame_buffer[offset] &= ~(1 << (((y)&7))); \
    } while(0)

/**************************************************
* Data Types
**************************************************/

/* None */

/**************************************************
* Function Prototypes
**************************************************/

static void write_lcd(
    const uint8_t* p_data,
    size_t data_len,
    bool is_command
);

static void set_normal(void);
static void extended_command(uint8_t command);
static void set_bias(uint8_t bias);
static void set_contrast(uint8_t constrast);

static void paint_lcd(void);

/**************************************************
* Public Data
**************************************************/

/* None */

/**************************************************
* Private Data
**************************************************/

static int spi_fd;
static uint8_t spi_mode = SPI_MODE_0;
static uint8_t spi_bpw = 8;
static uint32_t spi_speed = 1000000;

static uint8_t frame_buffer[NUM_STRIPES * LCD_WIDTH];
/* Force a repaint */
static unsigned int top_stripe = NUM_STRIPES;
static unsigned int bottom_stripe = 0;

/**************************************************
* Public Functions
***************************************************/

/**
 * Will set up the GPIO and SPI for driving the LCD.
 *
 * See header file for pinout.
 *
 * @param p_filename Path to an spi device, e.g. /dev/spidev0.0
 * @return 0 for success, else failure
 */
int lcd_init(const char *p_filename)
{
    int retval = 0;

    spi_fd = open(p_filename, O_RDWR);
    if (spi_fd < 0)
    {
        perror("Can't open SPI device");
        retval = 1;
    }
    else if (ioctl(spi_fd, SPI_IOC_WR_MODE, &spi_mode) < 0)
    {
        printf("Can't set SPI mode to %u\r\n", spi_mode);
        retval = 1 ;
    }
    else if (ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bpw) < 0)
    {
        printf("Can't set SPI bit len to %u\r\n", spi_bpw);
        retval = 1 ;
    }
    else if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed) < 0)
    {
        printf("Can't set SPI speed to %"PRIu32"\r\n", spi_speed);
        retval = 1 ;
    }

    if (retval == 0)
    {
        gpio_make_output(LCD_DC_PIN, 0);

        gpio_make_output(LCD_RST_PIN, 0);
        gpio_set_output(LCD_RST_PIN, 0);
        delay_ms(100);
        gpio_set_output(LCD_RST_PIN, 1);

        set_bias(4);
        set_contrast(60);
        set_normal();
    }

    return retval;
}

/* Make all pins inputs */
void lcd_deinit(void)
{
    gpio_set_output(LCD_RST_PIN, 0);
    close(spi_fd);
}

/**
 * Flushes the framebuffer to the LCD.
 */
void lcd_flush(void)
{
    paint_lcd();
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
    if (bg == LCD_BLACK)
    {
        if ((y1 == LCD_FIRST_ROW) && (y2 == LCD_LAST_ROW) && (x1 == LCD_FIRST_COLUMN) && (x2 == LCD_LAST_COLUMN))
        {
            memset(frame_buffer, 0, sizeof(frame_buffer));
        }
        else
        {
            for(lcd_col_t x = x1; x <= x2; x++)
            {
                for(lcd_row_t y = y1; y <= y2; y++)
                {
                    CLR_PIXEL(x, y);
                }
            }
        }
    }
    else
    {
        if ((y1 == LCD_FIRST_ROW) && (y2 == LCD_LAST_ROW) && (x1 == LCD_FIRST_COLUMN) && (x2 == LCD_LAST_COLUMN))
        {
            memset(frame_buffer, 0xFF, sizeof(frame_buffer));
        }
        else
        {
            for(lcd_col_t x = x1; x <= x2; x++)
            {
                for(lcd_row_t y = y1; y <= y2; y++)
                {
                    SET_PIXEL(x, y);
                }
            }
        }

    }
    top_stripe = MIN(top_stripe, (y1 / STRIPE_SIZE)); 
    bottom_stripe = MAX(top_stripe, (y2 / STRIPE_SIZE)); 
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
    uint8_t pixel = *p_pixels++;
    uint8_t mask = 0x80;
    for(lcd_row_t y = y1; y <= y2; y++)
    {
        for(lcd_col_t x = x1; x <= x2; x++)
        {
            if (pixel & mask)
            {
                SET_PIXEL(x, y);
            }
            else
            {
                CLR_PIXEL(x, y);
            }
            mask >>= 1;
            if (mask == 0)
            {
                pixel = *p_pixels++;
                mask = 0x80;
            }
        }
    }
    top_stripe = MIN(top_stripe, (y1 / STRIPE_SIZE)); 
    bottom_stripe = MAX(top_stripe, (y2 / STRIPE_SIZE)); 
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
    /* Not implemented */
}

/**************************************************
* Private Functions
***************************************************/

/*
 * Write some data to the LCD
 *
 * @param p_data The bytes to write
 * @param data_len How many bytes to write
 * @param is_command COMMAND for commands, DATA for data
 */
static void write_lcd(
    const uint8_t* p_data,
    size_t data_len,
    bool is_command
    )
{
    gpio_set_output(LCD_DC_PIN, (is_command == COMMAND) ? 0 : 1);
    int ret = write(spi_fd, p_data, data_len);
    if (ret < 1)
    {
        printf("Error writing to SPI %d\r\n", ret);
    }
}

/*
 * Enable the display, non inverted.
 */
static void set_normal(void)
{
    uint8_t data[] = {
        PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYNORMAL
    };
    write_lcd(data, NUMELTS(data), COMMAND);
}

/*
 * Send an extended command, by entering extended mode
 * and then leaving it again afterwards.
 *
 * @param command The command to send (0x00..0xFF)
 */
static void extended_command(uint8_t command)
{
    uint8_t data[] = {
        PCD8544_FUNCTIONSET | PCD8544_EXTENDEDINSTRUCTION,
        command,
        PCD8544_FUNCTIONSET,
    };
    write_lcd(data, NUMELTS(data), COMMAND);
}

/*
 * Set the "bias system". 4 is a good figure (1:34).
 *
 * @param bias 0 (1:100) to 7 (1:9)
 */
static void set_bias(uint8_t bias)
{
    extended_command(PCD8544_SETBIAS | bias);
}

/*
 * Set the display contrast. 60 is a good figure.
 *
 * @param contrast 0x00..0x7F (or 127)
 */
static void set_contrast(uint8_t contrast)
{
    if (contrast > 0x7F)
    {
        contrast = 0x7F;
    }
    extended_command(PCD8544_SETVOP | contrast);
}

/*
 * Paints the whole framebuffer in to the LCD
 */
static void paint_lcd(void)
{
    uint8_t cmd[2] = { PCD8544_SETYADDR | 0, PCD8544_SETXADDR | 0 };
    write_lcd(cmd, NUMELTS(cmd), COMMAND);
    write_lcd(frame_buffer + (top_stripe*LCD_WIDTH), LCD_WIDTH * (1 + bottom_stripe - top_stripe), DATA);
    //bottom_stripe = 0;
    //top_stripe = NUM_STRIPES;
    //memset(frame_buffer, 0xFF, sizeof(frame_buffer));
    //write_lcd(frame_buffer, NUMELTS(frame_buffer), DATA);
}

#endif

/**************************************************
* End of file
***************************************************/

