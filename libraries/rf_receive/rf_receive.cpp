/* 
 * Copyright by R.Koenig
 * Inspired by code from Dirk Tostmann
 * License: GPL v2
 */
#ifndef ESP8266
  #include <avr/io.h>
  #include <avr/interrupt.h>
#endif
#include <stdio.h>
#include <parity.h>
#include <string.h>

#include "board.h"
#include "delay.h"
#include "rf_send.h"
#include "rf_receive.h"
#include "stringfunc.h"
#include "led.h"
#include "cc1100.h"
#include "display.h"
#include "clock.h"
#include "fncollection.h"
#include "fht.h"
#ifdef HAS_LCD
#  include "pcf8833.h"
#endif
#ifdef HAS_FASTRF
#  include "fastrf.h"
#endif
#include "rf_router.h"

#ifdef HAS_ASKSIN
#  include "rf_asksin.h"
#endif
#ifdef HAS_FLAMINGO
#  include "intertechno.h"
#endif
#ifdef HAS_MBUS
#  include "rf_mbus.h"
#endif

//////////////////////////
// With a CUL measured RF timings, in us, high/low sum
//           Bit zero        Bit one
//  KS300:  854/366 1220    366/854 1220
//  HRM:    992/448 1440    528/928 1456
//  EM:     400/320  720    432/784 1216
//  S300:   784/368 1152    304/864 1168
//  FHT:    362/368  730    565/586 1151
//  FS20:   376/357  733    592/578 1170
//  Revolt:  96/208  304    224/208  432
//  IT:     500/1000 1500   1000/500 1500


#define TSCALE(x)  (x/16)      // Scaling time to enable 8bit arithmetic
#define TDIFF      TSCALE(200) // tolerated diff to previous/avg high/low/total
#define TDIFFIT    TSCALE(350) // tolerated diff to previous/avg high/low/total

#ifdef ESP8266
#  define SILENCE    20000        // End of message
#  ifdef HAS_TCM97001
#    define SILENCE_4600L = 23000
#  endif
#  ifdef HAS_ESA
#    define SILENCE_1000 = 5000
#  endif
#else
#  define SILENCE    4000        // End of message
#  ifdef HAS_TCM97001
#    define SILENCE_4600L = 4600L
#  endif
#  ifdef HAS_ESA
#    define SILENCE_1000 = 1000
#  endif
#endif

#define STATE_RESET   0
#define STATE_INIT    1
#define STATE_SYNC    2
#define STATE_COLLECT 3
#define STATE_HMS     4
#define STATE_ESA     5
#define STATE_REVOLT  6
#define STATE_IT      7
#define STATE_TCM97001 8
#define STATE_ITV3     9
#define STATE_FAZ     10
#define STATE_FLAMINGO   11

uint8_t tx_report;              // global verbose / output-filter

void RfReceiveClass::tx_init(void)
{
  SET_BIT  ( CC1100_OUT_DDR,  CC1100_OUT_PIN);
  CLEAR_BIT( CC1100_OUT_PORT, CC1100_OUT_PIN);

  CLEAR_BIT( CC1100_IN_DDR,   CC1100_IN_PIN);
  SET_BIT( CC1100_EICR, CC1100_ISC);  // Any edge of INTx generates an int.

  RfSend.credit_10ms = MAX_CREDIT/2;

  for(int i = 1; i < RCV_BUCKETS; i ++)
    bucket_array[i].state = STATE_RESET;
  cc_on = 0;
}

void RfReceiveClass::set_txrestore()
{
#ifdef HAS_MBUS	
  if(mbus_mode != WMBUS_NONE) {
    // rf_mbus.c handles cc1101 configuration on its own.
    // if mbus is activated the configuration must not be
    // changed here, that leads to a crash!
    return;
  }
#endif
  if(tx_report) {
    CC1100.set_ccon();
    CC1100.ccRX();

  } else {
    CC1100.set_ccoff();

  }
}

void RfReceiveClass::set_txreport(char *in)
{
  if(in[1] == 0) {              // Report Value
    DH2(tx_report);
    DU(RfSend.credit_10ms, 5);
    DNL();
    return;
  }

  STRINGFUNC.fromhex(in+1, &tx_report, 1);
  set_txrestore();
}

////////////////////////////////////////////////////
// Receiver

uint8_t RfReceiveClass::cksum1(uint8_t s, uint8_t *buf, uint8_t len)    // FS20 / FHT
{
  while(len)
    s += buf[--len];
  return s;
}

uint8_t RfReceiveClass::cksum2(uint8_t *buf, uint8_t len)               // EM /FAZ3000
{
  uint8_t s = 0;
  while(len)
    s ^= buf[--len];
  return s;
}

uint8_t RfReceiveClass::cksum3(uint8_t *buf, uint8_t len)               // KS300
{
  uint8_t x = 0, y = 5, cnt = 0;
  while(len) {
    uint8_t d = buf[--len];
    x ^= (d>>4);
    y += (d>>4);
    if(!nibble || cnt) {
      x ^= (d&0xf);
      y += (d&0xf);
    }
    cnt++;
  }
  y += x;
  return (y<<4)|x;
}

uint8_t RfReceiveClass::analyze(bucket_t *b, uint8_t t)
{
  uint8_t cnt=0, max, iby = 0;
  int8_t ibi=7, obi=7;

  nibble = 0;
  oby = 0;
  max = b->byteidx*8+(7-b->bitidx);
  obuf[0] = 0;
  while(cnt++ < max) {

    uint8_t bit = (b->data[iby] & _BV(ibi)) ? 1 : 0;     // Input bit
    if(ibi-- == 0) {
      iby++;
      ibi=7;
    }

    if(t == TYPE_KS300 && obi == 3) {                   // nibble check
      if(!nibble) {
        if(!bit)
          return 0;
        nibble = !nibble;
        continue;
      }
      nibble = !nibble;
    }

    if(obi == -1) {                                    // next byte
      if(t == TYPE_FS20) {
        if(parity_even_bit(obuf[oby]) != bit)
          return 0;
      }
      if(t == TYPE_EM || t == TYPE_KS300) {
        if(!bit)
          return 0;
      }
      obuf[++oby] = 0;
      obi = 7;

    } else {                                           // Normal bits
      if(bit) {
        if(t == TYPE_FS20)
          obuf[oby] |= _BV(obi);
        if(t == TYPE_EM || t == TYPE_KS300)            // LSB
          obuf[oby] |= _BV(7-obi);
      }
      obi--;
    }
  }
  if(cnt <= max)
    return 0;
  else if(t == TYPE_EM && obi == -1)                  // missing last stopbit
    oby++;
  else if(nibble)                                     // half byte msg 
    oby++;

  if(oby == 0)
    return 0;
  return 1;
}

uint8_t RfReceiveClass::getbit(input_t *in)
{
  uint8_t bit = (in->data[in->byte] & _BV(in->bit)) ? 1 : 0;
  if(in->bit-- == 0) {
    in->byte++;
    in->bit=7;
  }
  return bit;
}

uint8_t RfReceiveClass::getbits(input_t* in, uint8_t nbits, uint8_t msb)
{
  uint8_t ret = 0, i;
  for (i = 0; i < nbits; i++) {
    if (getbit(in) )
      ret = ret | _BV( msb ? nbits-i-1 : i );
  }
  return ret;
}

uint8_t RfReceiveClass::analyze_hms(bucket_t *b)
{
  input_t in;
  in.byte = 0;
  in.bit = 7;
  in.data = b->data;

  oby = 0;
  if(b->byteidx*8 + (7-b->bitidx) < 69) 
    return 0;

  uint8_t crc = 0;
  for(oby = 0; oby < 6; oby++) {
    obuf[oby] = getbits(&in, 8, 0);
    if(parity_even_bit(obuf[oby]) != getbit( &in ))
      return 0;
    if(getbit(&in))
      return 0;
    crc = crc ^ obuf[oby];
  }

  // Read crc
  uint8_t CRC = getbits(&in, 8, 0);
  if(parity_even_bit(CRC) != getbit(&in))
    return 0;
  if(crc!=CRC)
    return 0;
  return 1;
}

#ifdef HAS_FAZ
uint8_t RfReceiveClass::analyze_faz(bucket_t *b)
{
  input_t in;
  in.byte = 0;
  in.bit = 7;
  in.data = b->data;
  
  oby = 0;
  //8*8+5=69/10=6R9, 8*8+6=70/10=6R10 ?, 9*8+7=79/10=7R9 bit
  // 3 Versionen: 8_5, 8_6, 9_7
  if( (b->byteidx*8 + (7-b->bitidx) < 69) || (b->byteidx*8 + (7-b->bitidx) > 79) ) 
    return 0;

  uint8_t crc = 0x55;
  uint8_t errorParity = 0;
  uint8_t errorStopbit = 0;
  uint8_t errorCRC = 0;
  // Restbits lesen sinnvoll? Sonst ... -2;
  for(oby = 0; oby < b->byteidx-2; oby++) {
    obuf[oby] = getbits(&in, 8, 0);
    if(parity_even_bit(obuf[oby]) != getbit( &in ))
	  errorParity++; //return 0; //{DS("pe");DU(oby,3);DU(obuf[oby],3);}//
    if(getbit(&in))
	  errorStopbit++; //return 0; //{DS("pn1");DU(oby,3);}//
    crc = crc ^ obuf[oby];
  }
  // Read crc
  uint8_t CRC = getbits(&in, 8, 0);

  if(parity_even_bit(CRC) != getbit(&in)) {
	//obuf[++oby] = CRC;
	return 0; //errorParity++; //{DS("ce");DU(CRC,3);}//
  } else {
	//obuf[++oby] = 0;
  }
  if(crc!=CRC) {
	//obuf[oby++] = CRC;
	//obuf[oby++] = crc;
	return 0; //errorCRC++; //{DS("cr");DU(crc,3);DU(CRC,3);}//
  } else {
	//obuf[oby++] = 0;
	//obuf[oby++] = 0;
  }
  //obuf[oby++] = errorParity;
  //obuf[oby++] = errorStopbit;
  //obuf[oby++] = 0xFF;
  
  // faz ok
  /* tests
  DS(" sync");DU(b->sync,3);
  DS(" pShort");DU(pulseTooShort,5);
  DS(" sMax");DU(shortMax,5);
  DS(" lMin");DU(longMin,5);
  pulseTooShort = 0;
  shortMax = 0;
  longMin = 0;
  DNL();
  */

  // encoding?
  /*
  uint8_t dec[10];
  uint8_t l;
		dec[0] = obuf[0];
		dec[1] = (~obuf[1]) ^ 0x89;
		for (l=2; l < oby-3; l++) dec[l] = (obuf[l-1] + 0xdc) ^ obuf[l];
		dec[l] = obuf[l] ^ dec[2];
      DC('X');
      for(uint8_t i=0; i < oby; i++)
        DH2(dec[i]);
      DNL();
  */
  return 1;
}
#endif
#ifdef HAS_ESA
uint8_t RfReceiveClass::analyze_esa(bucket_t *b)
{
  input_t in;
  in.byte = 0;
  in.bit = 7;
  in.data = b->data;


  if (b->state != STATE_ESA)
       return 0;

  if( (b->byteidx*8 + (7-b->bitidx)) != 144 )
       return 0;

  uint8_t salt = 0x89;
  uint16_t crc = 0xf00f;
  
  for (oby = 0; oby < 15; oby++) {
  
       uint8_t byte = getbits(&in, 8, 1);
     
       crc += byte;
    
       obuf[oby] = byte ^ salt;
       salt = byte + 0x24;
       
  }
  
  obuf[oby] = getbits(&in, 8, 1);
  crc += obuf[oby];
  obuf[oby++] ^= 0xff;

  crc -= (getbits(&in, 8, 1)<<8);
  crc -= getbits(&in, 8, 1);

  if (crc) 
       return 0;

  return 1;
}
#endif

#ifdef HAS_TX3
uint8_t RfReceiveClass::analyze_TX3(bucket_t *b)
{
  input_t in;
  in.byte = 0;
  in.bit = 7;
  in.data = b->data;
  uint8_t n, crc = 0;

  if(b->byteidx != 4 || b->bitidx != 1)
    return 0;

  for(oby = 0; oby < 4; oby++) {
    if(oby == 0) {
      n = 0x80 | getbits(&in, 7, 1);
    } else {
      n = getbits(&in, 8, 1);
    }
    crc = crc + (n>>4) + (n&0xf);
    obuf[oby] = n;
  }

  obuf[oby] = getbits(&in, 7, 1) << 1;
  crc = (crc + (obuf[oby]>>4)) & 0xF;
  oby++;

  if((crc >> 4) != 0 || (obuf[0]>>4) != 0xA)
    return 0;

  return 1;
}
#endif

#ifdef HAS_IT
uint8_t RfReceiveClass::analyze_it(bucket_t *b)
{
Serial.print("IT? ");Serial.print(b->state);

  if ((b->state != STATE_IT || b->byteidx != 3 || b->bitidx != 7) 
    && (b->state != STATE_ITV3 || b->byteidx != 8 || b->bitidx != 7)) {
			Serial.println(" failed");
      return 0;//Todo: 0
    }
  for (oby=0;oby<b->byteidx;oby++)
      obuf[oby]=b->data[oby];
  Serial.println(" ok");
  return 1;
}
#endif

#ifdef HAS_FLAMINGO
uint8_t RfReceiveClass::analyze_flamingo(bucket_t *b)
{
	uint8_t repeat[18];
	if (b->state != STATE_FLAMINGO || b->byteidx < 9) {
        return 0;
    }
	// 8 x Daten, mehrfache Übereinstimmung?
	for (oby=0;oby<b->byteidx;oby++) {
		repeat[oby] = 1;
		if (repeat[oby] > repeat[(oby % 3) + 15])
		{
			obuf[oby % 3]=b->data[oby];
			repeat[(oby % 3) + 15] = repeat[oby];
		}
		for (uint8_t j = 0; j <= 12; j=j+3) {
			if (oby >= 3+j && b->data[oby % 3 + j] == b->data[oby]) {
				repeat[oby % 3 + j]++;
				if (repeat[oby % 3 + j] > repeat[(oby % 3) + 15])
				{
					obuf[oby % 3]=b->data[oby];
					repeat[(oby % 3) + 15] = repeat[oby % 3 + j];
				}
			}
		}
	}
	if (repeat[15] > 2 && repeat[16] > 2 && repeat[17] > 2) {
        it.FlamingoDecrypt(obuf);
		oby = 5;
		return 1;
	}
	return 0;
}
#endif

#ifdef HAS_TCM97001
uint8_t RfReceiveClass::analyze_tcm97001(bucket_t *b)
{
  if (b->byteidx != 3 || b->bitidx != 7 || b->state != STATE_TCM97001) {  
		return 0;

  }
  for (oby=0;oby<b->byteidx;oby++) {
    obuf[oby]=b->data[oby];
  }
  return 1;
}
#endif

#ifdef HAS_REVOLT
uint8_t RfReceiveClass::analyze_revolt(bucket_t *b)
{
  uint8_t sum=0;
  if (b->byteidx != 12 || b->state != STATE_REVOLT || b->bitidx != 0)
    return 0;
  for (oby=0;oby<11;oby++) {
    sum+=b->data[oby];
    obuf[oby]=b->data[oby];
  }
  if (sum!=b->data[11])
      return 0;
  return 1;
}
#endif

/*
 * Check for repeted message.
 * When Package is for e.g. IT or TCM, than there must be received two packages
 * with the same message. Otherwise the package are ignored.
 */
void RfReceiveClass::checkForRepeatedPackage(uint8_t *datatype, bucket_t *b) {
	#if defined (HAS_IT) || defined (HAS_TCM97001)
		if (*datatype == TYPE_IT || (*datatype == TYPE_TCM97001)) {
				if (packetCheckValues.isrep == 1 && packetCheckValues.isnotrep == 0) { 
				  // first rep.
					packetCheckValues.isnotrep = 1;
					packetCheckValues.packageOK = 1;
				} else if (packetCheckValues.isrep == 1) {
					packetCheckValues.packageOK = 0;
				}
		} else {
	#endif
			if (!packetCheckValues.isrep) {
				packetCheckValues.packageOK = 1;
			}
  #if defined (HAS_IT) || defined (HAS_TCM97001)
    }
  #endif
}

//////////////////////////////////////////////////////////////////////
void RfReceiveClass::RfAnalyze_Task(void)
{
	// IsrTimer1 set bucket_in++
	uint8_t datatype = 0;
	bucket_t *b;
	
	b = bucket_array + bucket_out; // Todo

	if(lowtime) {
		#ifndef NO_RF_DEBUG
			//DH(tx_report,1);
			//tx_report = 0xff;
			if(tx_report & REP_LCDMON) {
				#ifdef HAS_LCD
					lcd_txmon(hightime, lowtime);
				#else
					uint8_t rssi = CC1100.readStatus(CC1100_RSSI);    //  0..256
					rssi = (rssi >= 128 ? rssi-128 : rssi+128);    // Swap
					if(rssi < 64)                                  // Drop low and high 25%
						rssi = 0;
					else if(rssi >= 192)
						rssi = 15;
					else 
						rssi = (rssi-80)>>3;
					DC('a'+rssi);
				#endif
			}
			/* loose too many packets
      if(tx_report & REP_MONITOR) {
				DC('r'); if(tx_report & REP_BINTIME) DU(hightime*16,4);
				DC('f'); if(tx_report & REP_BINTIME) DU(lowtime*16,4);
				if(silence == 1) {
					DC('.');
					DNL();
					silence = 2;
				}
			}
			*/
			if( (tx_report & REP_BITS) && overflow) {
				//DS_P(PSTR("BOVF\r\n"));            
				DS("BOVF");
				DNL();
				overflow = 0;
			}
		#endif // NO_RF_DEBUG
		lowtime = 0;
	}


	if(bucket_nrused == 0)
		return;

	#ifdef RF_DEBUG
		uint8_t state = 255;
		Serial.print("RfAnalyseTask, resets ");Serial.print(gdo2reset);Serial.println("");
		for(uint8_t i=0; i < b->bitused; i++) {
			if(state != b->bitstate[i]){
				DNL();DC('p');DC('S');DU(b->bitstate[i],2);
				state = b->bitstate[i];
			}
			DC(' H');DU(b->bithigh[i]*16,5);
			DC(' L');DU(b->bitlow[i]*16,5);
		  wdt_reset();
		}
		DNL();
		wdt_reset();
	#endif

	LED_ON();

#ifdef HAS_FLAMINGO
	if(b->state == STATE_FLAMINGO) {
		if(!datatype && analyze_flamingo(b)) { 
			datatype = TYPE_IT;
		}
	}
#endif
#ifdef HAS_IT
	if(b->state == STATE_IT || b->state == STATE_ITV3) {
		if(!datatype && analyze_it(b)) { 
		datatype = TYPE_IT;
		}
	}
#endif
#ifdef HAS_TCM97001
	if(!datatype && analyze_tcm97001(b))
		datatype = TYPE_TCM97001;
#endif
#ifdef HAS_REVOLT
	if(!datatype && analyze_revolt(b))
		datatype = TYPE_REVOLT;
#endif
#ifdef LONG_PULSE
	if(b->state != STATE_REVOLT && b->state != STATE_IT && b->state != STATE_TCM97001) {
#endif
#ifdef HAS_ESA
	if(!datatype && analyze_esa(b))
		datatype = TYPE_ESA;
#endif
	if(!datatype && analyze(b, TYPE_FS20)) { // Can be FS10 (433Mhz) or FS20 (868MHz)
		oby--;                                  // Separate the checksum byte
		uint8_t fs_csum = cksum1(6,obuf,oby);
		if(fs_csum == obuf[oby] && oby >= 4) {
			datatype = TYPE_FS20;

		} else if(fs_csum+1 == obuf[oby] && oby >= 4) {     // Repeater
			datatype = TYPE_FS20;
			obuf[oby] = fs_csum;                  // do not report if we get both

		} else if(cksum1(12, obuf, oby) == obuf[oby] && oby >= 4) {
			datatype = TYPE_FHT;
		} else {
			datatype = 0;
		}
	}

	if(!datatype && analyze(b, TYPE_EM)) {
		oby--;                                 
		if(oby == 9 && cksum2(obuf, oby) == obuf[oby])
			datatype = TYPE_EM;
	}

	if(!datatype && analyze_hms(b))
		datatype = TYPE_HMS;

#ifdef HAS_TX3
	if(!datatype && analyze_TX3(b)) // Can be 433Mhz or 868MHz
		datatype = TYPE_TX3;
#endif
#ifdef HAS_FAZ
  if(!datatype && analyze_faz(b)) // 868MHz
    datatype = TYPE_FAZ;
#endif

	if(!datatype) {
		// As there is no last rise, we have to add the last bit by hand
		addbit(b, wave_equals(&b->one, hightime, b->one.lowtime, b->state));
		if(analyze(b, TYPE_KS300)) {
			oby--;                                 
			if(cksum3(obuf, oby) == obuf[oby-nibble])
				datatype = TYPE_KS300;
		}
		if(!datatype)
			delbit(b);
	}

#ifdef HAS_HOERMANN
	// This protocol is not yet understood. It should be last in the row!
	if(!datatype && b->byteidx == 4 && b->bitidx == 4 &&
		 wave_equals(&b->zero, TSCALE(960), TSCALE(480), b->state)) {

		addbit(b, wave_equals(&b->one, hightime, TSCALE(480), b->state));
		for(oby=0; oby < 5; oby++)
			obuf[oby] = b->data[oby];
		datatype = TYPE_HRM;
	}
#endif
#ifdef LONG_PULSE
	}
#endif

	if(datatype && (tx_report & REP_KNOWN)) {

		packetCheckValues.isrep = 0;
		packetCheckValues.packageOK = 0; 
		if(!(tx_report & REP_REPEATED)) {      // Filter repeated messages
			
			// compare the data
			if(roby == oby) {
				for(roby = 0; roby < oby; roby++)
					if(robuf[roby] != obuf[roby]) {
						packetCheckValues.isnotrep = 0;
						break;
					}

				if(roby == oby && (CLOCKClass::ticks - reptime < REPTIME)) // 38/125 = 0.3 sec
					packetCheckValues.isrep = 1;
			}

			// save the data
			for(roby = 0; roby < oby; roby++)
				robuf[roby] = obuf[roby];
			reptime = CLOCKClass::ticks;

		}

		if(datatype == TYPE_FHT && !(tx_report & REP_FHTPROTO) &&
			 oby > 4 &&
			 (obuf[2] == FHT_ACK        || obuf[2] == FHT_ACK2    ||
				obuf[2] == FHT_CAN_XMIT   || obuf[2] == FHT_CAN_RCV ||
				obuf[2] == FHT_START_XMIT || obuf[2] == FHT_END_XMIT ||
				(obuf[3] & 0x70) == 0x70))
			packetCheckValues.isrep = 1;

		checkForRepeatedPackage(&datatype, b);

		#if defined(HAS_RF_ROUTER) && defined(HAS_FHT_80b)
			if(datatype == TYPE_FHT && RfRouter.rf_router_target && !FHT.fht_hc0) // Forum #50756
				packetCheckValues.packageOK = 0;
		#endif
        #ifdef HAS_FLAMINGO
	      if(b->state == STATE_FLAMINGO && datatype == TYPE_IT) {
		    packetCheckValues.packageOK = 1;
	      }
        #endif
		#ifdef HAS_IT
			if(b->state == STATE_ITV3 && datatype == TYPE_IT) {
				packetCheckValues.packageOK = !(packetCheckValues.isrep);
			}
			// ersetzt durch 9 Zeilen hoeher #ifdef HAS_FLAMINGO
			//	else if(b->state == STATE_FLAMINGO && datatype == TYPE_IT) {
			//		packetCheckValues.packageOK = !(packetCheckValues.isrep);
			//	}
			//#endif		
		#endif		
		Serial.print("packageOK ");Serial.print(b->state);Serial.print(datatype);Serial.print(packetCheckValues.isrep);Serial.print(packetCheckValues.isnotrep);Serial.println(packetCheckValues.packageOK);
		if(packetCheckValues.packageOK) {
			DC(datatype);
			if(nibble)
				oby--;
			for(uint8_t i=0; i < oby; i++)
				DH2(obuf[i]);
			if(nibble)
				DH(obuf[oby]&0xf,1);
			if(tx_report & REP_RSSI)
				DH2(CC1100.readStatus(CC1100_RSSI));
			DNL();
		}

	}

	#ifndef NO_RF_DEBUG
		if(tx_report & REP_BITS) {
			DNL();
			DC('p');
			DU(b->state,        2);
			DU(b->zero.hightime*16, 5);
			DU(b->zero.lowtime *16, 5);
			DU(b->one.hightime *16, 5);
			DU(b->one.lowtime  *16, 5);
			DU(b->sync,         3);
			DU(b->byteidx,      3);
			DU(7-b->bitidx,     2);
			DC(' ');
			if(tx_report & REP_RSSI) {
				DH2(CC1100.readStatus(CC1100_RSSI));
				DC(' ');
			}
			if(b->bitidx != 7)
				b->byteidx++;

			for(uint8_t i=0; i < b->byteidx; i++)
				 DH2(b->data[i]);
			DNL();

		}
	#endif

	b->state = STATE_RESET;
#ifdef RF_DEBUG
	b->bitused = 0;
	b->bitsaved = 0;
#endif
	bucket_nrused--;
	bucket_out++;
	if(bucket_out == RCV_BUCKETS)
		bucket_out = 0;

	LED_OFF();

	#ifdef HAS_FHT_80b
		if(datatype == TYPE_FHT) {
			FHT.fht_hook(obuf);
		}
	#endif
} //RfAnalyseTask

void ICACHE_RAM_ATTR RfReceiveClass::reset_input(void)
{
  isrtimer_disable;
  IsrReset();
  bucket_array[bucket_in].state = STATE_RESET;
  #ifdef RF_DEBUG
		bucket_array[bucket_in].bitused = 0;
		bucket_array[bucket_in].bitsaved = 0;
  #endif
	#if defined (HAS_IT) || defined (HAS_TCM97001)
		packetCheckValues.isnotrep = 0;
	#endif
}

//////////////////////////////////////////////////////////////////////
// Timer Compare Interrupt Handler. If we are called, then there was no
// data for SILENCE time, and we can put the data to be analysed
//esp8266 ISR(TIMER1_COMPA_vect)
void ICACHE_RAM_ATTR RfReceiveClass::IsrTimer1(void)
{
	isrtimer_disable;                     // Disable "us"
	#ifdef LONG_PULSE
		uint16_t tmp=OCR1A;
		OCR1A = TWRAP;                        // Wrap Timer
	  isrtimer_restart(tmp); // reinitialize timer to measure times > SILENCE
	#endif
  if(!silence) {
	  silence = 1;
  }

#ifdef RF_DEBUG
  if(bucket_array[bucket_in].state < STATE_COLLECT ||
     bucket_array[bucket_in].byteidx < 2) {    // false alarm
    reset_input();
    return;
  }
#endif
  if(bucket_nrused+1 == RCV_BUCKETS) {   // each bucket is full: reuse the last
    #ifndef NO_RF_DEBUG
			if(tx_report & REP_BITS)
				DS_P(PSTR("BOVF\r\n"));            // Bucket overflow
		#endif

		overflow = 1; // Bucket overflow
		reset_input();
  } else {
    bucket_nrused++;
    bucket_in++;
    if(bucket_in == RCV_BUCKETS)
      bucket_in = 0;
		#ifdef RF_DEBUG
			bucket_array[bucket_in].bitused = 0;
			bucket_array[bucket_in].bitsaved = 0;
		#endif
  }

} // IsrTimer1

uint8_t RfReceiveClass::wave_equals(wave_t *a, uint8_t htime, uint8_t ltime, uint8_t state)
{
  uint8_t tdiffVal = TDIFF;
#ifdef HAS_IT
  if(state == STATE_IT)
    tdiffVal = TDIFFIT;
#endif
  int16_t dlow = a->lowtime-ltime;
  int16_t dhigh = a->hightime-htime;
  int16_t dcomplete  = (a->lowtime+a->hightime) - (ltime+htime);
  if(dlow      < tdiffVal && dlow      > -tdiffVal &&
     dhigh     < tdiffVal && dhigh     > -tdiffVal &&
     dcomplete < tdiffVal && dcomplete > -tdiffVal)
    return 1;
  return 0;
}

#ifdef HAS_IT
uint8_t RfReceiveClass::wave_equals_itV3(uint8_t htime, uint8_t ltime)
{
  if(ltime - TDIFF > htime) {
    return 1;
  }
  return 0;
}
#endif

uint8_t RfReceiveClass::makeavg(uint8_t i, uint8_t j)
{
  return (i+i+i+j)/4;
}

void ICACHE_RAM_ATTR RfReceiveClass::addbit(bucket_t *b, uint8_t bit)
{
  if(b->byteidx>=sizeof(b->data)){
    reset_input();
    return;
  }
  if(bit)
    b->data[b->byteidx] |= _BV(b->bitidx);

  if(b->bitidx-- == 0) {           // next byte
    b->bitidx = 7;
    b->data[++b->byteidx] = 0;
  }
}

// Check for the validity of a 768:384us signal. Without PA ramping!
// Some CUL's generate 20% values wich are out of these bounderies.
uint8_t RfReceiveClass::check_rf_sync(uint8_t l, uint8_t s)
{
  return (l >= 0x25 &&          // 592
          l <= 0x3B &&          // 944
          s >= 0x0A &&          // 160
          s <= 0x26 &&          // 608
          l > s);
}

void RfReceiveClass::delbit(bucket_t *b)
{
  if(b->bitidx++ == 7) {           // prev byte
    b->bitidx = 0;
    b->byteidx--;
  }
}

void RfReceiveClass::IsrReset(void){
  if (!gdo2resetted){
    gdo2reset++;
    gdo2resetted = true;
  }
}


//////////////////////////////////////////////////////////////////////
// "Edge-Detected" Interrupt Handler
//esp8266 CC1100_INTVECT
void ICACHE_RAM_ATTR RfReceiveClass::IsrHandler()
{  
  silence = 0;

	#ifdef HAS___FASTRF
		if(FastRF.fastrf_on) {
			FastRF.fastrf_on = 2;
			return;
		}
	#endif

	#ifdef HAS_RF_ROUTER
		if(RfRouter.rf_router_status == RF_ROUTER_DATA_WAIT) {
			RfRouter.rf_router_status = RF_ROUTER_GOT_DATA;
			return;
		}
	#endif
	#ifdef ESP8266
	# ifdef LONG_PULSE
			uint16_t c = ((((T1L) - timer1_read())/5)>>4);               // catch the time and make it smaller
	# else
			uint8_t c = ((((T1L) - timer1_read())/5)>>4);               // catch the time and make it smaller
	# endif
	#else
	# ifdef LONG_PULSE
			uint16_t c = (TCNT1>>4);               // catch the time and make it smaller
	# else
			uint8_t c = (TCNT1>>4);               // catch the time and make it smaller
	# endif
	#endif

  if (c == 0) {
	  return;
  }
  
  bucket_t *b = bucket_array + bucket_in; // ToDo: where to fill in the bit

  if ( b->state == STATE_HMS ) {
    if(c < TSCALE(750))
      return;
    if(c > TSCALE(1250)) {
      reset_input();
      return;
    }
  }

#ifdef HAS___FAZ
  if ( b->state == STATE_FAZ ) {
    if(c < TSCALE(750))
    {
			pulseTooShort++;
			shortMax = max(shortMax, (uint32_t)c);
			return;
		}
		if(c > TSCALE(1250)) {
			pulseTooLong++;
			reset_input();
			return;
		}
		if (longMin == 0) {
			longMin = c;
		} else {
			longMin = min(longMin, (uint32_t)c);
		}
  }
#endif //HAS_FAZ

#ifdef HAS___ESA
  if (b->state == STATE_ESA) {
    if(c < TSCALE(375))
      return;
    if(c > TSCALE(625)) {
      reset_input();
      return;
    }
  }
#endif
  //////////////////
  // Falling edge
  if(!bit_is_set(CC1100_IN_PORT,CC1100_IN_PIN)) {
    if( (b->state == STATE_HMS)
			#ifdef HAS__ESA
					 || (b->state == STATE_ESA) 
			#endif
			#ifdef HAS__FAZ
     || (b->state == STATE_FAZ) 
			#endif
		) {
      addbit(b, 1);

	  isrtimer_restart(OCR1A); // restart timer
    }
    hightime = c;
    return;
  }

  lowtime = c-hightime;
  isrtimer_restart(OCR1A); // restart timer

	#ifdef RF_DEBUG
		if (b->bitused < MAXBIT) {	
		  b->bitstate[b->bitused] = b->state;
			b->bithigh[b->bitused] = hightime;
			b->bitlow[b->bitused++] = lowtime;
		}
	#endif
# ifdef HAS_FLAMINGO
	if(b->state == STATE_FLAMINGO) {
		if (b->sync && (hightime < TSCALE(1400)) && (lowtime < TSCALE(1400)) && (b->byteidx < sizeof(b->data))) {
			if (b->bit2) {
				if (b->one.hightime) {
				  b->one.hightime = makeavg(b->one.hightime, (hightime > lowtime ? hightime : lowtime));
				  b->one.lowtime  = makeavg(b->one.lowtime,  (hightime < lowtime ? hightime : lowtime));
				} else {
					b->one.hightime = (hightime > lowtime ? hightime : lowtime);
					b->one.lowtime  = (hightime < lowtime ? hightime : lowtime);
				}
			} else {
				if (b->zero.hightime) {
				  b->zero.hightime = makeavg(b->zero.hightime, (hightime > lowtime ? hightime : lowtime));
				  b->zero.lowtime  = makeavg(b->zero.lowtime,  (hightime < lowtime ? hightime : lowtime));
				} else {
					b->zero.hightime = (hightime > lowtime ? hightime : lowtime);
					b->zero.lowtime  = (hightime < lowtime ? hightime : lowtime);
				}
			}
			addbit(b, (hightime > lowtime) );
			b->bitsaved++;
		}
		// phase 1
		if (lowtime > TSCALE(2000) ) {
			// phase 2
			if ( (hightime > TSCALE(2900) ) && (lowtime > TSCALE(2900) ) )
				b->bit2 = 1;
			if ((b->bitsaved %24) > 0) {
				//ergänzen, wenn Paket zu kurz
				for (uint8_t i = (b->bitsaved %24);i<24;i++) {
					addbit(b, 0 );
				}
				b->bitsaved = 0;
        #ifdef RF_DEBUG
					b->bitstate[b->bitused] = b->state;
					b->bithigh[b->bitused] = 0;
					b->bitlow[b->bitused++] = 0;
        #endif
			}
		}
	}
# endif //HAS_FLAMINGO

#ifdef HAS_IT
  if(b->state == STATE_IT || b->state == STATE_ITV3) {
    if (lowtime > TSCALE(3000)) {
        b->sync = 0;
        return;
    }
    if (b->sync == 0) {
      if (lowtime > TSCALE(2400)) { 
        // this should be the start bit for IT V3
        b->state = STATE_ITV3;
        isrtimer_restart(OCR1A); // restart timer
        return;
      } else if (b->state == STATE_ITV3) {
        b->sync=1;
        if (lowtime-1 > hightime) {
          b->zero.hightime = hightime; 
		      b->zero.lowtime = lowtime;
        } else {
          b->zero.hightime = hightime; 
		      b->zero.lowtime = hightime*5;
        }
        b->one.hightime = hightime;
	      b->one.lowtime = hightime;
      } else {
        b->sync=1;
        if (hightime*2>lowtime) {
          // No IT, because times to near
          b->state = STATE_RESET;
		  IsrReset();
          return;
        }
        b->zero.hightime = hightime; 
        b->zero.lowtime = lowtime+1;
        b->one.hightime = lowtime+1;
        b->one.lowtime = hightime;
      }
    }
  }
#endif //HAS_IT

#ifdef HAS___TCM97001
 if (b->state == STATE_TCM97001 && b->sync == 0) {
	  b->sync=1;
		b->zero.hightime = hightime;
		b->one.hightime = hightime;

		if (lowtime < 187) { // < 3000
			b->zero.lowtime = lowtime;
			b->one.lowtime = b->zero.lowtime*2;
		} else {
			b->zero.lowtime = lowtime;
			b->one.lowtime = b->zero.lowtime/2;
		}
	}
#endif //HAS_TCM97001
  if((b->state == STATE_HMS)
#ifdef HAS___ESA
     || (b->state == STATE_ESA) 
#endif
#ifdef HAS___FTZ
     || (b->state == STATE_FAZ) 
#endif
  ) {
    addbit(b, 0);
    return;
  }
# ifdef HAS_FLAMINGO
	if(hightime < TSCALE(700)   && hightime > TSCALE(240) &&
              lowtime  < TSCALE(2600) && lowtime  > TSCALE(1900) ) {
		TIMSK1 = _BV(OCIE1A);
		b->state = STATE_FLAMINGO;
		b->byteidx = 0;
		b->bitidx  = 7;
		b->data[0] = 0;
		b->sync    = 1;
		//??OCR1A = 100000;//:20ms;  5000: 1 ms //SILENCE;
        OCR1A = 20000 * 5; //20000; //toom debug
        return;
	} 
#endif //HAS_FLAMINGO
  ///////////////////////
  // http://www.nongnu.org/avr-libc/user-manual/FAQ.html#faq_intbits
  isrtimer_clear;                 // clear Timers flags (?, important!)
return; //von HAS_TOOM !?

  if(b->state == STATE_RESET) {   // first sync bit, cannot compare yet

# ifdef HAS_FLAMINGO
	if(hightime < TSCALE(700)   && hightime > TSCALE(240) &&
              lowtime  < TSCALE(2600) && lowtime  > TSCALE(1900) ) {
		OCR1A = 100000;//:20ms;  5000: 1 ms //SILENCE;
		TIMSK1 = _BV(OCIE1A);
		b->state = STATE_FLAMINGO;
		b->byteidx = 0;
		b->bitidx  = 7;
		b->data[0] = 0;
		b->sync    = 1;
        return;
	} 
#endif
		
return; //von HAS_TOOM !?


retry_sync:
	#ifdef HAS___REVOLT
		if(hightime > TSCALE(9000) && hightime < TSCALE(12000) &&
			 lowtime  > TSCALE(150)  && lowtime  < TSCALE(540)) {
			b->zero.hightime = 6;
			b->zero.lowtime = 14;
			b->one.hightime = 19;
			b->one.lowtime = 14;
			b->sync=1;
			b->state = STATE_REVOLT;
			b->byteidx = 0;
			b->bitidx  = 7;
			b->data[0] = 0;
			isrtimer_set(SILENCE);
			return;
		}
	#endif

	#ifdef HAS___TCM97001
		if(hightime < TSCALE(530) && hightime > TSCALE(420) &&
			 lowtime  < TSCALE(9000) && lowtime > TSCALE(8500)) {
			b->sync=0;
			b->state = STATE_TCM97001;
			b->byteidx = 0;
			b->bitidx  = 7;
			b->data[0] = 0;
			isrtimer_set(SILENCE_4600L);
			return;
		}
	#endif //HAS_TCM97001


		#ifdef HAS_FLAMINGO
			if(hightime > TSCALE(240)  && hightime < TSCALE(700) &&
				lowtime  > TSCALE(1900) && lowtime  < TSCALE(2600) ) {
				b->state = STATE_FLAMINGO;
				b->byteidx = 0;
				b->bitidx  = 7;
				b->data[0] = 0;
				b->sync    = 1;
			  isrtimer_set(100000);//:20ms;  5000: 1 ms //SILENCE;
				return;
	} 
		# endif
#ifdef HAS_IT
		if(hightime > TSCALE(140)   && hightime < TSCALE(600) &&
			 lowtime  > TSCALE(2500)  && lowtime  < TSCALE(17000) ) {
			b->sync=0;
			b->state = STATE_IT;
			b->byteidx = 0;
			b->bitidx  = 7;
			b->data[0] = 0;
			isrtimer_set(SILENCE);
			return;
		} else
	#endif
		if(hightime > TSCALE(1600) || lowtime > TSCALE(1600)){
			//////// war nicht in HAS_TOOMisrtimer_set(SILENCE);
			return;
		}
  
    b->zero.hightime = hightime;
    b->zero.lowtime = lowtime;
    b->sync  = 1;
    b->state = STATE_SYNC;

  } else if(b->state == STATE_SYNC) {   // sync: lots of zeroes

    if(wave_equals(&b->zero, hightime, lowtime, b->state)) {
      b->zero.hightime = makeavg(b->zero.hightime, hightime);
      b->zero.lowtime  = makeavg(b->zero.lowtime,  lowtime);
      b->sync++;

    } else if(b->sync >= 4 ) {          // the one bit at the end of the 0-sync
      isrtimer_value(SILENCE);
			#ifdef HAS___FAZ
				if (b->sync >= 12 && (b->zero.hightime + b->zero.lowtime) > TSCALE(1600)) {
					b->state = STATE_FAZ;
			  } else 
			#endif
				if (b->sync >= 12 && (b->zero.hightime + b->zero.lowtime) > TSCALE(1600)) {
					b->state = STATE_HMS;

			#ifdef HAS___ESA
				} else if (b->sync >= 10 && (b->zero.hightime + b->zero.lowtime) < TSCALE(600)) {
					b->state = STATE_ESA;
					isrtimer_value(SILENCE_1000);
			#endif
			#ifdef HAS___RF_ROUTER
				} else if(RfRouter.rf_router_myid &&
									check_rf_sync(hightime, lowtime) &&
									check_rf_sync(b->zero.lowtime, b->zero.hightime)) {
					RfRouter.rf_router_status = RF_ROUTER_SYNC_RCVD;
					reset_input();
					return;
			#endif

		} else {
			b->state = STATE_COLLECT;
        }

			b->one.hightime = hightime;
			b->one.lowtime  = lowtime;
			b->byteidx = 0;
			b->bitidx  = 7;
			b->data[0] = 0;

            isrtimer_enable;                // hier oder weiter unten!? On timeout analyze the data

		} else {                            // too few sync bits
			b->state = STATE_RESET;
			//IsrReset();
			goto retry_sync;
		}

  } else 
  #ifdef HAS_REVOLT
	if(b->state==STATE_REVOLT) { //STATE_REVOLT
		if ((hightime < 11)) {
			addbit(b,0);
			b->zero.hightime = makeavg(b->zero.hightime, hightime);
			b->zero.lowtime  = makeavg(b->zero.lowtime,  lowtime);
		} else {
			addbit(b,1);
			b->one.hightime = makeavg(b->one.hightime, hightime);
			b->one.lowtime  = makeavg(b->one.lowtime,  lowtime);
		}
		} else 
  #endif

	#ifdef HAS_TCM97001
		if(b->state==STATE_TCM97001) {
			if (lowtime > 110 && lowtime < 140) {
				addbit(b,0);
				b->zero.hightime = makeavg(b->zero.hightime, hightime);
				b->zero.lowtime  = makeavg(b->zero.lowtime,  lowtime);
			} else if (lowtime > 230 && lowtime < 270) {
				addbit(b,1);
				b->one.hightime = makeavg(b->one.hightime, hightime);
				b->one.lowtime  = makeavg(b->one.lowtime,  lowtime);
			}

		} else
	#endif
  {
		#ifdef HAS_IT
			if(b->state==STATE_ITV3) {
				uint8_t value = wave_equals_itV3(hightime, lowtime);
				addbit(b, value);
			} else
		#endif 

    // STATE_COLLECT , STATE_IT
    if(wave_equals(&b->one, hightime, lowtime, b->state)) {
      addbit(b, 1);
      b->one.hightime = makeavg(b->one.hightime, hightime);
      b->one.lowtime  = makeavg(b->one.lowtime,  lowtime);
    } else if(wave_equals(&b->zero, hightime, lowtime, b->state)) {
      addbit(b, 0);
      b->zero.hightime = makeavg(b->zero.hightime, hightime);
      b->zero.lowtime  = makeavg(b->zero.lowtime,  lowtime);
    } else {
      if (b->state!=STATE_IT) 
        reset_input();
    }
  }
  isrtimer_enable;                  // hier oder weiter oben!? On timeout analyze the data
}

uint8_t RfReceiveClass::rf_isreceiving()
{
  uint8_t r = (bucket_array[bucket_in].state != STATE_RESET);
#ifdef HAS_FHT_80b
  r = (r || FHT.fht80b_timeout != FHT_TIMER_DISABLED);
#endif
  return r;
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RF_RECEIVE)
RfReceiveClass RfReceive;
#endif
