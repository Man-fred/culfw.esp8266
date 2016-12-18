#ifndef _CLOCK_H_
#define _CLOCK_H_

#include <stdint.h>

//volatile extern uint32_t ticks;  // 1/125 sec resolution

class CLOCKClass {
public:
	void gettime(char*);
	//public?
	void get_timestamp(uint32_t *ts);
	void IsrHandler();
	void Minute_Task(void);

	volatile uint32_t ticks; // 1/125 sec resolution
private:
	
#ifdef HAS_ETHERNET
	typedef uint16_t clock_time_t;
	clock_time_t clock_time(void);
#endif
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
