#ifndef _FREEMEM_H
#define _FREEMEM_H

class MemoryClass {
public:
  uint16_t freeMem(void);
  void getfreemem(char *unused);
  void testmem(char *unused);
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_MEMORY)
extern MemoryClass Memory;
#endif

#endif
