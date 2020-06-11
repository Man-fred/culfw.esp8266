#include "board.h"

#include <string.h>
#include <avr/pgmspace.h>
#include "fastrf.h"
#include "cc1100.h"
#include "delay.h"
#include "display.h"
#include "rf_receive.h"
#include "fncollection.h"
#include "clock.h"

void
FastRFClass::func(char *in)
{
  uint8_t len = strlen(in);

  if(in[1] == 'r') {                // Init
    CC1100.ccInitChip(EE_FASTRF_CFG);
    CC1100.ccRX();
    fastrf_on = FASTRF_MODE_ON;

  } else if(in[1] == 's') {         // Send

    CC1100_ASSERT;
    CC1100.cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);
    CC1100.cc1100_sendbyte( len-2 );
    for(uint8_t i=2; i< len; i++)
      CC1100.cc1100_sendbyte(in[i]);
    CC1100_DEASSERT;
    CC1100.ccTX();
    while(CC1100.cc1100_readReg(CC1100_TXBYTES) & 0x7f) // Wait for the data to be sent
      MYDELAY.my_delay_ms(1);
    CC1100.ccRX();                         // set reception again. MCSM1 does not work.

  } else {
    fastrf_on = FASTRF_MODE_OFF;

  }
}

void
FastRFClass::Task(void)
{
  if(!fastrf_on)
    return;

  if(fastrf_on == FASTRF_MODE_ON) {
    static uint8_t lasttick;         // Querying all the time affects reception.
    if(lasttick != (uint8_t)CLOCK.ticks) {
      if(CC1100.cc1100_readReg(CC1100_MARCSTATE) == MARCSTATE_RXFIFO_OVERFLOW) {
        CC1100.ccStrobe(CC1100_SFRX);
        CC1100.ccRX();
      }
      lasttick = (uint8_t)CLOCK.ticks;
    }
    return;
  }

  uint8_t len = CC1100.cc1100_readReg(CC1100_RXFIFO);
  uint8_t buf[TTY_BUFSIZE];

  if(len < sizeof(buf)) {
    CC1100_ASSERT;
    CC1100.cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
    for(uint8_t i=0; i<len; i++)
      buf[i] = CC1100.cc1100_sendbyte( 0 );
    CC1100_DEASSERT;
    //debug to USB
		uint8_t odc = display.channel;
		display.channel = DISPLAY_USB;
    uint8_t i;
    for(i=0; i<len; i++)
      DC(buf[i]);
    DNL();
    display.channel = odc;
  }

  fastrf_on = FASTRF_MODE_ON;
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_FASTRF)
FastRFClass FastRF;
#endif
