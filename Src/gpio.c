
#include <stdint.h>
#include <stdbool.h>
#include "nrf.h"
#include "nrf_erratas.h"
#include "nrf_gpio.h"
#include "gpio.h"

void GPIOInit (void)
{
	nrf_gpio_range_cfg_output(PIN_17, PIN_21);
	nrf_gpio_cfg_input (PIN_13, NRF_GPIO_PIN_PULLUP);
	//nrf_gpio_cfg_input (PIN_14, NRF_GPIO_PIN_PULLUP);	
	/*nrf_gpio_pin_write(PIN_17, 1);
	nrf_gpio_pin_write(PIN_18, 1);
	nrf_gpio_pin_write(PIN_19, 1);
	nrf_gpio_pin_write(PIN_20, 1);
	*/
}

void TestPinSet(void)
{
	nrf_gpio_pin_set(PIN_21);
}

void TestPinReset(void)
{
	nrf_gpio_pin_clear(PIN_21);
}

