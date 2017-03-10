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
#include <motor/motor.h>

#include <modes/modes.h>

/**************************************************
* Defines
***************************************************/

#ifdef LCD_SIM
#define LOOPS_PER_SECOND 10
#else
#define LOOPS_PER_SECOND 20
#endif

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
    {"serdev",   required_argument, 0, 's'},
    { 0 }
};

static const char *short_options = "vhj:l:s:";

static const char *sz_jsdev = "/dev/input/js0";
static const char *sz_lcddev = "/dev/spidev0.0";
static const char* sz_serdev = "/dev/ttyAMA0";

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
        printf("OK\r\nInit LCD...\r\n");
        retval = lcd_init(sz_lcddev);
    }

    if (retval == 0)
    {
        printf("OK\r\nInit Motor...\r\n");
        enum motor_status_t st = motor_init(sz_serdev);
        if (st != MOTOR_STATUS_OK)
        {
            retval = -st;
        }
    }

    if (retval == 0)
    {
        const char spinner[] = { '\\', '|', '/', '-'};
        size_t i = 0;
        printf("Verbose mode is %s\n", verbose_flag ? "on" : "off");

        lcd_paint_clear_screen();

        printf("Init Joystick...\r\n");
        do
        {
            char message[10];
            sprintf(message, "Start pad %c", spinner[i]);
            BOUNDS_INCREMENT(i, NUMELTS(spinner), 0);
            font_draw_text_small(2, 10, message, LCD_WHITE, LCD_BLACK, FONT_PROPORTIONAL);
            lcd_flush();
            retval = dualshock_init(sz_jsdev);
            sleep(1);
        } while(retval != 0);
        printf("Init Joystick done!\r\n");
    }


    if (retval == 0)
    {
        struct timeval delay_master = {
            .tv_sec = 0,
            .tv_usec = (1000 * 1000) / LOOPS_PER_SECOND
        };
        struct timeval loop_delay = delay_master;
        bool led_state = 1;
        gpio_io_pin_t led = GPIO_MAKE_IO_PIN(GPIO_PORT_A, 18);
        gpio_make_output(led, 0);
        while(1)
            {
            dualshock_read_or_timeout(&loop_delay);
            if (loop_delay.tv_sec == 0 && loop_delay.tv_usec == 0)
            {
                motor_poll();
                mode_handle();
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

        case 's':
            sz_serdev = optarg;
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
    fprintf(stderr, "    --lcddev / -l <device> - Specifies the /dev/spidevX.X device for the LCD\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    --serdev / -s <device> - Specifies the /dev/ttyXX device for the motor controller\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    --verbose / -v         - Enables more logging\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "    --help / -h            - Shows this help\n");
}

/**************************************************
* End of file
***************************************************/

