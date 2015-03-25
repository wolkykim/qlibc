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
 * LIFO Stack container.
 *
 * @file qstack.h
 */

#ifndef QSTACK_H
#define QSTACK_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "qlist.h"

#ifdef __cplusplus
extern "C" {
#endif

/* types */
typedef struct qstack_s qstack_t;

enum {
    QSTACK_THREADSAFE = (QLIST_THREADSAFE)  /*!< make it thread-safe */
};

/* member functions
 *
 * All the member functions can be accessed in both ways:
 *  - tbl->push(tbl, ...);    // easier to switch the container type to other kinds.
 *  - qstack_push(tbl, ...);  // where avoiding pointer overhead is preferred.
 */
extern qstack_t *qstack(int options);
extern size_t qstack_setsize(qstack_t *stack, size_t max);

extern bool qstack_push(qstack_t *stack, const void *data, size_t size);
extern bool qstack_pushstr(qstack_t *stack, const char *str);
extern bool qstack_pushint(qstack_t *stack, int64_t num);

extern void *qstack_pop(qstack_t *stack, size_t *size);
extern char *qstack_popstr(qstack_t *stack);
extern int64_t qstack_popint(qstack_t *stack);
extern void *qstack_popat(qstack_t *stack, int index, size_t *size);

extern void *qstack_get(qstack_t *stack, size_t *size, bool newmem);
extern char *qstack_getstr(qstack_t *stack);
extern int64_t qstack_getint(qstack_t *stack);
extern void *qstack_getat(qstack_t *stack, int index, size_t *size, bool newmem);

extern size_t qstack_size(qstack_t *stack);
extern void qstack_clear(qstack_t *stack);
extern bool qstack_debug(qstack_t *stack, FILE *out);
extern void qstack_free(qstack_t *stack);

/**
 * qstack container object structure
 */
struct qstack_s {
    /* encapsulated member functions */
    size_t (*setsize) (qstack_t *stack, size_t max);

    bool (*push) (qstack_t *stack, const void *data, size_t size);
    bool (*pushstr) (qstack_t *stack, const char *str);
    bool (*pushint) (qstack_t *stack, int64_t num);

    void *(*pop) (qstack_t *stack, size_t *size);
    char *(*popstr) (qstack_t *stack);
    int64_t (*popint) (qstack_t *stack);
    void *(*popat) (qstack_t *stack, int index, size_t *size);

    void *(*get) (qstack_t *stack, size_t *size, bool newmem);
    char *(*getstr) (qstack_t *stack);
    int64_t (*getint) (qstack_t *stack);
    void *(*getat) (qstack_t *stack, int index, size_t *size, bool newmem);

    size_t (*size) (qstack_t *stack);
    void (*clear) (qstack_t *stack);
    bool (*debug) (qstack_t *stack, FILE *out);
    void (*free) (qstack_t *stack);

    /* private variables - do not access directly */
    qlist_t  *list;  /*!< data container */
};

#ifdef __cplusplus
}
#endif

#endif /* QSTACK_H */
