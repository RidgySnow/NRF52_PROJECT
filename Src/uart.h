#ifndef UART_H
#define UART_H

#define SIZE_BUF_TX 200
#define SIZE_BUF_RX 200

/*Rx FIFO*/
typedef struct
{
  uint8_t buffer[SIZE_BUF_RX];
  uint16_t counterWrite;
	uint16_t counterRead;
} RxFIFO_TypeDef;

/*Tx FIFO*/
typedef struct
{
  uint8_t buffer[SIZE_BUF_TX];
	uint16_t counterWrite;
	uint16_t counterRead;
} TxFIFO_TypeDef;

/*Tx state*/
typedef enum
{
	TX_DONE = 0,
	TX_IN_PROGRESS = 1
} TxState_TypeDef;


/*-----------------------------------------------------------*/
void UART0Init (void);
uint8_t AvailableUARTData(void);
void ClearRxBuffer(void);
uint8_t ReadUART (uint8_t *data);
void TransmitUART(uint8_t data);
uint16_t GetWritePosition (void);
uint16_t GetReadPosition (void);

//uint8_t FSM (uint8_t symbol, uint32_t *data1, uint32_t *data2);
#endif
