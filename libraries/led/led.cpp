#include "board.h"
#include "led.h"

LEDClass::LEDClass()
{
	pinMode(LED_PIN, OUTPUT);
}

void LEDClass::set(unsigned char state)
{
  LedState = state;
  digitalWrite(LED_PIN, LedState);
}

void LEDClass::toggle()
{
  LedState = !LedState;
  digitalWrite(LED_PIN, LedState);
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_CLOCK)
LEDClass LED;
#endif
