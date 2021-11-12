//------------------------------------
//This program is for nRF52832_xxAA
//Device sends message ?DATDx0Dx0A by pressing Button 1 on board
//Device resieves special message and transforms data to double value of Distance, Elevation angle, Tilt angle
//------------------------------------


#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "nrf.h"
#include "main.h"
#include "gpio.h"
#include "uart.h"
#include "nrf_gpio.h"

#define UART

uint8_t txBuffer[RF_TX_BUFFER_LEN];

void ClockInitialization(void);

#ifdef UART
	uint8_t cr_0[9] = {0};
	uint8_t cr[6] = {0};
	uint8_t cr_all [28] = {0};
	uint8_t cr_d [8] = {0};
	uint8_t cr_e [8] = {0};
	uint8_t cr_t [8] = {0};

	uint8_t INFO[7] = {'?','D','A','T','D',0x0D,0x0A};//System message
  float DIST = 000.000F; //Resieved distantion, meters
	float ELEV_ANG = 000.000F;//Resieved angle, grad
	float TILT_ANG = 000.000F;//Resieved tilt angle, grad
  int D_cnt = 0;//Distance counter
  int E_cnt = 0;//Elevation counter
  int T_cnt = 0;//Tilt counter
//Otladka
	bool flag = 0; 
	
	unsigned int tmoutCnt = 0;//Timeout counter

	uint16_t rxCounter = 0;
	/*Rx state*/
	typedef enum
	{
		RX_STANDBY,
		RX_DONE ,
		RX_IN_PROGRESS
		
	} RxState_Typedef;
	
	RxState_Typedef state = RX_STANDBY;
	

#endif
/*-----------------------------------------------------------*/
/*Main function*/
int main(void)
{
	ClockInitialization();

	
#ifdef RADIO_TX
	RadioInit();
	__enable_irq();
	
	for(i=0; i < RF_TX_BUFFER_LEN; i++) txBuffer[i] = i +10;
	
	while(1)
	{
		RfSendPacket(txBuffer, RF_TX_BUFFER_LEN);
	}	
#endif
	
#ifdef RADIO_RX
	RadioInit();
	__enable_irq();
	
	RfReadPacket(txBuffer, RF_TX_BUFFER_LEN);
	while(1)
	{
		if(IsRxDataEnable() == CRC_OK)
		{
			for(i=0;i < 1000000;i++) dummy++;
			RfReadPacket(txBuffer, RF_TX_BUFFER_LEN);
		}
	}	
#endif

#ifdef UART
	UART0Init();
	GPIOInit();
	
	__enable_irq();
	
	while(1)
	{


		switch (state)
		{
			case RX_STANDBY:
				
				if(AvailableUARTData())
				{
					nrf_gpio_pin_write(PIN_17, 1);
					nrf_gpio_pin_write(PIN_18, 1);
					nrf_gpio_pin_write(PIN_19, 1);
					nrf_gpio_pin_write(PIN_20, 1);
					rxCounter = 0;
					state = RX_IN_PROGRESS;
				}
				if ((!(nrf_gpio_pin_read(PIN_13))) && !flag)//for direct test
				{
							for (int i = 0; i<7;i++)
							{
								TransmitUART(INFO[i]);
							}
							flag = 1;
				}
				if (flag && (nrf_gpio_pin_read(PIN_13)))//debounce 
				{
					flag = 0;
				}
				/*
				if ((!(nrf_gpio_pin_read(PIN_14))) && !flag1)//for direct test
				{
					//Message formation
							
					TransmitUART(DIST_TX[0]);
					TransmitUART(DIST_TX[1]);
					TransmitUART(DIST_TX[2]);
					TransmitUART(DIST_TX[3]);
					TransmitUART(DIST_TX[4]);
					TransmitUART(DIST_TX[5]);
					TransmitUART(DIST_TX[6]);
					TransmitUART(DIST_TX[7]);

					TransmitUART(' ');

					TransmitUART(ELEV_ANG_TX[0]);
					TransmitUART(ELEV_ANG_TX[1]);
					TransmitUART(ELEV_ANG_TX[2]);
					TransmitUART(ELEV_ANG_TX[3]);
					TransmitUART(ELEV_ANG_TX[4]);
					TransmitUART(ELEV_ANG_TX[5]);
					TransmitUART(ELEV_ANG_TX[6]);
					
					TransmitUART(' ');

					TransmitUART(TILT_ANG_TX[0]);
					TransmitUART(TILT_ANG_TX[1]);
					TransmitUART(TILT_ANG_TX[2]);
					TransmitUART(TILT_ANG_TX[3]);
					TransmitUART(TILT_ANG_TX[4]);
					TransmitUART(TILT_ANG_TX[5]);
					TransmitUART(TILT_ANG_TX[6]);
					
					flag1 = 1;
				}
				if (flag1 && (nrf_gpio_pin_read(PIN_14)))//debounce 
				{
					flag1 = 0;
				}
				*/
					
			break;
			
			case RX_IN_PROGRESS:

				if ((rxCounter == GetWritePosition())&& (tmoutCnt == 52000))
				{
					state = RX_DONE;
					tmoutCnt = 0;
				}
				else
				{
					tmoutCnt++;
					rxCounter = GetWritePosition();
				}
			
			break;
			
			case RX_DONE:
				
				for (int i=0; i<9; i++)
				{
					ReadUART (&cr_0[i]);
				}
				
				ReadUART (&cr[0]);
				
				if (cr[0]=='+')
				{					
						for (int i=1; i<6; i++)
						{
							ReadUART(&cr[i]);
							nrf_gpio_pin_write(PIN_20, 0);
						}
					if ((cr[0] == '+') && (cr[1] == 'D') && (cr[2] == 'A') && (cr[3] == 'T') && (cr[4] == 'D') && (cr[5]==0x20)) //TRash out
					{
							for (int i=0;i<28;i++)
							{
								ReadUART(&cr_all[i]);
							}
							
							while (cr_all[D_cnt] != 0x20)//till not space
							{D_cnt++;}

							while (cr_all[D_cnt + 1 + E_cnt] != 0x20)//till not space
							{E_cnt++;}

							while (cr_all[D_cnt + 1 + E_cnt + 1 + T_cnt] != 0x0D)//till not space
							{T_cnt++;}


							for (int i = 0; i < (D_cnt + 1); i++)
							{
								cr_d[i]=cr_all[i];
							}

							for (int i = D_cnt + 1; i < D_cnt + 1 + E_cnt; i++)
							{
								cr_e[i-(D_cnt + 1)]=cr_all[i];
							}

							for (int i = D_cnt + E_cnt + 2; i < D_cnt + E_cnt + 2 + T_cnt; i++)
							{
								cr_t[i-(D_cnt + E_cnt + 2)]=cr_all[i];
							}
							
				//-----------Conversion to float---------------------------------- 
							if (cr_d[0]== '-')
							{
								if (D_cnt == 8)
								{DIST = (-1)*((cr_d[1]-0x30)*100000 + (cr_d[2]-0x30)*10000 + (cr_d[3]-0x30)*1000 + (cr_d[5]-0x30)*100 + (cr_d[6]-0x30)*10 + (cr_d[7]-0x30)*1); 
								 DIST = DIST / 1000.000F;
								}
								if (D_cnt == 7 )
								{DIST = (-1)*((cr_d[1]-0x30)*10000 + (cr_d[2]-0x30)*1000 + (cr_d[4]-0x30)*100 + (cr_d[5]-0x30)*10 + (cr_d[6]-0x30)*1);
								 DIST = DIST / 1000.000F;
								}
								if (D_cnt == 6)
								{DIST = (-1)*((cr_d[1]-0x30)*1000 + (cr_d[3]-0x30)*100 + (cr_d[4]-0x30)*10 + (cr_d[5]-0x30)*1);
								 DIST = DIST / 1000.000F;
								}				

							}
							else 
							{
								if (D_cnt == 7)
								{DIST = ((cr_d[0]-0x30)*100000 + (cr_d[1]-0x30)*10000 + (cr_d[2]-0x30)*1000 + (cr_d[4]-0x30)*100 + (cr_d[5]-0x30)*10 + (cr_d[6]-0x30)*1);
								 DIST = DIST / 1000.000F;
								}
								if (D_cnt == 6)
								{DIST = ((cr_d[0]-0x30)*10000 + (cr_d[1]-0x30)*1000 + (cr_d[3]-0x30)*100 + (cr_d[4]-0x30)*10 + (cr_d[5]-0x30)*1);
								 DIST = DIST / 1000.000F;
								}
								if (D_cnt == 5)
								{DIST = ((cr_d[0]-0x30)*1000 + (cr_d[2]-0x30)*100 + (cr_d[3]-0x30)*10 + (cr_d[4]-0x30)*1);
								 DIST = DIST / 1000.000F;
								}					
							}
							
							if (cr_e[0]== '-')
							{
								if (E_cnt == 8)
								{ELEV_ANG = (-1)*((cr_e[1]-0x30)*100000 + (cr_e[2]-0x30)*10000 + (cr_e[3]-0x30)*1000 + (cr_e[5]-0x30)*100 + (cr_e[6]-0x30)*10 + (cr_e[7]-0x30)*1);
								 ELEV_ANG = ELEV_ANG / 1000.000F;
								}
								if (E_cnt == 7)
								{ELEV_ANG = (-1)*((cr_e[1]-0x30)*10000 + (cr_e[2]-0x30)*1000 + (cr_e[4]-0x30)*100 + (cr_e[5]-0x30)*10 + (cr_e[6]-0x30)*1);
								 ELEV_ANG = ELEV_ANG / 1000.000F;
								}
								if (E_cnt == 6)
								{ELEV_ANG = (-1)*((cr_e[1]-0x30)*1000 + (cr_e[3]-0x30)*100 + (cr_e[4]-0x30)*10 + (cr_e[5]-0x30)*1);
								 ELEV_ANG = ELEV_ANG / 1000.000F;
								}
							}
							else 
							{
								if (E_cnt == 7)
								{ELEV_ANG = ((cr_e[0]-0x30)*100000 + (cr_e[1]-0x30)*10000 + (cr_e[2]-0x30)*1000 + (cr_e[4]-0x30)*100 + (cr_e[5]-0x30)*10 + (cr_e[6]-0x30)*1);
								 ELEV_ANG = ELEV_ANG / 1000.000F;
								}
								if (E_cnt == 6)
								{ELEV_ANG = ((cr_e[0]-0x30)*10000 + (cr_e[1]-0x30)*1000 + (cr_e[3]-0x30)*100 + (cr_e[4]-0x30)*10 + (cr_e[5]-0x30)*1);
								 ELEV_ANG = ELEV_ANG / 1000.000F;
								}
								if (E_cnt == 5)
								{ELEV_ANG = ((cr_e[0]-0x30)*1000 + (cr_e[2]-0x30)*100 + (cr_e[3]-0x30)*10 + (cr_e[4]-0x30)*1);
								 ELEV_ANG = ELEV_ANG / 1000.000F;	}
								
							}
							
							if (cr_t[0]== '-')
							{
								if (T_cnt == 8)
								{TILT_ANG = (-1)*((cr_t[1]-0x30)*100000 + (cr_t[2]-0x30)*10000 + (cr_t[3]-0x30)*1000 + (cr_t[5]-0x30)*100 + (cr_t[6]-0x30)*10 + (cr_t[7]-0x30)*1);
								 TILT_ANG = TILT_ANG / 1000.000F;
								}
								if (T_cnt == 7)
								{TILT_ANG = (-1)*((cr_t[1]-0x30)*10000 + (cr_t[2]-0x30)*1000 + (cr_t[4]-0x30)*100 + (cr_t[5]-0x30)*10 + (cr_t[6]-0x30)*1);
								 TILT_ANG = TILT_ANG / 1000.000F;
								}
								if (T_cnt == 6)
								{TILT_ANG = (-1)*((cr_t[1]-0x30)*1000 + (cr_t[3]-0x30)*100 + (cr_t[4]-0x30)*10 + (cr_t[5]-0x30)*1);
								 TILT_ANG = TILT_ANG / 1000.000F;
								}

							}
							else 
							{
								if (T_cnt == 7)
								{TILT_ANG = ((cr_t[0]-0x30)*100000 + (cr_t[1]-0x30)*10000 + (cr_t[2]-0x30)*1000 + (cr_t[4]-0x30)*100 + (cr_t[5]-0x30)*10 + (cr_t[6]-0x30)*1);
									 TILT_ANG = TILT_ANG / 1000.000F;
								}
								if (T_cnt == 6)
								{TILT_ANG = ((cr_t[0]-0x30)*10000 + (cr_t[1]-0x30)*1000 + (cr_t[3]-0x30)*100 + (cr_t[4]-0x30)*10 + (cr_t[5]-0x30)*1);
								 TILT_ANG = TILT_ANG / 1000.000F;
								}
								if (T_cnt == 5)
								{TILT_ANG = ((cr_t[0]-0x30)*1000 + (cr_t[2]-0x30)*100 + (cr_t[3]-0x30)*10 + (cr_t[4]-0x30)*1);
								 TILT_ANG = TILT_ANG / 1000.000F;
								}	
							}
					}
					else 
					{

							ClearRxBuffer();
					}
				}
				ClearRxBuffer();
				rxCounter = 0;
				state = RX_STANDBY;
				memset (&cr[0],0,sizeof(cr));
				memset (&cr_all[0],0,sizeof(cr_all));
				memset (&cr_d[0],0,sizeof(cr_d));
				memset (&cr_e[0],0,sizeof(cr_e));
				memset (&cr_t[0],0,sizeof(cr_t));
			break;
			
				
			default:
				state = RX_STANDBY;
		}

			
	}	
#endif

#ifdef ADC
	SAADCInit();
	__enable_irq();
	
	while(1)
	{

	}	
#endif
}


/*-----------------------------------------------------------*/
/*Function for initialization clock*/
void ClockInitialization(void)
{
    /* Запуск внешнего резонатора 32 МГц, системная тактовая частота -> 64 МГц*/
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;

    /* Ожидание запуска внешнего резонатора*/
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
}
