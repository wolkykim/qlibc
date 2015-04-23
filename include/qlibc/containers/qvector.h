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
/* This code is written and updated by following people and released under
 * the same license as above qLibc license.
 * Copyright (c) 2015 Zhenjiang Xie - https://github.com/Charles0429
 *****************************************************************************/

/**
 * Vector container.
 *
 * @file qvector.h
 */

#ifndef QVECTOR_H
#define QVECTOR_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* types */
typedef struct qvector_s qvector_t;
typedef struct qvector_obj_s qvector_obj_t;

/* public functions */
enum {
    QVECTOR_THREADSAFE = (0x01),  /*!< make it thread-safe */
    QVECTOR_RESIZE_DOUBLE = (0x02), /*!< double the size when vector is full*/
    QVECTOR_RESIZE_LINEAR = (0x04), /*!< add the size with initial num when vector is full*/
    QVECTOR_RESIZE_EXACT = (0x08) /*!< add up as much as needed*/
};

extern qvector_t *qvector(size_t max, size_t objsize, int options);

extern bool qvector_addfirst(qvector_t *vector, const void *data);
extern bool qvector_addlast(qvector_t *vector, const void *data);
extern bool qvector_addat(qvector_t *vector, int index, const void *data);

extern void *qvector_getfirst(qvector_t *vector, bool newmem);
extern void *qvector_getlast(qvector_t *vector, bool newmem);
extern void *qvector_getat(qvector_t *vector, int index, bool newmem);

extern bool qvector_setfirst(qvector_t *vector, const void *data);
extern bool qvector_setlast(qvector_t *vector, const void *data);
extern bool qvector_setat(qvector_t *vector, int index, const void *data);

extern void *qvector_popfirst(qvector_t *vector);
extern void *qvector_poplast(qvector_t *vector);
extern void *qvector_popat(qvector_t *vector, int index);

extern bool qvector_removefirst(qvector_t *vector);
extern bool qvector_removelast(qvector_t *vector);
extern bool qvector_removeat(qvector_t *vector, int index);

extern size_t qvector_size(qvector_t *vector);
extern bool qvector_resize(qvector_t *vector, size_t newmax);

extern void *qvector_toarray(qvector_t *vector, size_t *size);

extern void qvector_lock(qvector_t *vector);
extern void qvector_unlock(qvector_t *vector);

extern void qvector_clear(qvector_t *vector);
extern bool qvector_debug(qvector_t *vector, FILE *out);
extern void qvector_free(qvector_t *vector);

extern void qvector_reverse(qvector_t *vector);
extern bool qvector_getnext(qvector_t *vector, qvector_obj_t *obj, bool newmem);

/**
 * qvector container object
 */
struct qvector_s {
    /* capsulated member functions */

    bool (*addfirst)(qvector_t *vector, const void *object);
    bool (*addlast)(qvector_t *vector, const void *data);
    bool (*addat)(qvector_t *vector, int index, const void *data);

    void *(*getfirst)(qvector_t *vector, bool newmem);
    void *(*getlast)(qvector_t *vector, bool newmem);
    void *(*getat)(qvector_t *vector, int index, bool newmem);

    bool (*setfirst)(qvector_t *vector, const void *data);
    bool (*setlast)(qvector_t *vector, const void *data);
    bool (*setat)(qvector_t *vector, int index, const void *data);

    void *(*popfirst)(qvector_t *vector);
    void *(*poplast)(qvector_t *vector);
    void *(*popat)(qvector_t *vector, int index);

    bool (*removefirst)(qvector_t *vector);
    bool (*removelast)(qvector_t *vector);
    bool (*removeat)(qvector_t *vector, int index);

    size_t (*size)(qvector_t *vector);
    bool   (*resize)(qvector_t *vector, size_t newmax);

    void *(*toarray)(qvector_t *vector, size_t *size);

    void (*lock)(qvector_t *vector);
    void (*unlock)(qvector_t *vector);

    void (*clear)(qvector_t *vector);
    bool (*debug)(qvector_t *vector, FILE *out);
    void (*free)(qvector_t *vector);

    void (*reverse)(qvector_t *vector);
    bool (*getnext)(qvector_t *vector, qvector_obj_t *obj, bool newmem);

    /* private variables - do not access directly */
    void *qmutex;
    void *data;
    size_t num; /*number of elements*/
    size_t objsize; /*the size of each element*/
    size_t max; /*allocated number of elements*/
    int options;
    size_t initnum;
};

struct qvector_obj_s { 
    void *data;
    int index;
};

#ifdef __cplusplus
}
#endif

#endif /* QVECTOR_H */
