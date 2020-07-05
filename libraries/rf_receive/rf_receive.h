#ifndef _RF_RECEIVE_H
#define _RF_RECEIVE_H

#include <stdint.h>

#define TYPE_EM      'E'
#define TYPE_HMS     'H'
#define TYPE_FHT     'T'
#define TYPE_FS20    'F'
#define TYPE_KS300   'K'
#define TYPE_HRM     'R'        // Hoermann
#define TYPE_ESA     'S'
#define TYPE_TX3     't'
#define TYPE_TCM97001 's'

#define TYPE_REVOLT	 'r'
#define TYPE_IT  	   'i'
#define TYPE_FTZ     'Z'
                            // X21 X67 X3F X35
#define REP_KNOWN    _BV(0) //  x   x   x   x
#define REP_REPEATED _BV(1) //      x   x
#define REP_BITS     _BV(2) //      x   x  
#define REP_MONITOR  _BV(3) //          x   x
#define REP_BINTIME  _BV(4) //          x   x
#define REP_RSSI     _BV(5) //  x   x   x   x
#define REP_FHTPROTO _BV(6) //      x 
#define REP_LCDMON   _BV(7)

#define RF_DEBUG

#ifdef ESP8266
#  define TWRAP		100000
#else
#  define TWRAP		20000
#endif

#ifndef REPTIME
#  define REPTIME      38
#endif

/* public prototypes */
#ifdef RF_DEBUG
#  define MAXBIT 500               // for debugging timing high/low
#  define MAXMSG 100
#else
	#ifdef HAS_ESA
	#  define MAXMSG 20               // ESA messages
	#else
	//esp8266#  define MAXMSG 12               // EMEM messages
	#  define MAXMSG 20               // EMEM messages
	#endif
#endif

#ifdef HAS_IT
#  ifndef LONG_PULSE
#    define LONG_PULSE
#  endif
#endif

#ifdef HAS_REVOLT
#  ifndef LONG_PULSE
#    define LONG_PULSE
#  endif
#endif

class RfReceiveClass {
public:
	RfReceiveClass(): debugLast(0), debugNext(0) {};
	void set_txreport(char *in);
	void set_txrestore(void);
	void tx_init(void);
	uint8_t rf_isreceiving(void);
	uint8_t cksum1(uint8_t s, uint8_t *buf, uint8_t len);
	uint8_t cksum2(uint8_t *buf, uint8_t len);
	uint8_t cksum3(uint8_t *buf, uint8_t len);

	void RfAnalyze_Task(void);
	void IsrHandler();
	void IsrTimer1(void);
private:
	typedef struct  {
	  uint8_t *data;
	  uint8_t byte, bit;
	} input_t;

	typedef struct {
	  uint8_t hightime, lowtime;
	} wave_t;

	// This struct has the bits for receive check
	struct {
	   uint8_t isrep:1; // 1 Bit for is repeated
	   uint8_t isnotrep:1; // 1 Bit for is repeated value
	   uint8_t packageOK:1; // Received packet is ok
	   // 5 bits free
	} packetCheckValues;

	// One bucket to collect the "raw" bits
	typedef struct {
	  uint8_t state, byteidx, sync, bitidx; 
	  uint8_t data[MAXMSG];         // contains parity and checksum, but no sync
	  wave_t zero, one; 
		#ifdef RF_DEBUG
				uint8_t bithigh[MAXBIT]; // debug timing
				uint8_t bitlow[MAXBIT]; // debug timing
				uint8_t bitused;
		#endif
		#ifdef HAS_FLAMINGO
				uint8_t bit2;
				uint8_t bitsaved;
		//	  uint8_t bitrepeated;
		#endif
	} bucket_t;
    bucket_t bucket_array[RCV_BUCKETS];

	uint8_t bucket_in;                 // Pointer to the in(terrupt) queue
	uint8_t bucket_out;                // Pointer to the out (analyze) queue
	uint8_t bucket_nrused;             // Number of unprocessed buckets
	uint8_t oby, obuf[MAXMSG], nibble; // parity-stripped output
	uint8_t roby, robuf[MAXMSG];       // for Repeat check: buffer and time
	uint32_t reptime;
	#ifdef LONG_PULSE
		uint16_t hightime, lowtime;
	#else
		uint8_t hightime, lowtime;
	#endif

	uint32_t silence;
	uint32_t overflow;
	uint32_t pulseTooShort;
	uint32_t shortMax;
	uint32_t longMin;
	uint32_t pulseTooLong;
	uint8_t debugLast;
	uint8_t debugNext;

	void addbit(bucket_t *b, uint8_t bit);
	void delbit(bucket_t *b);
	uint8_t getbit(input_t *in);
	uint8_t getbits(input_t* in, uint8_t nbits, uint8_t msb);

	uint8_t wave_equals(wave_t *a, uint8_t htime, uint8_t ltime, uint8_t state);
	uint8_t analyze(bucket_t *b, uint8_t t);
	uint8_t analyze_hms(bucket_t *b);
#ifdef HAS_ESA
	uint8_t analyze_esa(bucket_t *b);
#endif
#ifdef HAS_TX3
	uint8_t analyze_TX3(bucket_t *b);
#endif
#ifdef HAS_IT
	uint8_t analyze_it(bucket_t *b);
	uint8_t wave_equals_itV3(uint8_t htime, uint8_t ltime);
#endif
#ifdef HAS_FLAMINGO
	uint8_t analyze_flamingo(bucket_t *b);
#endif
#ifdef HAS_TCM97001
    uint8_t analyze_tcm97001(bucket_t *b);
#endif
#ifdef HAS_REVOLT
	uint8_t analyze_revolt(bucket_t *b);
#endif
#ifdef HAS_FTZ
	uint8_t analyze_ftz(bucket_t *b);
#endif
	void checkForRepeatedPackage(uint8_t *datatype, bucket_t *b);
	void reset_input(void);
	uint8_t makeavg(uint8_t i, uint8_t j);
	uint8_t check_rf_sync(uint8_t l, uint8_t s);
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RF_RECEIVE)
extern RfReceiveClass RfReceive;
#endif

extern uint8_t tx_report;


#endif
