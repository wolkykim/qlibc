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
 ******************************************************************************/

/**
 * Generic database interface.
 *
 * This is a qLibc extension implementing generic database interface.
 * For now, it supports only MySQL database.
 *
 * @file qdatabase.h
 */

#ifndef QDATABASE_H
#define QDATABASE_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* database header files should be included before this header file. */
#ifdef _mysql_h
#define Q_ENABLE_MYSQL  (1)
#endif /* _mysql_h */

#ifdef LIBPQ_FE_H
#define Q_ENABLE_PGSQL  (1)
#endif /* LIBPQ_FE_H */

#if defined(Q_ENABLE_MYSQL) && defined(Q_ENABLE_PGSQL)
#error "only can enable one in the same time"
#endif

/* types */
typedef struct qdbresult_s qdbresult_t;
typedef struct qdb_s qdb_t;

/* public functions */
extern qdb_t *qdb(const char *dbtype,
                  const char *addr, int port, const char *database,
                  const char *username, const char *password, bool autocommit);

/**
 * qdbresult object structure
 */
struct qdbresult_s {
    /* encapsulated member functions */
    const char *(*get_str) (qdbresult_t *result, const char *field);
    const char *(*get_str_at) (qdbresult_t *result, int idx);
    int (*get_int) (qdbresult_t *result, const char *field);
    int (*get_int_at) (qdbresult_t *result, int idx);
    bool (*get_next) (qdbresult_t *result);

    int (*get_cols) (qdbresult_t *result);
    int (*get_rows) (qdbresult_t *result);
    int (*get_row) (qdbresult_t *result);

    void (*free) (qdbresult_t *result);

    bool fetchtype;
    int cursor;
    int cols;

#ifdef Q_ENABLE_MYSQL
    /* private variables for mysql database - do not access directly */
    MYSQL_RES  *rs;
    MYSQL_FIELD  *fields;
    MYSQL_ROW  row;
#endif

#ifdef Q_ENABLE_PGSQL
    /* private variables for pgsql database - do not access directly */
    void *rs;
    const void *row;
#endif
};

/* qdb object structure */
struct qdb_s {
    /* encapsulated member functions */
    bool (*open) (qdb_t *db);
    bool (*close) (qdb_t *db);

    int (*execute_update) (qdb_t *db, const char *query);
    int (*execute_updatef) (qdb_t *db, const char *format, ...);

    qdbresult_t *(*execute_query) (qdb_t *db, const char *query);
    qdbresult_t *(*execute_queryf) (qdb_t *db, const char *format, ...);

    bool (*begin_tran) (qdb_t *db);
    bool (*end_tran) (qdb_t *db, bool commit);
    bool (*commit) (qdb_t *db);
    bool (*rollback) (qdb_t *db);

    bool (*set_fetchtype) (qdb_t *db, bool fromdb);
    bool (*get_conn_status) (qdb_t *db);
    bool (*ping) (qdb_t *db);
    const char *(*get_error) (qdb_t *db, unsigned int *errorno);
    void (*free) (qdb_t *db);

    /* private variables - do not access directly */
    void *qmutex;

    bool connected;   /*!< if opened true, if closed false */

    struct {
        char *dbtype;
        char *addr;
        int port;
        char *username;
        char *password;
        char *database;
        bool autocommit;
        bool fetchtype;
    } info;   /*!< database connection information */

#ifdef Q_ENABLE_MYSQL
    /* private variables for mysql database - do not access directly */
    MYSQL  *mysql;
#endif

#ifdef Q_ENABLE_PGSQL
    /* private variables for pgsql database - do not access directly */
    void  *pgsql;
#endif
};

#ifdef __cplusplus
}
#endif

#endif /* QDATABASE_H */
