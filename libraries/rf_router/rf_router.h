#ifndef _RF_ROUTER_H
#define _RF_ROUTER_H

#include "ringbuffer.h"
class RfRouterClass {
public:
	void init(void);
	void func(char *);
	void task(void);
	void flush(void);

	uint8_t rf_router_status;
	uint8_t rf_router_myid;// Own id
	uint8_t rf_router_target;// id of the router
	uint8_t rf_router_hsec;
	uint8_t rf_router_sendtime; // relative ticks
	uint8_t rf_nr_send_checks;// relative ticks

#ifndef RFR_SHADOW
	RingbufferClass RFR_Buffer;
#endif
#ifdef RFR_DEBUG
	uint16_t nr_t, nr_f, nr_e, nr_k, nr_h, nr_r, nr_plus;
#endif
#ifdef RFR_FILTER
    uint8_t filter[6];
#endif
#undef RFR_USBECHO  // 1. mal gesehen in 1.67 !?
private:
	void send(uint8_t);
	void usbMsg(char *s);
	void ping(void);
	void sethigh(uint16_t dur);
	void setlow(uint16_t dur);
};

#ifdef RFR_SHADOW
#define RFR_Buffer TTY_Tx_Buffer
#endif
extern RingbufferClass RFR_Buffer;

#define RF_ROUTER_PROTO_ID    'u'      // 117 / 0x75

#define RF_ROUTER_INACTIVE    0
#define RF_ROUTER_SYNC_RCVD   1
#define RF_ROUTER_DATA_WAIT   2
#define RF_ROUTER_GOT_DATA    3
#define RF_ROUTER_SENDING     4

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RF_ROUTER)
extern RfRouterClass RfRouter;
#endif

#endif
