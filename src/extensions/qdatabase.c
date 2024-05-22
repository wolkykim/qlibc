/******************************************************************************
 * qLibc
 *
 * Copyright (c) 2010-2015 Seungyoung Kim.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

/**
 * @file qdatabase.c Database wrapper.
 *
 * Database header files should be included prior to qlibcext.h in your source
 * codes like below.
 *
 * @code
 *   #include "mysql.h"
 *   #include "qlibcext.h"
 * @endcode
 *
 * @code
 *   qdb_t *db = NULL;
 *   qdbresult_t *result = NULL;
 *
 *   db = qdb("MYSQL", "dbhost.qdecoder.org", 3306,
 *            "test", "secret", "sampledb", true);
 *   if (db == NULL) {
 *     printf("ERROR: Not supported database type.\n");
 *     return -1;
 *   }
 *
 *   // try to connect
 *   if (db->open(db) == false) {
 *     printf("WARNING: Can't connect to database.\n");
 *     return -1;
 *   }
 *
 *   // get results
 *   result = db->execute_query(db, "SELECT name, population FROM City");
 *   if (result != NULL) {
 *     printf("COLS : %d , ROWS : %d\n",
 *            result->get_cols(result), result->get_rows(result));
 *     while (result->get_next(result) == true) {
 *       char *pszName = result->get_str(result, "name");
 *       int   nPopulation = result->get_int(result, "population");
 *       printf("Country : %s , Population : %d\n", pszName, nPopulation);
 *     }
 *     result->free(result);
 *   }
 *
 *   // close connection
 *   db->close(db);
 *
 *   // free db object
 *   db->free(db);
 * @endcode
 */

#ifndef DISABLE_QDATABASE

#if defined(ENABLE_MYSQL) || defined(ENABLE_PGSQL) ||defined( _DOXYGEN_SKIP)

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#define qtype_cast(_type, _src) ((_type)_src)

#ifdef ENABLE_MYSQL
#include "mysql.h"
/* mysql specific connector options */
#define Q_MYSQL_OPT_RECONNECT          (1)
#define Q_MYSQL_OPT_CONNECT_TIMEOUT    (10)
#define Q_MYSQL_OPT_READ_TIMEOUT       (30)
#define Q_MYSQL_OPT_WRITE_TIMEOUT      (30)
#endif /* ENABLE_MYSQL */

#ifdef ENABLE_PGSQL
#include "libpq-fe.h"

/*
Wrap the low-level functions of the libpq library.
Provide interfaces with naming convention similar to mysql_xyz().
*/

typedef enum {
    PQ_UNKNOWN,
    PQ_PING,
    PQ_QUERY,
    PQ_UPDATE,
    PQ_TRANSACTION,
} pgquery_t;

typedef struct {
    char   *emsg;
    PGconn *pgconn;

    PGresult *pgresult;
    int rows;
    int cols;
    int cursor;
} pgsql_t;

static inline pgsql_t* pgsql_init(void)
{
    pgsql_t* pgsql = (pgsql_t *)calloc(1, sizeof(pgsql_t));
    return pgsql;
}

static inline const char* pgsql_set_emsg(pgsql_t* pgsql, const char* emsg)
{
    if (pgsql == NULL || emsg == NULL) {
        return NULL;
    }

    if (pgsql->emsg) {
        free(pgsql->emsg);
    }

    pgsql->emsg = strdup(emsg);
    return pgsql->emsg;
}

static inline void pgsql_close(pgsql_t* pgsql)
{
    if (pgsql == NULL) {
        return;
    }

    if (pgsql->emsg) {
        free(pgsql->emsg);
        pgsql->emsg = NULL;
    }

    if (pgsql->pgresult) {
        PQclear(pgsql->pgresult);
        pgsql->pgresult = NULL;
    }

    if (pgsql->pgconn) {
        PQfinish(pgsql->pgconn);
        pgsql->pgconn = NULL;
    }

    free(pgsql);
}

static inline void pgsql_query_result(pgsql_t* pgsql)
{
    if (pgsql->pgresult) {
        PQclear(pgsql->pgresult);
        pgsql->pgresult = NULL;
    }
}

static inline int pgsql_query(pgsql_t* pgsql, bool fetchtype,
                                const char *query, pgquery_t type)
{
    if (pgsql == NULL || pgsql->pgconn == NULL) {
        return -1; /* error */
    }

    /* free previous query results */
    pgsql_query_result(pgsql);

    if (type == PQ_PING) {
        query = "";
    }

    PGresult *result = NULL;
    ExecStatusType status = -1;

    if (fetchtype && type == PQ_QUERY) {
        if (PQsendQuery(pgsql->pgconn, query)) {
            if (PQsetSingleRowMode(pgsql->pgconn)) {
                result = PQgetResult(pgsql->pgconn);
            } else {
                return -1; /* error */
            }
        } else {
            return -1; /* error */
        }
    } else {
        result = PQexec(pgsql->pgconn, query);
    }

    if (result == NULL) {
        return -1; /* error */
    }

    status = PQresultStatus(result);

    switch (type) {
        case PQ_PING: {
            if (status != PGRES_EMPTY_QUERY) {
                return -1; /* error */
            }
            break;
        }

        case PQ_QUERY: {
            if (status != PGRES_TUPLES_OK &&
                status != PGRES_SINGLE_TUPLE) {
                return -1; /* error */
            }
            break;
        }

        default: {
            if (status != PGRES_COMMAND_OK) {
                return -1; /* error */
            }
            break;
        }
    }

    pgsql->pgresult = result;

    return 0; /* ok */
}

static inline int pgsql_affected_rows(pgsql_t* pgsql)
{
    if (pgsql && pgsql->pgresult) {
        return atoi(PQcmdTuples(pgsql->pgresult));
    }

    /* error */
    return -1;
}

static inline int pgsql_num_rows(pgsql_t* pgsql)
{
    if (pgsql && pgsql->pgresult) {
        return PQntuples(pgsql->pgresult);
    }

    /* error */
    return -1;
}

static inline int pgsql_num_fields(pgsql_t* pgsql)
{
    if (pgsql && pgsql->pgresult) {
        return PQnfields(pgsql->pgresult);
    }

    /* error */
    return -1;
}

static inline int pgsql_fetch_row(pgsql_t* pgsql)
{
    if (pgsql && pgsql->pgconn) {
        PGresult *result = PQgetResult(pgsql->pgconn);
        if (result) {
            pgsql_query_result(pgsql);
            pgsql->pgresult = result;
            pgsql->rows = pgsql_num_rows(pgsql);
            pgsql->cols = pgsql_num_fields(pgsql);
            return 0;
        } else {
            /* error */
            return -1;
        }
    }

    /* error */
    return -1;
}

#endif /* ENABLE_PGSQL */

#include "qinternal.h"
#include "extensions/qdatabase.h"

/*
 * Member method protos
 */
#ifndef _DOXYGEN_SKIP
// qdb_t object
static bool open_(qdb_t *db);
static bool close_(qdb_t *db);

static int execute_update(qdb_t *db, const char *query);
static int execute_updatef(qdb_t *db, const char *format, ...);
static qdbresult_t *execute_query(qdb_t *db, const char *query);
static qdbresult_t *execute_queryf(qdb_t *db, const char *format, ...);

static bool begin_tran(qdb_t *db);
static bool commit(qdb_t *db);
static bool rollback(qdb_t *db);

static bool set_fetchtype(qdb_t *db, bool use);
static bool get_conn_status(qdb_t *db);
static bool ping(qdb_t *db);
static const char *get_error(qdb_t *db, unsigned int *errorno);
static void free_(qdb_t *db);

// qdbresult_t object
static const char *result_get_str(qdbresult_t *result, const char *field);
static const char *result_get_str_at(qdbresult_t *result, int idx);
static int result_get_int(qdbresult_t *result, const char *field);
static int result_get_int_at(qdbresult_t *result, int idx);
static bool result_get_next(qdbresult_t *result);

static int result_get_cols(qdbresult_t *result);
static int result_get_rows(qdbresult_t *result);
static int result_get_row(qdbresult_t *result);

static void result_free(qdbresult_t *result);

#endif /* _DOXYGEN_SKIP */

/**
 * Initialize internal connector structure
 *
 * @param dbtype    database server type. currently "MYSQL" and "PGSQL" are supported
 * @param addr      ip or fqdn address.
 * @param port      port number
 * @param database  database name.
 * @param username  database username
 * @param password  database password
 * @param autocommit sets autocommit mode on if autocommit is true, off if
 *                   autocommit is false
 *                   (can only be set to true, when dbtype is "PGSQL")
 *
 * @return a pointer of qdb_t object in case of successful,
 *         otherwise returns NULL.
 *
 * @code
 *   qdb_t *db = qdb("MYSQL", "dbhost.qdecoder.org", 3306,
 *                   "sampledb", "test", "secret", true);
 *   if (db == NULL) {
 *     printf("ERROR: Not supported database type.\n");
 *     return -1;
 *   }
 * @endcode
 *
 * @code
 *   qdb_t *db = qdb("PGSQL", "127.0.0.1", 5432,
 *                   "sampledb", "test", "secret", true);
 *   if (db == NULL) {
 *     printf("ERROR: Not supported database type.\n");
 *     return -1;
 *   }
 * @endcode
 *
 * @note
 *  The pgsql server and libpq library does not provide a setting for autocommit.<br>
 *  The following instructions are from the official manual(https://www.postgresql.org/docs/15/sql-begin.html):<br>
 *  BEGIN initiates a transaction block, that is, all statements after a BEGIN command
 *  will be executed in a single transaction until an explicit COMMIT or ROLLBACK is given.
 *  By default (without BEGIN), PostgreSQL executes transactions in “autocommit” mode,
 *  that is, each statement is executed in its own transaction
 *  and a commit is implicitly performed at the end of the statement
 *  (if execution was successful, otherwise a rollback is done).
 */
qdb_t *qdb(const char *dbtype,
           const char *addr, int port, const char *database,
           const char *username, const char *password, bool autocommit)
{
    // check db type
#ifdef Q_ENABLE_MYSQL
    if (strcmp(dbtype, "MYSQL")) {
        errno = ENOTSUP;
        return NULL;
    }
#elif defined(Q_ENABLE_PGSQL)
    if (strcmp(dbtype, "PGSQL")) {
        errno = ENOTSUP;
        return NULL;
    }
#else
    return NULL;
#endif
    if (dbtype == NULL
            || addr == NULL
            || username == NULL
            || password == NULL
            || database == NULL) {
        return NULL;
    }

#ifdef Q_ENABLE_PGSQL
    if (autocommit == false) {
        errno = EINVAL;
        return NULL;
    }
#endif

    // initialize
    qdb_t *db;
    if ((db = (qdb_t *)malloc(sizeof(qdb_t))) == NULL) return NULL;
    memset((void *)db, 0, sizeof(qdb_t));
    db->connected = false;

    // set common structure
    db->info.dbtype = strdup(dbtype);
    db->info.addr = strdup(addr);
    db->info.port = port;
    db->info.username = strdup(username);
    db->info.password = strdup(password);
    db->info.database = strdup(database);
    db->info.autocommit = autocommit;
    db->info.fetchtype = false;// store mode

    // set db handler
#ifdef Q_ENABLE_MYSQL
    db->mysql = NULL;
#elif defined(Q_ENABLE_PGSQL)
    db->pgsql = NULL;
#endif

    // assign methods
    db->open = open_;
    db->close = close_;

    db->execute_update = execute_update;
    db->execute_updatef = execute_updatef;
    db->execute_query = execute_query;
    db->execute_queryf = execute_queryf;

    db->begin_tran = begin_tran;
    db->commit = commit;
    db->rollback = rollback;

    db->set_fetchtype = set_fetchtype;
    db->get_conn_status = get_conn_status;
    db->ping = ping;
    db->get_error = get_error;
    db->free = free_;

    // initialize recrusive mutex
    Q_MUTEX_NEW(db->qmutex, true);

    return db;
}

/**
 * qdb->open(): Connect to database server
 *
 * @param db        a pointer of qdb_t object
 *
 * @return true if successful, otherwise returns false.
 */
static bool open_(qdb_t *db)
{
    if (db == NULL) return false;

    // if connected, close first
    if (db->connected == true) {
        close_(db);
    }

#ifdef Q_ENABLE_MYSQL
    Q_MUTEX_ENTER(db->qmutex);

    // initialize handler
    if (db->mysql != NULL) close_(db);

    if (mysql_library_init(0, NULL, NULL) != 0) {
        Q_MUTEX_LEAVE(db->qmutex);
        return false;
    }

    if ((db->mysql = mysql_init(NULL)) == NULL) {
        Q_MUTEX_LEAVE(db->qmutex);
        return false;
    }

    // set options
    my_bool reconnect = Q_MYSQL_OPT_RECONNECT;
    unsigned int connect_timeout = Q_MYSQL_OPT_CONNECT_TIMEOUT;
    unsigned int read_timeout = Q_MYSQL_OPT_READ_TIMEOUT;
    unsigned int write_timeout = Q_MYSQL_OPT_WRITE_TIMEOUT;

    if (reconnect != false) {
        mysql_options(db->mysql,
                MYSQL_OPT_RECONNECT,
                (char *)&reconnect);
    }
    if (connect_timeout > 0) {
        mysql_options(db->mysql,
                MYSQL_OPT_CONNECT_TIMEOUT,
                (char *)&connect_timeout);
    }
    if (read_timeout > 0) {
        mysql_options(db->mysql,
                MYSQL_OPT_READ_TIMEOUT,
                (char *)&read_timeout);
    }
    if (write_timeout > 0) {
        mysql_options(db->mysql,
                MYSQL_OPT_WRITE_TIMEOUT,
                (char *)&write_timeout);
    }

    // try to connect
    if (mysql_real_connect(db->mysql,
                    db->info.addr,
                    db->info.username,
                    db->info.password,
                    db->info.database,
                    db->info.port, NULL, 0) == NULL) {
        close_(db);  // free mysql handler
        Q_MUTEX_LEAVE(db->qmutex);
        return false;
    }

    // set auto-commit
    if (mysql_autocommit(db->mysql, db->info.autocommit) != 0) {
        close_(db);  // free mysql handler
        Q_MUTEX_LEAVE(db->qmutex);
        return false;
    }

    // set flag
    db->connected = true;
    Q_MUTEX_LEAVE(db->qmutex);
    return true;
#elif defined(Q_ENABLE_PGSQL)
    Q_MUTEX_ENTER(db->qmutex);

    bool ret = true;
    PGconn* pgconn = NULL;

    // initialize handler
    if (db->pgsql != NULL) close_(db);

    if ((db->pgsql = pgsql_init()) == NULL) {
        ret = false;
        goto lable_quit;
    }

    // set options
    /* do nothing */

    /* int to char* */
    char pgport[10];
    snprintf(pgport, sizeof(pgport), "%d", db->info.port);

    // try to connect
    pgconn = PQsetdbLogin(db->info.addr, pgport,
                    NULL, NULL,
                    db->info.database, db->info.username, db->info.password);
    if (pgconn == NULL) {
        close_(db);  // free pgsql handler
        ret = false;
        goto lable_quit;
    }

    if (PQstatus(pgconn) != CONNECTION_OK) {
        const char* emsg = PQerrorMessage(pgconn);
        pgsql_set_emsg(db->pgsql, emsg);
        // fprintf(stderr, "%s", emsg);
        ret = false;
        goto lable_quit;
    }

    qtype_cast(pgsql_t*, db->pgsql)->pgconn = pgconn;

    // set flag
    db->connected = true;

lable_quit:

    if (ret == false) {
        PQfinish(pgconn);
    }

    Q_MUTEX_LEAVE(db->qmutex);
    return ret;
#else
    return false;
#endif
}

/**
 * qdb->close(): Disconnect from database server
 *
 * @param db        a pointer of qdb_t object
 *
 * @return true if successful, otherwise returns false.
 *
 * @note
 *  Unless you call qdb->free(), qdb_t object will keep the database
 *  information. So you can re-connect to database using qdb->open().
 */
static bool close_(qdb_t *db)
{
    if (db == NULL) return false;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)
    Q_MUTEX_ENTER(db->qmutex);

#if defined(Q_ENABLE_MYSQL)
    if (db->mysql != NULL) {
        mysql_close(db->mysql);
        db->mysql = NULL;
        mysql_library_end();
    }
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    if (db->pgsql != NULL) {
        pgsql_close(db->pgsql);
        db->pgsql = NULL;
    }
#endif /* Q_ENABLE_PGSQL */

    db->connected = false;
    Q_MUTEX_LEAVE(db->qmutex);
    return true;
#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    return false;
#endif
}

/**
 * qdb->execute_update(): Executes the update DML
 *
 * @param db        a pointer of qdb_t object
 * @param query     query string
 *
 * @return a number of affected rows
 */
static int execute_update(qdb_t *db, const char *query)
{
    if (db == NULL || db->connected == false) return -1;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)
    Q_MUTEX_ENTER(db->qmutex);

    int affected = -1;

    // query
    DEBUG("%s", query);
#if defined(Q_ENABLE_MYSQL)
    if (mysql_query(db->mysql, query) == 0) {
        /* get affected rows */
        if ((affected = mysql_affected_rows(db->mysql)) < 0) affected = -1;
    }
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    if (pgsql_query(db->pgsql, db->info.fetchtype, query, PQ_UPDATE) == 0) {
        /* get affected rows */
        if ((affected = pgsql_affected_rows(db->pgsql)) < 0) affected = -1;
    }
#endif /* Q_ENABLE_PGSQL */

    Q_MUTEX_LEAVE(db->qmutex);
    return affected;
#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    return -1;
#endif
}

/**
 * qdb->execute_updatef(): Executes the formatted update DML
 *
 * @param db        a pointer of qdb_t object
 * @param format    query string format
 *
 * @return a number of affected rows, otherwise returns -1
 */
static int execute_updatef(qdb_t *db, const char *format, ...)
{
    char *query;
    DYNAMIC_VSPRINTF(query, format);
    if (query == NULL) return -1;

    int affected = execute_update(db, query);
    free(query);

    return affected;
}

/**
 * qdb->execute_query(): Executes the query
 *
 * @param db        a pointer of qdb_t object
 * @param query     query string
 *
 * @return a pointer of qdbresult_t if successful, otherwise returns NULL
 */
static qdbresult_t *execute_query(qdb_t *db, const char *query)
{
    if (db == NULL || db->connected == false) return NULL;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)

    DEBUG("%s", query);

#if defined(Q_ENABLE_MYSQL)
    // query
    if (mysql_query(db->mysql, query)) return NULL;

    // store
    qdbresult_t *result = (qdbresult_t *)malloc(sizeof(qdbresult_t));
    if (result == NULL) return NULL;

    result->fetchtype = db->info.fetchtype;
    if (result->fetchtype == false) {
        result->rs = mysql_store_result(db->mysql);
    } else {
        result->rs = mysql_use_result(db->mysql);
    }
    if (result->rs == NULL) {
        free(result);
        return NULL;
    }

    /* get meta data */
    result->fields = NULL;
    result->row = NULL;
    result->cols = mysql_num_fields(result->rs);
    result->cursor = 0;
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    // query
    if (pgsql_query(db->pgsql, db->info.fetchtype, query, PQ_QUERY)) return NULL;

    // store
    qdbresult_t *result = (qdbresult_t *)malloc(sizeof(qdbresult_t));
    if (result == NULL) return NULL;
    result->pgsql = db->pgsql;

    /* get meta data */
    // result->row = NULL;
    qtype_cast(pgsql_t*, result->pgsql)->rows = pgsql_num_rows(result->pgsql);
    qtype_cast(pgsql_t*, result->pgsql)->cols = pgsql_num_fields(result->pgsql);
    qtype_cast(pgsql_t*, result->pgsql)->cursor = 0;
#endif /* Q_ENABLE_PGSQL */

    /* assign methods */
    result->get_str = result_get_str;
    result->get_str_at = result_get_str_at;
    result->get_int = result_get_int;
    result->get_int_at = result_get_int_at;
    result->get_next = result_get_next;

    result->get_cols = result_get_cols;
    result->get_rows = result_get_rows;
    result->get_row = result_get_row;

    result->free = result_free;

    return result;
#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    return NULL;
#endif
}

/**
 * qdb->execute_queryf(): Executes the formatted query
 *
 * @param db        a pointer of qdb_t object
 * @param format    query string format
 *
 * @return a pointer of qdbresult_t if successful, otherwise returns NULL
 */
static qdbresult_t *execute_queryf(qdb_t *db, const char *format, ...)
{
    char *query;
    DYNAMIC_VSPRINTF(query, format);
    if (query == NULL) return NULL;

    qdbresult_t *ret = db->execute_query(db, query);
    free(query);
    return ret;
}

/**
 * qdb->begin_tran(): Start transaction
 *
 * @param db        a pointer of qdb_t object
 *
 * @return true if successful, otherwise returns false
 *
 * @code
 *   db->begin_tran(db);
 *   (... insert/update/delete ...)
 *   db->commit(db);
 * @endcode
 */
static bool begin_tran(qdb_t *db)
{
    if (db == NULL) return false;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)
    Q_MUTEX_ENTER(db->qmutex);
    if (qtype_cast(qmutex_t*, db->qmutex)->count != 1) {
        Q_MUTEX_LEAVE(db->qmutex);
        return false;
    }

#if defined(Q_ENABLE_MYSQL)
    qdbresult_t *result;
    result = db->execute_query(db, "START TRANSACTION");
    if (result == NULL) {
        Q_MUTEX_LEAVE(db->qmutex);
        return false;
    }
    result->free(result);
    return true;
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    if (pgsql_query(db->pgsql, db->info.fetchtype, "START TRANSACTION", PQ_TRANSACTION) < 0) {
        Q_MUTEX_LEAVE(db->qmutex);
        return false;
    }
    return true;
#endif /* Q_ENABLE_PGSQL */

#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    return false;
#endif
}

/**
 * qdb->commit(): Commit transaction
 *
 * @param db        a pointer of qdb_t object
 *
 * @return true if successful, otherwise returns false
 */
static bool commit(qdb_t *db)
{
    if (db == NULL) return false;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)
    bool ret = false;

#if defined(Q_ENABLE_MYSQL)
    if (mysql_commit(db->mysql) == 0) {
        ret = true;
    }
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    if (pgsql_query(db->pgsql, db->info.fetchtype, "COMMIT", PQ_TRANSACTION) == 0) {
        ret = true;
    }
#endif /* Q_ENABLE_PGSQL */

    if (qtype_cast(qmutex_t*, db->qmutex)->count > 0) {
        Q_MUTEX_LEAVE(db->qmutex);
    }
    return ret;
#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    return false;
#endif
}

/**
 * qdb->rellback(): Roll-back and abort transaction
 *
 * @param db        a pointer of qdb_t object
 *
 * @return true if successful, otherwise returns false
 */
static bool rollback(qdb_t *db)
{
    if (db == NULL) return false;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)
    bool ret = false;

#if defined(Q_ENABLE_MYSQL)
    if (mysql_rollback(db->mysql) == 0) {
        ret = true;
    }
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    if (pgsql_query(db->pgsql, db->info.fetchtype, "ROLLBACK", PQ_TRANSACTION) == 0) {
        ret = true;
    }
#endif /* Q_ENABLE_PGSQL */

    if (qtype_cast(qmutex_t*, db->qmutex)->count > 0) {
        Q_MUTEX_LEAVE(db->qmutex);
    }
    return ret;
#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    return 0;
#endif
}

/**
 * qdb->set_fetchtype(): Set result fetching type
 *
 * @param db        a pointer of qdb_t object
 * @param fromdb    false for storing the results to client (default mode),
 *                  true for fetching directly from server,
 *
 * @return true if successful otherwise returns false
 *
 * @note
 *  If qdb->set_fetchtype(db, true) is called, the results does not
 *  actually read into the client. Instead, each row must be retrieved
 *  individually by making calls to qdbresult->get_next().
 *  This reads the result of a query directly from the server without storing
 *  it in local buffer, which is somewhat faster and uses much less memory than
 *  default behavior qdb->set_fetchtype(db, false).
 */
static bool set_fetchtype(qdb_t *db, bool fromdb)
{
    if (db == NULL) return false;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)

#if defined(Q_ENABLE_MYSQL)
    db->info.fetchtype = fromdb;
    return true;
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    db->info.fetchtype = fromdb;
    return true;
#endif /* Q_ENABLE_PGSQL */

#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    return false;
#endif
}

/**
 * qdb->get_conn_status(): Get last connection status
 *
 * @param db        a pointer of qdb_t object
 *
 * @return true if the connection flag is set to alive, otherwise returns false
 *
 * @note
 * This function just returns the the connection status flag.
 */
static bool get_conn_status(qdb_t *db)
{
    if (db == NULL) return false;

    return db->connected;
}

/**
 * qdb->ping(): Checks whether the connection to the server is working.
 *
 * @param db        a pointer of qdb_t object
 *
 * @return true if connection is alive, false if connection is not available
 *         and failed to reconnect
 *
 * @note
 * If the connection has gone down, an attempt to reconnect.
 */
static bool ping(qdb_t *db)
{
    if (db == NULL) return false;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)
    bool ping_failed = false;

#if defined(Q_ENABLE_MYSQL)
    if (db->connected == true && mysql_ping(db->mysql) == 0) {
        return true;
    }
    ping_failed = true;
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    if (db->connected == true && pgsql_query(db->pgsql, db->info.fetchtype, NULL, PQ_PING) == 0) {
        return true;
    }
    ping_failed = true;
#endif /* Q_ENABLE_PGSQL */

    if (ping_failed) { // ping test failed
        if (open_(db) == true) {  // try re-connect
            DEBUG("Connection recovered.");
            return true;
        }
    }

    return false;
#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    return false;
#endif
}

/**
 * qdb->get_error(): Get error number and message
 *
 * @param db        a pointer of qdb_t object
 * @param errorno   if not NULL, error number will be stored
 *
 * @return a pointer of error message string
 *
 * @note
 * Do not free returned error message
 */
static const char *get_error(qdb_t *db, unsigned int *errorno)
{
    if (db == NULL || db->connected == false) return "(no opened db)";

    unsigned int eno = 0;
    const char *emsg;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)
#if defined(Q_ENABLE_MYSQL)
    eno = mysql_errno(db->mysql);
    if (eno == 0) emsg = "(no error)";
    else emsg = mysql_error(db->mysql);
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    emsg = qtype_cast(pgsql_t*, db->pgsql)->emsg;
    if (emsg == NULL) {
        eno  = 0;
        emsg = "(no error)";
    } else {
        eno = 1;
    }
#endif /* Q_ENABLE_PGSQL */
#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    emsg = "(not implemented)";
#endif

    if (errorno != NULL) *errorno = eno;
    return emsg;
}

/**
 * qdb->free(): De-allocate qdb_t structure
 *
 * @param db        a pointer of qdb_t object
 */
static void free_(qdb_t *db)
{
    if (db == NULL) return;

    Q_MUTEX_ENTER(db->qmutex);

    close_(db);

    free(db->info.dbtype);
    free(db->info.addr);
    free(db->info.username);
    free(db->info.password);
    free(db->info.database);
    free(db);

    Q_MUTEX_LEAVE(db->qmutex);
    Q_MUTEX_DESTROY(db->qmutex);

    return;
}

/**
 * qdbresult->get_str(): Get the result as string by field name
 *
 * @param result    a pointer of qdbresult_t
 * @param field     column name
 *
 * @return a string pointer if successful, otherwise returns NULL.
 *
 * @note
 * Do not free returned string.
 */
static const char *result_get_str(qdbresult_t *result, const char *field)
{
    if (result == NULL) return NULL;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)
#if defined(Q_ENABLE_MYSQL)
    if (result->rs == NULL || result->cols <= 0) return NULL;

    if (result->fields == NULL) result->fields = mysql_fetch_fields(result->rs);

    int i;
    for (i = 0; i < result->cols; i++) {
        if (!strcasecmp(result->fields[i].name, field)) {
            return result->get_str_at(result, i + 1);
        }
    }

    return NULL;
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    pgsql_t* pgsql = result->pgsql;
    if (pgsql == NULL || pgsql->cols <= 0) {
        return NULL;
    }

    /* get field num  */
    const char *value = NULL;
    int field_num = PQfnumber(pgsql->pgresult, field);
    if (field_num == -1) {
        return NULL;
    }

    /* get row num  */
    int row_num = -1;
    if (pgsql->rows == 1) {
        row_num = 0;
    } else {
        row_num = pgsql->cursor;
    }

    /* get value */
    value = PQgetvalue(pgsql->pgresult, row_num, field_num);
    return value;
#endif /* Q_ENABLE_PGSQL */
#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    return NULL;
#endif
}

/**
 * qdbresult->get_str_at(): Get the result as string by column number
 *
 * @param result    a pointer of qdbresult_t
 * @param idx       column number (first column is 1)
 *
 * @return a string pointer if successful, otherwise returns NULL.
 */
static const char *result_get_str_at(qdbresult_t *result, int idx)
{
    if (result == NULL) return NULL;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)
#if defined(Q_ENABLE_MYSQL)
    if (result->rs == NULL
        || result->cursor == 0
        || idx <= 0
        || idx > result->cols ) {
        return NULL;
    }
    return result->row[idx-1];
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    pgsql_t* pgsql = result->pgsql;
    if (result->pgsql == NULL
        || pgsql->rows == 0
        || pgsql->cols == 0
        || idx <= 0
        || idx > pgsql->cols) {
        return NULL;
    }

    /* get row num  */
    int row_num = -1;
    if (pgsql->rows == 1) {
        row_num = 0;
    } else {
        row_num = pgsql->cursor;
    }

    /* get value */
    const char *value = PQgetvalue(pgsql->pgresult, row_num, idx - 1);
    return value;
#endif /* Q_ENABLE_PGSQL */
#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    return NULL;
#endif
}

/**
 * qdbresult->get_int(): Get the result as integer by field name
 *
 * @param result    a pointer of qdbresult_t
 * @param field     column name
 *
 * @return a integer converted value
 */
static int result_get_int(qdbresult_t *result, const char *field)
{
    const char *val = result->get_str(result, field);
    if (val == NULL) return 0;
    return atoi(val);
}

/**
 * qdbresult->get_int_at(): Get the result as integer by column number
 *
 * @param result    a pointer of qdbresult_t
 * @param idx       column number (first column is 1)
 *
 * @return a integer converted value
 */
static int result_get_int_at(qdbresult_t *result, int idx)
{
    const char *val = result->get_str_at(result, idx);
    if (val == NULL) return 0;
    return atoi(val);
}

/**
 * qdbresult->get_next(): Retrieves the next row of a result set
 *
 * @param result    a pointer of qdbresult_t
 *
 * @return true if successful, false if no more rows are left
 */
static bool result_get_next(qdbresult_t *result)
{
    if (result == NULL) return false;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)
#if defined(Q_ENABLE_MYSQL)
    if (result->rs == NULL) return false;

    if ((result->row = mysql_fetch_row(result->rs)) == NULL) return false;
    result->cursor++;

    return true;
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    pgsql_t* pgsql = result->pgsql;

    if (pgsql == NULL || pgsql->pgresult == NULL) {
        return false;
    }

    int cursor = pgsql->cursor;
    if (++cursor >= pgsql->rows) {
        if (pgsql_fetch_row(pgsql) != 0) {
            return false;
        }
    }

    pgsql->cursor = cursor;
    return true;
#endif /* Q_ENABLE_PGSQL */
#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    return false;
#endif
}

/**
 * qdbresult->get_cols(): Get the number of columns in the result set
 *
 * @param result    a pointer of qdbresult_t
 *
 * @return the number of columns in the result set
 */
static int result_get_cols(qdbresult_t *result)
{
    if (result == NULL) return 0;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)
#if defined(Q_ENABLE_MYSQL)
    if (result->rs == NULL) return 0;
    return result->cols;
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    if (result->pgsql == NULL) return 0;
    return qtype_cast(pgsql_t*, result->pgsql)->cols;
#endif /* Q_ENABLE_PGSQL */
#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    return 0;
#endif
}

/**
 * qdbresult->get_rows(): Get the number of rows in the result set
 *
 * @param result    a pointer of qdbresult_t
 *
 * @return the number of rows in the result set
 */
static int result_get_rows(qdbresult_t *result)
{
    if (result == NULL) return 0;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)
#if defined(Q_ENABLE_MYSQL)
    if (result->rs == NULL) return 0;
    return mysql_num_rows(result->rs);
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    if (result->pgsql == NULL) return 0;
    return qtype_cast(pgsql_t*, result->pgsql)->rows;
#endif /* Q_ENABLE_PGSQL */
#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    return 0;
#endif
}

/**
 * qdbresult->get_row(): Get the current row number
 *
 * @param result    a pointer of qdbresult_t
 *
 * @return current fetching row number of the result set
 *
 * @note
 * This number is sequencial counter which is started from 1.
 */
static int result_get_row(qdbresult_t *result)
{
    if (result == NULL) return 0;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)
#if defined(Q_ENABLE_MYSQL)
    if (result->rs == NULL) return 0;
    return result->cursor;
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    if (result->pgsql == NULL) return 0;
    return qtype_cast(pgsql_t*, result->pgsql)->cursor;
#endif /* Q_ENABLE_PGSQL */
#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    return 0;
#endif
}

/**
 * qdbresult->free(): De-allocate the result
 *
 * @param result    a pointer of qdbresult_t
 */
static void result_free(qdbresult_t *result)
{
    if (result == NULL) return;

#if defined(Q_ENABLE_MYSQL) || defined(Q_ENABLE_PGSQL)
#if defined(Q_ENABLE_MYSQL)
    if (result->rs != NULL) {
        if (result->fetchtype == true) {
            while (mysql_fetch_row(result->rs) != NULL);
        }
        mysql_free_result(result->rs);
        result->rs = NULL;
    }
    free(result);
    return;
#endif /* Q_ENABLE_MYSQL */

#if defined(Q_ENABLE_PGSQL)
    result->pgsql = NULL;
    free(result);
    return;
#endif /* Q_ENABLE_PGSQL */
#else /* Q_ENABLE_MYSQL || Q_ENABLE_PGSQL */
    return;
#endif
}

#endif

#endif /* DISABLE_QDATABASE */
