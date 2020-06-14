#ifndef _RF_MORITZ_H
#define _RF_MORITZ_H

#include "board.h"

#define MAX_MORITZ_MSG 30

extern uint8_t moritz_on;

class RfMoritzClass {
public:
  RfMoritzClass();
	void init(void);
	void task(void);
	void func(char *in);
	static uint8_t autoAckAddr[3];
	static uint8_t fakeWallThermostatAddr[3];
	uint8_t on(uint8_t onNew = 2);
private:
  uint8_t onState;
  uint32_t lastSendingTicks;

  void send(char *in);
	void sendraw(uint8_t* buf, int longPreamble);
	void sendAck(uint8_t* enc);
	void handleAutoAck(uint8_t* enc);
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RF_MORITZ)
extern RfMoritzClass Moritz;
#endif

#endif
