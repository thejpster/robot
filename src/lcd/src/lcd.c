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
* or suchlike.
*
* The screen is 84 pixels across by 48 pixels high.
* The 48 pixels are arranged as six stripes of eight pixels.
* Each vertical column in a stripe is one byte, with the MSB
* the uppermost pixel. So, a full screen is 84 x 6 bytes.
*
* For performance reasons, this implementation maintains
* an internal frame buffer which is blitted to the
* screen on flush.
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

#define DEFAULT_BIAS 4
#define DEFAULT_CONTRAST 60

#define LCD_DC_PIN GPIO_MAKE_IO_PIN(0, 25)
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

/* Which which stripe a row value is in, from 0..NUM_STRIPES */
#define FIND_STRIPE(y) ((y)/STRIPE_SIZE)

/* Calculates which byte in the framebuffer holds the given pixel */
#define CALC_OFFSET(x, y) ((x) + (FIND_STRIPE(y)*LCD_WIDTH))

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

static void write_lcd(const uint8_t *p_data, size_t data_len, bool is_command);

static void set_normal(void);
static void extended_command(uint8_t command);
static void set_bias(uint8_t bias);
static void set_contrast(uint8_t constrast);
static void damage_rows(lcd_row_t y1, lcd_row_t y2);
static void clear_damage(void);

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

static uint8_t frame_buffer[NUM_STRIPES *LCD_WIDTH];
/* We paint only the damaged stripes (which to start off
 * with, is none of them) */
static unsigned int top_stripe;
static unsigned int bottom_stripe;

/**************************************************
* Public Functions
***************************************************/

/**
 * Initialises the Nokia 5110 LCD.
 *
 * This will set up the GPIO and SPI for driving the LCD
 * and do a screen reset.
 *
 * @param[int] p_filename The path to the device file. In
 *                        this driver, this is the /dev/spidevX.X entry.
 * @return 0 on success, anything else on error
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
        delay_ms(100);
        gpio_set_output(LCD_RST_PIN, 1);

        set_bias(DEFAULT_BIAS);
        set_contrast(DEFAULT_CONTRAST);
        set_normal();
    }

    clear_damage();
    return retval;
}


/**
 * De-initialise the LCD.
 *
 * This will reduce power consumption and allows the display
 * to be powered off without back-powering through the IO
 * lines.
 */
void lcd_deinit(void)
{
    gpio_set_output(LCD_RST_PIN, 0);
    gpio_make_input(LCD_DC_PIN);
    close(spi_fd);
}

/**
 * Flushes the framebuffer to the LCD.
 *
 * We only flush the damaged portion, marked by top_stripe and bottom_stripe.
 * These values are then reset to indicate that none of the buffer is damaged.
 */
void lcd_flush(void)
{
    uint8_t cmd[2] = { PCD8544_SETYADDR | 0, PCD8544_SETXADDR | 0 };
    write_lcd(cmd, NUMELTS(cmd), COMMAND);
    write_lcd(frame_buffer + (top_stripe * LCD_WIDTH), LCD_WIDTH * (1 + bottom_stripe - top_stripe), DATA);
    clear_damage();
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
    if (bg == LCD_BLACK)
    {
        if ((y1 == LCD_FIRST_ROW) && (y2 == LCD_LAST_ROW) && (x1 == LCD_FIRST_COLUMN) && (x2 == LCD_LAST_COLUMN))
        {
            /* Cheat in the common case */
            memset(frame_buffer, 0, sizeof(frame_buffer));
        }
        else
        {
            /* We could optimise this if required, paying attention to stripes */
            for (lcd_col_t x = x1; x <= x2; x++)
            {
                for (lcd_row_t y = y1; y <= y2; y++)
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
            /* Cheat in the common case */
            memset(frame_buffer, 0xFF, sizeof(frame_buffer));
        }
        else
        {
            /* We could optimise this if required, paying attention to stripes */
            for (lcd_col_t x = x1; x <= x2; x++)
            {
                for (lcd_row_t y = y1; y <= y2; y++)
                {
                    SET_PIXEL(x, y);
                }
            }
        }

    }
    damage_rows(y1, y2);
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
    uint8_t pixel = *p_pixels++;
    uint8_t mask = 0x80;
    for (lcd_row_t y = y1; y <= y2; y++)
    {
        for (lcd_col_t x = x1; x <= x2; x++)
        {
            bool pixel_set = (pixel & mask);
            if (bg != LCD_BLACK) 
            {
                pixel_set = !pixel_set;
            }
            if (pixel_set)
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
    damage_rows(y1, y2);
}


/**************************************************
* Private Functions
***************************************************/

/*
 * Write some data to the LCD
 *
 * @param[in] p_data The bytes to write
 * @param[in] data_len How many bytes to write
 * @param[in] is_command COMMAND for commands, DATA for data
 */
static void write_lcd(
    const uint8_t *p_data,
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


/**
 * Enable the display. Use non inverted mode.
 */
static void set_normal(void)
{
    uint8_t data[] =
    {
        PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYNORMAL
    };
    write_lcd(data, NUMELTS(data), COMMAND);
}

/**
 * Send an extended command. Enter extended mode
 * and then leave it again afterwards.
 *
 * @param[in] command The command to send (0x00..0xFF)
 */
static void extended_command(uint8_t command)
{
    uint8_t data[] =
    {
        PCD8544_FUNCTIONSET | PCD8544_EXTENDEDINSTRUCTION,
        command,
        PCD8544_FUNCTIONSET,
    };
    write_lcd(data, NUMELTS(data), COMMAND);
}


/**
 * Set the "bias system". 4 is a good figure (1:34).
 *
 * @param[in] bias 0 (1:100) to 7 (1:9)
 */
static void set_bias(uint8_t bias)
{
    extended_command(PCD8544_SETBIAS | bias);
}


/**
 * Set the display contrast. 60 is a good figure.
 *
 * @param[in] contrast 0x00..0x7F (or 127)
 */
static void set_contrast(uint8_t contrast)
{
    if (contrast > 0x7F)
    {
        contrast = 0x7F;
    }
    extended_command(PCD8544_SETVOP | contrast);
}


/**
 * Marks the specified rows as damaged.
 *
 * @param[in] y1 The uppermost row
 * @param[in] y2 The bottommost row
 */
static void damage_rows(lcd_row_t y1, lcd_row_t y2)
{
    top_stripe = MIN(top_stripe, FIND_STRIPE(y1));
    bottom_stripe = MAX(bottom_stripe, FIND_STRIPE(y2));
}

/**
 * Marks no rows as damaged.
 */
static void clear_damage(void)
{
    /* Off the scale values. */
    bottom_stripe = 0;
    top_stripe = NUM_STRIPES;
}

/**************************************************
* Placeholder
***************************************************/

#else

/* We get warnings if this file is empty. Shut them up. */
extern void lcd_cant_be_empty(void);

#endif

/**************************************************
* End of file
***************************************************/

