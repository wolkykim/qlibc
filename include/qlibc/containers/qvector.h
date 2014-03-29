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
 * Vector container that handles growable objects.
 *
 * @file qvector.h
 */

#ifndef _QVECTOR_H
#define _QVECTOR_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "qlist.h"

#ifdef __cplusplus
extern "C" {
#endif

/* types */
typedef struct qvector_s qvector_t;

/* public functions */
enum {
    QVECTOR_THREADSAFE = (QLIST_THREADSAFE)  /*!< make it thread-safe */
};

extern qvector_t *qvector(int options);

/**
 * qvector container object
 */
struct qvector_s {
    /* capsulated member functions */
    bool (*add) (qvector_t *vector, const void *data, size_t size);
    bool (*addstr) (qvector_t *vector, const char *str);
    bool (*addstrf) (qvector_t *vector, const char *format, ...);

    void *(*toarray) (qvector_t *vector, size_t *size);
    char *(*tostring) (qvector_t *vector);

    size_t (*size) (qvector_t *vector);
    size_t (*datasize) (qvector_t *vector);
    void (*clear) (qvector_t *vector);
    bool (*debug) (qvector_t *vector, FILE *out);

    void (*free) (qvector_t *vector);

    /* private variables - do not access directly */
    qlist_t *list;  /*!< data container */
};

#ifdef __cplusplus
}
#endif

#endif /*_QVECTOR_H */
