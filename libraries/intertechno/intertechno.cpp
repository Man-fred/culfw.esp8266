/* 
 * Copyright by O.Droegehorn / 
 *              DHS-Computertechnik GmbH
 * License: GPL v2
 */
#include "board.h"

#ifndef ESP8266
  #include <avr/io.h>
  #include <avr/interrupt.h>
  #include <util/parity.h>
#endif
#include <stdio.h>
#include <parity.h> //flamingo?
#include <string.h>


//#ifdef HAS_INTERTECHNO

#include "delay.h"
#include "rf_send.h"
#include "rf_receive.h"
#include "led.h"
#include "cc1100.h"
#include "display.h"
#include "fncollection.h"
#include "fht.h"
#include "intertechno.h"
#include "stringfunc.h"

#ifdef HAS_ASKSIN
#include "rf_asksin.h"
#endif

#ifdef HAS_MORITZ
#include "rf_moritz.h"
#endif

const uint8_t PROGMEM CC1100_ITCFG[EE_CC1100_CFG_SIZE] = {
// CULFW   IDX NAME     RESET STUDIO COMMENT
   0x0D, // 00 IOCFG2   *29   *0B    GDO2 as serial output
   0x2E, // 01 IOCFG1    2E    2E    Tri-State
   0x2D, // 02 IOCFG0   *3F   *0C    GDO0 for input
   0x07, // 03 FIFOTHR   07   *47    
   0xD3, // 04 SYNC1     D3    D3    
   0x91, // 05 SYNC0     91    91    
   0x3D, // 06 PKTLEN   *FF    3D    
   0x04, // 07 PKTCTRL1  04    04    
   0x32, // 08 PKTCTRL0 *45    32    
   0x00, // 09 ADDR      00    00    
   0x00, // 0A CHANNR    00    00    
   0x06, // 0B FSCTRL1  *0F    06    152kHz IF Frquency
   0x00, // 0C FSCTRL0   00    00    
   0x10, // 0D FREQ2    *1E    21    433.92 (InterTechno Frequency)
   0xb0, // 0E FREQ1    *C4    65    
   0x71, // 0F FREQ0    *EC    e8    
   0x55, // 10 MDMCFG4  *8C    55    bWidth 325kHz
   0xe4, // 11 MDMCFG3  *22   *43    Drate:1500 ((256+228)*2^5)*26000000/2^28
   0x30, // 12 MDMCFG2  *02   *B0    Modulation: ASK
   0x23, // 13 MDMCFG1  *22    23    
   0xb9, // 14 MDMCFG0  *F8    b9    ChannelSpace: 350kHz
   0x00, // 15 DEVIATN  *47    00    
   0x07, // 16 MCSM2     07    07    
   0x00, // 17 MCSM1     30    30    
   0x18, // 18 MCSM0    *04    18    Calibration: RX/TX->IDLE
   0x14, // 19 FOCCFG   *36    14    
   0x6C, // 1A BSCFG     6C    6C    
   0x07, // 1B AGCCTRL2 *03   *03    42 dB instead of 33dB
   0x00, // 1C AGCCTRL1 *40   *40    
   0x90, // 1D AGCCTRL0 *91   *92    4dB decision boundery
   0x87, // 1E WOREVT1   87    87    
   0x6B, // 1F WOREVT0   6B    6B    
   0xF8, // 20 WORCTRL   F8    F8    
   0x56, // 21 FREND1    56    56    
   0x11, // 22 FREND0   *16    17    0x11 for no PA ramping
   0xE9, // 23 FSCAL3   *A9    E9    
   0x2A, // 24 FSCAL2   *0A    2A
   0x00, // 25 FSCAL1    20    00
   0x1F, // 26 FSCAL0    0D    1F    
   0x41, // 27 RCCTRL1   41    41    
   0x00, // 28 RCCTRL0   00    00    
};

void
IntertechnoClass::tunein(void)
{
  if (!on){
    int8_t i;
    CC1100.manualReset(0);
        
    CC1100_ASSERT;                             // load configuration
    CC1100.cc1100_sendbyte( 0 | CC1100_WRITE_BURST );
    for(i = 0; i < 13; i++) {
      //CC1100.cc1100_sendbyte(__LPM(CC1100_ITCFG+i));
      CC1100.cc1100_sendbyte(pgm_read_byte(&CC1100_ITCFG[i]));
    }                                                    // Tune to standard IT-Frequency
    CC1100.cc1100_sendbyte(it_frequency[0]);                    // Modify Freq. for 433.92MHZ, or whatever
    CC1100.cc1100_sendbyte(it_frequency[1]);
    CC1100.cc1100_sendbyte(it_frequency[2]);      
    for (i = 16; i<EE_CC1100_CFG_SIZE; i++) {
      //CC1100.cc1100_sendbyte(__LPM(CC1100_ITCFG+i));
      CC1100.cc1100_sendbyte(pgm_read_byte(&CC1100_ITCFG[i]));
    }
    CC1100_DEASSERT;
    
    // not in c -->
    MYDELAY.my_delay_us(10);
    // not in c <--

    uint8_t pa = EE_CC1100_PA;
    CC1100_ASSERT;                             // setup PA table
    CC1100.cc1100_sendbyte( CC1100_PATABLE | CC1100_WRITE_BURST );
    for (uint8_t i = 0;i<8;i++) {
      CC1100.cc1100_sendbyte(FNcol.erb(pa++));
    }
    CC1100_DEASSERT;

    CC1100.ccStrobe( CC1100_SCAL );
    MYDELAY.my_delay_ms(1);
    cc_on = 1;                                  // Set CC_ON  

    CC1100.ccRX();
    on = 1;
  }
}

void
IntertechnoClass::send_bit(uint8_t bit)
{
  if (bit == 1) {
    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval * it_faktor);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval);

    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval * it_faktor);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval);
  } else if (bit == 0) {
    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval * it_faktor);

    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval * it_faktor);
  } else if (bit == 2) {
    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval * it_faktor);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval);

    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval * it_faktor);
  } else { // (bit == 3 / F)
    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval * it_faktor);

    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval * it_faktor);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval);
  }
}

void IntertechnoClass::send_bit_F(uint8_t bit)
{
  if (bit == 1) {
    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval * it_faktor);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval);
  } else if (bit == 0) {
    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval * it_faktor);
  }
}

void
IntertechnoClass::send_start_V3(void) {
    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
  MYDELAY.my_delay_us(it_interval);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
  MYDELAY.my_delay_us(it_interval_v3 * 10);
}

void
IntertechnoClass::send_stop_V3(void) {
    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
  MYDELAY.my_delay_us(it_interval_v3);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
  MYDELAY.my_delay_us(it_interval_v3 * 40);
}

void
IntertechnoClass::send_bit_V3(uint8_t bit)
{
  if (bit == 1) {
    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval_v3);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval_v3 * 5);

    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval_v3);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval_v3);
  } else if (bit == 0) {
    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval_v3);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval_v3);

    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval_v3);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval_v3 * 5);
  } else {
    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval_v3);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval_v3);

    DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
    MYDELAY.my_delay_us(it_interval_v3);
     DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
    MYDELAY.my_delay_us(it_interval_v3);    
  }
}

/*
  Encode and decode routines are based on
  http ://cpp.sh/9ye4
  https ://github.com/r10r/he853-remote
  Thanks to fuchks from the FHEM forum for providing this
  see also https ://forum.fhem.de/index.php/topic,36399.60.html
*/

/*
    Extracts from the input (28-Bit code needs to be aligned to the right: 0x0.......):
    button = receiverId
    value (0=OFF, 1=ON, DIM)
    rolling code 0..n
    transmitterId of the remote control.
    */
void IntertechnoClass::FlamingoDecrypt(uint8_t *in)
{

    uint8_t ikey[16] = {7,14,8,3,13,10,2,12,4,5,6,0,9,1,11,15};  //invers cryptokey (exchanged index & value)
    uint8_t mn[6];  // message separated in nibbles

    mn[0] = (in[2] & 0x0F);
    mn[1] = (in[2] & 0xF0) >> 0x4;
    mn[2] = (in[1] & 0x0F);
    mn[3] = (in[1] & 0xF0) >> 0x4;
    mn[4] = (in[0] & 0x0F);
    mn[5] = (in[0] & 0xF0) >> 0x4;

    //XOR decryption
  for (uint8_t i = 4; i >= 1; i--)
  {                          // decrypt 4 nibbles
    mn[i] = ikey[mn[i]] ^ mn[i - 1];  // decrypted with predecessor & key
  }
  mn[0] = ikey[mn[0]];          //decrypt first nibble

    //Output decrypted message 
    in[0] = (mn[5] << 4) + mn[4]; // transmitterId
    in[1] = (mn[3] << 4) + mn[2]; // transmitterId
    in[2] =  mn[0] & 0x7;         // receiverId
    in[3] = (mn[1] >> 1) & 0x1;   // on/off
    in[4] = (mn[1] >> 2) & 0x3;   // rolling code
}


/*
    Encrypts the
    button = receiverId
    value (0=OFF, 1=ON; DIM)
    rolling code 0..n
    transmitterId of the remote control.
  28-Bit code is aligned to the right (0x0.......)!
    */
void IntertechnoClass::FlamingoEncrypt(uint8_t *transmitterId)
{
    uint8_t key[16] = {11,13,6,3,8,9,10,0,2,12,5,14,7,4,1,15}; //cryptokey 
    uint8_t mn[6];
    
    mn[0] = 8 + transmitterId[2];                // mn[0] = 1iiib i=receiver-ID
    mn[1] = (transmitterId[4] << 2) & 15;         // 2 lowest bits of rolling-code
    if (transmitterId[3] > 0)
    {                        // ON or OFF
        mn[1] |= 2;
    }                        // mn[1] = rrs0b r=rolling-code, s=ON/OFF, 0=const 0?
    mn[2] =  transmitterId[1] & 15;            // mn[2..4] = ttttb t=transmitterId in nibbles -> 4x ttttb
    mn[3] = (transmitterId[1] >> 4) & 15;            // mn[2..4] = ttttb t=transmitterId in nibbles -> 4x ttttb
    mn[4] =  transmitterId[0] & 15;
    mn[5] = (transmitterId[0] >> 4) & 15;

    //XOR encryption 1 round
  mn[0] = key[mn[0]];          // encrypt first nibble
  for (uint8_t i = 1; i <= 4; i++)
  {                      // encrypt 4 nibbles
    mn[i] = key[(mn[i] ^ mn[i - 1])];// crypted with predecessor & key
  }
    //mn[6] = mn[6] ^ 9;                // no  encryption

    //Output encrypted message 
    transmitterId[0] = (mn[5] << 4) + mn[4]; // transmitterId
    transmitterId[1] = (mn[3] << 4) + mn[2]; // transmitterId
    transmitterId[2] = (mn[1] << 4) + mn[0]; // on/off, rolling code, receiverId
}

void
IntertechnoClass::send (char *in) {  
  int8_t i, j, k;

  LED_ON();

  #if defined (HAS_IRRX) || defined (HAS_IRTX) //Blockout IR_Reception for the moment
    cli(); 
  #endif
      
  // If NOT InterTechno mode
  if(!on)  {
    #ifdef HAS_ASKSIN
      if (asksin_on) {
        restore_asksin = 1;
        Asksin.on = 0;
        }
    #endif
    #ifdef HAS_MORITZ
      if(Moritz.on()) {
        restore_moritz = 1;
        Moritz.on(0);
      }
    #endif
    tunein();
    MYDELAY.my_delay_ms(3);             // 3ms: Found by trial and error
  }
  CC1100.ccStrobe(CC1100_SIDLE);
  CC1100.ccStrobe(CC1100_SFRX );
  CC1100.ccStrobe(CC1100_SFTX );

  CC1100.ccTX();                       // Enable TX 
  
  int8_t sizeOfPackage = strlen(in)-1; // IT-V1 = 14, IT-V3 = 33
    
  for(i = 0; i < it_repetition; i++)  {
    if (sizeOfPackage == 33) {      
      send_start_V3();
    } else if (intertek) {
      if (i < 4) {
        it_interval = 375;
        it_faktor   = 3;
      } else {
        it_interval = 510;
        it_faktor   = 2;
      }

      // Sync-Bit
      DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
      for(k = 0; k < (i<4 ? 1 : 6); k++)  {
        MYDELAY.my_delay_us(it_interval);
      }
      DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
      //for(k = 0; k < 31; k++)  {
      for(k = 0; k < 6; k++)  {
        MYDELAY.my_delay_us(it_interval);
      }
    }
    for(j = 1; j < sizeOfPackage; j++)  {
      if (intertek) {
        for(k = 7; k >= 0; k--) {
          send_bit_F(in[j+1] & _BV(k));
        }
      } else if(in[j+1] == '0') {
        if (sizeOfPackage == 33) {
          send_bit_V3(0);
        } else {
          send_bit(0);
        }      
      } else if (in[j+1] == '1') {
        if (sizeOfPackage == 33) {
          send_bit_V3(1);
        } else {
          send_bit(1);
        }  
      } else if (in[j+1] == '2') {
        if (sizeOfPackage == 33) {
          send_bit_V3(2);
        } else {
          send_bit(2);
        }
      } else {
        if (sizeOfPackage == 33) {
          send_bit_V3(3);
        } else {
          send_bit(3);
        }
      }
    }
    if (sizeOfPackage == 33) {  
      send_stop_V3();
    } else if (!intertek) {
      // Sync-Bit
      DIGITAL_HIGH(CC1100_OUT_PORT, CC1100_OUT_PIN);     // High          
      MYDELAY.my_delay_us(it_interval);
      DIGITAL_LOW(CC1100_OUT_PORT, CC1100_OUT_PIN);       // Low
      for(k = 0; k < 31; k++)  {
        MYDELAY.my_delay_us(it_interval);
      }
    }
  } //Do it n Times

  if(on) {
    if(tx_report) {                               // Enable RX
      CC1100.ccRX();
    } else {
      CC1100.ccStrobe(CC1100_SIDLE);
    }
  } 
  #ifdef HAS_ASKSIN
    else if (restore_asksin) {
      restore_asksin = 0;
      rf_asksin_init();
      asksin_on = 1;
          CC1100.ccRX();
    }  
  #endif
  #ifdef HAS_MORITZ
    else if (restore_moritz) {
      restore_moritz = 0;
      Moritz.init();
    }
  #endif
  else {
    RfReceive.set_txrestore();
  }  

  #if defined (HAS_IRRX) || defined (HAS_IRTX) //Activate IR_Reception again
    sei(); 
  #endif      

  LED_OFF();

  DC('i');DC('s');
  for(j = 1; j < sizeOfPackage; j++)  {
    if(in[j+1] == '0') {
      DC('0');
    } else if (in[j+1] == '1') {
      DC('1');
      } else if (in[j+1] == '2') {
        DC('2');
    } else {
      DC('F');
    }
  }
  DNL();
}


void
IntertechnoClass::func(char *in)
{
  if (in[1] == 't') {
      STRINGFUNC.fromdec (in+2, (uint8_t *)&it_interval);
      DU(it_interval,0); DNL();
  } else if (in[1] == 's') {
      if (in[2] == 'r') {    // Modify Repetition-counter
        STRINGFUNC.fromdec (in+3, (uint8_t *)&it_repetition);
        DU(it_repetition,0); DNL();
      } else {
        send (in);        // Sending real data
    } //sending real data
  } else if (in[1] == 'i') { // set type intertek
    intertek = (in[2] == '1');
    if (intertek) {
      tunein ();
      // raw x08 // pa 5dB ohne ramping
      // raw X21 // pa aktivieren 
    }
    DC('i');DU(intertek,0); DNL();
  } else if (in[1] == 'r') { // Start of "Set Frequency" (f)
    #ifdef HAS_ASKSIN
      if (asksin_on) {
        restore_asksin = 1;
        asksin_on = 0;
      }
    #endif
    #ifdef HAS_MORITZ
      if (Moritz.on()) {
        restore_moritz = 1;
        Moritz.on(0);
      }
    #endif
    tunein ();
  } else if (in[1] == 'f') { // Set Frequency
      if (in[2] == '0' ) {
        it_frequency[0] = 0x10;
        it_frequency[1] = 0xb0;
        it_frequency[2] = 0x71;
      } else {
        STRINGFUNC.fromhex (in+2, it_frequency, 3);
      }
      DC('i');DC('f');DC(':');
      DH2(it_frequency[0]);
      DH2(it_frequency[1]);
      DH2(it_frequency[2]);
      DNL();
  } else if (in[1] == 'x') {                         // Reset Frequency back to Eeprom value
    if(0) { ;
    #ifdef HAS_ASKSIN
    } else if (restore_asksin) {
      restore_asksin = 0;
      rf_asksin_init();
      asksin_on = 1;
      CC1100.ccRX();
    #endif
    #ifdef HAS_MORITZ
    } else if (restore_moritz) {
      restore_moritz = 0;
      Moritz.init();
    #endif
    } else {
      CC1100.ccInitChip(EE_CC1100_CFG);                    // Set back to Eeprom Values
      if(tx_report) {                               // Enable RX
        CC1100.ccRX();
      } else {
        CC1100.ccStrobe(CC1100_SIDLE);
      }
    }
    on = 0;
  } else if (in[1] == 'c') {    // Modify Clock-counter
        STRINGFUNC.fromdec (in+1, (uint8_t *)&it_interval);
    }
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_INTERTECHNO)
IntertechnoClass it;
#endif
