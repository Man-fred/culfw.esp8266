#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include "board.h"

#define DC display.chr
#define DS(a) display.string((char*)a)
#define DS_P display.string_P
#define DU(a,b) display.udec(a,b,' ')
#define DH(a,b) display.hex(a,b,'0')
#define DH2(a) display.hex2(a)
#define DNL display.nL

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
	#ifdef ESP8266
	  void string_P(const __FlashStringHelper *s);
	#endif
	void string_P(const char *s);
	void udec(uint16_t d, int8_t pad, uint8_t padc);
	void hex(uint16_t h, int8_t pad, uint8_t padc);
	void hex2(uint8_t h);
	void nL(void);
  void func(char *in);
	uint8_t channel;
	uint8_t log_enabled;
	uint8_t echo_serial;
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_DISPLAY)
extern DisplayClass display;
#endif

#endif