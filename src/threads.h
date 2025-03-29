#include <stdint.h>
#include <stdbool.h>

#include "posixthread.h"
#include "packet.h"

#ifndef __INCL_THREADS
#define __INCL_THREADS

class NRFListenThread : public PosixThread {
    private:
        weather_transform_t * transformWeatherPacket(weather_packet_t * weatherPacket);
        
    public:
        NRFListenThread() : PosixThread() {}

        void * run();
};

class DBUpdateThread : public PosixThread {
    public:
        DBUpdateThread() : PosixThread() {}

        void * run();
};

class WoWUpdateThread : public PosixThread {
    public:
        WoWUpdateThread() : PosixThread() {}

        void * run();
};

class ThreadManager {
    public:
        static ThreadManager & getInstance() {
            static ThreadManager instance;
            return instance;
        }

    private:
        ThreadManager() {}

        NRFListenThread nrfListenThread;
        DBUpdateThread dbUpdateThread;
        WoWUpdateThread wowUpdateThread;

    public:
        void start();
        void kill();
};

#endif
