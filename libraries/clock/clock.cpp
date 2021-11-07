#include "clock.h"

#include <stdint.h>
#ifndef ESP8266
  #include <avr/io.h>
  #include <avr/wdt.h>
  #include <avr/interrupt.h>
#endif

#include "led.h"
#ifdef XLED
#  include "xled.h"
#endif
#include "fncollection.h"
#include "display.h"
#if defined(HAS_LCD) && defined(BAT_PIN)
#  include "battery.h"
#endif
#ifdef JOY_PIN1
#  include "joy.h"
#endif
#ifdef HAS_FHT_TF
#  include "fht.h"
#endif
//#include "fswrapper.h"                 // fs_sync();
#include "rf_send.h"                   // credit_10ms
#ifdef HAS_SLEEP
#  include "mysleep.h"
#endif
#ifdef HAS_LCD
  #include "pcf8833.h"
#endif
#ifdef HAS_CDC
  #include "cdc.h"
#endif
#include "rf_router.h"                  // rf_router_flush();
#ifdef HAS_NTP
#  include "ntp.h"
#endif
#ifdef HAS_ONEWIRE
#  include "onewire.h"
#endif
#ifdef HAS_VZ
#  include "vz.h"
#endif
#if defined (HAS_IRRX) || defined (HAS_IRTX)
#  include "ir.h"
#endif

#ifndef ESP8266
CLOCKClass *CLOCKClass::ClassInterrupt::owner = 0;

void CLOCKClass::ClassInterrupt::record(CLOCKClass *t) {
	owner = t;
}

void CLOCKClass::ClassInterrupt::serviceRoutine() {
	if(owner != 0)
		CLOCKClass::IsrHandler();
}
CLOCKClass::CLOCKClass() {
	ClassInterrupt::record(this);
        TCCR2A |= (1 << CS20) | (1 << CS21) | (1 << CS22);
        TIMSK0 |= 1 << TOIE2;
	sei();
}
#endif

// count & compute in the interrupt, else long runnning tasks would block
// a "minute" task too long
//ISR(TIMER0_COMPA_vect, ISR_BLOCK)
#ifndef ESP8266
void CLOCKClass::IsrHandler()
#else
void ICACHE_RAM_ATTR CLOCKClass::IsrHandler()
#endif
{
	#ifndef ESP8266
		#ifdef HAS_IRTX     //IS IRTX defined ?
			if(! IR.send_data() ) {   //If IR-Sending is in progress, don't receive
		#ifdef HAS_IRRX  //IF also IRRX is define
				IR.sample(); // call IR sample handler
		#endif
			}
		#elif defined (HAS_IRRX)
			IR.sample(); // call IR sample handler
		#endif
		#if defined (HAS_IRTX) || defined (HAS_IRRX)
			// if IRRX is compiled in, timer runs 125x faster ... 
			if (++ir_ticks<125) 
				return;
				
			ir_ticks = 0;
		#endif
	#endif //ESP8266

  // 125Hz
  ticks++; 
  clock_hsec++;

#ifdef HAS_NTP
  ntp_hsec++;
  if(ntp_hsec >= 125) {
    ntp_sec++;
    ntp_hsec = 0;
  }
#endif

#ifdef HAS_FHT_TF
  // iterate over all TFs
  for(uint8_t i = 0; i < FHT_TF_NUM; i++) {
    if(FHT.fht_tf_timeout_Array[3 * i] != -1 && FHT.fht_tf_timeout_Array[3 * i] > 0) {
      // decrement timeout
      FHT.fht_tf_timeout_Array[3 * i]--;
    }
  }
#endif
#ifdef HAS_FHT_8v
  if(FHT.fht8v_timeout)
	{
    FHT.fht8v_timeout--;
	}
#endif
#ifdef HAS_FHT_80b
  if(FHT.fht80b_timeout != FHT_TIMER_DISABLED)
    FHT.fht80b_timeout--;
#endif
}

void CLOCKClass::get_timestamp(uint32_t *ts)
{
  //uint8_t l = SREG;
  //cli(); 
  *ts = ticks;
  //SREG = l;
}

void CLOCKClass::gettime(char *unused)
{
  uint32_t actticks;
  get_timestamp(&actticks);
  uint8_t *p = (uint8_t *)&actticks;
  DH2(p[3]);
  DH2(p[2]);
  DH2(p[1]);
  DH2(p[0]);
  DNL();
}

#ifdef HAS_ETHERNET
//Return time
clock_time_t CLOCKClass::clock_time()
{
  uint32_t actticks;
  get_timestamp(&actticks);
  return (clock_time_t)actticks;
}
#endif

void CLOCKClass::Minute_Task(void)
{
  if((uint8_t)ticks == last_tick)
    return;
  last_tick = (uint8_t)ticks;
//esp8266  wdt_reset();

  // 125Hz
#ifdef XLED
  if ((ticks % 12) == 0) {
    if ( xled_pattern & _BV(xled_pos++) ) {
      LED_ON();
    } else {
      LED_OFF();
    }
  }
  xled_pos &= 15;
#endif
#ifdef HAS_FHT_TF
  // iterate over all TFs
  for(uint8_t i = 0; i < FHT_TF_NUM; i++) {
    // if timed out -> call fht_tf_timer to send out data
    if(FHT.fht_tf_timeout_Array[3 * i] == 0) {
      FHT.fht_tf_timer(i);
    }
  }
#endif
#ifdef HAS_FHT_8v
  if(FHT.fht8v_timeout == 0)
	{
    FHT.fht8v_timer();
	}
#endif
#ifdef HAS_FHT_80b
  if(FHT.fht80b_timeout == 0)
    FHT.fht80b_timer();
#endif
#ifdef HAS_RF_ROUTER
  if(RfRouter.rf_router_sendtime && --RfRouter.rf_router_sendtime == 0)
    RfRouter.flush();
#endif

#ifdef HAS_ONEWIRE
  // Check if a running conversion is done
  // if HMS Emulation is on, and the Minute timer has expired
  Onewire.HsecTask ();
#endif

  if(clock_hsec<125)
    return;
  clock_hsec = 0;       // once per second from here on.

#ifndef XLED
  if(led_mode & 2)
    LED_TOGGLE();
#endif

  if (RfSend.credit_10ms < MAX_CREDIT) // 10ms/1s == 1% -> allowed talk-time without CD
    RfSend.credit_10ms += 1;
  // ToDo: only testing, not for productive rollout
  //RfSend.credit_10ms = MAX_CREDIT;
#ifdef HAS_ONEWIRE
  // if HMS Emulation is on, check the HMS timer
  Onewire.SecTask ();
#endif
#ifdef HAS_VZ
  vz_sectask();
#endif

#if defined(HAS_SLEEP) && defined(JOY_PIN1)
  if(joy_inactive < 255)
    joy_inactive++;

  if(sleep_time && joy_inactive == sleep_time) {
    if(USB_IsConnected)
      lcd_switch(0);
    else
      dosleep();
  }
#endif

#ifdef HAS_NTP
  if((ntp_sec & NTP_INTERVAL_MASK) == 0)
    ntp_sendpacket();
#endif

  static uint8_t clock_sec;
  clock_sec++;
if(clock_sec != 60)       // once per minute from here on
    return;
  clock_sec = 0;

#ifdef HAS_FHT_80b
  FHT.fht80b_minute++;
  if(FHT.fht80b_minute >= 60)
    FHT.fht80b_minute = 0;
#endif

#if defined(HAS_LCD) && defined(BAT_PIN)
  bat_drawstate();
#endif
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_CLOCK)
CLOCKClass CLOCK;
#endif
