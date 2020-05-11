#ifndef _CLOCK_H_
#define _CLOCK_H_

#include <stdint.h>
//#include <avr/io.h>
// #include <wdt.h>
//#include <avr/interrupt.h>

#include "board.h"
#include "led.h"
#ifdef XLED
#  include "xled.h"
#endif
#include "fncollection.h"
#include "display.h"
#if defined(HAS_LCD) && defined(BAT_PIN)
#  include "battery.h"
#endif
#ifdef JOY_PIN1
#  include "joy.h"
#endif
#ifdef HAS_FHT_TF
#  include "fht.h"
#endif
//#include "fswrapper.h"                 // fs_sync();
#include "rf_send.h"                   // credit_10ms
#ifdef HAS_SLEEP
#  include "mysleep.h"
#endif
////////////////#include "pcf8833.h"
#ifdef HAS_USB
//#  include "cdc.h"
#endif
#include "rf_router.h"                  // rf_router_flush();
#ifdef HAS_NTP
#  include "ntp.h"
#endif
#ifdef HAS_ONEWIRE
#  include "onewire.h"
#endif
#ifdef HAS_VZ
#  include "vz.h"
#endif
#if defined (HAS_IRRX) || defined (HAS_IRTX)
#  include "ir.h"
#endif

//volatile extern uint32_t ticks;  // 1/125 sec resolution
#ifdef HAS_ETHERNET
	typedef uint16_t clock_time_t;
#endif

class CLOCKClass {
public:
	void gettime(char*);
	//public?
	void get_timestamp(uint32_t *ts);
	void IsrHandler();
	void Minute_Task(void);

	volatile uint32_t ticks; // 1/125 sec resolution
#ifdef HAS_ETHERNET
	clock_time_t clock_time(void);
#endif
private:
	
  uint8_t last_tick;
	volatile uint8_t  clock_hsec;
#if defined (HAS_IRRX) || defined (HAS_IRTX)
	uint8_t ir_ticks = 0;
	uint8_t ir_ticks_thrd = 0;
#endif
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_CLOCK)
extern CLOCKClass CLOCK;
#endif

#endif
