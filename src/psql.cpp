#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <postgresql/libpq-fe.h>

#include "logger.h"
#include "psql.h"

#define CONNECTION_STRING_LENGTH            256

PGconn * dbConnect(const char * host, int port, const char * database, const char * username, const char * password) {
    char            szConnection[CONNECTION_STRING_LENGTH];
    PGconn *        connection;

    logger & log = logger::getInstance();

    snprintf(
        szConnection, 
        CONNECTION_STRING_LENGTH,
        "host=%s port=%d dbname=%s user=%s password=%s", 
        host,
        port,
        database,
        username,
        password);

    connection = PQconnectdb(szConnection);

    if (PQstatus(connection) != CONNECTION_OK) {
        log.logError("Could not connect to database: %s:%s", database, PQerrorMessage(connection));

        return NULL;
    }

    return connection;
}

void dbFinish(PGconn * connection) {
    PQfinish(connection);
}

int dbTransactionBegin(PGconn * connection) {
	PGresult *			queryResult;
    
    logger & log = logger::getInstance();
    
    queryResult = PQexec(connection, "BEGIN");

    if (PQresultStatus(queryResult) != PGRES_COMMAND_OK) {
        log.logError("Error beginning transaction [%s]", PQerrorMessage(connection));
        PQclear(queryResult);
        
        return -1;
    }

    log.logDebug("Transaction - Open");

    PQclear(queryResult);

    return 0;
}

int dbTransactionEnd(PGconn * connection) {
	PGresult *			queryResult;
    
    logger & log = logger::getInstance();
    
    queryResult = PQexec(connection, "END");

    if (PQresultStatus(queryResult) != PGRES_COMMAND_OK) {
        log.logError("Error ending transaction [%s]", PQerrorMessage(connection));
        PQclear(queryResult);
        
        return -1;
    }

    log.logDebug("Transaction - Closed");

    PQclear(queryResult);

    return 0;
}

PGresult * dbExecute(PGconn * connection, const char * sql) {
    PGresult *          r;

    logger & log = logger::getInstance();
    
    dbTransactionBegin(connection);

    r = PQexec(connection, sql);

    if (PQresultStatus(r) != PGRES_COMMAND_OK && PQresultStatus(r) != PGRES_TUPLES_OK) {
        log.logError("Error issuing statement [%s]: '%s'", sql, PQerrorMessage(connection));

        if (r != NULL) {
            PQclear(r);
        }

        dbTransactionEnd(connection);

        return NULL;
    }
    else {
        log.logDebug("Successfully executed statement [%s]", sql);
    }

    dbTransactionEnd(connection);

    return r;
}
