/* Copyright Rudolf Koenig, 2008.
   Released under the GPL Licence, Version 2
   Inpired by the MyUSB USBtoSerial demo, Copyright (C) Dean Camera, 2008.
*/
// #include <avr/boot.h>
//#include <power.h>
// #include <avr/eeprom.h>
// #include <avr/interrupt.h>
// #include <avr/io.h>
//#include <pgmspace.h>
//#include <avr/wdt.h>
#include <stdint.h>

unsigned char PORTB;
unsigned char PORTD;
unsigned char PINB;
unsigned char DDRD; //
unsigned char ISC20;
unsigned char EICRA;
// unsigned char EIMSK; //cc1100
unsigned char TIMSK1;
unsigned char TIFR1;
uint32_t      TCNT1;     // Zähler für Timer1, im Gegensatz zu Arduino bei esp8266 von OCR1A -> 0
unsigned int  OCR1A;     // Dauer für Timer1: Faktor 5 notwendig, da Timer1 nicht 1MHz sondern 5MHz
unsigned char OCF1A;
unsigned char OCIE1A;
#include "board.h"

byte GDO0state = 0;
unsigned long GDO0Toggle = 0;
byte GDO2state = 0;
unsigned long GDO2Toggle = 0;
unsigned long TimerMicros;
unsigned long Timer125Hz;
unsigned long Timer1Hz;
unsigned long Timer20s = 0;
unsigned long Timer0Cycles = 0;
unsigned long timer0count=0;
unsigned long timer1count=0;
unsigned long gdo2count=0;
byte CheckGDO(void)
{
  if(GDO0state) {    //receive data
    GDO0state = digitalRead(CC1100_OUT_PIN); 
    // fallende Flanke: Daten vorhanden
    if (GDO0state == 0)
      GDO0Toggle++; 
    //return (CheckReceiveFlag0Toggle == 0);
  } else {             // no data
    GDO0state = digitalRead(CC1100_OUT_PIN); 
    if (GDO0state)
      GDO0Toggle++; 
    //return 0;
  }
  if(GDO2state)     //receive data
  {
    GDO2state = digitalRead(CC1100_IN_PIN); 
    // fallende Flanke: Daten vorhanden
    if (GDO2state == 0)
      GDO2Toggle++; 
    //return (CheckReceiveFlag2Toggle == 0);
  }
  else              // no data
  {
    GDO2state = digitalRead(CC1100_IN_PIN); 
    if (GDO2state == 1)
      GDO2Toggle++; 
  }
  return 0;
}

// #include <string.h>

//#include <USB.h>     // USB Functionality

#include <SPI.h>

//----#include "ELECHOUSE_CC1101.h"
#include "ttydata.h"
//#include "ringbuffer.h"
#include "display.h"
#include "cc1100.h"

#include "fncollection.h"
//#include "cdc.h"
#include "clock.h"
//#include "delay.h"
#include "led.h"    // ledfunc
#include "rf_receive.h" 
#include "rf_send.h"    // fs20send u.a
#include "fht.h"    // fhtsend
//#include "fastrf.h"   // fastrf_func
#include "rf_router.h"    // rf_router_func

#ifdef HAS_ETHERNET
#  include "ethernet.h"
#endif
#ifdef HAS_MEMFN
#  include "memory.h"   // getfreemem
#endif
#ifdef HAS_ASKSIN
#  include "rf_asksin.h"
#endif
#ifdef HAS_MORITZ
#  include "rf_moritz.h"
#endif
#ifdef HAS_RWE
#  include "rf_rwe.h"
#endif
#ifdef HAS_RFNATIVE
#  include "rf_native.h"
#endif
#ifdef HAS_INTERTECHNO
#  include "intertechno.h"
#endif
#ifdef HAS_SOMFY_RTS
#  include "somfy_rts.h"
#endif
#ifdef HAS_MBUS
#  include "rf_mbus.h"
#endif
#ifdef HAS_KOPP_FC
#  include "kopp-fc.h"
#endif
#ifdef HAS_BELFOX
#  include "belfox.h"
#endif
#ifdef HAS_ZWAVE
#  include "rf_zwave.h"
#endif
#ifdef HAS_EVOHOME
#  include "rf_evohome.h"
#endif

void start_bootloader(void)
{
  cli();

  /* move interrupt vectors to bootloader section and jump to bootloader */
//AVR?  MCUCR = _BV(IVCE);
//AVR?  MCUCR = _BV(IVSEL);

#if defined(CUL_V3) || defined(CUL_V4)
#  define jump_to_bootloader ((void(*)(void))0x3800)
#endif
#if defined(CUL_V2)
#  define jump_to_bootloader ((void(*)(void))0x1800)
#endif
//AVR?  jump_to_bootloader();
}

void spi_init() {
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
}

void Serial_Task() {
  if (Serial.available() > 0) {
    uint8_t data = Serial.read();
    TTYdata.rxBuffer.put(data);
  }
  //???output_flush_func = CDC_Task;
  //input_handle_func(DISPLAY_USB);
  TTYdata.analyze_ttydata(1);//DISPLAY_USB
}

//////////////////////////////////////////////////////////////////////
// count & compute in the interrupt, else long runnning tasks would block
// a "minute" task too long
//ISR(TIMER0_COMPA_vect, ISR_BLOCK)
void inline IsrTimer0 (void){
  timer0count++;
  Timer0Cycles = Timer0Cycles + 640000;
  timer0_write(Timer0Cycles);
  CLOCK.IsrHandler();
}

//////////////////////////////////////////////////////////////////////
// "Edge-Detected" Interrupt Handler
//ISR(CC1100_INTVECT)
void inline IsrHandler (void){
  gdo2count++;
  RfReceive.IsrHandler();
}

//////////////////////////////////////////////////////////////////////
// Timer Compare Interrupt Handler. If we are called, then there was no
// data for SILENCE time, and we can put the data to be analysed
//ISR(TIMER1_COMPA_vect)
void IsrTimer1(void)
{
  timer1count++;
  RfReceive.IsrTimer1();
}

void loop20s(unsigned long counter) {
  Serial.print(micros());
  Serial.print(" loop ");
  Serial.print(TimerMicros);
  Serial.print(" - ");
  Serial.print(Timer125Hz);
  Serial.print(" - ");
  Serial.print(Timer1Hz);
  Serial.print(" ticks ");
  Serial.print(CLOCK.ticks);
  Serial.print(" T1 ");
  Serial.print(timer1_read());
  //Serial.print(" T0c ");
  //Serial.print(timer0count);
  Serial.print(" T1c ");
  Serial.print(timer1count);
  Serial.print(" gdo2INT ");
  Serial.print(gdo2count);
  Serial.print(" GDO0t ");
  Serial.print(GDO0Toggle);
  Serial.print(" GDO2t ");
  Serial.println(GDO2Toggle);
  GDO0Toggle = 0;
  GDO2Toggle = 0;
}

void loop1Hz(unsigned long counter) {
  //LED.toggle();
}

void loop125Hz(unsigned long counter) {
}

void ccreg(char *data)                {CC1100.ccreg(data); };
void ccsetpa(char *data)              { CC1100.ccsetpa(data); };
void eeprom_factory_reset(char *data) { FNcol.eeprom_factory_reset(data); };
void em_send(char *data)              { RfSend.em_send(data); };
void eth_func(char *data)             { Ethernet.func(data); }
void fhtsend(char *data)              { FHT.fhtsend(data); };
void fs20send(char *data)             { RfSend.fs20send(data); };
void ftz_send(char *data)             { RfSend.ftz_send(data); };
void gettime(char *data)              { CLOCK.gettime(data); };
void ks_send(char *data)              { RfSend.ks_send(data); };
void ledfunc(char *data)              { FNcol.ledfunc(data); };
void native_func(char *data)          { RfNative.native_func(data); };
void prepare_boot(char *data)         { FNcol.prepare_boot(data); }
void rawsend(char *data)              { RfSend.rawsend(data); };
void read_eeprom(char *data)          { FNcol.read_eeprom(data); };
void rf_mbus_func(char *data)         { Serial.println("rf_mbus_func"); };
void rf_router_func(char *data)       { RfRouter.func(data); }
void set_txreport(char *data)         { RfReceive.set_txreport(data); };
void tcplink_close(char *data)        { Ethernet.close(data); }
void version(char *data)              { FNcol.version(data); };
void write_eeprom(char *data)         { FNcol.write_eeprom(data); };

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  int i = 0;
#ifdef HAS_ASKSIN
  TTYdata.fntab[i++] = { 'A', asksin_func };
#endif
  TTYdata.fntab[i++] = { 'B', prepare_boot };
  #ifdef HAS_MBUS
    TTYdata.fntab[i++] = { 'b', rf_mbus_func };
  #endif
  TTYdata.fntab[i++] = { 'C', ccreg };
  #ifdef HAS_RWE
    { 'E', rwe_func },
  #endif
  TTYdata.fntab[i++] = { 'e', eeprom_factory_reset };
  TTYdata.fntab[i++] = { 'F', fs20send };
  #ifdef HAS_FASTRF
    { 'f', fastrf_func },
  #endif
  #ifdef HAS_RAWSEND
    TTYdata.fntab[i++] = { 'G', rawsend };
  #endif
  #ifdef HAS_HOERMANN_SEND
    { 'h', hm_send },
  #endif
  #ifdef HAS_INTERTECHNO
    { 'i', it_func },
  #endif
  #ifdef HAS_RAWSEND
    TTYdata.fntab[i++] = { 'K', ks_send };
  #endif
  #ifdef HAS_KOPP_FC
    { 'k', kopp_fc_func },
  #endif
  #ifdef HAS_BELFOX
    { 'L', send_belfox },
  #endif
  TTYdata.fntab[i++] = { 'l', ledfunc };
  #ifdef HAS_RAWSEND
    TTYdata.fntab[i++] = { 'M', em_send };
  #endif
  #ifdef HAS_MEMFN
    { 'm', getfreemem },
  #endif
  #ifdef HAS_RFNATIVE
    TTYdata.fntab[i++] = { 'N', native_func };
  #endif
  #ifdef HAS_ETHERNET
    TTYdata.fntab[i++] = { 'q', tcplink_close };
  #endif
  TTYdata.fntab[i++] = { 'R', read_eeprom };
  TTYdata.fntab[i++] = { 'T', fhtsend };
  TTYdata.fntab[i++] = { 't', gettime };
  #ifdef HAS_UNIROLL
    { 'U', ur_send },
  #endif
  #ifdef HAS_RF_ROUTER
    TTYdata.fntab[i++] = { 'u', rf_router_func };
  #endif
  TTYdata.fntab[i++] = { 'V', version };
  #ifdef HAS_EVOHOME
    { 'v', rf_evohome_func },
  #endif
  TTYdata.fntab[i++] = { 'W', write_eeprom };
  TTYdata.fntab[i++] = { 'X', set_txreport };
  TTYdata.fntab[i++] = { 'x', ccsetpa };
  #ifdef HAS_SOMFY_RTS
    { 'Y', somfy_rts_func },
  #endif
  TTYdata.fntab[i++] = { 'Z', ftz_send };
  #ifdef HAS_MORITZ
    { 'Z', moritz_func },
  #endif
  #ifdef HAS_ZWAVE
    { 'z', zwave_func },
  #endif
  //doppelt, eigene Kuerzel!
  #ifdef HAS_ETHERNET
    TTYdata.fntab[i++] = { '1', eth_func }; //'E'
  #endif
  #ifdef HAS_NTP
    TTYdata.fntab[i++] = { '2', ntp_func }; // 'c'
  #endif
  TTYdata.fntab[i++] = { 0, 0 };

//  wdt_enable(WDTO_2S);
//esp8266   clock_prescale_set(clock_div_1);

//esp8266   MARK433_PORT |= _BV( MARK433_BIT ); // Pull 433MHz marker
//esp8266   MARK915_PORT |= _BV( MARK915_BIT ); // Pull 915MHz marker

  // if we had been restarted by watchdog check the REQ BootLoader byte in the
  // EEPROM ...
/*esp8266 
  if(bit_is_set(MCUSR,WDRF) && erb(EE_REQBL)) {
    ewb( EE_REQBL, 0 ); // clear flag
    start_bootloader();
  }
*/

  // Setup the timers. Are needed for watchdog-reset
#ifndef ESP8266
  OCR0A  = 249;                            // Timer0: 0.008s = 8MHz/256/250
  TCCR0B = _BV(CS02);
  TCCR0A = _BV(WGM01);
  TIMSK0 = _BV(OCIE0A);
  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12);         // Timer1: 1us = 8MHz/8 -> 0 bis 4.000 
#else
  pinMode(CC1100_IN_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(CC1100_IN_PIN), IsrHandler, CHANGE);
  pinMode(CC1100_OUT_PIN, OUTPUT);

  Timer0Cycles = ESP.getCycleCount() + 640000;
  noInterrupts();
  timer0_isr_init();
  timer0_attachInterrupt(IsrTimer0);
  timer0_write(Timer0Cycles); // soll 80000000 (1sec) /125 -> 125 Hz
  OCR1A = 20000; // SILENCE 4 ms= 4 * 5.000
  timer1_isr_init();
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP); // TIM_DIV16 : 80 MHz / 16 -> 5.000.000 Ticks je s
  timer1_write(OCR1A); 
  timer1_attachInterrupt(IsrTimer1);
  interrupts();
#endif

//esp8266   MCUSR &= ~(1 << WDRF);                   // Enable the watchdog

#ifdef HAS_RF_ROUTER
  display.channel = (DISPLAY_USB|DISPLAY_RFROUTER);
  //display.channel = DISPLAY_USB;
#else
  display.channel = DISPLAY_USB;
#endif
  led_init();
  spi_init();
  FNcol.eeprom_init();
  Serial.println("eeprom_init");
//  USB_Init();
  FHT.fht_init();
  RfReceive.tx_init();
  //???????????????????????????????????ttydata.input_handle_func = *ttydata.analyze_ttydata;
#ifdef HAS_RF_ROUTER
  RfRouter.init();
#endif
#ifdef HAS_ETHERNET
  Ethernet.init();
#endif
  Serial.print("CC1100_PARTNUM 0x00: "); Serial.println(CC1100.readStatus(0x30), HEX);
  Serial.print("CC1100_VERSION 0x14: "); Serial.println(CC1100.readStatus(0x31), HEX);

  LED_OFF();
}

void loop() {
  // put your main code here, to run repeatedly:
  /*
  TimerMicros = micros();
  if (TimerMicros/8000 != Timer125Hz) {
    Timer125Hz = TimerMicros/8000;
    loop125Hz(Timer125Hz);
    if (Timer125Hz/125 != Timer1Hz) {
      Timer1Hz = Timer125Hz/125;
      loop1Hz(Timer1Hz);
      if (Timer1Hz/20 != Timer20s) {
        Timer20s = Timer1Hz/20;
        loop20s(Timer20s);
      }
    }
  }
  CheckGDO();
  */
  Serial_Task();
//    USB_USBTask();
//    CDC_Task();
    RfReceive.RfAnalyze_Task();
    CLOCK.Minute_Task();
#ifdef HAS_FASTRF
    FastRF_Task();
#endif
#ifdef HAS_RF_ROUTER
    RfRouter.task();
#endif
#ifdef HAS_ASKSIN
    rf_asksin_task();
#endif
#ifdef HAS_MORITZ
    rf_moritz_task();
#endif
#ifdef HAS_RWE
    rf_rwe_task();
#endif
#ifdef HAS_RFNATIVE
    RfNative.native_task();
#endif
#ifdef HAS_KOPP_FC
    kopp_fc_task();
#endif
#ifdef HAS_MBUS
    rf_mbus_task();
#endif
#ifdef HAS_ZWAVE
    rf_zwave_task();
#endif
#ifdef HAS_EVOHOME
    rf_evohome_task();
#endif
#ifdef HAS_ETHERNET
    Ethernet.Task();
#endif

}
