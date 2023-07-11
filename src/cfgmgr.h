#include <stdbool.h>

#ifndef __INCL_CFGMGR
#define __INCL_CFGMGR

struct _cfg_handle_t;
typedef struct _cfg_handle_t            cfg_handle_t;

int             cfgOpen(const char * pszConfigFileName);
void            cfgClose(void);
const char *    cfgGetValue(const char * key);
bool            cfgGetValueAsBoolean(const char * key);
int             cfgGetValueAsInteger(const char * key);
int32_t         cfgGetValueAsLongInteger(const char * key);
uint32_t        cfgGetValueAsLongUnsigned(const char * key);
void            cfgDumpConfig(void);
#endif
