#include "rf_moritz.h"

#include <string.h>
#ifndef ESP8266
  #include <avr/pgmspace.h>
  #include <avr/interrupt.h>
  #include <avr/io.h>
#endif
#include "stringfunc.h"
#include "cc1100.h"
#include "delay.h"
#include "rf_receive.h"
#include "display.h"
#include "clock.h"
#include "rf_send.h" //credit_10ms

#include "fncollection.h"


/*
 * CC1100_PKTCTRL0.LENGTH_CONFIG = 1 //Variable packet length mode. Packet length configured by the first byte after sync word
 *                 CRC_EN = 1
 *                 PKT_FORMAT = 00 //Use FIFOs
 *                 WHITE_DATA = 0
 * MDMCFG2.SYNC_MODE = 3: 30/32 sync word bits detected
 *        .MANCHESTER_EN = 0
 *        .MOD_FORMAT = 0: 2-FSK
 *        .DEM_DCFILT_OFF = 0
 *
 * EVENT0 = 34667
 * t_Event0 = 750/26Mhz * EVENT0 * 2^(5*WOR_RES) = 1 second
 *
 * One message with 12 payload bytes takes (4 byte preamble + 4 byte sync + 12 byte payload) / 1kbit/s = 160 ms.
 */
const uint8_t PROGMEM MORITZ_CFG[EE_CC1100_CFG_SIZE] = {
//   MAX   IDX NAME     RESET STUDIO CULFW COMMENT for MAX
//     0x00, 0x08,
   0x07,//MAX-Actor: 0x08, // 00 IOCFG2:               0D?    GDO2_CFG=7: Asserts when a packet has been received with CRC OK. De-asserts when the first byte is read from the RX FIFO
   0x2E, // 01 IOCFG1    2E+   2E    2E    Tri-State
	 0x46, // 02 IOCFG0                2D?    Deasserts when sync word has been sent / received
   0x07, // 03 FIFOTHR   07+  *47    07    FIFO TX/RX 33/32
   0xC6, // 04 SYNC1     D3    D3    D3?    high byte
   0x26, // 05 SYNC0     91    91    91?    low byte
   0xFF, // 06 PKTLEN   *FF+   3D    3D?
   0x4C,//.MAX-Actor: 0x4C, culfw 0x0C// 07 PKTCTRL1  04    04    04?    automatic flush + RSSI, LQI
   0x45, // 08 PKTCTRL0 *45+   32    32?
   0x00, // 09 ADDR      00+   00    00
   0x00, // 0A CHANNR    00+   00    00
   0x06, // 0B FSCTRL1  *0F    06    06    152kHz IF Frquency
   0x00, // 0C FSCTRL0   00+   00    00
   0x21, // 0D FREQ2    *1E    21    21    868.3 (def:800MHz)
   0x65, // 0E FREQ1    *C4    65    65    
   0x6A, // 0F FREQ0    *EC    e8    6A    see CC1100         
   0xC8, // 10 MDMCFG4               55    MAX!: DRATE_E=8,DRATE_M=147, data rate = (256+DRATE_M)*2^DRATE_E/2^28*f_xosc = (9992.599) ~1kbit/s (at f_xosc=26 Mhz)
   0x93, //    MDMCFG3               E4    MAX!: 
   0x03, //    MDMCFG2               30?    see above
   0x22, // 13 MDMCFG1               23?    CHANSPC_E=2, NUM_PREAMBLE=2 (4 bytes), FEC_EN = 0 (disabled)
         //0x13, 0x72,  //MDMCFG1          CHANSPC_E=2, NUM_PREAMBLE=7 (24 bytes), FEC_EN = 0 (disabled)
   0xb9, // 14 MDMCFG0  *F8    b9+   b9    ChannelSpace: 350kHz
   0x34, // 15 DEVIATN               00?
   0x07,//MAX-Actor: 0x1C, // 16 MCSM2     07+    07   07    RX_TIME = 7 (Timeout for sync word search in RX for both WOR mode and normal RX operation = Until end of packet) RX_TIME_QUAL=0 (check if sync word is found)
   0x3F,//aus Wiki:0x00,//0x3F, // 17 MCSM1                 00?   TXOFF=RX, RXOFF=RX, CCA_MODE=3:If RSSI below threshold unless currently receiving a packet
   0x18, // 18 MCSM0                 18?   orig.MAX 28; PO_TIMEOUT=64, FS_AUTOCAL=2: When going from idle to RX or TX automatically
   0x16, // 19 FOCCFG                14?
   0x6C, // 1A BSCFG     6C+    6C   6c
   0x43, // 1B AGCTRL2               07?
   0x00, // 1C AGCCTRL1 *40   *40    00    see CC1100 
   0x90, // 1D AGCCTRL0 *91   *92    90    see CC1100 4dB decision boundery
   0x87, // 1E WOREVT1   87    87    87    EVENT0[high]
   0x6B, // 1F WOREVT0   6B    6B    6b    EVENT0[low] 
   0x78,//MAX-Actor: 0x78, culfw 0xF8 // 20 WORCTRL   F8    F8    F8    WOR_RES=00 (1.8-1.9 sec) EVENT1=7 (48, i.e. 1.333 – 1.385 ms)
   0x56, // 21 FREND1    56    56    56
   0x10, // 22 FREND0   *16    17    11    in culfw 0x11, see CC1100 0x11 for no PA ramping
   0xE9, // 23 FSCAL3   *A9    E9+   E9 
   0x2A, // 24 FSCAL2   *0A    2A+   2A
   0x00, // 25 FSCAL1    20    00    00
   0x11, // 26 FSCAL0    0D    1F    1F?
   0x41, // 27 RCCTRL1   41+   41    41
   0x00  // 28 RCCTRL0   00+   00    00
		 
   //  0x29, 0x59, //FSTEST  "For test only. Do not write to this register."
   //  0x2C, 0x81, //TEST2
   //  0x2D, 0x35, //TEST1
   //  0x3E, 0xC3, //?? Readonly PATABLE?
};

uint8_t RfMoritzClass::autoAckAddr[] = {0, 0, 0};
uint8_t RfMoritzClass::fakeWallThermostatAddr[] = {0, 0, 0};

RfMoritzClass::RfMoritzClass(){
  onState = 0;
  lastSendingTicks = 0;
}

uint8_t RfMoritzClass::on(uint8_t onNew){
	if (onNew == 3){
		DC('Zi');DH2(onNew);DNL();
		onState = 0;
	} else if (onNew < 2){
	  if (onNew != onState){
			onState = onNew;
			DC('D');DC('Z');DH2(onState);DNL();
		}
	}
	return onState;
}

void RfMoritzClass::init(void)
{
  CC1100.manualReset(0);
  //test: +10db, no PA-ramping, see cc1100.cpp, CC1100_PA[]
  CC1100.cc1100_writeReg( CC1100_PATABLE, 0xC3);

	// not in c -->
	//CC1100.ccStrobe(CC1100_SFRX);
  //CC1100.ccStrobe(CC1100_SFTX);
	// not in c <--
  
	// load configuration
  CC1100_ASSERT;                             
  CC1100.cc1100_sendbyte( 0 | CC1100_WRITE_BURST );
  for(uint8_t i = 0; i < 0x29; i++) {
	  CC1100.cc1100_sendbyte(pgm_read_byte(&MORITZ_CFG[i]));
  }
  CC1100_DEASSERT;

  //auto? CC1100.ccStrobe( CC1100_SCAL );
  //auto? MYDELAY.my_delay_ms(4); // 4ms: Found by trial and error

  //This is ccRx() but without enabling the interrupt
  uint8_t cnt = 0xff;
  //Enable RX. Perform calibration first if coming from IDLE and MCSM0.FS_AUTOCAL=1.
  while(cnt-- && (CC1100.ccStrobe( CC1100_SRX ) & CC1100_STATUS_STATE_BM) != CC1100_STATE_RX) // != 1 ?
    MYDELAY.my_delay_us(10);
  if (cnt)
    on(1);
}

void RfMoritzClass::handleAutoAck(uint8_t* enc)
{
  /* Debug ouput
  DC('D');
  DC('Z');
  DH2(autoAckAddr[0]);
  DH2(autoAckAddr[1]);
  DH2(autoAckAddr[2]);
  DC('-');
  DH2(enc[0]);
  DH2(enc[3]);
  DH2(enc[7]);
  DH2(enc[8]);
  DH2(enc[9]);
  DNL();
  //   */

  //Send acks to when required by "spec"
  if((autoAckAddr[0] != 0 || autoAckAddr[1] != 0 || autoAckAddr[2] != 0) /* auto-ack enabled */
      && (
           enc[3] == 0x30 /* type ShutterContactState */
        || enc[3] == 0x40 /* type SetTemperature */
        || enc[3] == 0x50 /* type PushButtonState */
        )
      && enc[7] == autoAckAddr[0] /* dest */
      && enc[8] == autoAckAddr[1]
      && enc[9] == autoAckAddr[2])
    sendAck(enc);

  if((fakeWallThermostatAddr[0] != 0 || fakeWallThermostatAddr[1] != 0 || fakeWallThermostatAddr[2] != 0) /* fake enabled */
      && enc[0] == 11 /* len */
      && enc[3] == 0x40 /* type SetTemperature */
      && enc[7] == fakeWallThermostatAddr[0] /* dest */
      && enc[8] == fakeWallThermostatAddr[1]
      && enc[9] == fakeWallThermostatAddr[2])
    sendAck(enc);

  return;
}

void RfMoritzClass::task(void)
{
  uint8_t enc[MAX_MORITZ_MSG];
  uint8_t rssi, LQI;

  if(!onState)
    return;
  // see if a CRC OK pkt has been arrived (GDO2 high)
  if(bit_is_set( CC1100_IN_PORT, CC1100_IN_PIN )) {
    //errata #1 does not affect us, because we wait until packet is completely received
    enc[0] = CC1100.cc1100_readReg( CC1100_RXFIFO ) & 0x7f; // read len

    if (enc[0]>=MAX_MORITZ_MSG)
         enc[0] = MAX_MORITZ_MSG-1;

    CC1100_ASSERT;
    CC1100.cc1100_sendbyte( CC1100_READ_BURST | CC1100_RXFIFO );

    for (uint8_t i=0; i<enc[0]; i++) {
         enc[i+1] = CC1100.cc1100_sendbyte( 0 );
    }

    // RSSI is appended to RXFIFO
    rssi = CC1100.cc1100_sendbyte( 0 );

    // And Link quality indicator, too
    LQI = CC1100.cc1100_sendbyte( 0 );

    CC1100_DEASSERT;

    handleAutoAck(enc);

    if (tx_report & REP_BINTIME) {

      DC('z');
      for (uint8_t i=0; i<=enc[0]; i++)
      DC( enc[i] );
    } else {
      DC('Z');
      for (uint8_t i=0; i<=enc[0]; i++)
        DH2( enc[i] );
      if (tx_report & REP_RSSI)
			{
        DH2(rssi);
        //DH2(LQI);
			}
      DNL();
    }

    return;
  }

  if((CC1100.readStatus( CC1100_MARCSTATE ) & 0x1F) == MARCSTATE_RXFIFO_OVERFLOW) {
    CC1100.ccStrobe( CC1100_SFRX  );
    CC1100.ccStrobe( CC1100_SIDLE );
    CC1100.ccStrobe( CC1100_SRX   );
  }
}

void RfMoritzClass::send(char *in)
{
  /* we are not affected by CC1101 errata #6, because MDMCFG2.SYNC_MODE != 0 */
  uint8_t dec[MAX_MORITZ_MSG];

  uint8_t hblen = STRINGFUNC.fromhex(in+1, dec, MAX_MORITZ_MSG-1);

  if ((hblen-1) != dec[0]) {
    //DS_P(PSTR("LENERR\r\n"));
		DS("LENERR\r\n");
    return;
  }
  sendraw(dec, 1);
}

/* longPreamble is necessary for unsolicited messages to wakeup the receiver */
void RfMoritzClass::sendraw(uint8_t *dec, int longPreamble)
{
	uint8_t in_moritz = 1;
  uint8_t hblen = dec[0]+1;
	uint8_t tst[MAX_MORITZ_MSG+1];
	uint8_t res[MAX_MORITZ_MSG+1];
	uint8_t mst[MAX_MORITZ_MSG+1];
  //10kb/s = 10 bit/ms. we send 1 sec preamble + hblen*8 bits
  uint32_t sum = (longPreamble ? 100 : 0) + (hblen*8)/100;
  if (RfSend.credit_10ms < sum) {
    //DS_P(PSTR("LOVF\r\n"));
		DS("LOVF\r\n");
    return;
  }
  RfSend.credit_10ms -= sum;

  // in Moritz mode already?
  if(!onState) {
		// no -> temporary moritz-mode
		in_moritz = 0;
    init();
  }
	
  uint8_t marcstate = (CC1100.readStatus( CC1100_MARCSTATE ) & 0x1F);
  if(marcstate != MARCSTATE_RX) { //error
    DC('Z');
    DC('E');
    DC('R');
    DC('R');
    DC('1');
    DH2(marcstate);
    DNL();
    init();
    return;
  }

  /* We have to keep at least 20 ms of silence between two sends
   * (found out by trial and error). ticks runs at 125 Hz (8 ms per tick),
   * so we wait for 3 ticks.
   * This looks a bit cumbersome but handles overflows of ticks gracefully.
   */
  if(lastSendingTicks)
    while(CLOCKClass::ticks == lastSendingTicks || CLOCKClass::ticks == lastSendingTicks+1)
      MYDELAY.my_delay_ms(1);

  /* Enable TX. Perform calibration first if MCSM0.FS_AUTOCAL=1 (this is the case) (takes 809μs)
   * start sending - CC1101 will send preamble continuously until TXFIFO is filled.
   * The preamble will wake up devices. See http://e2e.ti.com/support/low_power_rf/f/156/t/142864.aspx
   * It will not go into TX mode instantly if channel is not clear (see CCA_MODE), thus ccTX tries multiple times */

  CC1100.ccTX();
	
  marcstate = (CC1100.readStatus( CC1100_MARCSTATE ) & 0x1F);
  if(marcstate != MARCSTATE_TX) { //error
    DC('Z');
    DC('E');
    DC('R');
    DC('R');
    DC('2');
    DH2(marcstate);
    DNL();
    init();
    return;
  }

  if(longPreamble) {
    // Send preamble for 1 sec. Keep in mind that waiting for too long may trigger the watchdog (2 seconds on CUL) 
    for(int i=0;i<10;++i)
      MYDELAY.my_delay_ms(100); //arg is uint_8, so loop
  }

  // send
	uint8_t i=0;
	uint8_t j=0;
	uint8_t loop = 99;

	CC1100_ASSERT;
  //res[j] = CC1100.cc1100_sendbyte(CC1100_SFTX);
	//mst[j] = SPI.transfer(0);
	res[j] = SPI.transfer(CC1100_MARCSTATE|CC1100_READ_BURST);
	mst[j] = SPI.transfer(0);
	while (((mst[j] & 0x1F) != MARCSTATE_TX) && j < hblen && loop--){
	  res[j+1] = SPI.transfer(CC1100_MARCSTATE|CC1100_READ_BURST);
	  mst[j+1] = SPI.transfer(0);
		if (res[j+1] != res[j] || mst[j+1] != mst[j])
			j++;
	}
	tst[hblen] = CC1100.cc1100_sendbyte(0x7F);//CC1100_WRITE_BURST | CC1100_TXFIFO);
  for(i = 0; i < hblen; i++) {
		//tst[i] = CC1100.cc1100_sendbyte( CC1100_TXFIFO|CC1100_WRITE_SINGLE );
		MYDELAY.my_delay_us(50);
    tst[i] = CC1100.cc1100_sendbyte(dec[i]);
  }
	MYDELAY.my_delay_us(50);
  CC1100_DEASSERT;
	
	delayMicroseconds(4);
	CC1100_ASSERT;
	res[++j] = SPI.transfer(CC1100_TXBYTES|CC1100_READ_BURST);
  mst[j] = SPI.transfer(0);
  CC1100_DEASSERT;
  /*
  for( i = 0; i < hblen; i++) {
    Serial.print(dec[i]<0x10 ? "  " : " ");Serial.print(dec[i],HEX);
  }
  Serial.println(" dec");
  for( i = 0; i <= hblen; i++) {
    Serial.print(tst[i]<0x10 ? "  " : " ");Serial.print(tst[i],HEX);
  }
  Serial.println(" tst");
  for( i = 0; i <=j; i++) {
    Serial.print(res[i]<0x10 ? "  " : " ");Serial.print(res[i],HEX);
  }
  Serial.println(" res");
  for( i = 0; i <=j; i++) {
    Serial.print(mst[i]<0x10 ? "  " : " ");Serial.print(mst[i],HEX);
  }
  Serial.println(" mst");
	*/
  // * only debug? /Wait for sending to finish (CC1101 will go to RX state automatically
  //after sending

  for(i=0; i< 200;++i) {
    marcstate = (CC1100.readStatus( CC1100_MARCSTATE ) & 0x1F);
    if( marcstate == MARCSTATE_RX)
      break; //now in RX, good
    //if( CC1100.cc1100_readReg( CC1100_MARCSTATE ) != MARCSTATE_TX)
    //  break; //neither in RX nor TX, probably some error
    MYDELAY.my_delay_us(100);
  }

  if(marcstate != MARCSTATE_RX) { //error
    DC('Z');
    DC('E');
    DC('R');
    DC('R');
    DC('3');
    DH2(marcstate);
    DNL();
    init();
  }
  // only debug? */
  if(!in_moritz) {
    RfReceive.set_txrestore();
  }
  lastSendingTicks = CLOCKClass::ticks;
}

void RfMoritzClass::sendAck(uint8_t* enc)
{
  uint8_t ackPacket[12];
  ackPacket[0] = 11; /* len*/
  ackPacket[1] = enc[1]; /* msgcnt */
  ackPacket[2] = 0; /* flag */
  ackPacket[3] = 2; /* type = Ack */
  for(int i=0;i<3;++i) /* src = enc_dst*/
    ackPacket[4+i] = enc[7+i];
  for(int i=0;i<3;++i) /* dst = enc_src */
    ackPacket[7+i] = enc[4+i];
  ackPacket[10] = 0; /* groupid */
  ackPacket[11] = 0; /* payload */

  MYDELAY.my_delay_ms(20); /* by experiments */

  sendraw(ackPacket, 0);

  //Inform FHEM that we send an autoack
  DC('Z');
  for (uint8_t i=0; i < ackPacket[0]+1; i++)
    DH2( ackPacket[i] );
  if (tx_report & REP_RSSI)
    DH2( 0 ); //fake some rssi
  DNL();
}

void RfMoritzClass::func(char *in)
{
  if(in[1] == 'r') {                // Reception on
    init();

  } else if(in[1] == 's' || in[1] == 'f' ) {         // Send/Send fast
    uint8_t dec[MAX_MORITZ_MSG];
    uint8_t hblen = STRINGFUNC.fromhex(in+2, dec, MAX_MORITZ_MSG-1);
    if ((hblen-1) != dec[0]) {
      //DS_P(PSTR("LENERR\r\n"));
		  DS("LENERR\r\n");
      return;
    }
    sendraw(dec, in[1] == 's');

  } else if(in[1] == 'a') {         // Auto-Ack
    STRINGFUNC.fromhex(in+2, autoAckAddr, 3);

  } else if(in[1] == 'w') {         // Fake Wall-Thermostat
    STRINGFUNC.fromhex(in+2, fakeWallThermostatAddr, 3);

  } else {                          // Off
    on(0);
  }
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RF_MORITZ)
RfMoritzClass Moritz;
#endif

