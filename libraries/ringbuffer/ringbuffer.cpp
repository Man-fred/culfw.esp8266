//#define TESTING
#ifdef TESTING
#define TTY_BUFSIZE 64
typedef unsigned char uint8_t;
#include <stdio.h>
#endif

#include "ringbuffer.h"

RingbufferClass::RingbufferClass() {
	reset();
}

void RingbufferClass::reset()
{
  getoff = putoff = nbytes = 0;
}

void RingbufferClass::put(uint8_t data)
{
//esp8266    uint8_t sreg;
//esp8266  sreg = SREG;
//esp8266    cli();
  if(nbytes >= TTY_BUFSIZE) {
//esp8266      SREG = sreg;
    return;
  }
  nbytes++;
  buf[putoff++] = data;
  if(putoff == TTY_BUFSIZE)
    putoff = 0;
//esp8266    SREG = sreg;
}

uint8_t RingbufferClass::get()
{
//esp8266    uint8_t sreg;
  uint8_t ret;
//esp8266    sreg = SREG;
//esp8266    cli();
  if(nbytes == 0) {
//esp8266      SREG = sreg;
    return 0;
  }
  nbytes--;
  ret = buf[getoff++];
  if(getoff == TTY_BUFSIZE)
    getoff = 0;
//esp8266    SREG = sreg;
  return ret;
}

uint8_t RingbufferClass::getNbytes()
{
  return nbytes;
}

#ifdef HAS_RF_ROUTER
/////////////////////////////////////////
// If there are 2 FHT commands in the buffer, delete rssif and separator from
// the first one, and prefix, address and initiator from the second. i.e from
// the second commands 14 bytes 10 are dropped.
void RingbufferClass::FHT_compress()
{
  uint8_t *bp1 = (uint8_t *)buf;
  if(bp1[0] != 'T')
    return;

  uint8_t off = 13;
  while(off < putoff && bp1[off] != ';')
    off++;

  if(off != putoff-15)       // expect a "normal" FHT message (13+1 bytes)
    return;

  uint8_t *bp2 = bp1+off+1;
  if(bp2[0] != 'T' ||            // compress only messages from the same FHT
     bp1[1]!=bp2[1] || bp1[2]!=bp2[2] || bp1[3]!=bp2[3] || bp1[4]!=bp2[4])
    return;

  bp1 += (off-2);
  bp2 += 5;
  *bp1++ = *bp2++;          // cmd
  *bp1++ = *bp2; bp2 += 3;  // cmd
  *bp1++ = *bp2++;          // val
  *bp1++ = *bp2++;          // val
  *bp1++ = *bp2++;          // rssi
  *bp1++ = *bp2++;          // rssi
  *bp1++ = *bp2;            // ;
  putoff -= 10;
  nbytes -= 10;
}
#endif

#ifdef TESTING
void RingbufferClass::d_s(char *s)
{
  while(*s)
    put(*s++);
}

void RingbufferClass::p()
{
  uint8_t c;
  while((c = get())) {
    putchar(c);
  }
  putchar('\n');
}


int RingbufferClass::main(int ac, char **av)
{
  reset();
  //d_s(&buffer, "T40484269E72E;T40484369001F;");
  d_s("T40484269E743001F;T404847690118;");
  FHT_compress(&buffer);
  p();
}
#endif

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RINGBUFFER)
//extern RingbufferClass Ringbuffer;
RingbufferClass RFR_Buffer;
#endif
