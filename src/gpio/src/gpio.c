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

#include <stdlib.h>

#ifdef USE_WIRINGPI
#include <wiringPi.h>
#endif

#include "../gpio.h"

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

/* None */

/**************************************************
* Public Functions
***************************************************/

/*
 * Initialise GPIO subsystem
 */
void gpio_init(void)
{
#ifdef USE_WIRINGPI
    /* Use GPIO numbering */
    wiringPiSetupGpio();
#endif
}

/*
 * Set pin as output (low or high)
 */
void gpio_make_output(gpio_io_pin_t pin, int level)
{
#ifdef USE_WIRINGPI
    digitalWrite(GPIO_GET_PIN(pin), level);
    pinMode(GPIO_GET_PIN(pin), OUTPUT);
#else
    printf("gpio_make_output(pin=%u, level=%d)\n", pin, level);
#endif
}

/*
 * Set pin as input
 */
void gpio_make_input(gpio_io_pin_t pin)
{
#ifdef USE_WIRINGPI
    pinMode(GPIO_GET_PIN(pin), INPUT);
#else
    printf("gpio_make_input(pin=%u)\n", pin);
#endif
}

/*
 * Set pin as input with a pullup.
 */
void gpio_make_input_pullup(gpio_io_pin_t pin)
{
#ifdef USE_WIRINGPI
    pinMode(GPIO_GET_PIN(pin), INPUT);
    pullUpDnControl(GPIO_GET_PIN(pin), PUD_UP);
#else
    printf("gpio_make_input_pullup(pin=%u)\n", pin);
#endif
}

/*
 * If a pin is already an output, set the level
 */
void gpio_set_output(gpio_io_pin_t pin, int level)
{
#ifdef USE_WIRINGPI
    digitalWrite(GPIO_GET_PIN(pin), level ? 1 : 0);
#else
    printf("gpio_set_outputs(pin=%u, level=%u)\n", pin, level);
#endif
}

/*
 * Set multiple output pins on a port.
 */
void gpio_set_outputs(gpio_port_t port, uint32_t outputs, uint32_t mask)
{
#ifdef USE_WIRINGPI
    gpio_io_pin_t pin = 0;
    while(mask)
    {
        if (mask & 1)
        {
            digitalWrite(pin, outputs & 1);
        }
        mask >>= 1;
        outputs >>= 1;
        pin++;
    }
#else
    printf("gpio_set_outputs(port=%u, outputs=0x%02x, mask=0x%02x)\n", port, outputs, mask);
#endif
}

/*
 * If a pin is already an input, read the level. 0 for low, 1 for high.
 */
int gpio_read_input(gpio_io_pin_t pin)
{
#ifdef USE_WIRINGPI
    return digitalRead(pin);
#else
    return 0;
#endif
}

/*
 * If a many pins on a port are already inputs, read the levels. 0 for low, 1 for high.
 */
uint32_t gpio_read_inputs(gpio_port_t port, uint32_t mask)
{
#ifdef USE_WIRINGPI
    uint32_t result = 0;
    gpio_io_pin_t pin = 0;
    while(mask)
    {
        result <<= 1;
        if (mask & 0x80000000)
        {
            result |= digitalRead(pin);
        }
        mask <<= 1;
        pin++;
    }
#else
    return 0;
#endif
}

/**************************************************
* Private Functions
***************************************************/

/* None */

/**************************************************
* End of file
***************************************************/

