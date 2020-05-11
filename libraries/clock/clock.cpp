#include "clock.h"

// count & compute in the interrupt, else long runnning tasks would block
// a "minute" task too long
//esp8266 ISR(TIMER0_COMPA_vect, ISR_BLOCK)
void CLOCKClass::IsrHandler()
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
    FHT.fht8v_timeout--;
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
    FHT.fht8v_timer();
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
  onewire_HsecTask ();
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

#ifdef HAS_ONEWIRE
  // if HMS Emulation is on, check the HMS timer
  onewire_SecTask ();
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
