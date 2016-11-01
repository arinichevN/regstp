
#include "db.h"

int initDB(PGconn **conn, char *conninfo) {
    *conn = PQconnectdb(conninfo);
    if (PQstatus(*conn) == CONNECTION_BAD) {
        fprintf(stderr, "%s\n", PQerrorMessage(*conn));
        return 0;
    }
    return 1;
}

void freeDB(PGconn **conn) {
    if (*conn != NULL) {
        PQfinish(*conn);
        *conn = NULL;
    }
}

PGresult *dbGetDataC(PGconn *conn, char *q, char *ms) {
    PGresult *r;
    r = PQexec(conn, q);
    if (r == NULL) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s: no result: %s\n", ms, PQerrorMessage(conn));
#endif
        return r;
    }
    if (PQresultStatus(r) != PGRES_COMMAND_OK) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s: command is not ok\n", ms);
#endif
        PQclear(r);
        r = NULL;
    }
    return r;
}

PGresult *dbGetDataT(PGconn *conn, char *q, char *ms) {
    PGresult *r;
    r = PQexec(conn, q);
    if (r == NULL) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s: no result: %s\n", ms, PQerrorMessage(conn));
#endif
        return r;
    }
    if (PQresultStatus(r) != PGRES_TUPLES_OK) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s: tuples are not ok\n", ms);
#endif
        PQclear(r);
        r = NULL;
    }
    return r;
}
//for use with select count(*)

int dbGetDataN(PGconn *conn, char *q, char *ms) {
    PGresult *r;
    r = PQexec(conn, q);
    if (r == NULL) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s: no result: %s\n", ms, PQerrorMessage(conn));
#endif
        return -1;
    }
    if (PQresultStatus(r) != PGRES_TUPLES_OK) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s: tuples are not ok\n", ms);
#endif
        PQclear(r);
        return -1;
    }
    int n;
    n = PQntuples(r);
    if (n != 1) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s: tuples: need only one, but %d given\n", ms, n);
#endif
        PQclear(r);
        return -1;
    }
    int out = atoi(PQgetvalue(r, 0, 0));
    PQclear(r);
    return out;
}

int dbConninfoParse(const char *buf, char *host, int *port, char *dbname, char *user, size_t str_size) {
    int i, host_found = 0, port_found = 0, dbname_found = 0, user_found = 0;
    *port = 0;
    memset(host, 0, str_size);
    memset(dbname, 0, str_size);
    memset(user, 0, str_size);
    for (i = 0; i < strlen(buf); i++) {
        switch (buf[i]) {
            case 'h':
                if (host_found) {
                    break;
                }
                if (sscanf(buf + i, "host=%s ", host) == 1) {
                    host_found = 1;
                }
                break;
            case 'p':
                if (port_found) {
                    break;
                }
                if (sscanf(buf + i, "port=%d ", port) == 1) {
                    port_found = 1;
                }
                break;
            case 'd':
                if (dbname_found) {
                    break;
                }
                if (sscanf(buf + i, "dbname=%s ", dbname) == 1) {
                    dbname_found = 1;
                }
                break;
            case 'u':
                if (dbname_found) {
                    break;
                }
                if (sscanf(buf + i, "user=%s ", user) == 1) {
                    user_found = 1;
                }
                break;
        }
    }
    return 1;
}

int dbConninfoEq(char *c1, char *c2) {
    char host1[NAME_SIZE], host2[NAME_SIZE];
    int port1, port2;
    char dbname1[NAME_SIZE], dbname2[NAME_SIZE];
    char user1[NAME_SIZE], user2[NAME_SIZE];
    if (!dbConninfoParse(c1, host1, &port1, dbname1, user1, NAME_SIZE)) {
        return 0;
    }
    if (!dbConninfoParse(c2, host2, &port2, dbname2, user2, NAME_SIZE)) {
        return 0;
    }
    if (strcmp(host1, host2) != 0) {
        return 0;
    }
    if (port1 != port2) {
        return 0;
    }
    if (strcmp(dbname1, dbname2) != 0) {
        return 0;
    }
    if (strcmp(user1, user2) != 0) {
        return 0;
    }
    return 1;
}