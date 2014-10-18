/*****************************************************
*
* Pi Wars Robot Software (PWRS)
*
* Copyright (c) 2014 theJPster (pwrs@hejpster.org.uk)
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
*****************************************************/

/**************************************************
* Includes
***************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>

#include <time.h>
#include <getopt.h>

#include <dualshock/dualshock.h>
#include <font/font.h>
#include <gpio/gpio.h>
#include <lcd/lcd.h>
#include <menu/menu_pwrs.h>

/**************************************************
* Defines
***************************************************/

#define LOOPS_PER_SECOND 5

/**************************************************
* Data Types
**************************************************/

/* None */

/**************************************************
* Function Prototypes
**************************************************/

static int process_arguments(int argc, char** argv);
static void print_help(void);

/**************************************************
* Public Data
**************************************************/

int verbose_flag = 0;

/**************************************************
* Private Data
**************************************************/

static struct option long_options[] =
{
    /* These options set a flag. */
    {"verbose", no_argument,       &verbose_flag, 1},
    {"help",    no_argument,       0, 'h'},
    {"jsdev",   required_argument, 0, 'j'},
    {"lcd",     required_argument, 0, 'l'},
    { 0 }
};

static const char *short_options = "vhj:l:";

static char *sz_jsdev = "/dev/input/js0";
static char *sz_lcddev = "/dev/spidev0.0";

/**************************************************
* Public Functions
***************************************************/

/**
 *
 * main
 *
 * Entry point to the PWRS software.
 */
int main(int argc, char **argv)
{
    int retval;

    printf("Welcome to PWRS\r\n");

    gpio_init();

    retval = process_arguments(argc, argv);

    if (retval == 0)
    {
        printf("Verbose mode is %s\n", verbose_flag ? "on" : "off");

        retval = dualshock_init(sz_jsdev);
    }

    if (retval == 0)
    {
        retval = lcd_init(sz_lcddev);
    }

    if (retval == 0)
    {
        struct timeval delay_master = {
            .tv_sec = 0,
            .tv_usec = (1000 * 1000) / LOOPS_PER_SECOND
        };
        struct timeval loop_delay = delay_master;
        //menu_pwrs_init();
        bool led_state = 1;
        gpio_io_pin_t led = GPIO_MAKE_IO_PIN(GPIO_PORT_A, 18);
        gpio_make_output(led, 0);
        while(1)
            {
            dualshock_read_or_timeout(&loop_delay);
            if (loop_delay.tv_sec == 0 && loop_delay.tv_usec == 0)
            {
                #if 0
                printf("DUALSHOCK_AXIS_LX = %d\n", dualshock_read_axis(DUALSHOCK_AXIS_LX));
                printf("DUALSHOCK_AXIS_LY = %d\n", dualshock_read_axis(DUALSHOCK_AXIS_LY));
                printf("DUALSHOCK_AXIS_RX = %d\n", dualshock_read_axis(DUALSHOCK_AXIS_RX));
                printf("DUALSHOCK_AXIS_RY = %d\n", dualshock_read_axis(DUALSHOCK_AXIS_RY));
                printf("DUALSHOCK_AXIS_L1 = %d\n", dualshock_read_axis(DUALSHOCK_AXIS_L1));
                printf("DUALSHOCK_AXIS_L2 = %d\n", dualshock_read_axis(DUALSHOCK_AXIS_L2));
                printf("DUALSHOCK_AXIS_R1 = %d\n", dualshock_read_axis(DUALSHOCK_AXIS_R1));
                printf("DUALSHOCK_AXIS_R2 = %d\n", dualshock_read_axis(DUALSHOCK_AXIS_R2));
                printf("DUALSHOCK_BUTTON_SQUARE = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_SQUARE) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_CIRCLE = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_CIRCLE) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_TRIANGLE = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_TRIANGLE) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_CROSS = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_CROSS) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_PS = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_PS) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_START = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_START) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_SELECT = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_SELECT) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_LEFTSTICK = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_LEFTSTICK) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_RIGHTSTICK = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_RIGHTSTICK) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_UP = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_UP) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_DOWN = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_DOWN) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_LEFT = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_LEFT) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_RIGHT = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_RIGHT) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_L1 = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_L1) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_L2 = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_L2) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_R1 = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_R1) ? "pressed" : "not-pressed");
                printf("DUALSHOCK_BUTTON_R2 = %s\n", dualshock_read_button(DUALSHOCK_BUTTON_R2) ? "pressed" : "not-pressed");
                #endif
                char msg[14];
                sprintf(msg, "l2=0x%04x", dualshock_read_axis(DUALSHOCK_AXIS_L2));
                font_draw_text_small(0, 0, msg, LCD_WHITE, LCD_BLACK, true);
                sprintf(msg, "r2=0x%04x", dualshock_read_axis(DUALSHOCK_AXIS_R2));
                font_draw_text_small(0, 10, msg, LCD_WHITE, LCD_BLACK, true);
                lcd_flush();
                gpio_set_output(led, led_state);
                led_state = !led_state;
                loop_delay = delay_master;
            }
        }
    }

    return retval;
}

/**************************************************
* Private Functions
***************************************************/

static int process_arguments(int argc, char** argv)
{
    int retval = 0;
    while (1)
    {
        /* getopt_long stores the option index here. */
        int option_index = 0;
        int c;

        c = getopt_long(argc, argv, short_options, long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
        case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
            {
                break;
            }
            printf ("option %s", long_options[option_index].name);
            if (optarg)
            {
                printf (" with arg %s", optarg);
            }
            printf ("\n");
            break;

        case 'v':
            verbose_flag = 1;
            break;

        case 'j':
            sz_jsdev = optarg;
            break;

        case 'l':
            sz_lcddev = optarg;
            break;

        case 'h':
            print_help();
            retval = 1;
            break;

        default:
            print_help();
            retval = 1;
            break;
        }
    }

    return retval;
}

static void print_help(void)
{
    fprintf(stderr, "\n");
    fprintf(stderr, "Pi Wars Robot Software\n");
    fprintf(stderr, "======================\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    pwrs <options>\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Options are:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    --jsdev / -j <device>  - Specifies the /dev/event/foo device for the joystick\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    --lcddev / -l <device> - Specifies the /dev/spidev0.0 device for the LCD\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    --verbose / -v         - Enables more logging\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    --help / -h            - Shows this help\n");
}

/**************************************************
* End of file
***************************************************/

