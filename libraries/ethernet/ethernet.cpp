#include <pgmspace.h>
#ifndef ESP8266
#  include <avr/boot.h>
#endif
#include "board.h"
#include "ethernet.h"
#include "fncollection.h"
#include "stringfunc.h"
#include "display.h"
#ifndef ESP8266
#  include "delay.h"
#  include "timer.h"
#  include "ntp.h"
#  include "mdns_sd.h"
#  include "led.h"
#  include "uip_arp.h"
#  include "drivers/interfaces/network.h"
#  include "apps/dhcpc/dhcpc.h"
#  include "delay.h"
#endif

uint8_t eth_debug = 0;

static uint8_t dhcp_state;

EthernetClass::EthernetClass() {
	ReplyPos = 0;
}

void EthernetClass::init(void)
{
#ifndef ESP8266
  // reset Ethernet
  ENC28J60_RESET_DDR  |= _BV( ENC28J60_RESET_BIT );
  ENC28J60_RESET_PORT &= ~_BV( ENC28J60_RESET_BIT );

  MYDELAY.my_delay_ms( 200 );
  // unreset Ethernet
  ENC28J60_RESET_PORT |= _BV( ENC28J60_RESET_BIT );

  MYDELAY.my_delay_ms( 200 );
  network_init();
  mac.addr[0] = FNcol.erb(EE_MAC_ADDR+0);
  mac.addr[1] = FNcol.erb(EE_MAC_ADDR+1);
  mac.addr[2] = FNcol.erb(EE_MAC_ADDR+2);
  mac.addr[3] = FNcol.erb(EE_MAC_ADDR+3);
  mac.addr[4] = FNcol.erb(EE_MAC_ADDR+4);
  mac.addr[5] = FNcol.erb(EE_MAC_ADDR+5);
  network_set_MAC(mac.addr);

  uip_setethaddr(mac);
  uip_init();
  ntp_conn = 0;

  // setup two periodic timers
  timer_set(&periodic_timer, CLOCK_SECOND / 4);
  timer_set(&arp_timer, CLOCK_SECOND * 10);
  
  if(FNcol.erb(EE_USE_DHCP)) {
    network_set_led(0x4A6);// LED A: Link Status  LED B: Blink slow
    dhcpc_init(&mac);
    dhcp_state = PT_WAITING;

  } else {
    dhcp_state = PT_ENDED;
    set_eeprom_addr();

  }
#else
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  //Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    //Serial.print(".");
  }
  //Serial.println();

  eth_initialized = Udp.begin(2323);
  server.begin(2323);
  IPAddress localIP = WiFi.localIP();
  uip_hostaddr[0] = localIP[0]<<8 | localIP[1];
  uip_hostaddr[1] = localIP[2]<<8 | localIP[3];
  WiFi.macAddress(uip_ethaddr.addr);
  Serial.printf("\nUDP %d, TCP %d on %s:%d\n", eth_initialized, tcp_initialized, WiFi.localIP().toString().c_str(), 2323);
#endif
}

void EthernetClass::close(char *in)
{
    Serial.println("[Client disonnected]");
}

void EthernetClass::reset(bool commit = true)
{
  char buf[21];
  uint16_t serial = 0;

  buf[1] = 'i';
  buf[2] = 'd'; strcpy_P(buf+3, PSTR("1"));             FNcol.write_eeprom(buf, false);//DHCP
  buf[2] = 'a'; strcpy_P(buf+3, PSTR("192.168.178.244")); FNcol.write_eeprom(buf, false);//IP
  buf[2] = 'n'; strcpy_P(buf+3, PSTR("255.255.255.0")); FNcol.write_eeprom(buf, false);
  buf[2] = 'g'; strcpy_P(buf+3, PSTR("192.168.178.1"));   FNcol.write_eeprom(buf, false);//GW
  buf[2] = 'p'; strcpy_P(buf+3, PSTR("2323"));          FNcol.write_eeprom(buf, false);
  buf[2] = 'N'; strcpy_P(buf+3, PSTR("0.0.0.0"));       FNcol.write_eeprom(buf, false);//==GW
  buf[2] = 'o'; strcpy_P(buf+3, PSTR("00"));            FNcol.write_eeprom(buf, false);//GMT

#ifdef EE_DUDETTE_MAC
  // check for mac stored during manufacture
  uint8_t *ee = EE_DUDETTE_MAC;
  if (FNcol.erb( ee++ ) == 0xa4)
    if (FNcol.erb( ee++ ) == 0x50)
      if (FNcol.erb( ee++ ) == 0x55) {
        buf[2] = 'm'; strcpy_P(buf+3, PSTR("A45055"));        // busware.de OUI range
        STRINGFUNC.tohex(FNcol.erb( ee++ ), (uint8_t*)buf+9);
        STRINGFUNC.tohex(FNcol.erb( ee++ ), (uint8_t*)buf+11);
        STRINGFUNC.tohex(FNcol.erb( ee++ ), (uint8_t*)buf+13);
        buf[15] = 0;
        FNcol.write_eeprom(buf, commit);
        return;
      } 
#endif

  // Generate a "unique" MAC address from the unique serial number
  buf[2] = 'm'; strcpy_P(buf+3, PSTR("A45055"));        // busware.de OUI range
#define bsbg boot_signature_byte_get

//  STRINGFUNC.tohex(bsbg(0x0e)+bsbg(0x0f), (uint8_t*)buf+9);
//  STRINGFUNC.tohex(bsbg(0x10)+bsbg(0x11), (uint8_t*)buf+11);
//  STRINGFUNC.tohex(bsbg(0x12)+bsbg(0x13), (uint8_t*)buf+13);

#ifndef ESP8266 
  for (uint8_t i = 0x00; i < 0x20; i++) 
       serial += bsbg(i);
#endif

  STRINGFUNC.tohex(0, (uint8_t*)buf+9);
  STRINGFUNC.tohex((serial>>8) & 0xff, (uint8_t*)buf+11);
  STRINGFUNC.tohex(serial & 0xff, (uint8_t*)buf+13);
  
  buf[15] = 0;
  FNcol.write_eeprom(buf, commit);
}

// 1 char senden
//__attribute__((__noinline__))
void EthernetClass::putChar(char data)
{
	ReplyBuffer[ReplyPos++] = data;
	ReplyBuffer[ReplyPos] = 0;
	if (data == 0 || data == '\n' || data == '\r' || ReplyPos >= 70) {
		// send a reply, to the IP address and port that sent us the packet we received
		/*if (ReplyPos > 1 || (data != '\n' && data != '\r'))*/{
			if (ip_active == TCP_MAX){
				Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
				IPAddress ip(192, 168, 178, 59);
				//Serial.print("put2 ");
				if (Udp.beginPacket(ip, 2323)) {
				int n = Udp.write(ReplyBuffer);
				//Serial.print(n);
				n = Udp.endPacket();
				//Serial.println(n);
				}
			}
			if (ip_active >= 0 && ip_active < tcp_initialized) 
			{
				int n = Tcp[ip_active].print(ReplyBuffer);
			}
			//Serial.printf("\nsend to client %d %s:%d\n", ip_active, Tcp[ip_active].remoteIP().toString().c_str(), Tcp[ip_active].remotePort());
		}
		ReplyPos = 0;
	}
}

void EthernetClass::display_mac(uint8_t *a)
{
  uint8_t cnt = 6;
  while(cnt--) {
    DH2(*a++);
    if(cnt)
      DC(':');
  }
}

void EthernetClass::display_ip4(uint8_t *a)
{
  uint8_t cnt = 4;
  while(cnt--) {
    DU(*a++,1);
    if(cnt)
      DC('.');
  }
}

void EthernetClass::func(char *in)
{
  if(in[1] == 'i') {
    init();

  } else if(in[1] == 'c') {
    display_ip4((uint8_t *)uip_hostaddr); DC(' ');
    display_mac((uint8_t *)uip_ethaddr.addr);
    DNL();
  } else if(in[1] == 'd') {
    eth_debug = (eth_debug+1) & 0x3;
    DH2(eth_debug);
    DNL();

  } else if(in[1] == 'n') {
    // ntp_sendpacket();

  }
}

void EthernetClass::dumppkt(void)
{
  /*
  uint8_t *a = uip_buf;

  DC('e');DC(' ');
  DU(uip_len,5);

  display_channel &= ~DISPLAY_TCP;
  uint8_t ole = log_enabled;
  log_enabled = 0;
  DC(' '); DC('d'); DC(' '); display_mac(a); a+= sizeof(struct uip_eth_addr);
  DC(' '); DC('s'); DC(' '); display_mac(a); a+= sizeof(struct uip_eth_addr);
  DC(' '); DC('t'); DH2(*a++); DH2(*a++);
  /////////////////DNL();

  if(eth_debug > 2)
    dumpmem(a, uip_len - sizeof(struct uip_eth_hdr));
  display_channel |= DISPLAY_TCP;
  log_enabled = ole;
  */
}

void EthernetClass::Task(void) {
     int i;
#ifndef ESP8266
     ethernet_process();
     
     if(timer_expired(&periodic_timer)) {
	  timer_reset(&periodic_timer);
	  
	  for(i = 0; i < UIP_CONNS; i++) {
	       uip_periodic(i);
	       if(uip_len > 0) {
		    uip_arp_out();
		    network_send();
	       }
	  }
	  
	  for(i = 0; i < UIP_UDP_CONNS; i++) {
	       uip_udp_periodic(i);
	       if(uip_len > 0) {
		    uip_arp_out();
		    network_send();
	       }
	  }

	  interface_periodic();
	  
     }
     
     
     if(timer_expired(&arp_timer)) {
	  timer_reset(&arp_timer);
	  uip_arp_timer();
	  
     }
#else
  // if there's udp-data available, read a packet
  int packetSize = Udp.parsePacket();
  //Serial.print(packetSize);
  if (packetSize) {
    /*Serial.printf("Received packet of size %d from %s:%d\n    (to %s:%d, free heap = %d B)\n",
                  packetSize,
                  Udp.remoteIP().toString().c_str(), Udp.remotePort(),
                  Udp.destinationIP().toString().c_str(), Udp.localPort(),
                  ESP.getFreeHeap());*/

    // read the packet into packetBufffer
    int n = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    packetBuffer[n] = 0;
    //Serial.printf("UDP: %s\n", packetBuffer);
    //ip_active = TCP_MAX;
  }
  // tcp-data
  if (tcp_initialized < TCP_MAX){
	  Tcp[tcp_initialized] = server.available();
	  if (Tcp[tcp_initialized]) {
      //    Serial.printf("\nUDP %d, TCP %d to %s:%d\n", eth_initialized, tcp_initialized, Tcp[tcp_initialized].remoteIP().toString().c_str(), Tcp[tcp_initialized].remotePort());
		  tcp_initialized++;
	  }
  }
  for (uint8_t i = 0; i < tcp_initialized; i++) 
  {
		 yield();
    // we have a client sending some request
		if (Tcp[i].connected())
		{
			while (Tcp[i].available())
			{
				//int n = Tcp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
				//String line = Tcp.readStringUntil('\r');
				char line = Tcp[i].read();
				if(line > 0){
					//Serial.print(line);
					TTYdata.rxBuffer.put(line);
				}
			}
			ip_active = i;
			//TTYdata.analyze_ttydata(DISPLAY_USB|DISPLAY_TCP);
			TTYdata.analyze_ttydata(DISPLAY_TCP);
		} else {
			for(uint8_t k=i; k<TCP_MAX-1; k++){
				Tcp[k] = Tcp[k+1];
			}
			tcp_initialized--;
			if (i == ip_active)
				ip_active = -1;
		}
  }  
#endif
}

// EEPROM Read IP
void EthernetClass::erip(void *ip, uint8_t *addr)
{
#ifndef ESP8266
    uip_ipaddr(ip, FNcol.erb(addr[0]), FNcol.erb(addr[1]), FNcol.erb(addr[2]), FNcol.erb(addr[3]));
#else
#endif
}

// EEPROM Write IP
void EthernetClass::ewip(const uint16_t ip[2], uint8_t *addr)
{
  /*
  uint16_t ip0 = HTONS(ip[0]);
  uint16_t ip1 = HTONS(ip[1]);
  ewb(addr+0, ip0>>8);
  ewb(addr+1, ip0&0xff);
  ewb(addr+2, ip1>>8);
  ewb(addr+3, ip1&0xff);
  */
}

void EthernetClass::ip_initialized(void)
{
#ifndef ESP8266
  network_set_led(0x476);// LED A: Link Status  LED B: TX/RX
  tcplink_init();
  ntp_init();
#endif
#ifdef HAS_MDNS
  mdns_init();
#endif
}


void EthernetClass::dhcpc_configured(const struct dhcpc_state *s)
{
  if(s == 0) {
    set_eeprom_addr();
    return;
  }
#ifndef ESP8266
  ewip(s->ipaddr,         EE_IP4_ADDR);    uip_sethostaddr(s->ipaddr);
  ewip(s->default_router, EE_IP4_GATEWAY); uip_setdraddr(s->default_router);
  ewip(s->netmask,        EE_IP4_NETMASK); uip_setnetmask(s->netmask);
  //resolv_conf(s->dnsaddr);
  uip_udp_remove(s->conn);
  ip_initialized();
#endif
}

void EthernetClass::set_eeprom_addr()
{
#ifndef ESP8266
  uip_ipaddr_t ipaddr;
  erip(ipaddr, EE_IP4_ADDR);    uip_sethostaddr(ipaddr);
  erip(ipaddr, EE_IP4_GATEWAY); uip_setdraddr(ipaddr);
  erip(ipaddr, EE_IP4_NETMASK); uip_setnetmask(ipaddr);
  ip_initialized();
#endif
}

void EthernetClass::tcp_appcall()
{
  /*
  if(uip_conn->lport == tcplink_port)
    tcplink_appcall();
  */
}

void EthernetClass::udp_appcall()
{
  /*
  if(dhcp_state != PT_ENDED) {
    dhcp_state = handle_dhcp();

  } else if(uip_udp_conn &&
            uip_newdata() &&
            uip_udp_conn->lport == HTONS(NTP_PORT)) {
    ntp_digestpacket();
  }
#ifdef HAS_MDNS
   else if(uip_udp_conn &&
            uip_newdata() &&
            uip_udp_conn->lport == HTONS(MDNS_PORT)) {
    mdns_new_data();
  } 
#endif
  */
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_Ethernet)
EthernetClass Ethernet;
#endif

