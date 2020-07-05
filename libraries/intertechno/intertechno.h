#ifndef _InterTechno_H
#define _InterTechno_H

class InterTechnoClass {
/* public prototypes */
public:
  void func(char *in);
  void FlamingoDecrypt(uint8_t *in);
private:

  void tunein(void);
  void send_bit(uint8_t bit);
  void send_start_V3(void);
  void send_stop_V3(void) ;
  void send_bit_V3(uint8_t bit);
  void send_bit_F(uint8_t bit);
  void send (char *in);
  void FlamingoEncrypt(uint8_t *transmitterId);
  byte intertek;
  
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_INTERTECHNO)
extern InterTechnoClass InterTechno;
#endif

#endif
