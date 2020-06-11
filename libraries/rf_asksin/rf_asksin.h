#ifndef _RF_ASKSIN_H
#define _RF_ASKSIN_H

#define ASKSIN_WAIT_TICKS_CCA	188	//125 Hz

#ifndef HAS_ASKSIN_FUP
#define MAX_ASKSIN_MSG 30
#else
#define MAX_ASKSIN_MSG 50
#endif

class RfAsksinClass {
public:
  uint8_t on;

  void init(void);
  void task(void);
  void func(char *in);
private:
  static void reset_rx(void);
	void send(char *in);
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RF_ASKSIN)
extern RfAsksinClass RfAsksin;
#endif

#endif
