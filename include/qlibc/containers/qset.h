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

#ifdef __cplusplus
extern "C" {
#endif

/* types */
typedef struct qset_s qset_t;
typedef struct qset_obj_s qset_obj_t;
typedef uint64_t (*qset_hashfunction)(const char *key);

/* public functions */
enum {
    QSET_THREADSAFE = (0x01), /*!< make it thread-safe */ 
};

#define QSET_TRUE 0
#define QSET_FALSE -1

#define QSET_MALLERR -2 /* MALLOC ERROR - ENOMEM*/
#define QSET_CIRERR -3 /* CIRCULAR ERROR */
#define QSET_OCCERR -4 /* OCCUPIED ERROR */
#define QSET_PRESENT 1 /* ALREADY PRESENT */

#define QSET_RGREATER 3 /* RIGHT GREATER */
#define QSET_LGREATER 1 /* LEFT GREATER */
#define QSET_EQ 0 /* EQUAL */
#define QSET_NEQ 2 /* NOT EQUAL */

extern qset_t *qset(uint64_t num_els, qset_hashfunction hash, int options);


extern int qset_add(qset_t *set, const char *key);
extern int qset_remove(qset_t *set, const char *key);
extern int qset_contains(qset_t *set, const char *key);
extern uint64_t qset_length(qset_t *set);

extern qset_t *qset_union(qset_t *a, qset_t *b);
extern qset_t *qset_intersection(qset_t *a, qset_t *b);
extern qset_t *qset_difference(qset_t *a, qset_t *b);
extern qset_t *qset_symmetric_difference(qset_t *a, qset_t *b);

extern int qset_is_subset(qset_t *a, qset_t *b);
extern int qset_is_superset(qset_t *a, qset_t *b);
extern int qset_is_strsubset(qset_t *a, qset_t *b);
extern int qset_is_strsuperset(qset_t *a, qset_t *b);

extern int qset_cmp(qset_t *a, qset_t *b);

extern char **qset_toarray(qset_t *set, uint64_t *set_size);
extern void qset_lock(qset_t *set);
extern void qset_unlock(qset_t *set);

extern void qset_clear(qset_t *set);
extern void qset_free(qset_t *set);


struct qset_obj_s {
    char *key;
    uint64_t hash;
};

struct qset_s {

    int (*add)(qset_t*, const char*);
    int (*remove)(qset_t*, const char*);
    int (*contains)(qset_t*, const char*);
    uint64_t (*length)(qset_t*);
    void *(*toarray)(qset_t*, uint64_t*);
    void (*lock)(qset_t*);
    void (*unlock)(qset_t*);
    void (*clear)(qset_t*);
    void (*free)(qset_t*);

    /* private variables - do not access directly */
    void *qmutex;
    qset_obj_t **nodes;
    uint64_t num_nodes;
    uint64_t used_nodes;

    qset_hashfunction hash_func;
};


#ifdef __cplusplus
}
#endif

#endif
