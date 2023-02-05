#ifndef _RF_MBUS_H
#define _RF_MBUS_H

#include "board.h"

#ifdef HAS_MBUS

#include "mbus_defs.h"
#include "mbus_packet.h"
#include "manchester.h"
#include "3outof6.h"

#define	WMBUS_NONE 	0

#define RX_FIFO_THRESHOLD         0x07
#define RX_FIFO_START_THRESHOLD   0x00
#define RX_FIFO_SIZE              64
#define RX_OCCUPIED_FIFO          32    // Occupied space
#define RX_AVAILABLE_FIFO         32    // Free space

#define FIXED_PACKET_LENGTH       0x00
#define INFINITE_PACKET_LENGTH    0x02
#define INFINITE                  0
#define FIXED                     1
#define MAX_FIXED_LENGTH          256

#define RX_STATE_ERROR            3

#define TX_FIFO_THRESHOLD       0x07
#define TX_OCCUPIED_FIF0        32    // Occupied space
#define TX_AVAILABLE_FIFO       32    // Free space
#define TX_FIFO_SIZE            64

#define FIXED_PACKET_LENGTH     0x00
#define INFINITE_PACKET_LENGTH  0x02
#define INFINITE                0
#define FIXED                   1
#define MAX_FIXED_LENGTH        256

#define TX_OK                   0
#define TX_LENGTH_ERROR         1
#define TX_STATE_ERROR          2

#define GDO0_DDR  CC1100_OUT_DDR
#define GDO0_PORT CC1100_OUT_PORT
#define GDO0_PIN  CC1100_OUT_IN
#define GDO0_BIT  CC1100_OUT_PIN

#define GDO2_DDR  CC1100_IN_DDR
#define GDO2_PORT CC1100_IN_PORT
#define GDO2_PIN  CC1100_IN_IN
#define GDO2_BIT  CC1100_IN_PIN

// Radio Mode
#define RADIO_MODE_NONE  0
#define RADIO_MODE_TX    1
#define RADIO_MODE_RX    2

class RfMbusClass {
public:
	void task(void);
	void func(char *in);

	uint8_t mbus_mode = WMBUS_NONE;
private:
	typedef struct RXinfoDescr {
		uint8  lengthField;         // The L-field in the WMBUS packet
		uint16 length;              // Total number of bytes to to be read from the RX FIFO
		uint16 bytesLeft;           // Bytes left to to be read from the RX FIFO
		uint8 *pByteIndex;          // Pointer to current position in the byte array
		uint8 format;               // Infinite or fixed packet mode
		uint8 start;                // Start of Packet
		uint8 complete;             // Packet received complete
		uint8 mode;                 // S-mode or T-mode
		uint8 framemode;            // C-mode or T-mode frame
		uint8 frametype;            // Frame type A or B when in C-mode
		uint8 state;
	} RXinfoDescr;


	// Struct. used to hold information used for TX
	typedef struct TXinfoDescr {
		uint16 bytesLeft;           // Bytes left that are to be written to the TX FIFO
		uint8 *pByteIndex;          // Pointer to current position in the byte array
		uint8  format;              // Infinite or fixed length packet mode
		uint8  complete;            // Packet Sendt
	} TXinfoDescr;

	// Buffers
	uint8 MBpacket[291];
	uint8 MBbytes[584];


	uint8   radio_mode = RADIO_MODE_NONE;
	RXinfoDescr RXinfo;
	TXinfoDescr TXinfo;

    //static 
	void halRfReadFifo(uint8* data, uint8 length, uint8 *rssi, uint8 *lqi);
    uint8_t halRfWriteFifo(const uint8* data, uint8 length);
    //static 
	void halRfWriteReg( uint8_t reg, uint8_t value );
    uint8_t halRfGetTxStatus(void);
    //static 
	uint8_t on(uint8_t force);
    //static 
	void init(uint8_t mmode, uint8_t rmode);
    uint16 txSendPacket(uint8* pPacket, uint8* pBytes, uint8 mode);
    //static 
	void mbus_status(void);
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RFMBUS)
extern RfMbusClass RfMbus;
#endif

#endif
#endif
