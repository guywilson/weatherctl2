#include "posixthread.h"

#ifndef __INCL_THREADS
#define __INCL_THREADS

class RadioRxThread : public PosixThread
{
public:
    RadioRxThread() : PosixThread(false) {}

    void * run();
};

#endif
