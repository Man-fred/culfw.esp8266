#ifndef _InterTechno_H
#define _InterTechno_H

class IntertechnoClass{
  public:
		/* public prototypes */
		IntertechnoClass(): it_interval(420), it_interval_v3(260), it_repetition(6), on(0) {};
		void func(char *in);
    uint8_t on;

	private:
	  void tunein(void);
		void send_bit(uint8_t bit);
		void send_start_V3(void);
		void send_stop_V3(void);
		void send_bit_V3(uint8_t bit);
		void send (char *in);
		
		uint16_t it_interval;// = 420;
		uint16_t it_interval_v3;// = 260;
		uint16_t it_repetition;// = 6;
		uint8_t restore_asksin;
		uint8_t restore_moritz;
		unsigned char it_frequency[3] = {0x10, 0xb0, 0x71};
};

extern IntertechnoClass it;

#endif
