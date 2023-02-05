#include "posixthread.h"

#ifndef __INCL_THREADS
#define __INCL_THREADS

class RadioRxThread : public PosixThread
{
public:
    RadioRxThread() : PosixThread(true) {}

    void * run();
};

#endif
