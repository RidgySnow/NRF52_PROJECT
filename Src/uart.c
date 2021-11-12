/*-----------------------------------------------------------*/
/* MCU USART drivers
*/
#include "nrf_uart.h"
#include "nrf_gpio.h"
#include "uart.h"
#include "main.h"
/*-----------------------------------------------------------*/

/*FIFO buffers*/
RxFIFO_TypeDef rxFIFO = {
	.counterWrite = 0,
	.counterRead = 0};
TxFIFO_TypeDef txFIFO = {
	.counterWrite = 0,
	.counterRead = 0};

TxState_TypeDef txState = TX_DONE;
/*-----------------------------------------------------------*/
/*Init UART0*/
void UART0Init (void)
{
	/*Ports config*/
	nrf_gpio_cfg_output(Pin_UART0_TX);
	nrf_gpio_pin_set(Pin_UART0_TX);
  nrf_gpio_cfg_input(Pin_UART0_RX, NRF_GPIO_PIN_NOPULL);
	
	/*UART config*/
	nrf_uart_baudrate_set(NRF_UART0, NRF_UART_BAUDRATE_115200);
	nrf_uart_configure(NRF_UART0, NRF_UART_PARITY_EXCLUDED, NRF_UART_HWFC_DISABLED);
	nrf_uart_txrx_pins_set(NRF_UART0, Pin_UART0_TX, Pin_UART0_RX);
	
	/*UART interrupt config*/
	nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_TXDRDY);
	nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_RXDRDY);
	nrf_uart_int_enable(NRF_UART0, NRF_UART_INT_MASK_TXDRDY | NRF_UART_INT_MASK_RXDRDY);
	
	/*Enable UART interrupt*/
	NVIC_SetPriority(UARTE0_UART0_IRQn, 1);
	NVIC_ClearPendingIRQ(UARTE0_UART0_IRQn);
	NVIC_EnableIRQ(UARTE0_UART0_IRQn);
	
	/*UART start*/
	nrf_uart_enable(NRF_UART0);
	nrf_uart_task_trigger(NRF_UART0, NRF_UART_TASK_STARTRX);
	nrf_uart_task_trigger(NRF_UART0, NRF_UART_TASK_STARTTX);
}

/*-----------------------------------------------------------*/
/*UART interrupt handler*/
void UARTE0_UART0_IRQHandler(void)
{
	/*End Rx interrupt*/
	if (nrf_uart_int_enable_check(NRF_UART0, NRF_UART_INT_MASK_RXDRDY) &&
			nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_RXDRDY))
	{
		nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_RXDRDY);
		
		/*Read data to buffer*/
		rxFIFO.buffer[rxFIFO.counterWrite++] = nrf_uart_rxd_get(NRF_UART0);
		if (rxFIFO.counterWrite == SIZE_BUF_RX ) rxFIFO.counterWrite = 0;
	}

	/*End Tx interrupt*/
	if (nrf_uart_event_check(NRF_UART0, NRF_UART_EVENT_TXDRDY))
	{
		nrf_uart_event_clear(NRF_UART0, NRF_UART_EVENT_TXDRDY);
		
		if (txFIFO.counterWrite != txFIFO.counterRead)
		{
			nrf_uart_txd_set(NRF_UART0, txFIFO.buffer[txFIFO.counterRead++]);
      if (txFIFO.counterRead == SIZE_BUF_TX) txFIFO.counterRead = 0;
		}
		else
		{
			txState = TX_DONE;
		}
  }
}

/*-----------------------------------------------------------*/
/*Check Rx buffer*/
uint8_t AvailableUARTData(void)
{ 
  if (rxFIFO.counterWrite == rxFIFO.counterRead) return 0;
  else return 1;
}

/*-----------------------------------------------------------*/
/*Clear Rx buffer*/
void ClearRxBuffer(void)
{ 
  rxFIFO.counterRead = rxFIFO.counterWrite;
}

/*-----------------------------------------------------------*/
/*Read Rx buffer*/
uint8_t ReadUART (uint8_t *data)
{ 
  if (rxFIFO.counterRead == rxFIFO.counterWrite) return 1;   
  else
  {
    *data = rxFIFO.buffer[rxFIFO.counterRead++];
		if (rxFIFO.counterRead == SIZE_BUF_RX) rxFIFO.counterRead = 0;
		return 0;
  }
}

/*-----------------------------------------------------------*/
/*Write Tx buffer*/
void TransmitUART(uint8_t data)
{
  txFIFO.buffer[txFIFO.counterWrite++] = data;
  if (txFIFO.counterWrite == SIZE_BUF_TX) txFIFO.counterWrite = 0;
	
	if(txState == TX_DONE)
	{
		txState = TX_IN_PROGRESS;
		nrf_uart_txd_set(NRF_UART0, txFIFO.buffer[txFIFO.counterRead++]);
		if (txFIFO.counterRead == SIZE_BUF_TX) txFIFO.counterRead = 0;
	}
}
/*Get Write/Read Position*/
uint16_t GetWritePosition (void)
{
	return rxFIFO.counterWrite;
}

uint16_t GetReadPosition (void)
{
	return rxFIFO.counterRead;
}
/*-------------------------------------------------------------*/
///*-----------------------------------------------------------*/
///*Rx data analysis*/
//uint8_t FSM (uint8_t symbol, uint32_t *data1, uint32_t *data2)
//{
//	static uint8_t checkRxSum = 0;
//	static uint8_t state = WAIT_START;
//	static uint16_t counterSerialRx = 0;
//	static uint8_t counterByte = 0;
//	static uint32_t intData = 0;
//	uint8_t data = 0;
//	uint8_t flag = 0;
//	
//	switch (state)
//	{
//		case WAIT_START:
//			if (symbol == START_BYTE)
//			{
//				counterSerialRx++;
//				if (counterSerialRx == 4)
//				{
//					state = READ_DATA;
//					checkRxSum  = 0;
//					*data1 = 0;
//					*data2 = 0;
//					counterByte = 0;
//				}
//			}
//			else
//			{
//				counterSerialRx = 0;
//			}
//			break;
//		
//		case READ_DATA:
//			checkRxSum += symbol;
//			counterSerialRx++;
//		if(counterSerialRx > 10)
//			{
//				switch(counterByte)
//				{
//					case 0:
//						intData = symbol << 16;
//						break;
//					case 1:
//						intData |= symbol << 8;
//						break;
//					case 2:
//						intData |= symbol;
//						if(counterSerialRx > 10 + OFFSET_POINTS*6) *data1 += intData;
//						//test1[counter_test] = intData;
//						break;
//					case 3:
//						intData = symbol << 16;
//						break;
//					case 4:
//						intData |= symbol << 8;
//						break;
//					case 5:
//						intData |= symbol;
//						if(counterSerialRx > 10 + OFFSET_POINTS*6) *data2 += intData;
//						//test2[counter_test++] = intData;
//						break;
//					default:
//						break;
//				}
//				counterByte++;
//				if(counterByte == 6) counterByte = 0;
//			}		
//			if (counterSerialRx == NUMBER_POINTS*6+10)
//			{
//				state = WAIT_CH_SUM;
//				counterSerialRx = 0; 
//			}
//			break;
//		
//		case WAIT_CH_SUM:
//			data = ~checkRxSum;
//			if (symbol == data) flag = 1;
//			else flag = 0; 
//			state = WAIT_START;
//		//counter_test=0;
//			break;
//			
//		default:
//			break;
//	}
//	return flag;
//}
