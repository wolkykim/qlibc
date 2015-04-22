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
 * Grow container that handles growable objects.
 *
 * @file qgrow.h
 */

#ifndef QGROW_H
#define QGROW_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "qlist.h"

#ifdef __cplusplus
extern "C" {
#endif

/* types */
typedef struct qgrow_s qgrow_t;

/* public functions */
enum {
    QGROW_THREADSAFE = (QLIST_THREADSAFE)  /*!< make it thread-safe */
};

extern qgrow_t *qgrow(int options);

extern bool qgrow_add(qgrow_t *grow, const void *object, size_t size);
extern bool qgrow_addstr(qgrow_t *grow, const char *str);
extern bool qgrow_addstrf(qgrow_t *grow, const char *format, ...);

extern size_t qgrow_size(qgrow_t *grow);
extern size_t qgrow_datasize(qgrow_t *grow);

extern void *qgrow_toarray(qgrow_t *grow, size_t *size);
extern char *qgrow_tostring(qgrow_t *grow);

extern void qgrow_clear(qgrow_t *grow);
extern bool qgrow_debug(qgrow_t *grow, FILE *out);
extern void qgrow_free(qgrow_t *grow);

/**
 * qgrow container object
 */
struct qgrow_s {
    /* capsulated member functions */
    bool (*add) (qgrow_t *grow, const void *data, size_t size);
    bool (*addstr) (qgrow_t *grow, const char *str);
    bool (*addstrf) (qgrow_t *grow, const char *format, ...);

    size_t (*size) (qgrow_t *grow);
    size_t (*datasize) (qgrow_t *grow);

    void *(*toarray) (qgrow_t *grow, size_t *size);
    char *(*tostring) (qgrow_t *grow);

    void (*clear) (qgrow_t *grow);
    bool (*debug) (qgrow_t *grow, FILE *out);

    void (*free) (qgrow_t *grow);

    /* private variables - do not access directly */
    qlist_t *list;  /*!< data container */
};

#ifdef __cplusplus
}
#endif

#endif /* QGROW_H */
