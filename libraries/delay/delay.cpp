//#include <avr/io.h>
//#include "util/delay_basic.h"
#include <Arduino.h>
#include "delay.h"

void MYDELAYClass::my_delay_us( uint16_t d ){
  delayMicroseconds(d);
}

void MYDELAYClass::my_delay_ms( uint8_t d ){
  delay(d);
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_MYDELAY)
MYDELAYClass MYDELAY;
#endif
