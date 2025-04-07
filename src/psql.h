#include <string>
#include <exception>

#include <stdint.h>
#include <stdbool.h>

#include <postgresql/libpq-fe.h>

#include "logger.h"

using namespace std;

#ifndef __INCL_PSQL
#define __INCL_PSQL

class psql_error : public exception {
    private:
        string message;
        static const int MESSAGE_BUFFER_LEN = 4096;

    public:
        const char * getTitle() {
            return "LOG Error: ";
        }

        psql_error() {
            this->message.assign(getTitle());
        }

        psql_error(const char * msg) : psql_error() {
            this->message.append(msg);
        }

        psql_error(const char * msg, const char * file, int line) : psql_error() {
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

class psqlConnection {
    private:
        PGconn * connection;
        logger & log = logger::getInstance();

    public:
        psqlConnection(const string & host, int port, const string & database, const string & username, const string & password);
        ~psqlConnection();

        void beginTransaction();
        void endTransaction();

        PGresult * execute(const char * sql);
};

#endif
