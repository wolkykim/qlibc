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
 * Mathematical Set
 *
 * @file qset.h
 */
#ifndef QSET_H
#define QSET_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define U64 uint64_t
#define MAX_FULLNESS_PERCENT 0.25

/* types */
typedef struct qset_s qset_t;
typedef struct qset_obj_s qset_obj_t;
typedef U64 (*qset_hashfunction_t)(const char *key);
typedef enum qset_cmp_e qset_cmp_t;

/* public functions */
enum {
    QSET_THREADSAFE = (0x01), /*!< make it thread-safe */ 
};

enum qset_cmp_e {
    QSET_CMP_RGREATER,  /*      qset right greater   */
    QSET_CMP_LGREATER,  /*      qset left greater    */
    QSET_CMP_EQ,        /*      qset equal           */
    QSET_CMP_NEQ,       /*      qset not equal       */ 
};

extern qset_t *qset(size_t num_els, qset_hashfunction_t hash, int options);

extern bool qset_add(qset_t *set, const char *key);
extern bool qset_remove(qset_t *set, const char *key);
extern bool qset_contains(qset_t *set, const char *key);
extern size_t qset_length(qset_t *set);

extern qset_t *qset_union(qset_t *a, qset_t *b);
extern qset_t *qset_intersection(qset_t *a, qset_t *b);
extern qset_t *qset_difference(qset_t *a, qset_t *b);
extern qset_t *qset_symmetric_difference(qset_t *a, qset_t *b);

extern bool qset_is_subset(qset_t *a, qset_t *b);
extern bool qset_is_superset(qset_t *a, qset_t *b);
extern bool qset_is_strsubset(qset_t *a, qset_t *b);
extern bool qset_is_strsuperset(qset_t *a, qset_t *b);

extern qset_cmp_t qset_cmp(qset_t *a, qset_t *b);

extern char **qset_toarray(qset_t *set, size_t *set_size);
extern void qset_lock(qset_t *set);
extern void qset_unlock(qset_t *set);

extern void qset_clear(qset_t *set);
extern void qset_free(qset_t *set);

struct qset_obj_s {
    char *key;
    U64 hash;
};

struct qset_s {

    bool (*add)(qset_t*, const char*);
    bool (*remove)(qset_t*, const char*);
    bool (*contains)(qset_t*, const char*);
    size_t (*length)(qset_t*);
    void *(*toarray)(qset_t*, size_t*);
    void (*lock)(qset_t*);
    void (*unlock)(qset_t*);
    void (*clear)(qset_t*);
    void (*free)(qset_t*);

    /* private variables - do not access directly */
    void *qmutex;
    qset_obj_t **nodes;
    size_t num_nodes;
    size_t used_nodes;
    qset_hashfunction_t hash_func;
};


#ifdef __cplusplus
}
#endif

#endif
