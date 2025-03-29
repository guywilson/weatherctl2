#include <unistd.h>

#include "logger.h"

#ifndef _INCL_POSIXTHREAD
#define _INCL_POSIXTHREAD

class thread_error : public exception {
    private:
        string message;
        static const int MESSAGE_BUFFER_LEN = 4096;

    public:
        const char * getTitle() {
            return "CFG Error: ";
        }

        thread_error() {
            this->message.assign(getTitle());
        }

        thread_error(const char * msg) : thread_error() {
            this->message.append(msg);
        }

        thread_error(const char * msg, const char * file, int line) : thread_error() {
            char lineNumBuf[8];

            snprintf(lineNumBuf, 8, ":%d", line);

            this->message.append(msg);
            this->message.append(" at ");
            this->message.append(file);
            this->message.append(lineNumBuf);
        }

        virtual const char * what() const noexcept {
            return this->message.c_str();
        }

        static char * buildMsg(const char * fmt, ...) {
            va_list     args;
            char *      buffer;

            buffer = (char *)malloc(MESSAGE_BUFFER_LEN);
            
            va_start(args, fmt);
            vsnprintf(buffer, MESSAGE_BUFFER_LEN, fmt, args);
            va_end(args);

            return buffer;
        }
};

class PosixThread {
    private:
        pthread_t tid;
        void * threadParameters = NULL;

        logger & log = logger::getInstance();

    protected:
        virtual void * getThreadParameters() {
            return this->threadParameters;
        }

    public:
        bool isRestartable = true;

        PosixThread() {}
        ~PosixThread() {}

        static void sleep_us(unsigned long t) {
            usleep(t);
        }

        static void sleep_ms(unsigned long t) {
            usleep(t * 1000UL);
        }

        static void sleep(unsigned long t) {
            ::sleep(t);
        }

        static void sleep_mn(unsigned long t) {
            ::sleep(t * 60UL);
        }

        static void sleep_hr(unsigned long t) {
            ::sleep(t * 3600UL);
        }

        virtual bool start();
        virtual bool start(void * p);

        virtual void stop();

        virtual pthread_t getID() {
            return this->tid;
        }

        virtual void * run() = 0;
};

#endif
