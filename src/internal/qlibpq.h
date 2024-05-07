/******************************************************************************
 * qLibc
 *
 * Copyright (c) 2024 SunBeau.
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

/* Wrap the low-level functions of the libpq library */

#ifndef QLIBPQ_H
#define QLIBPQ_H

#include "libpq-fe.h"
#include <stdlib.h>
#include <string.h>

#define qtype_cast(_type, _src) ((_type)_src)

typedef enum {
    PQ_UNKNOWN,
    PQ_WRITE,
    PQ_READ,
    PQ_PING,
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

static inline int pgsql_query(pgsql_t* pgsql, const char *query, pgquery_t type)
{
    if (pgsql == NULL || pgsql->pgconn == NULL) {
        return -1; /* error */
    }

    /* free previous query results */
    pgsql_query_result(pgsql);

    if (type == PQ_PING) {
        query = "";
    }

    PGresult *result = PQexec(pgsql->pgconn, query);
    ExecStatusType status = PQresultStatus(result);

    switch (type) {
        case PQ_PING: {
            if (status != PGRES_EMPTY_QUERY) {
                return -1; /* error */
            }
            break;
        }

        case PQ_READ: {
            if (status != PGRES_TUPLES_OK) {
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

#endif /* QLIBPQ_H */
