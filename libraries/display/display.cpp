#include "display.h"
#include <HardwareSerial.h>

#include "ringbuffer.h"
#include "rf_router.h"
#ifndef ESP8266
	#ifdef HAS_USB
	  #ifdef HAS_CDC
	    #include "cdc.h"
		#endif
	#else
		#include "serial.h"
	#endif
	#include "led.h"
	#include "delay.h"
	#ifdef HAS_LCD
	  #include "pcf8833.h"
	#endif
	#include "ttydata.h"            // callfn
	#include "fht.h"                // fht_hc
	#include "rf_router.h"
	#include "clock.h"
	#include "log.h"
#endif
#ifdef HAS_PRIVATE_CHANNEL
  #include "private_channel.h"
#endif
#ifdef HAS_ETHERNET
  #include "ethernet.h"
#endif
#ifdef HAS_DOGM
  #include "dogm16x.h"
#endif

#include <stddef.h>
#include "WString.h"
#include "Printable.h"

//#include "stringfunc.h"

DisplayClass::DisplayClass() {
	log_enabled = 0;
	channel = 0;
	echo_serial = true;
}

//////////////////////////////////////////////////
// Display routines
void DisplayClass::chr(char data)
{
#ifdef RFR_SHADOW
  uint8_t buffer_free = 1;
# define buffer_used() buffer_free=0
#else
# define buffer_free 1
# define buffer_used()
#endif

/* don't change the channel in DisplayClass!
#ifdef HAS_RF_ROUTER
  channel = (DISPLAY_USB|DISPLAY_RFROUTER);
#else
  channel = DISPLAY_USB;
#endif
#ifdef HAS_ETHERNET
  channel |= DISPLAY_TCP;
#endif
*/

#ifdef HAS_ETHERNET
  if(channel & DISPLAY_TCP)
    Ethernet.putChar( data );
#endif

#ifdef HAS_PRIVATE_CHANNEL
    private_putchar( data );
#endif

#ifdef HAS_DOGM
  if(channel & DISPLAY_DOGM)
    dogm_putchar( data );
#endif

#ifdef HAS_USB
  if(/*todo USB_IsConnected && */ ((channel & DISPLAY_USB) || echo_serial)) {
		#ifndef ESP8266
		  #ifdef HAS_CDC
				if(TTY_Tx_Buffer.nbytes >= TTY_BUFSIZE)
					CDC_Task();
				TTY_Tx_Buffer.put(data);
				if(data == '\n')
					CDC_Task();
				buffer_used();
			#endif
		#else
			Serial.print(data);
		#endif
  }
#endif

#ifdef HAS_UART
  if((channel & DISPLAY_USB) || echo_serial) {
    if((TTY_Tx_Buffer.nbytes  < TTY_BUFSIZE-2) ||
       (TTY_Tx_Buffer.nbytes  < TTY_BUFSIZE && (data == '\r' || data == '\n')))
    TTY_Tx_Buffer.put(data);
  }
#endif

#ifdef HAS_RF_ROUTER
  if(RfRouter.rf_router_target &&
     (channel & DISPLAY_RFROUTER) &&
     data != '\n' && buffer_free) {
    RFR_Buffer.put(data == '\r' ? ';' : data);
    if(data == '\r')
      RFR_Buffer.FHT_compress();
    RfRouter.rf_router_sendtime = 3; 
    RfRouter.rf_nr_send_checks = 2;
  }
#endif

#ifdef HAS_LCD
  if(channel & DISPLAY_LCD) {
    static uint8_t buf[TITLE_LINECHARS+1];
    static uint8_t off = 0, cmdmode = 0;
    if(data == '\r')
      return;

    if(data == '\n') {
      buf[off] = 0;
      off = 0;
      if(cmdmode) {
        callfn((char *)buf);
      } else 
        lcd_putline(0, (char*)buf);
      cmdmode = 0;
    } else {
      // 
      if(off < TITLE_LINECHARS)   // or cmd: up to 12Byte: F12346448616c6c6f
        buf[off++] = data;

      if(cmdmode && cmdmode++ == 2) {
        off -= 2;
        STRINGFUNC.fromhex((char *)buf+off, buf+off, 1);   // replace the hexnumber
        off++;
        cmdmode = 1;
      }
    }

    // Check if its a message for us: F<HC>..., and set cmdmode
    if(!cmdmode && off == 5 && buf[0] == 'F') {
      uint8_t hb[2];
      STRINGFUNC.fromhex((char*)buf+1, hb, 2);
      if(hb[0] == fht_hc0 && hb[1] == fht_hc1) {
        cmdmode = 1;
        off = 0;
      }
    }
  }
#endif

#ifdef HAS_FS
  if(log_enabled) {
    static uint8_t buf[LOG_NETTOLINELEN+1];
    static uint8_t off = 0;
    if(data == '\r')
      return;

    if(data == '\n') {
      buf[off] = 0;
      off = 0;
      Log((char*)buf);
    } else {
      if(off < LOG_NETTOLINELEN)
        buf[off++] = data;
    }
  }
#endif
}

void DisplayClass::string(char *s)
{
  while(*s)
    chr(*s++);
}

#ifdef ESP8266
void DisplayClass::string_P(const __FlashStringHelper *s) {
	Serial.print(s);
}
#endif

void DisplayClass::string_P(const char *s)
{
  //Serial.print_P(s);
  //*esp8266
  uint8_t c;
  uint8_t i=0;
  while(c = s[i]) {
    chr(c);
    i++;
  }
  // */
}

void DisplayClass::nL()
{
  chr('\r');
  chr('\n');
}

void DisplayClass::udec(uint16_t d, int8_t pad, uint8_t padc)
{
  char buf[6];
  uint8_t i=6;

  buf[--i] = 0;
  do {
    buf[--i] = d%10 + '0';
    d /= 10;
    pad--;
  } while(d && i);

  while(--pad >= 0 && i > 0)
    buf[--i] = padc;
  display.string(buf+i);
}

void DisplayClass::hex(uint16_t h, int8_t pad, uint8_t padc)
{
  char buf[5];
  uint8_t i=5;

  buf[--i] = 0;
  do {
    uint8_t m = h%16;
    buf[--i] = (m < 10 ? '0'+m : 'A'+m-10);
    h /= 16;
    pad--;
  } while(h);

  while(--pad >= 0 && i > 0)
    buf[--i] = padc;
  display.string(buf+i);
}

void DisplayClass::hex2(uint8_t h)
{
  hex(h, 2, '0');
}

void DisplayClass::func(char *in)
{
  if(in[1] == 'd') {                // no echo on USB
    echo_serial = false;
  } else if(in[1] == 'e') {         // echo on USB
    echo_serial = true;
  }
  DC('s');DH2(echo_serial);DNL();
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_DISPLAY)
DisplayClass display;
#endif
