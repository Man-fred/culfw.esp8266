#include <EEPROM.h>
//esp8266 #include <wdt.h>
//esp8266 #include <interrupt.h>
#include <pgmspace.h>

#include "board.h"
#include "display.h"
#include "delay.h"
#include "fncollection.h"
#include "cc1100.h"
#include "stringfunc.h"

//#include "../version.h"
#ifdef HAS_USB
//#  include <Drivers/USB/USB.h>
#endif
#include "clock.h"
//#include "mysleep.h"
//#include "fswrapper.h"
#include "fastrf.h"
#include "rf_router.h"
#ifdef HAS_ETHERNET
#  include "ethernet.h"
#endif
// ??? doppelt ??? #include <avr/wdt.h>

uint8_t led_mode = 2;   // Start blinking
/*
#ifdef XLED
#include "xled.h"
#endif
*/
//////////////////////////////////////////////////
// EEprom
FNCOLLECTIONClass::FNCOLLECTIONClass() {
	EEPROM.begin(0xFF);
}

// eeprom_write_byte is inlined and it is too big
__attribute__((__noinline__)) 
void FNCOLLECTIONClass::ewb(uint8_t p, uint8_t v)
{
	EEPROM.write(p, v);
	EEPROM.commit();
//esp8266  eeprom_write_byte(p, v);
//esp8266  eeprom_busy_wait();
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
  //return eeprom_read_word((uint16_t *)p);
  return EEPROM.read(p++) << 8 + EEPROM.read(p);
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
    } else if(in[2] == 'N') { display_ee_ip4(EE_IP4_NTPSERVER);
    } else if(in[2] == 'o') { DH2(erb(EE_IP4_NTPOFFSET));
    } else if(in[2] == 'p') {
      DU(erw(EE_IP4_TCPLINK_PORT), 0);
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
  uint8_t hb[6], d = 0;

#ifdef HAS_ETHERNET
  if(in[1] == 'i') {
    uint8_t addr = 0;
           if(in[2] == 'm') { d=6; STRINGFUNC.fromhex(in+3,hb,6); addr=EE_MAC_ADDR;
    } else if(in[2] == 'd') { d=1; STRINGFUNC.fromdec(in+3,hb);   addr=EE_USE_DHCP;
    } else if(in[2] == 'a') { d=4; STRINGFUNC.fromip (in+3,hb,4); addr=EE_IP4_ADDR;
    } else if(in[2] == 'n') { d=4; STRINGFUNC.fromip (in+3,hb,4); addr=EE_IP4_NETMASK;
    } else if(in[2] == 'g') { d=4; STRINGFUNC.fromip (in+3,hb,4); addr=EE_IP4_GATEWAY;
    } else if(in[2] == 'p') { d=2; STRINGFUNC.fromdec(in+3,hb);   addr=EE_IP4_TCPLINK_PORT;
    } else if(in[2] == 'N') { d=4; STRINGFUNC.fromip (in+3,hb,4); addr=EE_IP4_NTPSERVER;
    } else if(in[2] == 'o') { d=1; STRINGFUNC.fromhex(in+3,hb,1); addr=EE_IP4_NTPOFFSET;
#ifdef HAS_NTP
      extern int8_t ntp_gmtoff;
      ntp_gmtoff = hb[0];
#endif
    }
    for(uint8_t i = 0; i < d; i++)
      ewb(addr++, hb[i]);

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
      ewb((uint8_t)++addr, hb[0]);
      in += 2;
    }
  }
}

void FNCOLLECTIONClass::eeprom_init(void)
{
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
  ewb(EE_MAGIC_OFFSET  , VERSION_1);
  ewb(EE_MAGIC_OFFSET+1, VERSION_2);

  CC1100.cc_factory_reset();

DS("B");DNL();
  ewb(EE_RF_ROUTER_ID, 0);
  ewb(EE_RF_ROUTER_ROUTER, 0);
  ewb(EE_REQBL, 0);
  ewb(EE_LED, 2);

#ifdef HAS_LCD
  ewb(EE_CONTRAST,   0x40);
  ewb(EE_BRIGHTNESS, 0x80);
  ewb(EE_SLEEPTIME, 30);
#endif
#ifdef HAS_ETHERNET
  Ethernet.reset();
#endif
#ifdef HAS_FS
  ewb(EE_LOGENABLED, 0x00);
#endif
#ifdef HAS_RF_ROUTER
  ewb(EE_RF_ROUTER_ID, 0x00);
  ewb(EE_RF_ROUTER_ROUTER, 0x00);
#endif
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
#else
  if(led_mode & 1)
    LED_ON();
  else
    LED_OFF();
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
    
  if(bl)                     // Next reboot we'd like to jump to the bootloader.
    ewb( EE_REQBL, 1 );      // Simply jumping to the bootloader from here
                             // wont't work. Neither helps to shutdown USB
                             // first.
                             
#ifdef HAS_USB
//  USB_ShutDown();            // ??? Needed?
#endif
#ifdef HAS_FS
  fs_sync(&fs);              // Sync the filesystem
#endif


//esp8266  TIMSK0 = 0;                // Disable the clock which resets the watchdog
  cli();
  
  wdt_enable(WDTO_15MS);       // Make sure the watchdog is running 
  while (1);                 // go to bed, the wathchdog will take us to reset
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
