#ifndef SPI_H
#define SPI_H

/*SPI transfer mode*/
typedef enum
{
	SPI_READ = 1,
	SPI_WRITE = 0
} SPITransferMode_TypeDef;

/*-----------------------------------------------------------*/
void SPI0Init (nrf_spi_mode_t mode, nrf_spi_bit_order_t order);
uint8_t TransferSPI (SPITransferMode_TypeDef mode, uint8_t address, uint8_t dataTx);
#endif
