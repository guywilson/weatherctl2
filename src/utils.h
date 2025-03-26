#include <stdint.h>

#ifndef __INCL_UTILS
#define __INCL_UTILS

int         strHexDump(char * pszBuffer, int strBufferLen, void * buffer, uint32_t bufferLen);
void        hexDump(void * buffer, uint32_t bufferLen);
void        daemonise(void);

#endif
