#ifndef _FASTRF_H
#define _FASTRF_H

#define FASTRF_MODE_ON	1
#define FASTRF_MODE_OFF	0

class FastRFClass {
public:
  uint8_t fastrf_on;
	
  //void mode(uint8_t on);
  void func(char *in);
  void Task(void);
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_FASTRF)
extern FastRFClass FastRF;
#endif

#endif
