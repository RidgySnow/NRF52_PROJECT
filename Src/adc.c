/*-----------------------------------------------------------*/
/* MCU ADC drivers
*/
#include "nrf_saadc.h"
#include "nrf_gpio.h"
#include "adc.h"
#include "main.h"
/*-----------------------------------------------------------*/

int16_t BufferADC1[BUFFER_LEN];
int16_t BufferADC2[BUFFER_LEN];

const nrf_saadc_channel_config_t saadcChConfig =
{
	.resistor_p = NRF_SAADC_RESISTOR_DISABLED,
	.resistor_n = NRF_SAADC_RESISTOR_DISABLED,
	.gain = NRF_SAADC_GAIN1_3,
	.reference = NRF_SAADC_REFERENCE_INTERNAL,
	.acq_time = NRF_SAADC_ACQTIME_10US,
	.mode = NRF_SAADC_MODE_SINGLE_ENDED,
	.pin_p = (nrf_saadc_input_t) (Pin_ANALOG - 1),
	.pin_n = NRF_SAADC_INPUT_DISABLED};

/*-----------------------------------------------------------*/
/*Init SAADC*/
void SAADCInit (void)
{	
	/*Ports config*/
  //nrf_gpio_cfg_input(Pin_ANALOG, NRF_GPIO_PIN_NOPULL);
	
	/*SAADC enable*/
	nrf_saadc_enable();
	
	/*SAADC channel config*/
	nrf_saadc_channel_init(0, &saadcChConfig);
	
	/*SAADC config*/
	nrf_saadc_resolution_set(NRF_SAADC_RESOLUTION_12BIT);
	nrf_saadc_oversample_set(NRF_SAADC_OVERSAMPLE_DISABLED);	
	nrf_saadc_mode_set(SAMPLERATE_MODE_TIMER, 320); //50 kHz
	
	/*SAADC buffer config*/
	nrf_saadc_buffer_init(BufferADC1, BUFFER_LEN);
	
	/*SAADC interrupt config*/
	nrf_saadc_event_clear(NRF_SAADC_EVENT_STARTED);
	nrf_saadc_event_clear(NRF_SAADC_EVENT_END);
	nrf_saadc_int_enable(NRF_SAADC_INT_STARTED | NRF_SAADC_INT_END);
	
	/*Enable SAADC interrupt*/
	NVIC_SetPriority(SAADC_IRQn, 1);
	NVIC_ClearPendingIRQ(SAADC_IRQn);
	NVIC_EnableIRQ(SAADC_IRQn);
	
	/*Start SAADC*/
	nrf_saadc_task_trigger(NRF_SAADC_TASK_START);
	nrf_saadc_task_trigger(NRF_SAADC_TASK_SAMPLE);
}

/*-----------------------------------------------------------*/
/*SAADC interrupt handler*/
void SAADC_IRQHandler (void)
{
  static uint8_t flag = 0;
	
	/*SAADC started interrupt*/
	if (nrf_saadc_int_enable_check(NRF_SAADC_INT_STARTED) &&
			nrf_saadc_event_check(NRF_SAADC_EVENT_STARTED))
	{
		nrf_saadc_event_clear(NRF_SAADC_EVENT_STARTED);
		
		if(flag)
		{
			flag = 0;
			nrf_saadc_buffer_init(BufferADC1, BUFFER_LEN);
		}
		else
		{
			flag = 1;
			nrf_saadc_buffer_init(BufferADC2, BUFFER_LEN);
		}
	}

	/*SAADC end interrupt*/
	if (nrf_saadc_int_enable_check(NRF_SAADC_INT_END) &&
			nrf_saadc_event_check(NRF_SAADC_EVENT_END))	
	{
		nrf_saadc_event_clear(NRF_SAADC_EVENT_END);
		
		nrf_saadc_task_trigger(NRF_SAADC_TASK_START);
		
		//process
		
	}
}
