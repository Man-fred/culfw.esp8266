#ifndef _MYDELAY_H
#define _MYDELAY_H
#include <stdint.h>

class MYDELAYClass {
public:
  void my_delay_us( uint16_t d );
  void my_delay_ms( uint8_t d );
	};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_MYDELAY)
extern MYDELAYClass MYDELAY;
#endif

#endif
     

