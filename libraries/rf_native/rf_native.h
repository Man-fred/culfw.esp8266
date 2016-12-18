#ifndef _RF_NATIVE_H
#define _RF_NATIVE_H

class RfNativeClass {
public:
	void native_task(void);
	void native_func(char *in);

#ifdef LACROSSE_HMS_EMU
	uint8_t payload[5];
#endif

private:
	void native_init(uint8_t mode);

    uint8_t native_on = 0;


};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RF_NATIVE)
extern RfNativeClass RfNative;
#endif

#endif
