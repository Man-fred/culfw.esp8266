#ifndef __CFS_H
#define __CFS_H

#include "qfs.h"

extern fs_t fs;

class FswClass {
	public:
		void read_file(char *in);
		void write_file(char *in);
		void test_file(char *in);
};

extern FswClass Fsw;

#endif
