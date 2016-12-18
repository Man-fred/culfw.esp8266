#include "board.h"
#ifdef HAS_RF_ROUTER
#include <string.h>
#include <pgmspace.h>
#include "rf_router.h"
#include "cc1100.h"
#include "delay.h"
#include "display.h"
#include "rf_receive.h"
#include "fncollection.h"
#include "clock.h"
#include "ringbuffer.h"
#include "ttydata.h"
#include "fht.h"
#include "stringfunc.h"


//void RfRouterClass::debug_out(uint8_t);

#undef RFR_USBECHO


void RfRouterClass::usbMsg(char *s)
{
  display.channel = DISPLAY_USB;
  display.string(s);
  DNL();
  display.channel = 0xff;
}

void RfRouterClass::init()
{
  rf_router_myid = FNcol.erb(EE_RF_ROUTER_ID);
  rf_router_target = FNcol.erb(EE_RF_ROUTER_ROUTER);
  if(rf_router_target) {
    tx_report = 0x21;
    RfReceive.set_txrestore();
  }
}

void RfRouterClass::func(char *in)
{
  if(in[1] == 0) {               // u: display id and router
    DH2(rf_router_myid);
    DH2(rf_router_target);
    DNL();
#ifdef RFR_DEBUG 
  } else if(in[1] == 'd') {     // ud: Debug
    DH((uint16_t)CLOCK.ticks, 4); DC('.');
    DH2(rf_router_sendtime);
    DNL();
  } else if(in[1] == 's') {     // us: Statistics
    DH(nr_t,1); DC('.');
    DH(nr_f,1); DC('.');
    DH(nr_e,1); DC('.');
    DH(nr_k,1); DC('.');
    DH(nr_h,1); DC('.');
    DH(nr_r,1); DC('.');
    DH(nr_plus,1);
    DNL();
#endif

  } else if(in[1] == 'i') {      // uiXXYY: set own id to XX and router id to YY
    STRINGFUNC.fromhex(in+2, &rf_router_myid, 1);
    FNcol.ewb(EE_RF_ROUTER_ID, rf_router_myid);
    STRINGFUNC.fromhex(in+4, &rf_router_target, 1);
    FNcol.ewb(EE_RF_ROUTER_ROUTER, rf_router_target);

  } else {                      // uYYDATA: send data to node with id YY
    RFR_Buffer.reset();
    while(*++in)
      RFR_Buffer.put(*in);
    send(0);

  }
}

#define RF_ROUTER_ZERO_HIGH 384
#define RF_ROUTER_ZERO_LOW  768
#define RF_ROUTER_ONE_HIGH  768
#define RF_ROUTER_ONE_LOW   384
//esp8266 #define SET_HIGH CC1100_OUT_PORT |= _BV(CC1100_OUT_PIN)
//#define SET_LOW  CC1100_OUT_PORT &= ~_BV(CC1100_OUT_PIN)
#define SET_HIGH digitalWrite(CC1100_OUT_PIN,1)
#define SET_LOW  digitalWrite(CC1100_OUT_PIN,0)
void RfRouterClass::sethigh(uint16_t dur) {
	SET_HIGH; 
	MYDELAY.my_delay_us(dur); 
}
void RfRouterClass::setlow(uint16_t dur) {
	SET_LOW; 
	MYDELAY.my_delay_us(dur); 
}

// Duration is 15ms, more than one tick!
void RfRouterClass::ping(void)
{
  CC1100.set_ccon();           // 1.7ms
  CC1100.ccTX();               // 4.8ms

  // Sync               // 8.5ms
  for(uint8_t i = 0; i < 6; i++) {
    sethigh(RF_ROUTER_ZERO_HIGH);
    setlow(RF_ROUTER_ZERO_LOW);
  }
  sethigh(RF_ROUTER_ONE_HIGH);
  setlow(RF_ROUTER_ONE_LOW);
  sethigh(RF_ROUTER_ONE_LOW);
  SET_LOW;
}

void RfRouterClass::send(uint8_t addAddr)
{
#ifdef RFR_DEBUG
       if(RFR_Buffer.buf[5] == 'T') nr_t++;
  else if(RFR_Buffer.buf[5] == 'F') nr_f++;
  else if(RFR_Buffer.buf[5] == 'E') nr_e++;
  else if(RFR_Buffer.buf[5] == 'K') nr_k++;
  else if(RFR_Buffer.buf[5] == 'H') nr_h++;
  else                              nr_r++;
#endif

  uint8_t buf[7], l = 1;
  buf[0] = RF_ROUTER_PROTO_ID;
  if(addAddr) {
    STRINGFUNC.tohex(rf_router_target, buf+1);
    STRINGFUNC.tohex(rf_router_myid,   buf+3),
    buf[5] = 'U';
    l = 6;
  }
  ping();           // 15ms
  CC1100.ccInitChip(EE_FASTRF_CFG);  // 1.6ms
  MYDELAY.my_delay_ms(3);             // 3ms: Found by trial and error

  CC1100_ASSERT;
  CC1100.cc1100_sendbyte(CC1100_WRITE_BURST | CC1100_TXFIFO);
#ifdef RFR_USBECHO
  uint8_t nbuf = RFR_Buffer.nbytes;
#endif
  CC1100.cc1100_sendbyte(RFR_Buffer.nbytes+l);
  for(uint8_t i = 0; i < l; i++)
    CC1100.cc1100_sendbyte(buf[i]);
  while(RFR_Buffer.nbytes)
    CC1100.cc1100_sendbyte(RFR_Buffer.get());
  CC1100_DEASSERT;
  CC1100.ccTX();
  RFR_Buffer.reset(); // needed by FHT_compress

  // Wait for the data to be sent
  uint8_t maxwait = 20;        // max 20ms
  while((CC1100.cc1100_readReg(CC1100_TXBYTES) & 0x7f) && maxwait--)
    MYDELAY.my_delay_ms(1);
  RfReceive.set_txrestore();
#ifdef RFR_USBECHO
#warning RFR USB DEBUGGING IS ACTIVE
  uint8_t odc = display.channel;
  display.channel = DISPLAY_USB;
  DC('.'); DU(nbuf, 2); DNL();
  display.channel = odc;
#endif
}

void RfRouterClass::task(void)
{
  if(rf_router_status == RF_ROUTER_INACTIVE)
    return;

  uint8_t hsec = (uint8_t)CLOCK.ticks;

  if(rf_router_status == RF_ROUTER_GOT_DATA) {

    uint8_t len = CC1100.cc1100_readReg(CC1100_RXFIFO);
    uint8_t proto = 0;

    if(len > 5) {
      TTYdata.rxBuffer.reset();
      CC1100_ASSERT;
      CC1100.cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );
      proto = CC1100.cc1100_sendbyte(0);
      while(--len)
        TTYdata.rxBuffer.put(CC1100.cc1100_sendbyte(0));
      CC1100_DEASSERT;
    }
    RfReceive.set_txrestore();
    rf_router_status = RF_ROUTER_INACTIVE;

    if(proto == RF_ROUTER_PROTO_ID) {
      uint8_t id;
      if(STRINGFUNC.fromhex(TTYdata.rxBuffer.buf, &id, 1) == 1 &&     // it is for us
         id == rf_router_myid) {

        if(TTYdata.rxBuffer.buf[4] == 'U') {               // "Display" the data
          while(TTYdata.rxBuffer.nbytes)                   // downlink: RFR->CUL
            DC(TTYdata.rxBuffer.get());
          DNL();

        } else {                                        // uplink: CUL->RFR
          TTYdata.rxBuffer.nbytes -= 4;                    // Skip dest/src bytes
          TTYdata.rxBuffer.getoff = 4;
          TTYdata.rxBuffer.put('\n');
//esp8266          input_handle_func(DISPLAY_RFROUTER);          // execute the command
          TTYdata.analyze_ttydata(DISPLAY_RFROUTER);          // execute the command
        }

      } else {
        TTYdata.rxBuffer.reset();
      }
    }

  } else if(rf_router_status == RF_ROUTER_DATA_WAIT) {
    uint8_t diff = hsec - rf_router_hsec;
    if(diff > 7) {              // 3 (delay above) + 3 ( ((4+64)*8)/250kBaud )
      RfReceive.set_txrestore();
      RfRouter.rf_router_status = RF_ROUTER_INACTIVE;
    }

  } else if(rf_router_status == RF_ROUTER_SYNC_RCVD) {
    CC1100.ccInitChip(EE_FASTRF_CFG);
    CC1100.ccRX();
    RfRouter.rf_router_status = RF_ROUTER_DATA_WAIT;
    RfRouter.rf_router_hsec = hsec;

  } 
}

void RfRouterClass::flush()
{
  if(RfReceive.rf_isreceiving()) {
    rf_nr_send_checks = 3;
#ifdef RFR_DEBUG
    nr_plus++;
#endif
  }

  if(--rf_nr_send_checks)
    rf_router_sendtime = 3;
  else
    send(1);   // duration is more than one tick
}


#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RF_ROUTER)
RfRouterClass RfRouter;
#endif

#endif
