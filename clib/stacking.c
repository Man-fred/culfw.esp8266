/*
* Support for the SCC stacking transceivers
*
* Written by TOSTi <tosti@busware.de>
* Placed under the modified BSD license
*
*/

#include "board.h"

#ifdef HAS_STACKING

#include "stacking.h"
#include "stringfunc.h"
#include "display.h"
#include "clock.h"
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// Receive a CRLF terminated message from downlink UART1 
// and forward to uplink UART0 by prefixing a '*'

#define BAUD 38400
#define USE_2X
#include <util/setbaud.h>

static char mycmd[256];  /* TODO: are 256 byte enough for EnOcean communication? */
static uint8_t mypos = 0;
static volatile uint8_t myEOL = 0;

void stacking_initialize(void) {
  // initialize I/O pins
  DDRD |= _BV( 3 ); // PD3: TX out

  // initialize the USART
  UBRR1H = UBRRH_VALUE;
  UBRR1L = UBRRL_VALUE;
  UCSR1A = 0;
#if USE_2X
  UCSR1A |= _BV(U2X1);
#endif
  UCSR1B = _BV(RXCIE1) | _BV(RXEN1)  | _BV(TXEN1); // RX Complete IRQ enabled, Receiver/Transmitter enable
  UCSR1C = _BV(UCSZ10) | _BV(UCSZ11); // 8 Bit, n, 1 Stop

  mycmd[mypos] = 0;
  myEOL = 0;
}

#ifdef HAS_STACK_ENOCEAN
#include <ctype.h>
static uint8_t enocean_connected = 0;
void uart_57600(void) {
#undef  BAUD  // avoid compiler warning
#undef  BAUD_TOL
#define BAUD 57600
#define BAUD_TOL 3  /* TODO: is 3% tolerance ok? */
#include <util/setbaud.h>
  UBRR1H = UBRRH_VALUE;
  UBRR1L = UBRRL_VALUE;
#if USE_2X
  UCSR1A |= _BV(U2X1);
#else
  UCSR1A &= ~_BV(U2X1);
#endif
}

enum {
    STATE_WAIT_SYNC = 0,
    STATE_WAIT_DATA_LENGTH_H,
    STATE_WAIT_DATA_LENGTH_L,
    STATE_WAIT_OPTIONAL_LENGTH,
    STATE_WAIT_PACKET_TYPE,
    STATE_WAIT_CRC,
    STATE_WAIT_END
};
static uint8_t state           = STATE_WAIT_SYNC;
static uint8_t data_length_h   = 0;
static uint8_t data_length_l   = 0;
static uint8_t optional_length = 0;
static uint8_t packet_type     = 0;
static uint8_t crc             = 0;
static uint8_t len             = 0;
static uint16_t packet_size    = 0;

/* From EnOcean Serial Protocol 3 (ESP3) V1.15 / Feb 28, 2011,
 * chapter 2.3 CRC8 calculation
 */
uint8_t u8CRC8Table[256] = {
  0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
  0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
  0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
  0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
  0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5,
  0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
  0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85,
  0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
  0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
  0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
  0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2,
  0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
  0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32,
  0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
  0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
  0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
  0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c,
  0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
  0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec,
  0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
  0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
  0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
  0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c,
  0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
  0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b,
  0x76, 0x71, 0x78, 0x7f, 0x6A, 0x6d, 0x64, 0x63,
  0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
  0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
  0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb,
  0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8D, 0x84, 0x83,
  0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb,
  0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3
};
#define proccrc8(u8CRC, u8Data) (u8CRC8Table[u8CRC ^ u8Data])
uint8_t crc_ok(void) {
  uint8_t u8CRC = 0;
  u8CRC = proccrc8(u8CRC, data_length_h);
  u8CRC = proccrc8(u8CRC, data_length_l);
  u8CRC = proccrc8(u8CRC, optional_length);
  u8CRC = proccrc8(u8CRC, packet_type);

  return u8CRC == crc;
}

uint8_t asc2int(uint8_t c) {
    c = toupper(c)-'0';
    return (c<10 ? c : c-7);
}
uint8_t int2asc(uint8_t i) {
    return (i<10 ? '0'+i : 'A'+i-10);
}
void put_in_mycmd(uint8_t b) {
  mycmd[mypos++] = int2asc((b>>4) & 0x0F);
  mycmd[mypos++] = int2asc(b & 0x0F);
}

/* Receive data from EnOcean Pi in binary format */
/* The packet structure has to be analyzed to find the packet start */
ISR(USART1_RX_vect) {
  uint8_t errors = UCSR1A;
  uint8_t data   = UDR1;

  if (errors & (_BV(FE1)|_BV(DOR1)))
	return;

  if (myEOL) // unprocessed message?
    return;

  if (enocean_connected == 1) 
  {
    switch (state)
    {
      case STATE_WAIT_SYNC:
        if (data == 0x55)
          state = STATE_WAIT_DATA_LENGTH_H;
        break;
      case STATE_WAIT_DATA_LENGTH_H:
        data_length_h = data;
        state = STATE_WAIT_DATA_LENGTH_L;
        break;
      case STATE_WAIT_DATA_LENGTH_L:
        data_length_l = data;
        state = STATE_WAIT_OPTIONAL_LENGTH;
        break;
      case STATE_WAIT_OPTIONAL_LENGTH:
        optional_length = data;
        state = STATE_WAIT_PACKET_TYPE;
        break;
      case STATE_WAIT_PACKET_TYPE:
        packet_type = data;
        state = STATE_WAIT_CRC;
        break;
      case STATE_WAIT_CRC:
        crc = data;
        if (crc_ok()) {
          mypos = 0;
          put_in_mycmd(0x55);
          put_in_mycmd(data_length_h);
          put_in_mycmd(data_length_l);
          put_in_mycmd(optional_length);
          put_in_mycmd(packet_type);
          put_in_mycmd(crc);
          len = 0;
          packet_size = (((uint16_t)data_length_h)<<8) + data_length_l + optional_length;
          state = STATE_WAIT_END;
        }
        else {
          /* sync byte might be in data_length_h, data_length_l, 
           * optional_length, packet_type or crc */
          if (data_length_h == 0x55) {
            data_length_h   = data_length_l;
            data_length_l   = optional_length;
            optional_length = packet_type;
            packet_type     = crc;
            state = STATE_WAIT_CRC;
          }
          else if (data_length_l == 0x55) {
            data_length_h   = optional_length;
            data_length_l   = packet_type;
            optional_length = crc;
            state = STATE_WAIT_PACKET_TYPE;
          }
          else if (optional_length == 0x55) {
            data_length_h = packet_type;
            data_length_l = crc;
            state = STATE_WAIT_OPTIONAL_LENGTH;
          }
          else if (packet_type == 0x55) {
            data_length_h     = crc;
            state = STATE_WAIT_DATA_LENGTH_L;
          }
          else if (crc == 0x55) {
            state = STATE_WAIT_DATA_LENGTH_H;
          }
          else {
            state = STATE_WAIT_SYNC;
          }
        }
        break;
      case STATE_WAIT_END:
        put_in_mycmd(data);
        ++len;
        if (len >= packet_size+1)
        {
          
          mycmd[mypos] = 0;
          myEOL = 1; 
          state = STATE_WAIT_SYNC;
        }
        break;
    }
  }
  else
  {
    if (data == 13)
      return;

    if (data == 10) {
      // EOL
      mycmd[mypos] = 0;
      myEOL = 1; 
      return;
    }

    mycmd[mypos++] = data;
  }
}

void stacking_func_eno(char* in) { 
  // downlink message to UART1 by removing one '*'
  uint8_t i = 1;
  uint8_t high = 0;
  uint8_t low = 0;
  
  if (enocean_connected == 0) 
  {
    uart_57600();
    enocean_connected = 1;
  }

  while (in[i]) { 
    while(!(UCSR1A&_BV(UDRE1))) ; // wait for UART1 clear
    
    /* Two ASCII characters give one byte */
    high = asc2int(in[i++]);
    if (in[i]) {
      low  = asc2int(in[i++]);
    }
    UDR1 = (high<<4) | low;
  }
}
#else
ISR(USART1_RX_vect) {
  uint8_t errors = UCSR1A;
  uint8_t data   = UDR1;

  if (errors & (_BV(FE1)|_BV(DOR1)))
	return;

  if (myEOL) // unprocessed message?
    return;

  if (data == 13)
    return;

  if (data == 10) {
    // EOL
    mycmd[mypos] = 0;
    myEOL = 1; 
    return;
  }

  mycmd[mypos++] = data;
}
#endif

void stacking_task(void) {
  if (!myEOL)
    return;

  // Process message
  DC( '*' );
  DS( mycmd );
  DNL();

  mypos = 0;
  mycmd[mypos] = 0;
  myEOL = 0;
};

void stacking_func(char* in) { 
  // downlink message to UART1 by removing one '*'
  uint8_t i = 1;

  while (in[i]) { 
    while(!(UCSR1A&_BV(UDRE1))); // wait for UART1 clear
    UDR1 = in[i++];
  }

  while(!(UCSR1A&_BV(UDRE1)));
  UDR1 = 13;

  while(!(UCSR1A&_BV(UDRE1)));
  UDR1 = 10;
}

#endif
