#include <stdint.h>
#include <stdbool.h>

#ifndef __INCL_THREADS
#define __INCL_THREADS

void        startThreads(void);
void        stopThreads(void);
void *      NRF_listen_thread(void * pParms);
void *      db_update_thread(void * pParms);

#endif