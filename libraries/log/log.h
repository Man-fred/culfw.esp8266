#ifndef _LOG_H
#define _LOG_H

#include "qfs.h"

#define LOG_NETTOLINELEN     32
#define LOG_TIMELEN          13 // 0314 09:00:00
#define LOG_LINELEN          (LOG_TIMELEN+1+LOG_NETTOLINELEN+1)
#define LOG_NRFILES           4

class LogClass {
	public:
	  LogClass(): syslog("Syslog.0"), logfd( 0xffff) {}
		void Log(char *);
		void display(char *);
		void init(void);
	private:
		void rotate(void);
		fs_inode_t logfd;
    uint16_t logoffset;
    char syslog[8];
};

extern LogClass Log; 
#endif
