#ifndef _LED_H
#define _LED_H   1

#include <Arduino.h>
#include <stdint.h>

#define HI8(x)  ((uint8_t)((x) >> 8))
#define LO8(x)  ((uint8_t)(x))

#define SET_BIT(PORT, BITNUM) ((PORT) |= (1<<(BITNUM)))
#define CLEAR_BIT(PORT, BITNUM) ((PORT) &= ~(1<<(BITNUM)))
#define TOGGLE_BIT(PORT, BITNUM) ((PORT) ^= (1<<(BITNUM)))

#include "board.h"

class LEDClass {
public:
    LEDClass();
	void set(unsigned char state);
	void toggle();
private:
	unsigned char LedState = 1;
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_DISPLAY)
extern LEDClass LED;
#endif

#ifdef XLED
#include "xled.h"
#define led_init()   LED_DDR  |= _BV(LED_PIN); xled_pos=0; xled_pattern=0xff00
#else
//esp8266 #define led_init()   LED_DDR  |= _BV(LED_PIN)
#	define led_init()
#endif

//esp8266 #define LED_TOGGLE() LED_PORT ^= _BV(LED_PIN)
#define LED_TOGGLE() LED.toggle()
#ifdef LED_INV
//esp8266 #define LED_OFF()    LED_PORT |= _BV(LED_PIN)
//esp8266 #define LED_ON( )    LED_PORT &= ~_BV(LED_PIN)
#  define LED_OFF()    LED.set(1)
#  define LED_ON()     LED.set(0)
#else
//esp8266 #define LED_ON()     LED_PORT |= _BV(LED_PIN)
//esp8266 #define LED_OFF( )   LED_PORT &= ~_BV(LED_PIN)
#  define LED_OFF()    LED.set(0)
#  define LED_ON()     LED.set(1)
#endif

#endif
