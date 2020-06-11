#ifndef _RF_SEND_H
#define _RF_SEND_H

#include <stdint.h>

#define MAX_CREDIT 3600// 900       // max 9 seconds burst / 25% of the hourly budget

class RfSendClass {
public:
	/* public prototypes */
	RfSendClass();
	void fs20send(char *in);
	void ftz_send(char *in);
	void rawsend(char *in);
	void em_send(char *in);
	void ks_send(char *in);
	void ur_send(char *in);
    void hm_send(char *in);
	void addParityAndSend(char *in, uint8_t startcs, uint8_t repeat);
	void addParityAndSendData(uint8_t *hb, uint8_t hblen,
							uint8_t startcs, uint8_t repeat);


    uint16_t credit_10ms;
private:
	void send_bit(uint8_t bit, uint8_t edge);
	void sendraw(uint8_t *msg, uint8_t sync, uint8_t nbyte, uint8_t bitoff, 
                uint8_t repeat, uint8_t pause, uint8_t edge, uint8_t addH, uint8_t addL);
	int abit(uint8_t b, uint8_t *obuf, uint8_t *obyp, uint8_t obi);


};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RF_SEND)
extern RfSendClass RfSend;
#endif

#endif
