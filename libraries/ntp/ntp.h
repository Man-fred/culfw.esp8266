#ifndef __NTP_H
#define __NTP_H

typedef uint32_t time_t;

typedef union {
  uint32_t u32;
  uint16_t u16[2];
  uint8_t  u8[4];
} ts_32_t;

typedef union {
  uint32_t u32[2];
  uint16_t u16[4];
  uint8_t  u8[8];
} ts_64_t;

typedef struct {
  uint8_t    li_vn_mode;          /* leap indicator, version and mode */
  uint8_t    stratum;
  uint8_t    poll;
  int8_t     precision;
  ts_32_t    delay;
  ts_32_t    dispersion;
  ts_32_t    refid;
  ts_64_t    ref_ts;
  ts_64_t    org_ts;
  ts_64_t    rcv_ts;
  ts_64_t    tx_ts;
} ntp_packet_t;

typedef struct tm {
  uint8_t    tm_year;
  uint8_t    tm_mon;
  uint8_t    tm_mday;
  uint8_t    tm_hour;
  uint8_t    tm_min;
  uint8_t    tm_sec;
} tm_t;

#define NTP_PORT 123
#define NTP_INTERVAL 8            // Max interval is 9 (512sec)
#define NTP_INTERVAL_MASK 0xff    // 264sec = 4.5min

class NtpClass {
public:

	void init(void);
	void digestpacket(void);
	void func(char *in);
	void get(uint8_t now[6]);
	void sec2tm(time_t sec, tm_t *t);
	time_t tm2sec(tm_t *t);

private:
	// Time of last sync.
	time_t  sec = 3461476149U; // 2009-09-09 09:09:09 (GMT)
	uint8_t hsec;
	int8_t  gmtoff;
	struct uip_udp_conn *conn = 0;
	
	void fill_packet(ntp_packet_t *p);
	void sendpacket(void);
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_FASTRF)
extern NtpClass Ntp;
#endif

#endif
