#ifndef IR_H
#define IR_H

class IrClass {
public:
	void init( void );
	void func(char *in);
	void task( void );
	void sample( void );
	uint8_t send_data (void);
private:
	uint8_t ir_mode = 0;
	uint8_t ir_internal = 1;
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_Ir)
extern IrClass IR;
#endif

#endif
