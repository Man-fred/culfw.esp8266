#include "board.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#ifndef ESP8266
  #include <avr/io.h>
	#include <avr/pgmspace.h>
#endif
#include "display.h"
#include "memory.h"

#ifndef ESP8266
  extern char * const __brkval;
#else
// https://github.com/esp8266/esp8266-wiki/wiki/Memory-Map
  #define RAMSTART  0x3FFE8000
  #define RAMSIZE   0x18000
  #define RAMEND    RAMSTART + RAMSIZE - 1

  #define __brkval            ESP.getSketchSize()
  #define __malloc_heap_start RAMSTART
	#define __malloc_heap_end   RAMEND
#endif

uint16_t MemoryClass::freeMem(void) {
#ifndef ESP8266
	char *brkval;
	char *cp;

	brkval = __brkval;
	if (brkval == 0) {
	  brkval = __malloc_heap_start;
  }
	cp = __malloc_heap_end;
	if (cp == 0) {
	  cp = ((char *)AVR_STACK_POINTER_REG) - __malloc_margin;
  }
  if (cp <= brkval) return 0;
     
  return cp - brkval;
#else
	return (uint16_t)ESP.getFreeHeap();
#endif
}


void MemoryClass::getfreemem(char *unused) {
     DC('B'); DU((uint16_t)__brkval,           5); DNL();
     DC('S'); DU((uint16_t)__malloc_heap_start,5); DNL();
     DC('E'); DU((uint16_t)__malloc_heap_end,  5); DNL();
     DC('F'); DU((uint16_t)freeMem(),          5); DNL();
}

void MemoryClass::testmem(char *unused) {
	char *buf;
	uint16_t size;

	DS_P( PSTR("testing ") );

	size = 32765;
	display.udec(size, 5,' ');

	DS_P( PSTR(" bytes of RAM - ") );

	buf = NULL;
	malloc( size );

	if (buf != NULL) {

	  // write
          memset( buf, 0x77, size );
	  for (uint16_t i = 0; i<size; i++)
	       *(buf+i) = (i & 0xff);
	  
	  // read
	  for (uint16_t i = 0; i<size; i++) {
	    if (*(buf+i) != (i & 0xff)) {
		    DS_P( PSTR("Problems at ") );
		    display.udec(i, 5,' ');
		    DNL();
		    free( buf );
		    return;
	    }
	       
	  }
	  
	  free( buf );
	  DS_P( PSTR("OK") );
  } else {

	  DS_P( PSTR("Alloc failed") );
  }
     
  DNL();
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_MEMORY)
MemoryClass Memory;
#endif
