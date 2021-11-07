#ifndef _CDC_H_
#define _CDC_H_

#include "board.h"

/* Macros: */
#define GET_LINE_CODING			0x21
#define SET_LINE_CODING			0x20
#define SET_CONTROL_LINE_STATE		0x22

/* Event Handlers: */
#ifdef LUFA
  #define USB_IsConnected (USB_DeviceState == DEVICE_STATE_Configured)
#endif

/* Type Defines: */
typedef struct
{
        uint32_t BaudRateBPS;
        uint8_t  CharFormat;
        uint8_t  ParityType;
        uint8_t  DataBits;
} CDC_Line_Coding_t;

/* Enums: */
enum CDC_Line_Coding_Format_t
{
        OneStopBit          = 0,
        OneAndAHalfStopBits = 1,
        TwoStopBits         = 2,
};

enum CDC_Line_Codeing_Parity_t
{
        Parity_None         = 0,
        Parity_Odd          = 1,
        Parity_Even         = 2,
        Parity_Mark         = 3,
        Parity_Space        = 4,
};

extern uint8_t cdctask_enabled;

class CDCClass {
	public:
    void Task(void);
    void flush(void);
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_CDC)
extern CDCClass CDC;
#endif

#endif
