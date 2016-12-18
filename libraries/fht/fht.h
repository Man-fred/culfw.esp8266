#ifndef __FHT_H
#define __FHT_H

#define FHT_ACTUATOR   0x00
#define FHT_ACK        0x4B
#define FHT_CAN_XMIT   0x53
#define FHT_CAN_RCV    0x54
#define FHT_ACK2       0x69
#define FHT_START_XMIT 0x7D
#define FHT_END_XMIT   0x7E
#define FHT_MINUTE     0x64
#define FHT_HOUR       0x63

#define FHT_DATA       0xff
#define FHT_FOREIGN    0xff

#define FHT_TIMER_DISABLED   0xff

#define FHT_CSUM_START   12

#define FHT_8V_NUM         8    // Needs 2 byte per 8v. For sync must by <14
#define FHT_8V_DISABLED 0xff

#define FHT_TF_START	0x69    // first byte of housecode starts above 0x69
#define FHT_TF_DATA        4  // 4 bytes per FHT 80 TF (HH HH AA BB)
#define FHT_TF_NUM         4  // supported window FHT 80 TF
#define FHT_TF_DISABLED 0xff  // TF not used
#ifdef HAS_FHT_8v
#  define FHT8V_CMD_SET  0x26
#  define FHT8V_CMD_SYNC 0x2c
#  define FHT8V_CMD_PAIR 0x2f
#endif

class FHTClass {
public:
	// Our housecode. The first byte for 80b communication, both together for
	// direct 8v controlling
	uint8_t fht_hc0, fht_hc1;

	void fhtsend(char *in);
	void fht_hook(uint8_t *in);
	void fht_init(void);
#ifdef HAS_FHT_80b
	void fht80b_timer(void);
    uint8_t fht80b_timeout;
    uint8_t fht80b_minute;
    uint8_t fht80b_state;    // 80b state machine
#endif
#ifdef HAS_FHT_TF
	void fht_tf_timer(uint8_t ind); // fht TF timer method
    // used in clock.c -> keeps the timeout for each TF
    int16_t fht_tf_timeout_Array[3 * FHT_TF_NUM]; // timeout,change,count
#endif
#ifdef HAS_FHT_8v
	void fht8v_timer(void);
	uint16_t fht8v_timeout;
#endif

private:
	void fht_display_buf(uint8_t ptr[]);
#ifdef HAS_FHT_80b
	uint8_t fht80b_ldata;    // last data waiting for ack
	uint8_t fht80b_out[6];   // Last sent packet. Reserve 1 byte for checksum
	uint8_t fht80b_repeatcnt;
	uint8_t fht80b_buf[FHTBUF_SIZE];
	uint8_t fht80b_bufoff;   // offset in the current fht80b_buf

	void    fht80b_sendpacket(void);
	void    fht80b_send_repeated(void);
	void    fht80b_print(uint8_t level);
	void    fht80b_initbuf(void);
	void    fht_delbuf(uint8_t *buf);
	uint8_t fht_addbuf(char *in);
	uint8_t fht_getbuf(uint8_t *buf);
	uint8_t* fht_lookbuf(uint8_t *buf);
	void    fht80b_reset_state(void);
	uint8_t fht_bufspace(void);
#endif 
#ifdef HAS_FHT_8v
	uint8_t fht8v_buf[2*FHT_8V_NUM];
	uint8_t fht8v_ctsync;
#endif
#ifdef HAS_FHT_TF
	void fht_tf_send(uint8_t index); // send date from TF at given index
	uint8_t send_tk_out[5];  // send raw data last byte is resv for CC
    uint8_t fht_tf_buf[FHT_TF_DATA * FHT_TF_NUM]; //current 4 window sensors
    uint8_t fht_tf_deactivated; // if tf not used -> value is 1
#endif
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_RF_RECEIVE)
	extern FHTClass FHT;
#endif

#endif
