/* 
 * Copyright by R.Koenig
 * Inspired by code from Dirk Tostmann
 * License: GPL v2
 */

//#include <avr/io.h>
//#include <avr/interrupt.h>
#include <stdio.h>
#include <parity.h>
#include <string.h>

#include "board.h"
#include "rf_send.h"
#include "cc1100.h"
#include "stringfunc.h"
#include "delay.h"
#include "rf_receive.h"
#include "led.h"
#include "display.h"
#include "fncollection.h"
//#include "fht.h"

#ifdef HAS_DMX
#  include "dmx.h"
#endif

#ifdef HAS_HELIOS
#  include "helios.h"
#endif

#ifdef HAS_MORITZ
#  include "rf_moritz.h"
#endif

// For FS20 we time the complete message, for KS300 the rise-fall distance
// FS20  NULL: 400us high, 400us low
// FS20  ONE:  600us high, 600us low
// KS300 NULL  854us high, 366us low
// KS300 ONE:  366us high, 854us low

#define FS20_ZERO      400     //   400uS
#define FS20_ONE       600     //   600uS
#define FS20_PAUSE      10     // 10000mS
#define EM_ONE         800     //   800uS
#define EM_ZERO        400     //   400uS


#ifdef HAS_RAWSEND

#define MAX_SNDMSG 12
#define MAX_SNDRAW 12

static uint8_t zerohigh, zerolow, onehigh, onelow;
#define TMUL(x) (x<<4)
#define TDIV(x) (x>>4)

void RfSendClass::send_bit(uint8_t bit, uint8_t edge = 0)
{
#ifdef ESP8266
  if (edge) {
	  MYDELAY.my_delay_us(onelow);
	  digitalWrite(CC1100_OUT_PIN,!bit);               
	  MYDELAY.my_delay_us(onelow);
	  digitalWrite(CC1100_OUT_PIN,bit);               
  } else {
	  digitalWrite(CC1100_OUT_PIN,1);                // High
	  MYDELAY.my_delay_us(bit ? TMUL(onehigh) : TMUL(zerohigh));

	  digitalWrite(CC1100_OUT_PIN,0);                // Low
	  MYDELAY.my_delay_us(bit ? TMUL(onelow) : TMUL(zerolow));
  }
#else
  CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);         // High
  MYDELAY.my_delay_us(bit ? TMUL(onehigh) : TMUL(zerohigh));

  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);       // Low
  MYDELAY.my_delay_us(bit ? TMUL(onelow) : TMUL(zerolow));
#endif
}

#else

#define MAX_SNDMSG 6    // FS20: 4 or 5 + CRC, FHT: 5+CRC
#define MAX_SNDRAW 7    // MAX_SNDMSG*9/8 (parity bit)

void RfSendClass::send_bit(uint8_t bit, uint8_t edge = 0)
{
#ifdef ESP8266
  ???? digitalWrite(CC1100_OUT_PIN,1);                // High
  MYDELAY.my_delay_us(bit ? FS20_ONE : FS20_ZERO);

  digitalWrite(CC1100_OUT_PIN,0);                // Low
  MYDELAY.my_delay_us(bit ? FS20_ONE : FS20_ZERO);
#else
  CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN);         // High
  MYDELAY.my_delay_us(bit ? FS20_ONE : FS20_ZERO);

  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN);       // Low
  MYDELAY.my_delay_us(bit ? FS20_ONE : FS20_ZERO);
#endif
}

#endif

// msg is with parity/checksum already added
void RfSendClass::sendraw(uint8_t *msg, uint8_t sync, uint8_t nbyte, uint8_t bitoff,
                uint8_t repeat, uint8_t pause, uint8_t edge = 0)
{
  // 12*800+1200+nbyte*(8*1000)+(bits*1000)+800+10000 
  // message len is < (nbyte+2)*repeat in 10ms units.
  int8_t i, j, sum = (nbyte+2)*repeat;
  int8_t prebit, bit;
  if (credit_10ms < sum) {
    DS_P(PSTR("LOVF\r\n"));
    return;
  }
  credit_10ms -= sum;

  LED_ON();

#if defined (HAS_IRRX) || defined (HAS_IRTX) // Block IR_Reception
  cli();
#endif

#ifdef HAS_MORITZ
  uint8_t restore_moritz = 0;
  if(moritz_on) {
    restore_moritz = 1;
    moritz_on = 0;
    set_txreport("21");
  }
#endif
  if(tx_report & REP_BITS) {
    DC('s');
    DU(edge,        2);
    DU(TMUL(zerohigh), 5);
    DU(TMUL(zerolow), 5);
    DU(TMUL(onehigh), 5);
    DU(TMUL(onelow), 5);
    DU(sync,         3);
    DU(nbyte,      3);
    DU(7-bitoff,     2);
    DC(' ');

    for(uint8_t i=0; i < nbyte; i++)
       DH2(msg[i]);
    if(bitoff != 7)
       DH2(msg[nbyte]);
    DNL();
  }

  if(!cc_on)
    CC1100.set_ccon();
  CC1100.ccTX();                                       // Enable TX 
  do {
    for(i = 0; i < sync; i++)                   // sync
      send_bit(0);
    if(sync)
      send_bit(1);

    for(j = 0; j < nbyte; j++) {                // whole bytes
      for(i = 7; i >= 0; i--) {
		bit = msg[j] & _BV(i);
        send_bit(msg[j] & _BV(i), edge);
		prebit = bit;
	  }
    }
    for(i = 7; i > bitoff; i--)  {               // broken bytes
      bit = msg[nbyte] & _BV(i);
      send_bit(msg[nbyte] & _BV(i), edge);
	  prebit = bit;
	}

    MYDELAY.my_delay_ms(pause);                         // pause

  } while(--repeat > 0);

  if(tx_report) {                               // Enable RX
    CC1100.ccRX();
  } else {
    CC1100.ccStrobe(CC1100_SIDLE);
  }

#if defined (HAS_IRRX) || defined (HAS_IRTX) // Activate IR_Reception
  sei(); 
#endif

#ifdef HAS_MORITZ
  if(restore_moritz)
    rf_moritz_init();
#endif
  LED_OFF();
}

int RfSendClass::abit(uint8_t b, uint8_t *obuf, uint8_t *obyp, uint8_t obi)
{
  uint8_t oby = * obyp;
  //debug b ? DU(1,1) : DU(0,1);
  if(b)
    obuf[oby] |= _BV(obi);
  if(obi-- == 0) {
    oby++;
    if(oby < MAX_SNDRAW)
      *obyp = oby;
    obi = 7; obuf[oby] = 0;
  }
  return obi;
}

void RfSendClass::addParityAndSendData(uint8_t *hb, uint8_t hblen,
                uint8_t startcs, uint8_t repeat)
{
  uint8_t iby, obuf[MAX_SNDRAW], oby;
  int8_t ibi, obi;

  hb[hblen] = RfReceive.cksum1(startcs, hb, hblen);
  hblen++;

  // Copy the message and add parity-bits
  iby=oby=0;
  ibi=obi=7;
  obuf[oby] = 0;

  while(iby<hblen) {
    obi = abit(hb[iby] & _BV(ibi), obuf, &oby, obi);
	if(ibi-- == 0) {
      obi = abit(parity_even_bit(hb[iby]), obuf, &oby, obi);
      ibi = 7; iby++;
    }
  }
  if(obi-- == 0) {                   // Trailing 0 bit: no need for a check
    oby++; obi = 7;
  }
#ifdef HAS_RAWSEND
  zerohigh = zerolow = TDIV(FS20_ZERO);
  onehigh = onelow = TDIV(FS20_ONE);
#endif
  sendraw(obuf, 12, oby, obi, repeat, FS20_PAUSE);
}

void RfSendClass::addParityAndSend(char *in, uint8_t startcs, uint8_t repeat)
{
  uint8_t hb[MAX_SNDMSG], hblen;
  hblen = STRINGFUNC.fromhex(in+1, hb, MAX_SNDMSG-1);
  addParityAndSendData(hb, hblen, startcs, repeat);
}

void RfSendClass::fs20send(char *in)
{
#ifdef HAS_DMX
  if (dmx_fs20_emu( in ))
	return;
#endif
#ifdef HAS_HELIOS
  if (helios_fs20_emu( in ))
	return;
#endif
  addParityAndSend(in, 6, 3);
}

#ifdef HAS_FAZ
void RfSendClass::faz_send(char *in)
{
  uint8_t iby, obuf[MAX_SNDRAW], oby;
  int8_t  ibi, obi;
  uint8_t hb[MAX_SNDMSG];

  uint8_t hblen = STRINGFUNC.fromhex(in+1, hb, MAX_SNDMSG-1);

//  KS may have different length - TODO: byte padding for some sensor types
//  if (hblen != 9) {
//  DS_P(PSTR("KS send\r\n"));
//    return;
//  }

  zerohigh = TDIV(880); 
  zerolow  = TDIV(1040);
  onehigh  = TDIV(880); 
  onelow   = TDIV(540);

  // calc checksum
  hb[hblen] = RfReceive.cksum2( hb, hblen );
  hb[hblen] ^= 85;
  hblen++;

  // Copy the message and add parity-bits
  iby=oby=0;
  ibi=obi=7;
  obuf[oby] = 0;

  while(iby<hblen) {
    obi = abit(hb[iby] & _BV(7-ibi), obuf, &oby, obi);
    if(ibi-- == 0) {
      obi = abit(parity_even_bit(hb[iby]), obuf, &oby, obi); // CRC
      obi = abit(0, obuf, &oby, obi); // CRC
      ibi = 7; iby++;
    }
  }
  //if(obi-- == 0) {                   // Trailing 0 bit: indicating EOM
  //  oby++; obi = 7;
  //}

  sendraw(obuf, 19, oby, obi, 1, 0, 1);
}
#endif

#ifdef HAS_RAWSEND
//G0843E540202040A78DE81D80
//G0843E540202040A78DE80F80

void RfSendClass::rawsend(char *in)
{
  uint8_t hb[16]; // 33/2: see ttydata.c
  uint8_t nby, nbi, pause, repeat, sync;

  STRINGFUNC.fromhex(in+1, hb, sizeof(hb));
  sync = hb[0];
  nby = (hb[1] >> 4);
  nbi = (hb[1] & 0xf);
  pause = (hb[2] >> 4);
  repeat = (hb[2] & 0xf);
  zerohigh = hb[3];
  zerolow  = hb[4];
  onehigh  = hb[5];
  onelow   = hb[6];
  sendraw(hb+7, sync, nby, nbi, repeat, pause);
}


// E0205E7000000000000
void RfSendClass::em_send(char *in)
{
  uint8_t iby, obuf[MAX_SNDRAW], oby;
  int8_t  ibi, obi;
  uint8_t hb[MAX_SNDMSG];

  uint8_t hblen = STRINGFUNC.fromhex(in+1, hb, MAX_SNDMSG-1);

  // EM is always 9 bytes payload!
  if (hblen != 9) {
//  DS_P(PSTR("LENERR\r\n"));
    return;
  }
  
  onehigh = zerohigh = zerolow = TDIV(EM_ZERO);
  onelow  = TDIV(EM_ONE);

  // calc checksum
  hb[hblen] = RfReceive.cksum2( hb, hblen );
  hblen++;

  // Copy the message and add parity-bits
  iby=oby=0;
  ibi=obi=7;
  obuf[oby] = 0;
  
  while(iby<hblen) {
    obi = abit(hb[iby] & _BV(7-ibi), obuf, &oby, obi);
    if(ibi-- == 0) {
      obi = abit(1, obuf, &oby, obi); // always 1
      ibi = 7; iby++;
    }
  }
  if(obi-- == 0) {                   // Trailing 0 bit: indicating EOM
    oby++; obi = 7;
  }

  sendraw(obuf, 12, oby, obi, 3, FS20_PAUSE);
}

void RfSendClass::ks_send(char *in)
{
  uint8_t iby, obuf[MAX_SNDRAW], oby;
  int8_t  ibi, obi;
  uint8_t hb[MAX_SNDMSG];

  uint8_t hblen = STRINGFUNC.fromhex(in+1, hb, MAX_SNDMSG-1);

//  KS may have different length - TODO: byte padding for some sensor types
//  if (hblen != 9) {
//  DS_P(PSTR("KS send\r\n"));
//    return;
//  }

  zerohigh = TDIV(855); 
  zerolow  = TDIV(366);
  onehigh  = TDIV(366); 
  onelow   = TDIV(855);

  // calc checksum
  hb[hblen] = RfReceive.cksum3( hb, hblen );
  hblen++;

  // Copy the message and add parity-bits
  iby=oby=0;
  ibi=obi=7;
  obuf[oby] = 0;

  while(iby<hblen) {

    for (ibi=0; ibi<4; ibi++)
      obi = abit(hb[iby] & _BV(ibi), obuf, &oby, obi);

    obi = abit(1, obuf, &oby, obi); // always 1

    for (ibi=4; ibi<8; ibi++)
      obi = abit(hb[iby] & _BV(ibi), obuf, &oby, obi);

    obi = abit(1, obuf, &oby, obi); // always 1

    iby++;
  }

  sendraw(obuf, 10, oby, obi, 3, FS20_PAUSE);
}

#endif

#ifdef HAS_UNIROLL
//G0031E368232368hhhhdc1
//16 bit group-address 4 bit channel-address 4 bit command 1 1-bit

void RfSendClass::ur_send(char *in)
{
  uint8_t hb[4];

  STRINGFUNC.fromhex(in+1, hb, 3);
  zerohigh = TDIV(1700);
  zerolow  = TDIV(590);
  onehigh  = TDIV(540);
  onelow   = TDIV(1700);
  hb[3] = 0x80;     //10000000
  sendraw(hb, 0, 3, 6, 3, 15);
}
#endif

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RF_SEND)
RfSendClass RfSend;
#endif

