/******************************************************************************
 * qLibc
 *
 * Copyright (c) 2010-2014 Seungyoung Kim.
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
 * Hash Table container.
 *
 * @file qhashtbl.h
 */

#ifndef _QHASHTBL_H
#define _QHASHTBL_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "qtype.h"

#ifdef __cplusplus
extern "C" {
#endif

/* types */
typedef struct qhashtbl_s qhashtbl_t;

/* public functions */
enum {
    QHASHTBL_OPT_THREADSAFE = (0x01)  /*!< make it thread-safe */
};

extern qhashtbl_t *qhashtbl(size_t range, int options);  /*!< qhashtbl constructor */

/**
 * qhashtbl container object structure
 */
struct qhashtbl_s {
    /* encapsulated member functions */
    bool (*put) (qhashtbl_t *tbl, const char *name, const void *data, size_t size);
    bool (*putstr) (qhashtbl_t *tbl, const char *name, const char *str);
    bool (*putstrf) (qhashtbl_t *tbl, const char *name, const char *format, ...);
    bool (*putint) (qhashtbl_t *tbl, const char *name, const int64_t num);

    void *(*get) (qhashtbl_t *tbl, const char *name, size_t *size, bool newmem);
    char *(*getstr) (qhashtbl_t *tbl, const char *name, bool newmem);
    int64_t (*getint) (qhashtbl_t *tbl, const char *name);

    bool (*getnext) (qhashtbl_t *tbl, qhnobj_t *obj, bool newmem);

    bool (*remove) (qhashtbl_t *tbl, const char *name);

    size_t (*size) (qhashtbl_t *tbl);
    void (*clear) (qhashtbl_t *tbl);
    bool (*debug) (qhashtbl_t *tbl, FILE *out);

    void (*lock) (qhashtbl_t *tbl);
    void (*unlock) (qhashtbl_t *tbl);

    void (*free) (qhashtbl_t *tbl);

    /* private variables - do not access directly */
    qmutex_t *qmutex;   /*!< initialized when QHASHTBL_OPT_THREADSAFE is given */
    size_t num;         /*!< number of objects in this table */
    size_t range;       /*!< hash range, vertical number of slots */
    qhnobj_t **slots;   /*!< slot pointer container */
};

#ifdef __cplusplus
}
#endif

#endif /*_QHASHTBL_H */
