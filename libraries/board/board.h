#ifndef _BOARD_H
#define _BOARD_H

// Ports for esp8266 see
// esp8266_peri.h - Peripheral registers exposed in more AVR style for esp8266
#ifndef ESP8266
//#  define ESP8266
#endif

#ifdef ESP8266
#  include <Esp.h>
#endif

#define VERSION_1               1          // original CUN
#define VERSION_2               67         // original CUN
#define VERSION                 "1.67"
#define VERSION_OTA             "V01-67-01" // for OTA
#define CUL_V3

// Feature definitions
#define BOARD_ID_STR            "CUL868"
#define BOARD_ID_USTR           L"CUL868"

//#define MULTI_FREQ_DEVICE	// available in multiple versions: 433MHz,868MHz
#define BOARD_ID_STR433         "CUL433"
#define BOARD_ID_USTR433        L"CUL433"

#define HAS_USB                  1
#define USB_BUFSIZE             64      // Must be a supported USB endpoint size
#define USB_MAX_POWER	       100
#define HAS_FHT_80b                     // PROGMEM: 1374b, RAM: 90b
#define HAS_RF_ROUTER                   // PROGMEM: 1248b  RAM: 44b
//1.67 #define RFR_FILTER                      // PROGMEM:   90b  RAM:  4b
//     #define HAS_HOERMANN
//     #define HAS_HOERMANN_SEND               // PROGMEM:  220
#define HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT	// PROGMEM: 118b
#define HAS_CC1101_PLL_LOCK_CHECK_MSG		// PROGMEM:  22b
#define HAS_CC1101_PLL_LOCK_CHECK_MSG_SW	// PROGMEM:  22b

#undef  RFR_DEBUG                       // PROGMEM:  354b  RAM: 14b
#define HAS_FASTRF                      // PROGMEM:  468b  RAM:  1b

#if defined(CUL_V3_ZWAVE)
#  define CUL_V3
#endif

#if defined(CUL_V3) || defined(CUL_V4)
//obsolet #define HAS_FTZ
#  define HAS_FHT_8v                    // PROGMEM:  586b  RAM: 23b
#  define HAS_FHT_TF
#  define FHTBUF_SIZE          174      //                 RAM: 174b
#  define RCV_BUCKETS            4      //                 RAM: 25b * bucket
#  define FULL_CC1100_PA                // PROGMEM:  108b
#  define HAS_RAWSEND                   //
//#  define HAS_ASKSIN                    // PROGMEM: 1314
//#  define HAS_ASKSIN_FUP                // PROGMEM:   78
#  define HAS_MORITZ                    // PROGMEM: 1696
//#  define HAS_ESA                       // PROGMEM:  286
#  define HAS_TX3                       // PROGMEM:  168
/* Intertechno Senden einschalten */
#  define HAS_INTERTECHNO
/* Intertechno Empfang einschalten */
#  define HAS_IT
#  define HAS_FLAMINGO
//#  define HAS_TCM97001                  // PROGMEM:  264
//#  define HAS_UNIROLL                   // PROGMEM:   92
#  define HAS_MEMFN                     // PROGMEM:  168
//#  define HAS_SOMFY_RTS                 // PROGMEM: 1716
//#  define HAS_BELFOX                    // PROGMEM:  214
#endif

#if defined(CUL_V4)
#  define HAS_ZWAVE                     // PROGMEM:  882
#  define TTY_BUFSIZE           64      // RAM: TTY_BUFSIZE*4
#endif

#if defined(CUL_V3)
#  define TTY_BUFSIZE          128      // RAM: TTY_BUFSIZE*4
#  undef HAS_MBUS                       // PROGMEM: 4255
#if defined(HAS_MBUS)
#  define MBUS_NO_TX                    // PROGMEM:  962
#endif
#  define HAS_RFNATIVE                  // PROGMEM:  580
//#  define HAS_KOPP_FC                   // PROGMEM: 3370
#endif

#if defined(CUL_V3_ZWAVE)
#  define HAS_ZWAVE                     // PROGMEM:  882
#  undef HAS_MBUS
#  undef HAS_KOPP_FC
#  undef HAS_RFNATIVE
#  define LACROSSE_HMS_EMU              // PROGMEM: 2206
#  define HAS_EVOHOME
#endif


#ifdef CUL_V2
#  define TTY_BUFSIZE           48
#  define FHTBUF_SIZE           74
#  define RCV_BUCKETS            2 
#  define RFR_SHADOW                    // PROGMEM: 10b    RAM: -(TTY_BUFSIZE+3)
#  define HAS_TX3
#  define NO_RF_DEBUG                   // squeeze out some bytes for hoermann_send
#  undef  HAS_CC1101_RX_PLL_LOCK_CHECK_TASK_WAIT
#endif

#ifdef CUL_V2_HM
#  define CUL_V2
#  define HAS_ASKSIN
#  define TTY_BUFSIZE           64
#  define RCV_BUCKETS            2 
#  undef  HAS_RF_ROUTER
#  undef  HAS_FHT_80b
#  define FHTBUF_SIZE            0
#  undef  BOARD_ID_STR
#  define BOARD_ID_STR            "CUL_HM"
#  undef  BOARD_ID_USTR
#  define BOARD_ID_USTR           L"CUL_HM"
#  define HAS_INTERTECHNO
#endif

#ifdef CUL_V2_MAX
#  define CUL_V2
#  define HAS_MORITZ
#  define TTY_BUFSIZE           64
#  define RCV_BUCKETS            2
#  undef  HAS_RF_ROUTER
#  undef  HAS_FHT_80b
#  define FHTBUF_SIZE            0
#  undef  BOARD_ID_STR
#  define BOARD_ID_STR            "CUL_MX"
#  undef  BOARD_ID_USTR
#  define BOARD_ID_USTR           L"CUL_MX"
#  define HAS_INTERTECHNO
#endif

// Ergaenzung wegen WLAN
#ifdef ESP8266
  #define BUSWARE_CUNO2
  #define HAS_ETHERNET            1   
  #define HAS_ETHERNET_KEEPALIVE  1
  #define ETHERNET_KEEPALIVE_TIME 30
  //#define HAS_NTP                 1   
#endif
// WLAN */

/*/ Ergaenzung wegen IR
#define HAS_IRRX
#define HAS_IRTX
#define IRSND_OCx               16  // D0 !? ESP GPIO pin to use. Recommended: 4 (D2).
#define IRMP_PIN                2 // D4 !? 
// IR */

/*/ Ergaenzung wegen ONEWIRE über i2c, nicht parallel zu cc1101 nutzbar
#define HAS_ONEWIRE         10      // OneWire Device Buffer, RAM: 10 * 8 Byte 
#define OW_SPU			                // enable StrongPullUp
// ONEWIRE */

// No features to define below
#ifndef ESP8266
#  include <avr/io.h>
#  include <avr/power.h>

#if !defined(clock_prescale_set) && __AVR_LIBC_VERSION__  < 10701UL
#  warning "avr/power.h needs patching for prescaler functions to work."
#  warning "for the m32u4 add __AVR_ATmega32U4__ for cpu types on prescale block"
#  warning "for the m32u2 add __AVR_ATmega32U2__ for cpu types on prescale block"
#endif
#endif

#ifdef ARDUINO_ESP8266_NODEMCU
#  define VERSION_BOARD ".culfw-esp8266.ino.nodemcu" 
#elif ARDUINO_ESP8266_WEMOS_D1MINI
#  define VERSION_BOARD ".culfw-esp8266.ino.d1_mini"
#else
#  define VERSION_BOARD ".culfw-esp8266.ino.arduino"
#endif

#ifdef ESP8266
//------------- speziell esp8266/nodemcu ------------------
#  define PORTB0 15     //         nodemcu D8 // PORTB0     (Nano PIN 10) // CS
#  define PORTB1 14     //         nodemcu D5 // PORTB1     (Nano PIN 13) // CLK
#  define PORTB2 13     //         nodemcu D7 // PORTB2     (Nano PIN 11) // MOSI
#  define PORTB3 12     //         nodemcu D6 // PORTB3     (Nano PIN 12) // MISO
#  define PB6 PORTB6 // nodemcu not connected 
#  define PORTD2 5      // esp8266/nodemcu D1 // PORTD2     (Nano PIN  2) // GDO2
#  define PORTD3 4      // esp8266/nodemcu D2 // PORTD3     (Nano PIN  3) // GDO0
#  define INT2 PD2 //nodemcu int wie GPO		0x02

#define EIMSK GPIE //ESP8266_REG(0x31C) //GPIO_STATUS R/W (Interrupt Enable)
//------------- speziell esp8266 ------------------
//dummy esp8266
extern unsigned char PORTB;  //unbenutzt wg. Anpassung in cc1100_cs
extern unsigned char PORTD;
extern unsigned char PINB;
extern unsigned char PIND;
extern unsigned char DDRB; //
extern unsigned char DDRD; //
extern unsigned char ISC20;
extern unsigned char EICRA;
//extern unsigned char EIMSK;
extern unsigned char TIMSK1;
extern unsigned char TIFR1;
extern unsigned int  TCNT1;

extern unsigned int  OCR0A;
extern unsigned int  OCR1A;
extern unsigned char OCF1A;
extern unsigned char OCIE1A;
// SPI-Register
#define SPDR SPI1W0
//dummy esp8266
#else // CUL_V3
#  define INT2 PORTB2
#  define BUILTIN_LED 6
#  define ICACHE_RAM_ATTR 
#endif

#if defined(CUL_V3)      // not sure why libc is missing those ...
#  define PB0 PORTB0
#  define PB1 PORTB1
#  define PB2 PORTB2
#  define PB3 PORTB3
#  define PB6 PORTB6
#  define PD2 PORTD2
#  define PD3 PORTD3
#  define INT2 PB2
/* External Interrupt Control Register A - EICRA */
//#define    ISC31        7
//#define    ISC30        6
//#define    ISC21        5
#define    ISC20        4
//#define    ISC11        3
//#define    ISC10        2
//#define    ISC01        1
//#define    ISC00        0
#endif  // CUL_V3

#define SPI_PORT		PORTB
#define SPI_DDR			DDRB
#define SPI_SS			PB0
#define SPI_MISO		PB3
#define SPI_MOSI		PB2
#define SPI_SCLK		PB1

#ifndef ESP8266
#  define bit_is_set(sfr, bit) (_SFR_BYTE(sfr) & _BV(bit))
#  define DIGITAL_HIGH(a,bit) a |= _BV(bit);
#  define DIGITAL_LOW(a,bit) a &= ~_BV(bit);
#else
#  define LED_INV
#  define bit_is_set(sfr, bit) digitalRead(bit)
#  define DIGITAL_HIGH(a,bit) digitalWrite(bit,1);   // GPOS = (1 << b)            
#  define DIGITAL_LOW(a,bit) digitalWrite(bit,0);    // GPOC = (1 << b)  
#  define USB_IsConnected 1
#  define __LPM(a) pgm_read_byte(a)
#endif

#if defined(CUL_V4)
#  define CC1100_CS_DDR			SPI_DDR
#  define CC1100_CS_PORT        SPI_PORT
#  define CC1100_CS_PIN			SPI_SS
#  define CC1100_OUT_DDR        DDRD
#  define CC1100_OUT_PORT       PORTD
#  define CC1100_OUT_PIN        PD3
#  define CC1100_IN_DDR         DDRD
#  define CC1100_IN_PORT        PIND
#  define CC1100_IN_PIN         PD2
#  define CC1100_INT			INT2
#  define CC1100_INTVECT        INT2_vect
#  define CC1100_ISC			ISC20
#  define CC1100_EICR           EICRA
#  define LED_DDR               DDRC
#  define LED_PORT              PORTC
#  define LED_PIN               D3 //PC5
#endif

#if defined(CUL_V3)
#  define CC1100_CS_DDR			SPI_DDR
#  define CC1100_CS_PORT        SPI_PORT
#  define CC1100_CS_PIN			SPI_SS
#  define CC1100_OUT_DDR        DDRD
#  define CC1100_OUT_PORT       PORTD
#  define CC1100_OUT_PIN        PD3
#  define CC1100_OUT_IN         PIND
#  define CC1100_IN_DDR			DDRD
#  define CC1100_IN_PORT        PIND
#  define CC1100_IN_PIN         PD2
#  define CC1100_IN_IN          PIND
#  define CC1100_INT			INT2
#  define CC1100_INTVECT        INT2_vect
#  define CC1100_ISC			ISC20
#  define CC1100_EICR           EICRA
#  define LED_DDR               DDRE
#  define LED_PORT              PORTE
#  define LED_PIN               BUILTIN_LED //BUD3 //6
#endif

#if defined(CUL_V2)
#  define CC1100_CS_DDR		DDRC
#  define CC1100_CS_PORT        PORTC
#  define CC1100_CS_PIN		PC5
#  define CC1100_IN_DDR		DDRC
#  define CC1100_IN_PORT        PINC
#  define CC1100_IN_PIN         PC7
#  define CC1100_OUT_DDR	DDRC
#  define CC1100_OUT_PORT       PORTC
#  define CC1100_OUT_PIN        PC6
#  define CC1100_INT		INT4
#  define CC1100_INTVECT        INT4_vect
#  define CC1100_ISC		ISC40
#  define CC1100_EICR           EICRB
#  define LED_DDR               DDRC
#  define LED_PORT              PORTC
#  define LED_PIN               PC4
#endif

#if defined(ESP8266)
#  define CUL_HW_REVISION "CUL_ESP"
#elif defined(CUL_V3)
#  define CUL_HW_REVISION "CUL_V3"
#elif defined(CUL_V4)
#  define CUL_HW_REVISION "CUL_V4"
#else
//#  define CUL_HW_REVISION "CUL_V2"    // No more mem for this feature
#endif

// only multifrequence-devices?
#define MARK433_PORT            PORTB
#define MARK433_PIN             PINB
#define MARK433_BIT             6
#define MARK915_PORT            PORTB
#define MARK915_PIN             PINB
#define MARK915_BIT             5
// helper for concatenating two char[]
#define con_cat(first, second) first second
// helper for lambda-functions
#ifdef ESP8266
  #define isrtimer_enable          //timer1_enable{T1C |= (1 << TCTE);timer1_write(OCR1A);}
  #define isrtimer_disable         //timer1_disable //T1C &= ~(1 << TCTE)
	#define isrtimer_restart(a)      timer1_write(a) //{T1C |= (1 << TCTE); timer1_write(a);}
  #define isrtimer_reinitialize(a) timer1_write(a) //{T1C |= (1 << TCTE); timer1_write(a);}
	#define isrtimer_clear           
	#define isrtimer_value(a)        OCR1A = a
  #define isrtimer_set(a)          isrtimer_restart(a)
	#define lambda(obj, func)        [&](char *data) { obj . func (data); }
#else
	#define isrtimer_enable          TIMSK1 = _BV(OCIE1A)
	#define isrtimer_disable         TIMSK1 = 0
  #define isrtimer_restart(a)      TCNT1=0
  #define isrtimer_reinitialize(a) TCNT1=a
	#define isrtimer_clear           TIFR1 = _BV(OCF1A)
	#define isrtimer_value(a)        OCR1A = a
	#define isrtimer_set(a)          {isrtimer_value(a); isrtimer_enable;}
  #define lambda(obj, func)        [& obj](char *data) { obj . func (data); }
#endif

#endif // __BOARD_H__
