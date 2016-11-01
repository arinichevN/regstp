#ifndef LIBPAS_DB_H
#define LIBPAS_DB_H
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "app.h"

extern int initDB(PGconn **conn, char *conninfo);

extern void freeDB(PGconn **conn);

extern PGresult *dbGetDataC(PGconn *conn, char *q, char *ms);

extern PGresult *dbGetDataT(PGconn *conn, char *q, char *ms);

extern int dbGetDataN(PGconn *conn, char *q, char *ms);

extern int dbConninfoParse(const char *buf, char *host, int *port, char *dbname, char *user, size_t str_size);

extern int dbConninfoEq(char *c1, char *c2);

#endif /* DB_H */

