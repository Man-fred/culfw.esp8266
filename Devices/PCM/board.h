#ifndef _PCMBOARD_H
#define _PCMBOARD_H

#include "../SCC/board.h"

#undef HAS_STACKING


#undef CC1100_IN_DDR
#undef CC1100_IN_PORT
#undef CC1100_IN_PIN
#undef CC1100_IN_IN
#undef CC1100_INT
#undef CC1100_INTVECT
#undef CC1100_ISC
#undef CC1100_EICR

#define CC1100_IN_DDR		DDRD
#define CC1100_IN_PORT          PIND
#define CC1100_IN_PIN           2
#define CC1100_IN_IN            PINB
#define CC1100_INT		INT0
#define CC1100_INTVECT          INT0_vect
#define CC1100_ISC		ISC00
#define CC1100_EICR             EICRA

#undef LED_DDR
#undef LED_PORT
#undef LED_PIN
#define LED_DDR                 DDRD
#define LED_PORT                PORTD
#define LED_PIN                 4

#define MULTI_FREQ_DEVICE       // available in multiple versions: 433MHz,868MHz,915MHz

#undef MARK433_PORT
#undef MARK433_PIN
#undef MARK433_BIT

#define MARK433_PORT            PORTC
#define MARK433_PIN             PINC    
#define MARK433_BIT             4

#undef MARK915_PORT
#undef MARK915_PIN
#undef MARK915_BIT

#define MARK915_PORT            PORTC
#define MARK915_PIN             PINC    
#define MARK915_BIT             5

#define BOARD_ID_STR915         "CSM915"



#endif

