#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <postgresql@14/libpq-fe.h>

#include "logger.h"
#include "psql.h"

PGconn * dbConnect(const char * host, int port, const char * database, const char * username, const char * password) {
    char            szConnection[256];
    PGconn *        connection;

    sprintf(
        szConnection, 
        "host=%s port=%d dbname=%s user=%s password=%s", 
        host,
        port,
        database,
        username,
        password);

    connection = PQconnectdb(szConnection);

    if (PQstatus(connection) != CONNECTION_OK) {
        lgLogError(lgGetHandle(), "Could not connect to database: %s:%s", database, PQerrorMessage(connection));

        return NULL;
    }

    return connection;
}

void dbFinish(PGconn * connection) {
    PQfinish(connection);
}

int dbTransactionBegin(PGconn * connection) {
	PGresult *			queryResult;
    
    queryResult = PQexec(connection, "BEGIN");

    if (PQresultStatus(queryResult) != PGRES_COMMAND_OK) {
        lgLogError(lgGetHandle(), "Error beginning transaction [%s]", PQerrorMessage(connection));
        PQclear(queryResult);
        
        return -1;
    }

    lgLogDebug(lgGetHandle(), "Transaction - Open");

    PQclear(queryResult);

    return 0;
}

int dbTransactionEnd(PGconn * connection) {
	PGresult *			queryResult;
    
    queryResult = PQexec(connection, "END");

    if (PQresultStatus(queryResult) != PGRES_COMMAND_OK) {
        lgLogError(lgGetHandle(), "Error ending transaction [%s]", PQerrorMessage(connection));
        PQclear(queryResult);
        
        return -1;
    }

    lgLogDebug(lgGetHandle(), "Transaction - Closed");

    PQclear(queryResult);

    return 0;
}

PGresult * dbExecute(PGconn * connection, const char * sql) {
    PGresult *          r;

    dbTransactionBegin(connection);

    r = PQexec(connection, sql);

    if (PQresultStatus(r) != PGRES_COMMAND_OK && PQresultStatus(r) != PGRES_TUPLES_OK) {
        lgLogError(lgGetHandle(), "Error issuing statement [%s]: '%s'", sql, PQerrorMessage(connection));

        if (r != NULL) {
            PQclear(r);
        }

        dbTransactionEnd(connection);

        return NULL;
    }
    else {
        lgLogDebug(lgGetHandle(), "Successfully executed statement [%s]", sql);
    }

    dbTransactionEnd(connection);

    return r;
}
