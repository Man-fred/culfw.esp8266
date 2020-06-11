#ifndef __STRINGFUNC_H
#define __STRINGFUNC_H

#include <stdint.h>

class STRINGFUNCClass {
public:
	int fromhex(const char *in, uint8_t *out, uint8_t outlen);
	int fromip(const char *in, uint8_t *out, uint8_t outlen);
	void fromdec(const char *in, uint8_t *out);
  void fromchars(const char *in, uint8_t *out, uint8_t max_length);
	void tohex(uint8_t in, uint8_t *out);
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_STRINGFUNC)
extern STRINGFUNCClass STRINGFUNC;
#endif

#endif
