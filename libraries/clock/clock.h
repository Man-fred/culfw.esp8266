#ifndef _CLOCK_H_
#define _CLOCK_H_

#include "board.h"

//volatile extern uint32_t ticks;  // 1/125 sec resolution
#ifdef HAS_ETHERNET
	typedef uint16_t clock_time_t;
#endif

class CLOCKClass {
	#ifndef ESP8266 // https://www.mikrocontroller.net/articles/AVR_Interrupt_Routinen_mit_C%2B%2B
		class ClassInterrupt {
			static CLOCKClass *owner;
			static void serviceRoutine() __asm__("__vector_14") __attribute__((__signal__, __used__, __externally_visible__));

			public:
				static void record(CLOCKClass *owner);
		};
		friend ClassInterrupt;
	#endif
public:
	void gettime(char*);
	//public?
	void get_timestamp(uint32_t *ts);
	#ifdef ESP8266
	  void IsrHandler();
  	//volatile uint32_t ticks; // 1/125 sec resolution
	  //volatile uint8_t  clock_hsec;
	#else
	  CLOCKClass();
    static void IsrHandler();
	#endif
	static volatile uint32_t ticks; // 1/125 sec resolution
	static volatile uint8_t  clock_hsec;
	void Minute_Task(void);

#ifdef HAS_ETHERNET
	clock_time_t clock_time(void);
#endif
private:
	
  uint8_t last_tick;
#if defined (HAS_IRRX) || defined (HAS_IRTX)
	uint8_t ir_ticks = 0;
	uint8_t ir_ticks_thrd = 0;
#endif
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_CLOCK)
extern CLOCKClass CLOCK;
#endif

#endif
