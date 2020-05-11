#ifndef _TTYDATA_H_
#define _TTYDATA_H_

#include <pgmspace.h>

#include "ringbuffer.h"

//esp8266	extern const PROGMEM t_fntab fntab[];

typedef struct t_fntab{
  unsigned char name;
  void (*fn)(char *);
} t_fntab;
	
class TTYdataClass {
public:
	TTYdataClass();
	void analyze_ttydata(uint8_t channel);
	uint8_t callfn(char *buf);

	void (*input_handle_func)(uint8_t channel);
	void (*output_flush_func)(void);

	RingbufferClass txBuffer;
	RingbufferClass rxBuffer;
	t_fntab fntab[20];
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_TTYDATA)
extern TTYdataClass TTYdata;
#endif

#endif
