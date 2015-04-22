/*
 * Copyright by D.Tostmann
 * License: GPL v2
 */
#include "board.h"
#ifdef HAS_ITPLUS
#include <string.h>
#include <avr/pgmspace.h>
#include "cc1100.h"
#include "delay.h"
#include "rf_receive.h"
#include "display.h"

#include "rf_itplus.h"
#include "cc1100.h"

static uint8_t itp_on = 0;
//static uint8_t payload[ITP_PAYLOAD_SIZE];

const uint8_t PROGMEM ITP_CFG[46] = {
  
  CC1100_FSCTRL1, 0x06,
  CC1100_FREQ2, 0x21,   // FREQ2     Frequency control word, high byte.
  CC1100_FREQ1, 0x65,   // FREQ1     Frequency control word, middle byte.
  CC1100_FREQ0, 0x6A,   // FREQ0     Frequency control word, low byte.
  
  CC1100_MCSM1, 0x00,   // always go into IDLE
  CC1100_MCSM0, 0x18,
  CC1100_FOCCFG, 0x16,
  CC1100_AGCCTRL2, 0x43,
  CC1100_AGCCTRL1, 0x68,
  CC1100_FSCAL1, 0x00,
  CC1100_FSCAL0, 0x11,
  
  CC1100_IOCFG2, 0x01,   // IOCFG2    GDO2 output pin configuration.
  CC1100_IOCFG0, 0x46,   // IOCFG0   GDO0 output pin configuration. Refer to SmartRF® Studio User Manual for detailed pseudo register explanation.
  
  CC1100_SYNC0, 0xd4,
  CC1100_SYNC1, 0x2d,
  
  CC1100_FIFOTHR, 2,     // 12 byte in RX
  
  CC1100_PKTCTRL1, 0x00,   // PKTCTRL1  Packet automation control.
  CC1100_PKTCTRL0, 0x02,   // PKTCTRL0  Packet automation control - infinite len
  
  CC1100_MDMCFG4, 0x89,   // MDMCFG4   Modem configuration.
  CC1100_MDMCFG3, 0x5C,   // MDMCFG3   Modem configuration.
  CC1100_MDMCFG2, 0x06,   // !! 05 !! MDMCFG2   Modem configuration.
  
  CC1100_DEVIATN, 0x56,   // DEVIATN   Modem deviation setting (when FSK modulation is enabled).
  
  0xff
};

// Default bitrate is 17.241 kbps - set BR for 9.579 kbps
//
void itplus_init(uint8_t BR) {

  EIMSK &= ~_BV(CC1100_INT);                 // disable INT - we'll poll...
  SET_BIT( CC1100_CS_DDR, CC1100_CS_PIN );   // CS as output

  CC1100_DEASSERT;                           // Toggle chip select signal
  my_delay_us(30);
  CC1100_ASSERT;
  my_delay_us(30);
  CC1100_DEASSERT;
  my_delay_us(45);

  ccStrobe( CC1100_SRES );                   // Send SRES command
  my_delay_us(100);

  // load configuration
  for (uint8_t i = 0; i<60; i += 2) {
       
    if (pgm_read_byte( &ITP_CFG[i] )>0x40)
      break;

    cc1100_writeReg( pgm_read_byte(&ITP_CFG[i]),
                     pgm_read_byte(&ITP_CFG[i+1]) );
  }

  if (BR) {
    cc1100_writeReg(CC1100_MDMCFG4, 0x88); 
    cc1100_writeReg(CC1100_MDMCFG3, 0x82);
  }

  ccStrobe( CC1100_SCAL );

  itp_on = 1;
  
  my_delay_ms(1);
}

void itplus_task(void) {
  uint8_t len;

  if(!itp_on)
    return;
  
  if (bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN )) {

    // start over syncing
    ccStrobe( CC1100_SIDLE );

    //    memset( payload, 0, ITP_PAYLOAD_SIZE );
    
    len = cc1100_readReg( CC1100_RXBYTES ) & 0x7f; // read len, transfer RX fifo
    
    if (len) {
      
      CC1100_ASSERT;
      cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );

      DC( 'E' );

      for (uint8_t i=0; i<len; i++) {
	//	payload[i] = cc1100_sendbyte( 0 );
	//	DH2( payload[i] );
	DH2( cc1100_sendbyte( 0 ) );
      }
      
      CC1100_DEASSERT;
      
      DNL();
      
    }

    return;
  }
       
  switch (cc1100_readReg( CC1100_MARCSTATE )) {
            
       // RX_OVERFLOW
  case 17:
       // IDLE
  case 1:
    ccStrobe( CC1100_SFRX  );
    ccStrobe( CC1100_SIDLE );
    ccStrobe( CC1100_SNOP  );
    ccStrobe( CC1100_SRX   );
    break;
       
  }

}


void itplus_func(char *in) {
  uint8_t bitrate = 0;

  if(in[1] == 'r') {                // Reception on
    
    // Default bitrate is 17.241 kbps - if "Er1" => 9.579 kbps
    if (in[2])
      fromdec(in+2, &bitrate);
    
    itplus_init(bitrate);
  } else {

    if (itp_on)
      ccStrobe( CC1100_SIDLE );
    
    itp_on = 0;
  }
  
}

#endif


