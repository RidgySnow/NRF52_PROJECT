/*-----------------------------------------------------------*/
/* MCU Timer drivers
*/
#include "nrf_timer.h"
#include "timer.h"
#include "main.h"
/*-----------------------------------------------------------*/

uint8_t timerFlag = 0;

/*-----------------------------------------------------------*/
/*Init Timer0*/
void Timer0Init (void)
{	
	uint32_t time_ticks = 1000000; // count timer ticks to interrupt every 1 sec
	
	/*Timer0 config*/
	nrf_timer_mode_set(NRF_TIMER0, NRF_TIMER_MODE_TIMER);
	nrf_timer_bit_width_set(NRF_TIMER0, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_frequency_set(NRF_TIMER0, NRF_TIMER_FREQ_1MHz);
	
	/*Timer0 shorts*/
	nrf_timer_shorts_disable(NRF_TIMER0, (TIMER_SHORTS_COMPARE0_STOP_Msk | TIMER_SHORTS_COMPARE0_CLEAR_Msk));
	nrf_timer_shorts_enable(NRF_TIMER0, TIMER_SHORTS_COMPARE0_CLEAR_Msk);
	
	/*Timer0 interrupt config*/
	nrf_timer_event_clear(NRF_TIMER0, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_int_enable(NRF_TIMER0, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_cc_write(NRF_TIMER0, NRF_TIMER_CC_CHANNEL0, time_ticks);
	
	/*Enable Timer0 interrupt*/
	NVIC_SetPriority(TIMER0_IRQn, 0);
	NVIC_ClearPendingIRQ(TIMER0_IRQn);
	NVIC_EnableIRQ(TIMER0_IRQn);
	
	/*Start Timer0*/
	nrf_timer_task_trigger(NRF_TIMER0, NRF_TIMER_TASK_START);
}

/*-----------------------------------------------------------*/
/*Timer0 interrupt handler*/
void TIMER0_IRQHandler (void)
{
	/*Compare0 interrupt*/
	if (nrf_timer_int_enable_check(NRF_TIMER0, NRF_TIMER_EVENT_COMPARE0) &&
			nrf_timer_event_check(NRF_TIMER0, NRF_TIMER_EVENT_COMPARE0))
	
	{
		nrf_timer_event_clear(NRF_TIMER0, NRF_TIMER_EVENT_COMPARE0);
		
		timerFlag = 1;
	}   
}
