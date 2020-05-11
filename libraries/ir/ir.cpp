/* Copyright DHS-Computertechnik GmbH, 
   Olaf Droegehorn, 2011.
   Released under the GPL Licence, Version 2
*/

#include "ir.h"

void IrClass::init( void ) {
	#if defined (HAS_IRRX) && !defined (ESP8266) 
	  irmp_init();  // initialize rc5
		 
	  ir_mode = 0;
	  #ifdef HAS_IR_POWER
		IRMP_POWER_DDR |= (1 << IRMP_POWER_BIT); // IRMP_POWER_DDR & BIT als Ausgang festlegen
		IRMP_POWER_PORT |= (1 << IRMP_POWER_BIT); // IRMP_POWER_PORT & BIT aktivieren 
	  #endif
		ir_internal = 1;
	#elif defined (HAS_IRRX) && defined (ESP8266)
	  ir_mode = 0;
	  irrecv.enableIRIn();  // Start the receiver
		ir_internal = 1;
	#endif 
	
	#if defined (HAS_IRTX) && !defined (ESP8266) 
	  irsnd_init();
	#elif defined (HAS_IRTX) && defined (ESP8266)
	  irsend.begin();
	#endif
}

void IrClass::task( void ) {
  #if defined (HAS_IRRX) && !defined (ESP8266) 
		IRMP_DATA irmp_data;

	  if (!ir_mode)
		  return;
		 
	  if (!irmp_get_data(&irmp_data))
			return;
		 
	  if ((ir_mode == 2) && (irmp_data.flags & IRMP_FLAG_REPETITION))
		  return;
		 
	  LED_ON();
	  DC('I');
	  DH(irmp_data.protocol, 2);
	  DH(irmp_data.address, 4);
	  DH(irmp_data.command, 4);
	  DH(irmp_data.flags, 2);
	  DNL();
	  LED_OFF();
	#elif defined (HAS_IRRX) && defined (ESP8266)
	  if (ir_mode && irrecv.decode(&results)) {
			if (ir_mode && ((ir_mode != 2) || (results.repeat == 0) ) ) {			
				//LED_ON();
				DC('I');
				DH(results.decode_type, 2);
				DH(results.value >> 16, 4);
				DH(results.value & 0xFFFF, 4);
				DH(results.repeat, 2);
				// print() & println() can't handle printing long longs. (uint64_t)
				Serial.print(" ");
				serialPrintUint64(results.value, HEX);
				Serial.print(" received");
				DNL();
			}
		  irrecv.resume();  // Receive the next value
	    //LED_OFF();
	  }
	#endif
}

void IrClass::sample( void ) {
  if (!ir_mode)
	  return;
  #ifndef ESP8266
    (void) irmp_ISR();  // call irmp ISR
	#endif
}

uint8_t IrClass::send_data (void) {
  #ifndef ESP8266
	  return irsnd_ISR();
	#endif
}

void IrClass::func(char *in) {
  #ifndef ESP8266
    IRMP_DATA irmp_data;
  #endif   	 
	uint8_t higha = 0, lowa = 0, highc = 0, lowc = 0, protocol = 0, flags  = 0;
	uint16_t address = 0, command = 0;
	 
	switch (in[1]) {
		case 'r': // receive
		if (in[2] && in[3]) {
					STRINGFUNC.fromhex(in+2, &ir_mode, 2);
			}
			DH(ir_mode,2);
			DNL();
		break;

		case 's': // send
			#ifdef HAS_IRTX
				STRINGFUNC.fromhex(in+2, &protocol, 1);
				STRINGFUNC.fromhex(in+4, &higha, 1);
				STRINGFUNC.fromhex(in+6, &lowa, 1);
				STRINGFUNC.fromhex(in+8, &highc, 1);
				STRINGFUNC.fromhex(in+10, &lowc, 1);
				STRINGFUNC.fromhex(in+12, &flags, 1);
				#if !defined (ESP8266) 
  				address = higha*256+lowa;
				  command = highc*256+lowc;
					irmp_data.protocol = protocol;
					irmp_data.address  = address;
					irmp_data.command  = command;
					irmp_data.flags    = (flags > 0);
				
					irsnd_send_data (&irmp_data, TRUE);
				#else
				  decode_results data;
          data.decode_type = (decode_type_t)protocol;
					data.value = (((higha * 256) + lowa) * 256 + highc) * 256 + lowc;
					data.repeat = flags;
					//DH2(protocol);DH2(higha);DH2(lowa);DH2(highc);DH2(lowc);DH2(flags);DNL();
					serialPrintUint64(data.value, HEX);
					Serial.println(" sent");
					//irsend.send((decode_type_t)protocol, (uint64_t)(address << 16 & command), irsend.defaultBits((decode_type_t)protocol), irsend.minRepeats((decode_type_t)protocol));
					irsend.send(data.decode_type, data.value, irsend.defaultBits(data.decode_type), irsend.minRepeats(data.decode_type));
				#endif	  
			#endif //HAS_IRTX
		break;
		
		case 'i': // Internal Receiver on/off
			#if !defined (ESP8266)
				#ifdef _HAS_IR_POWER
					if (ir_internal) {
						IRMP_POWER_PORT &= ~(1 << IRMP_POWER_BIT); // IRMP_POWER_PORT & BIT deaktivieren 
						ir_internal = 0;
					} else {
						IRMP_POWER_PORT |= (1 << IRMP_POWER_BIT); // IRMP_POWER_PORT & BIT  aktivieren 
						ir_internal = 1;
					}
				#endif
			#else
				if (ir_internal) {
					irrecv.disableIRIn();  // Start the receiver 
					ir_internal = 0;
				} else {
					irrecv.enableIRIn();  // Start the receiver 
					ir_internal = 1;
				}
			#endif //ESP8266
			DH(ir_internal,2);
			DNL();  		
		break;
	}

}
#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_Ir)
IrClass IR;
#endif
