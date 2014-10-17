/*****************************************************
*
* Taken from the Stellaris Launchpad Example Project
*
* Setup GPIOs for the Launchpad board
*
* Copyright (c) 2012-2013 theJPster (www.thejpster.org.uk)
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

#ifndef GPIO_GPIO_H
#define GPIO_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************
* Includes
***************************************************/

#include <util/util.h>

/**************************************************
* Public Defines
***************************************************/

/* Macros to pack a port and pin into one value */
/* Raspberry Pi only has one 'port' */
#define GPIO_MAKE_IO_PIN(port, pin)  ( (gpio_io_pin_t) (1U<<(pin)) )
#define GPIO_GET_PORT(io_pin)  (0)
#define GPIO_GET_PIN(io_pin)  (io_pin)

/**************************************************
* Public Data Types
**************************************************/

typedef enum gpio_port_t
{
    GPIO_PORT_A
} gpio_port_t;

#define GPIO_NUM_PORTS (GPIO_PORT_A + 1)

typedef int gpio_io_pin_t;

typedef enum gpio_interrupt_mode_t
{
    GPIO_INTERRUPT_MODE_RISING,
    GPIO_INTERRUPT_MODE_FALLING,
    GPIO_INTERRUPT_MODE_BOTH
} gpio_interrupt_mode_t;

/*
 * Functions matching this prototype can be registered and called when
 * timer interrupts fire.
 */
typedef void (*gpio_interrupt_handler_t)(gpio_io_pin_t pin, void* p_context, uint32_t n_context);

/**************************************************
* Public Data
**************************************************/

/* None */

/**************************************************
* Public Function Prototypes
***************************************************/

/*
 * Initialise GPIO subsystem
 */
extern void gpio_init(void);

/*
 * Set pin as output (low or high)
 */
extern void gpio_make_output(gpio_io_pin_t pin, int level);

/*
 * Set pin as input
 */
extern void gpio_make_input(gpio_io_pin_t pin);

/*
 * Set pin as input with a pullup.
 */
extern void gpio_make_input_pullup(gpio_io_pin_t pin);

/*
 * If a pin is already an output, set the level
 */
extern void gpio_set_output(gpio_io_pin_t pin, int level);

/*
 * Set multiple output pins on a port.
 */
extern void gpio_set_outputs(gpio_port_t port, uint32_t outputs, uint32_t mask);

/*
 * If a pin is already an input, read the level. 0 for low, 1 for high.
 */
extern int gpio_read_input(gpio_io_pin_t pin);

/*
 * If a many pins on a port are already inputs, read the levels. 0 for low, 1 for high.
 */
extern uint32_t gpio_read_inputs(gpio_port_t port, uint32_t mask);

#ifdef __cplusplus
}
#endif

#endif /* ndef GPIO_GPIO_H */

/**************************************************
* End of file
***************************************************/

