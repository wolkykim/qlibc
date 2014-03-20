/******************************************************************************
 * qLibc - http://www.qdecoder.org
 *
 * Copyright (c) 2010-2012 Seungyoung Kim.
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
 ******************************************************************************
 * $Id: qlibc.h 55 2012-04-03 03:15:00Z seungyoung.kim $
 ******************************************************************************/

/**
 * Generic database interface.
 *
 * This is a qLibc extension implementing generic database interface.
 * For now, it supports only MySQL database.
 *
 * @file qdatabase.h
 */

#ifndef _QDATABASE_H
#define _QDATABASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "qlibc.h"

/* database header files should be included before this header file. */
#ifdef _mysql_h
#define _Q_ENABLE_MYSQL  (1)
#endif /* _mysql_h */

/* types */
typedef struct qdb_s qdb_t;
typedef struct qdbresult_s qdbresult_t;

/* public functions */
extern qdb_t *qdb(const char *dbtype,
                  const char *addr, int port, const char *database,
                  const char *username, const char *password, bool autocommit);

/**
 * qdb structure
 */
struct qdb_s {
    /* capsulated member functions */
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
    qmutex_t  qmutex;  /*!< activated if compiled with --enable-threadsafe */

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
    } info;   /*!< database connection infomation */

#ifdef _Q_ENABLE_MYSQL
    /* private variables for mysql database - do not access directly */
    MYSQL  *mysql;
#endif
};

/**
 * qdbresult structure
 */
struct qdbresult_s {
    /* capsulated member functions */
    const char *(*getstr) (qdbresult_t *result, const char *field);
    const char *(*get_str_at) (qdbresult_t *result, int idx);
    int (*getint) (qdbresult_t *result, const char *field);
    int (*get_int_at) (qdbresult_t *result, int idx);
    bool (*getnext) (qdbresult_t *result);

    int (*get_cols) (qdbresult_t *result);
    int (*get_rows) (qdbresult_t *result);
    int (*get_row) (qdbresult_t *result);

    void (*free) (qdbresult_t *result);

#ifdef _Q_ENABLE_MYSQL
    /* private variables for mysql database - do not access directly */
    bool fetchtype;
    MYSQL_RES  *rs;
    MYSQL_FIELD  *fields;
    MYSQL_ROW  row;
    int cols;
    int cursor;
#endif
};

#ifdef __cplusplus
}
#endif

#endif /*_QDATABASE_H */
