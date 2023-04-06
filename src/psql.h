#include <stdint.h>
#include <stdbool.h>

#include <postgresql/libpq-fe.h>

#ifndef __INCL_PSQL
#define __INCL_PSQL

PGconn *    dbConnect(const char * host, int port, const char * database, const char * username, const char * password);
void        dbFinish(PGconn * connection);
int         dbTransactionBegin(PGconn * connection);
int         dbTransactionEnd(PGconn * connection);
PGresult *  dbExecute(PGconn * connection, const char * sql);

#endif
