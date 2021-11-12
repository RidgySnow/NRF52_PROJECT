#ifndef RADIO_H
#define RADIO_H

#include <stdint.h>

#define PACKET_BASE_ADDRESS_LENGTH  (4UL)                   // Packet base address length field size in bytes
#define PACKET_STATIC_LENGTH        (0UL)                   // Packet static length in bytes
#define PACKET_PAYLOAD_MAXSIZE      (0UL)  									// Packet payload maximum size in bytes
#define PACKET_S1_FIELD_SIZE      	(0UL)  									// Packet S1 field size in bits
#define PACKET_S0_FIELD_SIZE      	(0UL)  									// Packet S0 field size in bits
#define PACKET_LENGTH_FIELD_SIZE  	(0UL)  									// Packet length field size in bits

#define CRC_OK											(1U)
#define CRC_ERROR										(2U)

void RadioInit(void);
void RadioTxCarrier(uint8_t txpower, uint8_t mode, uint8_t channel);
void RadioModulatedTxCarrier(uint8_t txpower, uint8_t mode, uint8_t channel);
void RadioRxCarrier(uint8_t mode, uint8_t channel);
void RadioTxSweepStart(uint8_t txpower, uint8_t mode, uint8_t channel_start, uint8_t channel_end, uint8_t delayms);
void RadioRxSweepStart(uint8_t mode, uint8_t channel_start, uint8_t channel_end, uint8_t delayms);
void RadioSweepEnd(void);
uint8_t RfSendPacket(uint8_t *buffer, uint8_t len);
uint8_t RfReadPacket(uint8_t *buffer, uint8_t len);
uint8_t IsRxDataEnable (void);

#endif
