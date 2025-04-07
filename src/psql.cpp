#include <string>
#include <iostream>
#include <sstream>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <postgresql/libpq-fe.h>

#include "logger.h"
#include "psql.h"

using namespace std;

psqlConnection::psqlConnection(const string & host, int port, const string & database, const string & username, const string & password) {
    stringstream s;
    s << "host=" << host << " port=" << port << " user=" << username << " password=" << password;

    string connectionStr = s.str();
    
    connection = PQconnectdb(connectionStr.c_str());

    if (PQstatus(connection) != CONNECTION_OK) {
        throw psql_error(psql_error::buildMsg("Could not connect to database: %s:%s", database.c_str(), PQerrorMessage(connection)));
    }
    else {
        log.logInfo("Successfully connected to database '%s'", database.c_str());
    }
}

psqlConnection::~psqlConnection() {
    PQfinish(connection);
}

void psqlConnection::beginTransaction() {
    PGresult * result = PQexec(connection, "BEGIN");

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        PQclear(result);
        throw psql_error(psql_error::buildMsg("Error beginning transaction [%s]", PQerrorMessage(connection)));
    }

    log.logDebug("BEGIN TRANSACTION");

    PQclear(result);
}

void psqlConnection::endTransaction() {
    PGresult * result = PQexec(connection, "END");

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        PQclear(result);
        throw psql_error(psql_error::buildMsg("Error ending transaction [%s]", PQerrorMessage(connection)));
    }

    log.logDebug("END TRANSACTION");

    PQclear(result);
}

PGresult * psqlConnection::execute(const char * sql) {
    beginTransaction();

    PGresult * result = PQexec(connection, sql);

    if (PQresultStatus(result) != PGRES_COMMAND_OK && PQresultStatus(result) != PGRES_TUPLES_OK) {
        if (result != NULL) {
            PQclear(result);
        }

        endTransaction();

        throw psql_error(psql_error::buildMsg("Error issuing statement [%s]: '%s'", sql, PQerrorMessage(connection)));
    }
    else {
        log.logDebug("Successfully executed statement [%s]", sql);
    }

    endTransaction();

    return result;
}
