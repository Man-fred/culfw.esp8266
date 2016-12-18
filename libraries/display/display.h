#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include "board.h"
#include <stdint.h>
#include <pgmspace.h>

/*
#include "ringbuffer.h"
#ifdef HAS_USB
//#include "cdc.h"
#else
#include "serial.h"
#endif
#include "led.h"
#include "delay.h"
#include "pcf8833.h"
#include "ttydata.h"            // callfn
#include "fht.h"                // fht_hc
#include "rf_router.h"
#include "clock.h"
#include "log.h"
*/
#ifdef HAS_PRIVATE_CHANNEL
//#include "private_channel.h"
#endif
#ifdef HAS_ETHERNET
//#include "tcplink.h"
#endif
#ifdef HAS_DOGM
//#include "dogm16x.h"
#endif

#include <stdint.h>
#include <stddef.h>
#include "WString.h"
#include "Printable.h"

#include <pgmspace.h>
//#include "stringfunc.h"

#define DC display.chr
#define DS(a) display.string(a)
#define DS_P display.string_P
#define DU(a,b) display.udec(a,b,' ')
#define DH(a,b) display.hex(a,b,'0')
#define DH2(a) display.hex2(a)
#define DNL display.nl

#define DISPLAY_USB      (1<<0)
#define DISPLAY_LCD      (1<<1)
#define DISPLAY_TCP      (1<<2)
#define DISPLAY_RFROUTER (1<<3)
#define DISPLAY_DOGM     (1<<4)

class DisplayClass {
public:
	DisplayClass();
	void chr(char s);
	void string(char *s);
	void string_P(const __FlashStringHelper *s);
	void string_P(const char *s);
	void udec(uint16_t d, int8_t pad, uint8_t padc);
	void hex(uint16_t h, int8_t pad, uint8_t padc);
	void hex2(uint8_t h);
	void nl(void);
	uint8_t channel=0;
	uint8_t log_enabled;
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_DISPLAY)
extern DisplayClass display;
#endif

#endif