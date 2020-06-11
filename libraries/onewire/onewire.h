//*****************************************************************************
// This File is based on the  'ds2482.h'
// From the original Author		: Pascal Stang - Copyright (C) 2004
// Target MCU	: Atmel AVR Series
//
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#ifndef H
#define H


// constants/macros/typdefs

//AS the AD0 & AD1 of DS2482 are set to GND the I2C Device address is the following
#define DS2482_I2C_ADDR		0x30	//< Base I2C address of DS2482 devices

// DS2482 command defines
#define DS2482_CMD_DRST		0xF0	//< DS2482 Device Reset
#define DS2482_CMD_WCFG		0xD2	//< DS2482 Write Configuration
#define DS2482_CMD_CHSL		0xC3	//< DS2482 Channel Select
#define DS2482_CMD_SRP		0xE1	//< DS2482 Set Read Pointer
#define DS2482_CMD_1WRS		0xB4	//< DS2482 1-Wire Reset
#define DS2482_CMD_1WWB		0xA5	//< DS2482 1-Wire Write Byte
#define DS2482_CMD_1WRB		0x96	//< DS2482 1-Wire Read Byte
#define DS2482_CMD_1WSB		0x87	//< DS2482 1-Wire Single Bit
#define DS2482_CMD_1WT		0x78	//< DS2482 1-Wire Triplet

// DS2482 status register bit defines
#define DS2482_STATUS_1WB	0x01	//< DS2482 Status 1-Wire Busy
#define DS2482_STATUS_PPD	0x02	//< DS2482 Status Presence Pulse Detect
#define DS2482_STATUS_SD	0x04	//< DS2482 Status Short Detected
#define DS2482_STATUS_LL	0x08	//< DS2482 Status 1-Wire Logic Level
#define DS2482_STATUS_RST	0x10	//< DS2482 Status Device Reset
#define DS2482_STATUS_SBR	0x20	//< DS2482 Status Single Bit Result
#define DS2482_STATUS_TSB	0x40	//< DS2482 Status Triplet Second Bit
#define DS2482_STATUS_DIR	0x80	//< DS2482 Status Branch Direction Taken

// DS2482 configuration register bit defines
#define DS2482_CFG_APU		0x01	//< DS2482 Config Active Pull-Up
#define DS2482_CFG_PPM		0x02	//< DS2482 Config Presence Pulse Masking
#define DS2482_CFG_SPU		0x04	//< DS2482 Config Strong Pull-Up
#define DS2482_CFG_1WS		0x08	//< DS2482 Config 1-Wire Speed

// DS2482 channel selection code defines
#define DS2482_CH_IO0		0xF0	//< DS2482 Select Channel IO0

// DS2482 read pointer code defines
#define DS2482_READPTR_SR	0xF0	//< DS2482 Status Register
#define DS2482_READPTR_RDR	0xE1	//< DS2482 Read Data Register
#define DS2482_READPTR_CSR	0xD2	//< DS2482 Channel Selection Register
#define DS2482_READPTR_CR	0xC3	//< DS2482 Configuration Register

#define DS2482_TRUE 1
#define DS2482_FALSE 0

// functions
class OnewireClass {
public:

	void Init(void);

	void HsecTask (void) ;
	void SecTask (void);

	int
	rndup(double n);//round up a float type and show one decimal place

	unsigned char BusyWait(void);
	int Reset(void);
	void WriteBit(unsigned char data);
	void WriteByte(unsigned char data);
	unsigned char ReadByte(void);
	unsigned char ReadBit(void);
	void StartConversion(void);
	int CheckConversionRunning(void);
	int CheckAllConversionsRunning(void);
	void MatchRom(unsigned char*);
	void FullSearch(void);
	unsigned char docrc8(unsigned char value);
	void SearchReset(void);
	int Search(void);
	void ReadROMCodes(void);


	void ReadTemperature(void);

	void func(char *);

	int ds2482Init(void);
	int ds2482Reset(void);
	unsigned char ds2482SendCmd(unsigned char cmd);
	unsigned char ds2482SendCmdArg(unsigned char cmd, unsigned char arg);

	unsigned char i2cMasterSend(unsigned char deviceAddr, unsigned char length, unsigned char* data);
	unsigned char i2cMasterReceive(unsigned char deviceAddr, unsigned char length, unsigned char* data);
private:
	static constexpr unsigned char dscrc_table[256] = {
					0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
				157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
				 35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
				190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
				 70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
				219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
				101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
				248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
				140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
				 17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
				175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
				 50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
				202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
				 87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
				233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
				116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

	// Buffer for OnwWire Bus Devices
	unsigned char ROM_CODES[HAS_ONEWIRE * 8];
	int connecteddevices;
	int conversionrunning;
	int allconversionsrunning;
	int allconversiontimer;
	int hmsemulation;
	int hmsemulationstate;
	int hmsemulationtimer;
	int hmsemulationinterval;
	int hmsemulationdevicecounter;

	// Search states for the Search
	int LastDiscrepancy;
	int LastFamilyDiscrepancy;
	int LastDeviceFlag;
	int DeviceCounter;
	unsigned char crc8;
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_ONEWIRE)
  extern OnewireClass Onewire;
#endif

#endif
