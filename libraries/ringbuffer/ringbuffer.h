#ifndef __ringbuffer_H_
#define __ringbuffer_H_

#ifndef TESTING
#include "board.h"
#include <stdint.h>
#endif
#ifndef ESP8266 
  #include <avr/interrupt.h>
#endif

class RingbufferClass {
public:
#if TTY_BUFSIZE < 256
	uint8_t putoff;
	uint8_t getoff;
	uint8_t nbytes;       // Number of data bytes
#else
	uint16_t putoff;
	uint16_t getoff;
	uint16_t nbytes;       // Number of data bytes
#endif
	char buf[TTY_BUFSIZE];
	RingbufferClass();
    void put(uint8_t data);
    uint8_t get();
    void reset();
	uint8_t getNbytes();
#ifdef HAS_RF_ROUTER
	void FHT_compress();
#endif
private:

#ifdef TESTING
	void p();
	void d_s(char *s);
	int main(int ac, char **av)
#endif
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RINGBUFFER)
extern RingbufferClass RFR_Buffer;
#endif

#endif
