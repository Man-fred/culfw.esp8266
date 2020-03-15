#ifndef __ETHERNET_H_
#define __ETHERNET_H_

#include "board.h"

class EthernetClass {
public:
	void close(char *);
	void reset(void);
	void init(void);
	void Task(void);
#ifndef ESP8266

	// NOTE:
	// typedef struct tcplink_state uip_tcp_appstate_t;
	// typedef struct dhcpc_state uip_udp_appstate_t;
	extern uint8_t eth_debug;
	extern uint8_t eth_initialized;
#else
    void putchar(char data); //von tcp_link.h uebernommen
	uint8_t eth_debug;
	uint8_t eth_initialized;
#endif //ESP8266
	void erip(void *ip, uint8_t *addr);      // EEprom read IP

	void udp_appcall(void);
	void tcp_appcall(void);
	void func(char *);
};

#ifndef ESP8266
#   define UIP_UDP_APPCALL udp_appcall
#   define UIP_APPCALL     tcp_appcall
#endif //ESP8266

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_Ethernet)
extern EthernetClass Ethernet;
#endif

#endif
