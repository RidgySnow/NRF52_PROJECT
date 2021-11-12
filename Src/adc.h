#ifndef ADC_H
#define ADC_H

#define SAMPLERATE_MODE_TIMER		(0x01UL << 12)

#define BUFFER_LEN							200

/*SPI transfer mode*/
typedef enum
{
	SPI_READ = 1,
	SPI_WRITE = 0
} SPITransferMode_TypeDef;

/*-----------------------------------------------------------*/
__STATIC_INLINE void nrf_saadc_mode_set(uint32_t mode,
                                        uint16_t cc)
{
    NRF_SAADC->SAMPLERATE = mode | cc;
}

void SAADCInit (void);
#endif
