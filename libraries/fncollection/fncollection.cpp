#include <EEPROM.h>

#ifndef ESP8266
  #include <avr/wdt.h>
  #include <avr/interrupt.h>
  #include <avr/pgmspace.h>
//#include "mysleep.h"
//#include "fswrapper.h"
//#include "../version.h"
	#ifdef HAS_USB
		#  include <Drivers/USB/USB.h>
	#endif
#endif

#include "board.h"
#include "display.h"
#include "delay.h"
#include "fncollection.h"
#include "cc1100.h"
#include "stringfunc.h"

#include "clock.h"
#include "fastrf.h"
#include "rf_router.h"
#ifdef HAS_ETHERNET
#  include "ethernet.h"
#endif

uint8_t led_mode = 2;   // Start blinking
/*
#ifdef XLED
#include "xled.h"
#endif
*/
//////////////////////////////////////////////////
// EEprom
FNCOLLECTIONClass::FNCOLLECTIONClass() {
}

// eeprom_write_byte is inlined and it is too big
void FNCOLLECTIONClass::ewb(uint8_t p, uint8_t v)
{
	ewb(p, v, true);
}

__attribute__((__noinline__)) 
void FNCOLLECTIONClass::ewb(uint8_t p, uint8_t v, bool commit)
{ 
#ifdef ESP8266
	EEPROM.write(p, v);
	ewc(commit);
#else
  eeprom_write_byte(p, v);
  eeprom_busy_wait();
#endif
}

void FNCOLLECTIONClass::ewc(bool commit = true)
{
#ifdef ESP8266
	if (commit) {
	  EEPROM.commit();
	}
#endif
}

// eeprom_read_byte is inlined and it is too big
__attribute__((__noinline__)) 
uint8_t FNCOLLECTIONClass::erb(uint8_t p)
{
  //return eeprom_read_byte(p);
  return EEPROM.read(p);
}

// eeprom_read_word
__attribute__((__noinline__)) 
uint16_t FNCOLLECTIONClass::erw(uint8_t p)
{
#ifdef ESP8266
  return (uint16_t)EEPROM.read(p) | ((uint16_t)EEPROM.read(p+1) << 8);
#else
  return eeprom_read_word((uint16_t *)p);
#endif
}

void FNCOLLECTIONClass::ews(uint8_t p, String data, bool commit)
{
  uint8_t _size = data.length();
  for(uint8_t i=0;i<_size && i<19;i++)
  {
    ewb(p++,data[i]);
  }
  ewb(p,'\0');   //Add termination null character for String Data
	if (commit) {
	  ewc();
	}
}
 
String FNCOLLECTIONClass::ers(uint8_t p)
{
  char data[EE_MAX_ENTRY_LEN];
  int len=0;
  unsigned char k = 1;
  while(k != '\0' && len< (EE_MAX_ENTRY_LEN - 1))   //Read until null character
  {    
    k=erb(p++);
    data[len++]=k;
  }
  data[len]='\0';
  return String(data);
}

void FNCOLLECTIONClass::display_string(uint8_t a, uint8_t cnt)
{
	uint8_t s = 1;
  while(cnt-- && s>0) {
		s = erb(a++);
		if (s>0)
      DC(s);
  }

}

void FNCOLLECTIONClass::display_ee_bytes(uint8_t a, uint8_t cnt)
{
  while(cnt--) {
    DH2(erb(a++));
    if(cnt)
      DC(':');
  }

}

void FNCOLLECTIONClass::display_ee_mac(uint8_t a)
{
  display_ee_bytes( a, 6 );
}

#ifdef HAS_ETHERNET
void FNCOLLECTIONClass::display_ee_ip4(uint8_t a)
{
  uint8_t cnt = 4;
  while(cnt--) {
    DU(erb(a++),1);
    if(cnt)
      DC('.');
  }
  
}
#endif

void FNCOLLECTIONClass::read_eeprom(char *in)
{
  uint8_t hb[2], d;
  uint16_t addr;

#ifdef HAS_ETHERNET
  if(in[1] == 'i') {
           if(in[2] == 'm') { display_ee_mac(EE_MAC_ADDR);
    } else if(in[2] == 'd') { DH2(erb(EE_USE_DHCP));
    } else if(in[2] == 'a') { display_ee_ip4(EE_IP4_ADDR);
    } else if(in[2] == 'n') { display_ee_ip4(EE_IP4_NETMASK);
    } else if(in[2] == 'g') { display_ee_ip4(EE_IP4_GATEWAY);
    } else if(in[2] == 'p') { DU(erw(EE_IP4_TCPLINK_PORT),0);
    } else if(in[2] == 'N') { display_ee_ip4(EE_IP4_NTPSERVER);
    } else if(in[2] == 'o') { DH2(erb(EE_IP4_NTPOFFSET));
#   ifdef ESP8266
    } else if(in[2] == 's') { display_string(EE_WPA_SSID, EE_WPA_SSID_MAX);
    } else if(in[2] == 'k') { display_string(EE_WPA_KEY, EE_WPA_KEY_MAX);
    } else if(in[2] == 'D') { display_string(EE_NAME, EE_STR_LEN);
    } else if(in[2] == 'O') { display_ee_ip4(EE_OTA_SERVER);
#   endif
    }
  } else 
#endif
#ifdef HAS_MQTT
  if(in[1] == 'm') {
           if(in[2] == 'm') { display_ee_mac(EE_MAC_ADDR);
    } else if(in[2] == 'd') { DH2(erb(EE_USE_DHCP));
    } else if(in[2] == 'a') { display_ee_ip4(EE_IP4_ADDR);
    } else if(in[2] == 'n') { display_ee_ip4(EE_IP4_NETMASK);
    } else if(in[2] == 'g') { display_ee_ip4(EE_IP4_GATEWAY);
    } else if(in[2] == 'p') { DU(erw(EE_IP4_TCPLINK_PORT),0);
    } else if(in[2] == 'N') { display_ee_ip4(EE_IP4_NTPSERVER);
    }
  } else 
#endif
  if(in[1] == 'M') {
#ifdef HAS_DUDETTE
	  display_ee_mac(EE_DUDETTE_MAC);
#endif
  }  
  else if(in[1] == 'P') {
#ifdef HAS_DUDETTE
	  display_ee_bytes(EE_DUDETTE_PUBL, 16); 
#endif
	}  
  else {
    hb[0] = hb[1] = 0;
    d = STRINGFUNC.fromhex(in+1, hb, 2);
    if(d == 2)
      addr = (hb[0] << 8) | hb[1];
    else
      addr = hb[0];
    if(addr == 0x9999) {
      for(uint8_t i = 0; i < EE_CC1100_CFG; i++) {
        DH2(erb(i));
      }
      DNL();
      for(uint8_t i = 0; i < EE_CC1100_CFG_SIZE; i++) {
        DH2(erb(i+EE_CC1100_CFG));
        if((i&7) == 7)
          DNL();
      }
	  DNL();
      for(uint8_t i = 0; i < EE_CC1100_PA_SIZE; i++) {
        DH2(erb(i+EE_CC1100_PA));
        if((i&7) == 7)
          DNL();
      }
      DNL();
      for(uint8_t i = 0; i < 4; i++) {
        DH2(erb(i+EE_REQBL));
        if((i&7) == 7)
          DNL();
      }
      DNL();
      for(uint8_t i = 0; i < EE_CC1100_CFG_SIZE; i++) {
        DH2(erb(i+EE_FASTRF_CFG));
        if((i&7) == 7)
          DNL();
      }
    } else {
		d = erb((uint8_t)addr);
		DC('R');                    // prefix
		DH(addr,4);                 // register number
		//DS_P( PSTR(" = ") );
		DS(" = ");
		DH2(d);                    // result, hex
		//DS_P( PSTR(" / ") );
		DS(" / ");
		DU(d,2);                    // result, decimal
	}
  }
  DNL();
}

void FNCOLLECTIONClass::write_eeprom(char *in)
{
  write_eeprom(in, true);
}

void FNCOLLECTIONClass::write_eeprom(char *in, bool commit)
{
#ifdef ESP8266
  uint8_t hb[EE_MAX_ENTRY_LEN], d = 0;
#else
  uint8_t hb[6], d = 0;
#endif

#ifdef HAS_ETHERNET
  if(in[1] == 'i') {
    uint8_t addr = 0;
           if(in[2] == 'm') { d=6; STRINGFUNC.fromhex(in+3,hb,6); addr=EE_MAC_ADDR;
    } else if(in[2] == 'd') { d=1; STRINGFUNC.fromdec(in+3,hb);   addr=EE_USE_DHCP;
    } else if(in[2] == 'a') { d=4; STRINGFUNC.fromip (in+3,hb,4); addr=EE_IP4_ADDR;
    } else if(in[2] == 'n') { d=4; STRINGFUNC.fromip (in+3,hb,4); addr=EE_IP4_NETMASK;
    } else if(in[2] == 'g') { d=4; STRINGFUNC.fromip (in+3,hb,4); addr=EE_IP4_GATEWAY;
    } else if(in[2] == 'p') { d=2; STRINGFUNC.fromdec(in+3,hb);   addr=EE_IP4_TCPLINK_PORT; Serial.print(hb[0]);Serial.print(" ");Serial.println(hb[1]);
    } else if(in[2] == 'N') { d=4; STRINGFUNC.fromip (in+3,hb,4); addr=EE_IP4_NTPSERVER;
    } else if(in[2] == 'o') { d=1; STRINGFUNC.fromhex(in+3,hb,1); addr=EE_IP4_NTPOFFSET;
#   ifdef HAS_NTP
      extern int8_t ntp_gmtoff;
      ntp_gmtoff = hb[0];
#   endif
#   ifdef ESP8266
    } else if(in[2] == 's') { d=EE_WPA_SSID_MAX; STRINGFUNC.fromchars(in+3,hb, EE_WPA_SSID_MAX); addr=EE_WPA_SSID;
    } else if(in[2] == 'k') { d=EE_WPA_KEY_MAX; STRINGFUNC.fromchars(in+3,hb, EE_WPA_KEY_MAX); addr=EE_WPA_KEY;
    } else if(in[2] == 'D') {
			d=EE_STR_LEN; STRINGFUNC.fromchars(in+3,hb, EE_STR_LEN); addr=EE_NAME;
			uint8_t len = strlen((const char*)hb);
			bool compliant = (len <= 24);
			for (uint8_t i = 0; compliant && i < len; i++)
					if (!isalnum(hb[i]) && hb[i] != '-')
							compliant = false;
			if (hb[len - 1] == '-')
					compliant = false;

			if (!compliant) {
					Serial.print("WiD: hostname '");
					Serial.print((const char*)hb);
					Serial.println("' is not compliant with RFC952 (0-9 a-z A-Z -)");
					return;
			}
    } else if(in[2] == 'O') { d=4; STRINGFUNC.fromip (in+3,hb,4); addr=EE_OTA_SERVER;
#   endif
    }
    for(uint8_t i = 0; i < d; i++)
      ewb(addr++, hb[i], false);
	  ewc(commit);
  } else 
#endif
#ifdef HAS_MQTT
  if(in[1] == 'm') {
    uint8_t addr = 0;
           if(in[2] == 'm') { d=6; STRINGFUNC.fromhex(in+3,hb,6); addr=EE_MAC_ADDR;
    } else if(in[2] == 'd') { d=1; STRINGFUNC.fromdec(in+3,hb);   addr=EE_USE_DHCP;
    } else if(in[2] == 'a') { d=4; STRINGFUNC.fromip (in+3,hb,4); addr=EE_IP4_ADDR;
    } else if(in[2] == 'n') { d=4; STRINGFUNC.fromip (in+3,hb,4); addr=EE_IP4_NETMASK;
    } else if(in[2] == 'g') { d=4; STRINGFUNC.fromip (in+3,hb,4); addr=EE_IP4_GATEWAY;
    } else if(in[2] == 'p') { d=2; STRINGFUNC.fromdec(in+3,hb);   addr=EE_IP4_TCPLINK_PORT; 
    }
		for(uint8_t i = 0; i < d; i++)
      ewb(addr++, hb[i], false);
	  ewc(commit);
  } else 
#endif
  {
    uint16_t addr;
    d = STRINGFUNC.fromhex(in+1, hb, 3);
    if(d < 2)
      return;
    if(d == 2)
      addr = hb[0];
    else
      addr = (hb[0] << 8) | hb[1];
      
    ewb((uint8_t)addr, hb[d-1]);

    // If there are still bytes left, then write them too
    in += (2*d+1);
    while(in[0]) {
      if(!STRINGFUNC.fromhex(in, hb, 1))
        return;
      ewb((uint8_t)++addr, hb[0], false);
      in += 2;
    }
	ewc(commit);
  }
}

void FNCOLLECTIONClass::eeprom_init(void)
{
#ifdef ESP8266
  EEPROM.begin(0xFF);
#endif
  if(erb(EE_MAGIC_OFFSET)   != VERSION_1 ||
     erb(EE_MAGIC_OFFSET+1) != VERSION_2)
       eeprom_factory_reset(0);

  led_mode = erb(EE_LED);
#ifdef XLED
  switch (led_mode) {
    case 0:
      xled_pattern = 0;
      break;
    case 1:
      xled_pattern = 0xffff;
      break;
    case 2:
      xled_pattern = 0xff00;
      break;
    case 3:
      xled_pattern = 0xa000;
      break;
    case 4:
      xled_pattern = 0xaa00;
      break;
  }
#endif
#ifdef HAS_SLEEP
  sleep_time = erb(EE_SLEEPTIME);
#endif
}

void FNCOLLECTIONClass::eeprom_factory_reset(char *in)
{
  CC1100.cc_factory_reset(false);

  //DS("B");DNL();
  ewb(EE_RF_ROUTER_ID, 0, false);
  ewb(EE_RF_ROUTER_ROUTER, 0, false);
  ewb(EE_REQBL, 0, false);
  ewb(EE_LED, 2, false);

# ifdef HAS_LCD
    ewb(EE_CONTRAST,   0x40, false);
    ewb(EE_BRIGHTNESS, 0x80, false);
    ewb(EE_SLEEPTIME, 30, false);
# endif
# ifdef HAS_ETHERNET
    Ethernet.reset(false);
# endif
# ifdef HAS_FS
    ewb(EE_LOGENABLED, 0x00, false);
# endif
# ifdef HAS_RF_ROUTER
    ewb(EE_RF_ROUTER_ID, 0x00, false);
    ewb(EE_RF_ROUTER_ROUTER, 0x00, false);
# endif
  ewb(EE_MAGIC_OFFSET  , VERSION_1, false);
  Serial.println("ewb(EE_MAGIC_OFFSET)");
  ewb(EE_MAGIC_OFFSET+1, VERSION_2);
  return;
  if(in[1] != 'x')
    prepare_boot(0);
}

// LED
void FNCOLLECTIONClass::ledfunc(char *in)
{
  STRINGFUNC.fromhex(in+1, &led_mode, 1);
#ifdef XLED
  switch (led_mode) {
    case 0:
      xled_pattern = 0; 
      break;
    case 1:
      xled_pattern = 0xffff; 
      break;
    case 2:
      xled_pattern = 0xff00; 
      break;
    case 3:
      xled_pattern = 0xa000; 
      break;
    case 4:
      xled_pattern = 0xaa00; 
      break;
  }
  //Serial.println("~XLED");
#else
  if(led_mode & 1)
    LED_ON();
  else
    LED_OFF();
  //Serial.println("~LED");
#endif

  ewb(EE_LED, led_mode);
}

//////////////////////////////////////////////////
// boot
void FNCOLLECTIONClass::prepare_boot(char *in)
{
  uint8_t bl = 0;
  if(in)
    STRINGFUNC.fromhex(in+1, &bl, 1);

  if(bl == 0xff)             // Allow testing
    while(1);
    
  if(bl) {
  #ifndef ESP8266		         // Next reboot we'd like to jump to the bootloader. 
    ewb( EE_REQBL, 1 );      // Simply jumping to the bootloader from here
                             // wont't work. Neither helps to shutdown USB
                             // first.
	#else
		Ethernet.ota();          // on ESP8266 initialize over-the-air-update
	  return;
	#endif
	}
  #ifdef HAS_USB
  //  USB_ShutDown();            // ??? Needed?
  #endif
  #ifdef HAS_FS
    fs_sync(&fs);              // Sync the filesystem
  #endif

  #ifndef ESP8266
		TIMSK0 = 0;                // Disable the clock which resets the watchdog
		cli();
		
		wdt_enable(WDTO_15MS);       // Make sure the watchdog is running 
		while (1);                 // go to bed, the wathchdog will take us to reset
	#else
		ESP.restart();
	#endif
}

void FNCOLLECTIONClass::version(char *in)
{
#if defined(CUL_HW_REVISION)
  if (in[1] == 'H') {
    //DS_P( PSTR(CUL_HW_REVISION) );
	  DS(CUL_HW_REVISION);
    DNL();
    return;
  }
#endif
#if defined(VERSION_OTA)
  // version of image
  if (in[1] == 'I') {
	  DS(con_cat(VERSION_OTA, VERSION_BOARD));
    DNL();
    return;
  }
#endif

#ifdef MULTI_FREQ_DEVICE     // check 433MHz version marker
  if (!bit_is_set(MARK433_PIN, MARK433_BIT))
    DS("V " VERSION " " BOARD_ID_STR433 );
  else
#endif
    DS("V " VERSION " " BOARD_ID_STR);
  DNL();
}

void FNCOLLECTIONClass::dumpmem(uint8_t *addr, uint16_t len)
{
  for(uint16_t i = 0; i < len; i += 16) {
    uint8_t l = len;
    if(l > 16)
      l = 16;
    DH(i,4);
    DC(':'); DC(' ');
    for(uint8_t j = 0; j < l; j++) {
      DH2(addr[j]);
      if(j&1)
        DC(' ');
    }
    DC(' ');
    for(uint8_t j = 0; j < l; j++) {
      if(addr[j] >= ' ' && addr[j] <= '~')
        DC(addr[j]);
      else
        DC('.');
    }
    addr += 16;
    DNL();
  }
  DNL();
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_FNCOLLECTION)
FNCOLLECTIONClass FNcol;
#endif
