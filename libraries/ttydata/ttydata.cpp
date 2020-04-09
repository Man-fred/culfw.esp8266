#include "ttydata.h"
#include "display.h"

//esp8266 void (*input_handle_func)(uint8_t channel);
//extern DisplayClass display;

TTYdataClass::TTYdataClass() {}

uint8_t TTYdataClass::callfn(char *buf){
  for(uint8_t idx = 0; ; idx++) {
//esp8266 
    uint8_t n = fntab[idx].name;
	//display.hex2(n);
//esp8266 
    if(!n)
      return 0;
    if(buf == 0) {
      display.chr(' ');
      display.chr(n);
    } else if(buf[0] == n) {
      void (*fn)(char *) = (void (*)(char *))fntab[idx].fn;
	//	void (*fn)(char *) = fntab[idx].fn;
      fn(buf);
      return 1;
    } else {
	  //display.hex2(buf[0]);
	}
  }
  return 0;
}

void TTYdataClass::analyze_ttydata(uint8_t channel)
{
  static char cmdbuf[TTY_BUFSIZE+1];
  static int cmdlen;
  uint8_t ucCommand;
  uint8_t odc;
  
  odc = display.channel;
  display.channel = channel;
    
  while(rxBuffer.getNbytes()) {

    ucCommand = rxBuffer.get();

#ifdef RPI_TTY_FIX
    // eat RPi rubbish
    if (ucCommand == 0xff)
      continue;
#endif

    if(ucCommand == '\n' || ucCommand == '\r') {

      if(!cmdlen)       // empty return
        continue;

      cmdbuf[cmdlen] = 0;
      if(!callfn(cmdbuf)) {
        //display.string_P(PSTR("? ("));
        DS("? (");
        display.string(cmdbuf);
        //display.string_P(PSTR(" is unknown) Use one of"));
        DS(" is unknown) Use one of");
        callfn(0);
        display.nL();
      }
      cmdlen = 0;

    } else {
       if(cmdlen < sizeof(cmdbuf)-1)
         cmdbuf[cmdlen++] = ucCommand;
    }
  }
  display.channel = odc;
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_TTYDATA)
TTYdataClass TTYdata;
#endif
