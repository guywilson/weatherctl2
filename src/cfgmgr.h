#include <stdbool.h>

#ifndef __INCL_CFGMGR
#define __INCL_CFGMGR

struct _cfg_handle_t;
typedef struct _cfg_handle_t            cfg_handle_t;

cfg_handle_t *  cfgGetHandle(void);
int             cfgOpen(const char * pszConfigFileName);
void            cfgClose(cfg_handle_t * hcfg);
const char *    cfgGetValue(cfg_handle_t * hcfg, const char * key);
bool            cfgGetValueAsBoolean(cfg_handle_t * hcfg, const char * key);
int             cfgGetValueAsInteger(cfg_handle_t * hcfg, const char * key);
int32_t         cfgGetValueAsLongInteger(cfg_handle_t * hcfg, const char * key);
uint32_t        cfgGetValueAsLongUnsigned(cfg_handle_t * hcfg, const char * key);
void            cfgDumpConfig(cfg_handle_t * hcfg);
#endif
