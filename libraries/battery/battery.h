#ifndef _BATTERY_H
#define _BATTERY_H

class BatClass {
	public:
		void func(char *unused);
		void init(void);
		void drawstate(void);
    
		uint8_t state;
	private:
	  void raw2percent(uint16_t bv);
};

extern BatClass Bat; 

#endif
