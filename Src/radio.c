/*-----------------------------------------------------------*/
/* MCU Radio drivers
*/
#include <stdbool.h>
#include <stdint.h>

#include "nrf.h"
#include "nrf_assert.h"
#include "radio.h"
#include "main.h"
/*-----------------------------------------------------------*/

static uint8_t packet[256];

static uint8_t mode_;
static uint8_t txpower_;
static uint8_t channel_start_;
static uint8_t channel_end_;
static uint8_t channel_;
static bool    sweep_tx_;

static uint8_t TxBusyState = 0;
static uint8_t RxBusyState = 0;
static uint8_t RxEnableData = 0;

/*-----------------------------------------------------------*/
/*Internal function swap bits*/
static uint32_t swap_bits(uint32_t inp)
{
	uint32_t i;
	uint32_t retval = 0;
    
	inp = (inp & 0x000000FFUL);
    
	for (i = 0; i < 8; i++)
	{
		retval |= ((inp >> i) & 0x01) << (7 - i);     
	}
    
	return retval;    
}

/*-----------------------------------------------------------*/
/*Internal function swap bytes*/
static uint32_t bytewise_bitswap(uint32_t inp)
{
      return (swap_bits(inp >> 24) << 24)
           | (swap_bits(inp >> 16) << 16)
           | (swap_bits(inp >> 8) << 8)
           | (swap_bits(inp));
}

/*-----------------------------------------------------------*/
/*Function for generating an 8 bit random number using the internal random generator*/
static uint32_t rnd8(void)
{
	NRF_RNG->EVENTS_VALRDY = 0;
	while (NRF_RNG->EVENTS_VALRDY == 0)
	{
		// Do nothing.
	}
	return NRF_RNG->VALUE;
}

/*-----------------------------------------------------------*/
/*Function for generating a 32 bit random number using the internal random generator*/
static uint32_t rnd32(void)
{
	uint8_t  i;
	uint32_t val = 0;

	for (i=0; i<4; i++)
	{
		val <<= 8;
		val  |= rnd8();
	}
	return val;
}

/*-----------------------------------------------------------*/
/*Radio init*/
void RadioInit(void)
{
	/*Radio config*/
	NRF_RADIO->TXPOWER   = (RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos);
	NRF_RADIO->FREQUENCY = 7UL;  // Frequency bin 7, 2407MHz
	NRF_RADIO->MODE      = (RADIO_MODE_MODE_Nrf_1Mbit << RADIO_MODE_MODE_Pos);
	
	/*Shorts Config*/
	NRF_RADIO->SHORTS =	RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_DISABLE_Msk;

	/*Radio address config*/
	NRF_RADIO->PREFIX0 = 
				((uint32_t)swap_bits(0xC3) << 24) // Prefix byte of address 3 converted to nRF24L series format
			|	((uint32_t)swap_bits(0xC2) << 16) // Prefix byte of address 2 converted to nRF24L series format
			|	((uint32_t)swap_bits(0xC1) << 8)  // Prefix byte of address 1 converted to nRF24L series format
			|	((uint32_t)swap_bits(0xC0) << 0); // Prefix byte of address 0 converted to nRF24L series format
  
	NRF_RADIO->PREFIX1 = 
				((uint32_t)swap_bits(0xC7) << 24) // Prefix byte of address 7 converted to nRF24L series format
			|	((uint32_t)swap_bits(0xC6) << 16) // Prefix byte of address 6 converted to nRF24L series format
			|	((uint32_t)swap_bits(0xC4) << 0); // Prefix byte of address 4 converted to nRF24L series format

	NRF_RADIO->BASE0 = bytewise_bitswap(0x01234567UL);  // Base address for prefix 0 converted to nRF24L series format
	NRF_RADIO->BASE1 = bytewise_bitswap(0x89ABCDEFUL);  // Base address for prefix 1-7 converted to nRF24L series format
  
	NRF_RADIO->TXADDRESS   = 0x00UL;  // Set device address 0 to use when transmitting
	NRF_RADIO->RXADDRESSES = 0x01UL;  // Enable device address 0 to use to select which addresses to receive

	/*CRC Config*/
	NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Two << RADIO_CRCCNF_LEN_Pos); // Number of checksum bits
	if ((NRF_RADIO->CRCCNF & RADIO_CRCCNF_LEN_Msk) == (RADIO_CRCCNF_LEN_Two << RADIO_CRCCNF_LEN_Pos))
	{
		NRF_RADIO->CRCINIT = 0xFFFFUL;   // Initial value      
		NRF_RADIO->CRCPOLY = 0x11021UL;  // CRC poly: x^16+x^12^x^5+1
	}
	else if ((NRF_RADIO->CRCCNF & RADIO_CRCCNF_LEN_Msk) == (RADIO_CRCCNF_LEN_One << RADIO_CRCCNF_LEN_Pos))
	{
		NRF_RADIO->CRCINIT = 0xFFUL;   // Initial value
		NRF_RADIO->CRCPOLY = 0x107UL;  // CRC poly: x^8+x^2^x^1+1
	}
	
	/*Radio interrupt config*/
  NRF_RADIO->EVENTS_DISABLED	= 0x0UL; //Clear event
	NRF_RADIO->INTENSET					= RADIO_INTENSET_DISABLED_Msk; //Enable Disabled interrupt
	
	/*Enable Radio interrupt*/
	NVIC_SetPriority(RADIO_IRQn, 0);
	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);
}

/*-----------------------------------------------------------*/
/*Function for initializing Timer 0 in 24 bit timer mode with 1 us resolution*/
static void Timer1Init(uint8_t delayms)
{
	NRF_TIMER1->TASKS_STOP = 1;

	/* Create an Event-Task shortcut to clear Timer 1 on COMPARE[0] event*/
	NRF_TIMER1->SHORTS     = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);
	NRF_TIMER1->MODE       = TIMER_MODE_MODE_Timer;
	NRF_TIMER1->BITMODE    = (TIMER_BITMODE_BITMODE_24Bit << TIMER_BITMODE_BITMODE_Pos);
	NRF_TIMER1->PRESCALER  = 4;  // 1us resolution
	NRF_TIMER1->INTENSET   = (TIMER_INTENSET_COMPARE0_Set << TIMER_INTENSET_COMPARE0_Pos);
  
	/*	Sample update needs to happen as soon as possible. The earliest possible moment is MAX_SAMPLE_LEVELS
			ticks before changing the output duty cycle*/
	NRF_TIMER1->CC[0]       = (uint32_t)delayms * 1000;
	
	/*Enable Timer1 interrupt*/
	NVIC_SetPriority(TIMER1_IRQn, 2);
	NVIC_ClearPendingIRQ(TIMER1_IRQn);
	NVIC_EnableIRQ(TIMER1_IRQn);
	
	NRF_TIMER1->TASKS_START = 1;
}

/*-----------------------------------------------------------*/
/*Function for configuring the radio to use a random address and a 254 byte random payload*/
static void GenerateModulatedRfPacket(void)
{
	uint8_t i;

	NRF_RADIO->PREFIX0 = rnd8();
	NRF_RADIO->BASE0   = rnd32();

	// Packet configuration:
	// S1 size = 0 bits, S0 size = 0 bytes, payload length size = 8 bits
	NRF_RADIO->PCNF0  = (0UL << RADIO_PCNF0_S1LEN_Pos) |
											(0UL << RADIO_PCNF0_S0LEN_Pos) |
											(8UL << RADIO_PCNF0_LFLEN_Pos);
	// Packet configuration:
	// Bit 25: 1 Whitening enabled
	// Bit 24: 1 Big endian,
	// 4 byte base address length (5 byte full address length), 
	// 0 byte static length, max 255 byte payload .
	NRF_RADIO->PCNF1  = (RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos) |
											(RADIO_PCNF1_ENDIAN_Big << RADIO_PCNF1_ENDIAN_Pos) |
											(4UL << RADIO_PCNF1_BALEN_Pos) |
											(0UL << RADIO_PCNF1_STATLEN_Pos) |
											(255UL << RADIO_PCNF1_MAXLEN_Pos);
	NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Disabled << RADIO_CRCCNF_LEN_Pos);
	packet[0]         = 254;    // 254 byte payload.
  
	// Fill payload with random data.
	for (i = 0; i < 254; i++)
	{
		packet[i+1] = rnd8();
	}
	NRF_RADIO->PACKETPTR = (uint32_t)packet;
}

/*-----------------------------------------------------------*/
/*Function for disable radio*/
static void RadioDisable(void)
{
	NRF_RADIO->SHORTS          = 0;
	NRF_RADIO->EVENTS_DISABLED = 0;
#ifdef NRF51
	NRF_RADIO->TEST            = 0;
#endif
	NRF_RADIO->TASKS_DISABLE   = 1;
	while (NRF_RADIO->EVENTS_DISABLED == 0)
	{
		// Do nothing.
	}
	NRF_RADIO->EVENTS_DISABLED = 0;
}

/*-----------------------------------------------------------*/
/*Function for stopping Timer 0*/
void RadioSweepEnd(void)
{
	NRF_TIMER1->TASKS_STOP = 1;
	RadioDisable();
}

/*-----------------------------------------------------------*/
/*Function for turning on the TX carrier test mode*/
void RadioTxCarrier(uint8_t txpower, uint8_t mode, uint8_t channel)
{
	RadioDisable();
	NRF_RADIO->SHORTS     = RADIO_SHORTS_READY_START_Msk;
	NRF_RADIO->TXPOWER    = (txpower << RADIO_TXPOWER_TXPOWER_Pos);    
	NRF_RADIO->MODE       = (mode << RADIO_MODE_MODE_Pos);
	NRF_RADIO->FREQUENCY  = channel;
#ifdef NRF51
	NRF_RADIO->TEST       = (RADIO_TEST_CONST_CARRIER_Enabled << RADIO_TEST_CONST_CARRIER_Pos) \
                        | (RADIO_TEST_PLL_LOCK_Enabled << RADIO_TEST_PLL_LOCK_Pos);
#endif
	NRF_RADIO->TASKS_TXEN = 1;
}

/*-----------------------------------------------------------*/
/*Function for starting modulated TX carrier by repeatedly sending a packet with random address and 
 * random payload*/
void RadioModulatedTxCarrier(uint8_t txpower, uint8_t mode, uint8_t channel)
{
	RadioDisable();
	GenerateModulatedRfPacket();
	NRF_RADIO->SHORTS     = RADIO_SHORTS_END_DISABLE_Msk | RADIO_SHORTS_READY_START_Msk |
                          RADIO_SHORTS_DISABLED_TXEN_Msk;
	NRF_RADIO->TXPOWER    = (txpower << RADIO_TXPOWER_TXPOWER_Pos);
	NRF_RADIO->MODE       = (mode << RADIO_MODE_MODE_Pos);
	NRF_RADIO->FREQUENCY  = channel;
	NRF_RADIO->TASKS_TXEN = 1;
}

/*-----------------------------------------------------------*/
/*Function for turning on RX carrier*/
void RadioRxCarrier(uint8_t mode, uint8_t channel)
{
	RadioDisable();
	NRF_RADIO->SHORTS     = RADIO_SHORTS_READY_START_Msk;
	NRF_RADIO->FREQUENCY  = channel;
	NRF_RADIO->TASKS_RXEN = 1;
}

/*-----------------------------------------------------------*/
/*Function for turning on TX carrier sweep. This test uses Timer 1 to restart the TX carrier at different channels*/
void RadioTxSweepStart(uint8_t txpower, uint8_t mode, uint8_t channel_start, uint8_t channel_end, uint8_t delayms)
{
	txpower_       = txpower;
	mode_          = mode;
	channel_start_ = channel_start;
	channel_       = channel_start;
	channel_end_   = channel_end;
	sweep_tx_      = true;
	Timer1Init(delayms);
}

/*-----------------------------------------------------------*/
/*Function for turning on RX carrier sweep. This test uses Timer 1 to restart the RX carrier at different channels*/
void RadioRxSweepStart(uint8_t mode, uint8_t channel_start, uint8_t channel_end, uint8_t delayms)
{
	mode_          = mode;
	channel_start_ = channel_start;
	channel_       = channel_start;
	channel_end_   = channel_end;
	sweep_tx_      = false;
	Timer1Init(delayms);
}

/*-----------------------------------------------------------*/
/*Function for handling the Timer 0 interrupt used for TX/RX sweep. The carrier is started with the new channel,
 * and the channel is incremented for the next interrupt.*/
void TIMER1_IRQHandler(void) 
{
	if (sweep_tx_)
	{
		RadioTxCarrier(txpower_, mode_, channel_);
	}
	else
	{
		RadioRxCarrier(mode_, channel_);
	}
	channel_++;
	if (channel_ > channel_end_)
	{
		channel_ = channel_start_;
	}
	NRF_TIMER1->EVENTS_COMPARE[0] = 0;
}

/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
/*Function for sending packet*/
uint8_t RfSendPacket(uint8_t *buffer, uint8_t len)
{
	if(TxBusyState) return 0xFF; //Radio Tx busy
	
	/*Packet configuration*/
	NRF_RADIO->PCNF0 =	(PACKET_S1_FIELD_SIZE     << RADIO_PCNF0_S1LEN_Pos) |
											(PACKET_S0_FIELD_SIZE     << RADIO_PCNF0_S0LEN_Pos) |
											(PACKET_LENGTH_FIELD_SIZE << RADIO_PCNF0_LFLEN_Pos);

	NRF_RADIO->PCNF1 =	(RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |
											(RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos)  |
											(PACKET_BASE_ADDRESS_LENGTH   << RADIO_PCNF1_BALEN_Pos)   |
											(PACKET_STATIC_LENGTH         << RADIO_PCNF1_STATLEN_Pos) |
											((uint32_t)len					      << RADIO_PCNF1_MAXLEN_Pos);
	
	/*Set payload pointer*/
  NRF_RADIO->PACKETPTR = (uint32_t)buffer;
	
	/*Send the packet*/
	NRF_RADIO->EVENTS_READY = 0U;
	NRF_RADIO->TASKS_TXEN   = 1U;
	
	/*Radio Tx busy*/
	TxBusyState = 1;

//	while (NRF_RADIO->EVENTS_READY == 0U) {// wait}
//		
//	NRF_RADIO->EVENTS_END  = 0U;
//	NRF_RADIO->TASKS_START = 1U;

//	while (NRF_RADIO->EVENTS_END == 0U)	{// wait}
	
//	/*Disable radio*/
//	NRF_RADIO->EVENTS_DISABLED = 0U;
//	NRF_RADIO->TASKS_DISABLE = 1U;

//	while (NRF_RADIO->EVENTS_DISABLED == 0U) {// wait}
	return 0;
}

/*-----------------------------------------------------------*/
/*Function for reading packet*/
uint8_t RfReadPacket(uint8_t *buffer, uint8_t len)
{
	if(RxBusyState) return 0xFF; //Radio Rx busy
	
	/*Packet configuration*/
	NRF_RADIO->PCNF0 =	(PACKET_S1_FIELD_SIZE     << RADIO_PCNF0_S1LEN_Pos) |
											(PACKET_S0_FIELD_SIZE     << RADIO_PCNF0_S0LEN_Pos) |
											(PACKET_LENGTH_FIELD_SIZE << RADIO_PCNF0_LFLEN_Pos);

	NRF_RADIO->PCNF1 =	(RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos) |
											(RADIO_PCNF1_ENDIAN_Big       << RADIO_PCNF1_ENDIAN_Pos)  |
											(PACKET_BASE_ADDRESS_LENGTH   << RADIO_PCNF1_BALEN_Pos)   |
											(PACKET_STATIC_LENGTH         << RADIO_PCNF1_STATLEN_Pos) |
											((uint32_t)len					      << RADIO_PCNF1_MAXLEN_Pos);

	/*Set payload pointer*/
  NRF_RADIO->PACKETPTR = (uint32_t)buffer;
	
	/*Start read the packet*/
	NRF_RADIO->EVENTS_READY = 0U;
	NRF_RADIO->TASKS_RXEN = 1U;
	
	/*Radio Rx busy*/
	RxBusyState = 1;

//	while (NRF_RADIO->EVENTS_READY == 0U) {// wait}
//		
//	NRF_RADIO->EVENTS_END = 0U;
//	NRF_RADIO->TASKS_START = 1U;

//	while (NRF_RADIO->EVENTS_END == 0U) {// wait}

//	/*Disable radio*/
//	NRF_RADIO->EVENTS_DISABLED = 0U;
//	NRF_RADIO->TASKS_DISABLE = 1U;

//	while (NRF_RADIO->EVENTS_DISABLED == 0U) {// wait}
	return 0;
}

/*-----------------------------------------------------------*/
/*Radio interrupt handler*/
void RADIO_IRQHandler (void)
{
	if(	(NRF_RADIO->INTENSET & RADIO_INTENSET_DISABLED_Msk) &&
			(NRF_RADIO->EVENTS_DISABLED))
	{
		if (TxBusyState) TxBusyState = 0;
		
		if (RxBusyState)
		{
			RxBusyState = 0;
			if (NRF_RADIO->CRCSTATUS == 1U) RxEnableData = CRC_OK;
			else RxEnableData = CRC_ERROR;
		}
		NRF_RADIO->EVENTS_DISABLED	= 0x0UL; //Clear event
	}
}

/*-----------------------------------------------------------*/
/*Radio interrupt handler*/
uint8_t IsRxDataEnable (void)
{
	if(RxEnableData == CRC_OK)
	{
		RxEnableData = 0;
		return CRC_OK;	
	}
	else if(RxEnableData == CRC_ERROR)
	{
		RxEnableData = 0;
		return CRC_ERROR;	
	}
	else return 0;
}
