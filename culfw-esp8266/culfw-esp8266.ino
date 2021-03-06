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
unsigned char DDRB; // rf_asksin.cpp:65 SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN );   // CS as output
unsigned char DDRD; //
unsigned char ISC20;
unsigned char EICRA;
// unsigned char EIMSK; //cc1100
unsigned char TIMSK1;
unsigned char TIFR1;
uint32_t      TCNT1;     // Zähler für Timer1, im Gegensatz zu Arduino bei esp8266 von OCR1A -> 0
unsigned int  OCR0A;     // Divisor für Timer 0, genutzt in clock und ir
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
#ifdef HAS_FASTRF
#  include "fastrf.h"
#endif
#include "rf_router.h"    // rf_router_func

#ifdef HAS_ETHERNET
#  include "ethernet.h"
#endif
#ifdef HAS_ONEWIRE
#  include "onewire.h"
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
#if defined (HAS_IRRX) || defined (HAS_IRTX)
#  include "ir.h"
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
  SPI.setClockDivider(SPI_CLOCK_DIV2); // neu moritz
}

void Serial_Task() {
  if (Serial.available() > 0) {
    uint8_t data = Serial.read();
    TTYdata.rxBuffer.put(data);
    Serial.write(data);
  }
  //???output_flush_func = CDC_Task;
  //input_handle_func(DISPLAY_USB);
  TTYdata.analyze_ttydata(DISPLAY_USB);
}

//////////////////////////////////////////////////////////////////////
// count & compute in the interrupt, else long runnning tasks would block
// a "minute" task too long
//ISR(TIMER0_COMPA_vect, ISR_BLOCK)
inline void ICACHE_RAM_ATTR IsrTimer0 (void){
  //timer0count++;
  //Timer0Cycles = Timer0Cycles + OCR0A;
  //timer0_write(Timer0Cycles);
  CLOCK.IsrHandler();
}

//////////////////////////////////////////////////////////////////////
// "Edge-Detected" Interrupt Handler
//ISR(CC1100_INTVECT)
inline void ICACHE_RAM_ATTR IsrHandler (void){
  gdo2count++;
  RfReceive.IsrHandler();
}

//////////////////////////////////////////////////////////////////////
// Timer Compare Interrupt Handler. If we are called, then there was no
// data for SILENCE time, and we can put the data to be analysed
//ISR(TIMER1_COMPA_vect)
inline void ICACHE_RAM_ATTR IsrTimer1(void)
{
  timer1count++;
  RfReceive.IsrTimer1();
}

void loop20s(unsigned long counter) {
  /* timer/interrupt  debugging
  Serial.print(micros());    // Chip
  Serial.print(" loop ");
  Serial.print(TimerMicros); // last Loop()
  Serial.print(" - ");
  Serial.print(Timer125Hz);  // from last Loop()
  Serial.print(" - ");
  Serial.print(Timer1Hz);    // from last Loop()
  Serial.print(" ticks ");
  Serial.print(CLOCK.ticks); // from interrupt, sould be better than Timer125Hz
  Serial.print(" T1 ");
  Serial.print(timer1_read());//break before silence 20000 = 4ms
  //Serial.print(" T0c ");
  //Serial.print(timer0count);
  Serial.print(" T1c ");
  Serial.print(timer1count); // count silence
  Serial.print(" gdo2INT ");
  Serial.print(gdo2count);   // count gdo2 interrupts
  Serial.print(" GDO0t ");
  Serial.print(GDO0Toggle);
  Serial.print(" GDO2t ");
  Serial.println(GDO2Toggle);
  GDO0Toggle = 0;
  GDO2Toggle = 0;
  timer1count = 0;
  gdo2count = 0;
  */
}

void loop1Hz(unsigned long counter) {
  //LED.toggle();
}

void loop125Hz(unsigned long counter) {
}

// https://en.cppreference.com/w/cpp/language/lambda#Lambda_capture
const t_fntab fntab[] = {
#ifdef HAS_ASKSIN
  { 'A', [&](char *data) { RfAsksin.func(data); } },
#endif
  // 'a' CUR battery
  { 'B', [&](char *data) { FNcol.prepare_boot(data); } },
  #ifdef HAS_MBUS
    { 'b', rf_mbus_func },
  #endif
  { 'C', [&](char *data) { CC1100.ccreg(data); } },
  #ifdef HAS_NTP
    { 'c', ntp_func },
  #endif
  // 'D' TuxRadio
  // 'd' CUR LCD
  #ifdef HAS_RWE
    // double? (CUN only) eth debugging
    { 'E', rwe_func },
  #endif
  { 'e', [&](char *data) { FNcol.eeprom_factory_reset(data); } },
  { 'F', [&](char *data) { RfSend.fs20send(data); } },
  #ifdef HAS_FASTRF
    { 'f', [&](char *data) { FastRF.func(data); } },
  #endif
  #ifdef HAS_RAWSEND
    { 'G', [&](char *data) { RfSend.rawsend(data); } },
  #endif
  // 'H' HM485
  #ifdef HAS_HOERMANN_SEND
    { 'h', hm_send },
  #endif
  #if defined (HAS_IRRX) || defined (HAS_IRTX)
    { 'I', [&](char *data) { IR.func(data); } },
  #endif
  #ifdef HAS_INTERTECHNO
    { 'i', it_func },
  #endif
  #ifdef HAS_RAWSEND
    { 'K', [&](char *data) { RfSend.ks_send(data); } },
  #endif
  #ifdef HAS_KOPP_FC
    { 'k', kopp_fc_func },
  #endif
  #ifdef HAS_BELFOX
    { 'L', send_belfox },
  #endif
  { 'l', [&](char *data) { FNcol.ledfunc(data); } },
  #ifdef HAS_RAWSEND
    { 'M', [&](char *data) { RfSend.em_send(data); } },
  #endif
  #ifdef HAS_MEMFN
    { 'm', [&](char *data) { Memory.getfreemem(data); } },
  #endif
  #ifdef HAS_RFNATIVE
    { 'N', [&](char *data) { RfNative.native_func(data); } },
  #endif
  #ifdef HAS_ONEWIRE  
    { 'O', [&](char *data) { Onewire.func(data); } },
  #endif 
  // 'o' CUNO2 OBIS Command-Set
  // 'P' CUR picture
  #ifdef HAS_ETHERNET
    { 'q', [&](char *data) { Ethernet.close(data); } },
  #endif
  { 'R', [&](char *data) { FNcol.read_eeprom(data); } },
  // esp8266/CUN-special
  { 's', [&](char *data) { display.func(data); } },
  { 'T', [&](char *data) { FHT.fhtsend(data); } },
  { 't', [&](char *data) { CLOCK.gettime(data); } },
  #ifdef HAS_UNIROLL
    { 'U', ur_send },
  #endif
  #ifdef HAS_RF_ROUTER
    { 'u', [&](char *data) { RfRouter.func(data); } },
  #endif
  { 'V', [&](char *data) { FNcol.version(data); } },
  #ifdef HAS_EVOHOME
    { 'v', rf_evohome_func },
  #endif
  { 'W', [&](char *data) { FNcol.write_eeprom(data); } },
  // 'w' (CUR/CUN) write a file
  { 'X', [&](char *data) { RfReceive.set_txreport(data); } },
  { 'x', [&](char *data) { CC1100.ccsetpa(data); } },
  #ifdef HAS_SOMFY_RTS
    { 'Y', somfy_rts_func },
  #endif
  #ifdef HAS_FTZ
    // obsolet
    { 'Z', [&](char *data) { RfSend.ftz_send(data); } },
  #endif
  #ifdef HAS_MORITZ
     { 'Z', [&](char *data) { Moritz.func(data); } },
  #endif
  #ifdef HAS_ZWAVE
    { 'z', zwave_func },
  #endif
  //doppelt, eigene Kuerzel!
  #ifdef HAS_ETHERNET
    { '1', [&](char *data) { Ethernet.func(data); } }, //'E'
  #endif
  { 0, 0 }
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("eeprom_init");
  FNcol.eeprom_init();
  Serial.println("eeprom_init ok");

#ifndef ESP8266
  wdt_enable(WDTO_2S);
  clock_prescale_set(clock_div_1);

  MARK433_PORT |= _BV( MARK433_BIT ); // Pull 433MHz marker
  MARK915_PORT |= _BV( MARK915_BIT ); // Pull 915MHz marker

  // if we had been restarted by watchdog check the REQ BootLoader byte in the
  // EEPROM ...
  if(bit_is_set(MCUSR,WDRF) && erb(EE_REQBL)) {
    ewb( EE_REQBL, 0 ); // clear flag
    start_bootloader();
  }

  // Setup the timers. Are needed for watchdog-reset

  #if defined (HAS_IRRX) || defined (HAS_IRTX)
    IR.init();
    // IR uses highspeed TIMER0 for sampling 
    OCR0A  = 1;                              // Timer0: 0.008s = 8MHz/256/2   == 15625Hz Fac: 125
  #else
    OCR0A  = 249;                            // Timer0: 0.008s = 8MHz/256/250 == 125Hz
  # endif
  TCCR0B = _BV(CS02);
  TCCR0A = _BV(WGM01);
  TIMSK0 = _BV(OCIE0A);
  TCCR1A = 0;
  TCCR1B = _BV(CS11) | _BV(WGM12);         // Timer1: 1us = 8MHz/8 -> 0 bis 4.000 
  MCUSR &= ~(1 << WDRF);                   // Enable the watchdog
#else
  pinMode(CC1100_IN_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(CC1100_IN_PIN), IsrHandler, CHANGE);
  pinMode(CC1100_OUT_PIN, OUTPUT);
  #if defined (HAS_IRRX) || defined (HAS_IRTX)
    IR.init();
    // IR uses highspeed TIMER0 for sampling 
    OCR0A  = 640000; // Timer zu schnell? wird auch nicht benötigt, da eigene library! 5120;   // Timer0: 0.008s = 80MHz/256/2   == 15625Hz Fac: 125
  #else
    OCR0A  = 640000; // soll 80000000 Hz <=> 1sec /125 = 125 Hz
  # endif
  noInterrupts();
  OCR1A = 20000; // SILENCE 4 ms= 4 * 5.000
  timer1_isr_init();
  timer1_attachInterrupt(IsrTimer1);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE); // TIM_DIV16 : 80 MHz / 16 -> 5.000.000 Ticks je s
  timer1_write(OCR1A); 
  interrupts();
#endif

#ifdef HAS_RF_ROUTER
  display.channel = (DISPLAY_USB|DISPLAY_RFROUTER);
#else
  display.channel = DISPLAY_USB;
#endif
  led_init();
  spi_init();
//  USB_Init();
  FHT.fht_init();
  RfReceive.tx_init();
  //???????????????????????????????????ttydata.input_handle_func = *ttydata.analyze_ttydata;
#ifdef HAS_RF_ROUTER
  RfRouter.init();
#endif
#ifdef HAS_ETHERNET
  display.channel |= DISPLAY_TCP;
  display.channel &= ~DISPLAY_USB; //USB only when 
  Ethernet.init();
  Serial.printf("\nChannel %d \n", display.channel);
#endif
  Serial.print("CC1100_PARTNUM 0x00: "); Serial.println(CC1100.readStatus(0x30), HEX);
  Serial.print("CC1100_VERSION 0x14: "); Serial.println(CC1100.readStatus(0x31), HEX);

  LED_OFF();
}

void loop() {
  // put your main code here, to run repeatedly:
#ifdef HAS_ETHERNET
  if (!Ethernet.in_ota()){
#endif
  TimerMicros = micros();
  unsigned long temp = TimerMicros/8000;
  if (temp != Timer125Hz) {
    Timer125Hz = temp;
    CLOCK.IsrHandler();
    /*/loop125Hz(Timer125Hz);
    temp = Timer125Hz/125;
    if (temp != Timer1Hz) {
      Timer1Hz = temp;
      //loop1Hz(Timer1Hz);
      temp = Timer1Hz/20;
      if (temp != Timer20s) {
        Timer20s = temp;
        loop20s(Timer20s);
      }
    } // 1sec loop */
  }
  CheckGDO();
  
  Serial_Task();
  #ifndef ESP8266
    USB_USBTask();
    CDC_Task();
  #endif
  RfReceive.RfAnalyze_Task();
  CLOCK.Minute_Task();
  #ifdef HAS_FASTRF
    FastRF.Task();
  #endif
  #ifdef HAS_RF_ROUTER
    RfRouter.task();
  #endif
  #ifdef HAS_ASKSIN
    RfAsksin.task();
  #endif
  #ifdef HAS_IRRX
    IR.task();
  #endif
  #ifdef HAS_ETHERNET
    Ethernet.Task();
  #endif
  #ifdef HAS_MORITZ
    Moritz.task();
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
  } // !Ethernet.in_ota()
#endif
}
