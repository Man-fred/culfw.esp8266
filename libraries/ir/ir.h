#ifndef IR_H
#define IR_H

#include "board.h"

#ifndef ESP8266
  #include <avr/io.h>
  #include <avr/pgmspace.h>
  extern "C" {
    #include "irmpconfig.h"
    #include "irsndconfig.h"
    #include "irmp.h"
    #include "irsnd.h"
  }
#else
    #include <Arduino.h>
    #include <IRremoteESP8266.h>
    #ifdef HAS_IRRX
      #include <IRrecv.h>
    #endif
    #ifdef HAS_IRTX
      #include <IRsend.h>
    #endif
    #include <IRutils.h>
		typedef struct IRMP_DATA
		{
			uint8_t               protocol;                                               // protocol, i.e. NEC_PROTOCOL
			uint16_t              address;                                                // address
			uint16_t              command;                                                // command
			uint8_t               flags;                                                  // flags, e.g. repetition
		} IRMP_DATA;
#endif
#include "display.h"
#include "led.h"
#include "stringfunc.h"

//#include "ttydata.h"

class IrClass {
public:
    void init( void );
    void func(char *in);
    void task( void );
    void sample( void );
    uint8_t send_data (void);
private:
  #ifdef HAS_IRRX 
    IRrecv irrecv = IRrecv(IRMP_PIN);
    decode_results results;
  #endif
  #ifdef HAS_IRTX 
    IRsend irsend = IRsend(IRSND_OCx);  // Set the GPIO to be used to sending the message.
  #endif
  uint8_t ir_mode = 0;
  uint8_t ir_internal = 1;
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_Ir)
extern IrClass IR;
#endif

#endif
