/*-----------------------------------------------------------*/
/* MCU SPI drivers
*/
#include "nrf_spi.h"
#include "nrf_gpio.h"
#include "spi.h"
#include "main.h"
/*-----------------------------------------------------------*/


/*-----------------------------------------------------------*/
/*Init SPI0*/
void SPI0Init (nrf_spi_mode_t mode, nrf_spi_bit_order_t order)
{
	/*Ports config*/
	nrf_gpio_cfg_output(Pin_SPI0_MOSI);
	nrf_gpio_pin_set(Pin_SPI0_MOSI); //passive state
	nrf_gpio_cfg_output(Pin_SPI0_SCK);
	if((mode == NRF_SPI_MODE_0) || (mode == NRF_SPI_MODE_1)) nrf_gpio_pin_clear(Pin_SPI0_SCK);
	else nrf_gpio_pin_set(Pin_SPI0_SCK);
	nrf_gpio_cfg_output(Pin_SPI0_SS);
	nrf_gpio_pin_set(Pin_SPI0_SS);
  nrf_gpio_cfg_input(Pin_SPI0_MISO, NRF_GPIO_PIN_NOPULL);
	
	/*SPI config*/
	nrf_spi_pins_set(NRF_SPI0, Pin_SPI0_SCK, Pin_SPI0_MOSI, Pin_SPI0_MISO);
	nrf_spi_frequency_set(NRF_SPI0, NRF_SPI_FREQ_4M);
	nrf_spi_configure(NRF_SPI0, mode, order);
	
	nrf_spi_enable(NRF_SPI0);
}

/*-----------------------------------------------------------*/
/*Read Rx buffer*/
uint8_t TransferSPI (SPITransferMode_TypeDef mode, uint8_t address, uint8_t dataTx)
{ 
  uint8_t data;
	
	nrf_spi_event_clear(NRF_SPI0, NRF_SPI_EVENT_READY);
	
	nrf_gpio_pin_clear(Pin_SPI0_SS);
	nrf_spi_txd_set(NRF_SPI0, address|(mode<<7));
	nrf_spi_txd_set(NRF_SPI0, dataTx);
	
	while (!nrf_spi_event_check(NRF_SPI0, NRF_SPI_EVENT_READY)) {}
	nrf_spi_event_clear(NRF_SPI0, NRF_SPI_EVENT_READY);	
	nrf_spi_rxd_get(NRF_SPI0);
		
	while (!nrf_spi_event_check(NRF_SPI0, NRF_SPI_EVENT_READY)) {}
	nrf_spi_event_clear(NRF_SPI0, NRF_SPI_EVENT_READY);	
	data = nrf_spi_rxd_get(NRF_SPI0);
		
	nrf_gpio_pin_set(Pin_SPI0_SS);
	
	return data;
}
