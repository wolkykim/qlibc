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
 * SUBSTITUTE GOODS OR SERVICES {} LOSS OF USE, DATA, OR PROFITS {} OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "qinternal.h"
#include "containers/qset.h"

#ifndef _DOXYGEN_SKIP
static uint64_t __default_hash(const char *key);
static int __get_index(qset_t *set, const char *key, uint64_t hash, uint64_t *index);
static int __assign_node(qset_t *set, const char *key, uint64_t hash, uint64_t index);
static void __free_index(qset_t *set, uint64_t index);
static int __set_contains(qset_t *set, const char *key, uint64_t hash);
static int __set_add(qset_t *set, const char *key, uint64_t hash);
static void __relayout_nodes(qset_t *set, uint64_t start, short end_on_null);

static void __set_union(qset_t *a, qset_t *b); 
static void __set_intersection(qset_t *a, qset_t *b);
static void __set_difference(qset_t *a, qset_t *b);
static void __set_symmetric_difference(qset_t *a, qset_t *b);

static int __set_is_subset(qset_t *a , qset_t *b);
static int __set_is_superset(qset_t *a, qset_t *b);
static int __set_is_strsubset(qset_t *a, qset_t *b);
static int __set_is_strsuperset(qset_t *a, qset_t *b);
#endif

/**
 * Create new qset_t container
 * 
 * @param num_els number of elements
 * @param hash hash function callback
 * @param options combination of initialization options.
 * 
 * @return a pointer of malloced qset_t container, otherwise returns NULL
 * @retval errno will be set in error condition.
 *   - ENOMEM and QSET_MALLERR : Malloc failure.
 *   - EINVAL : Invalid argument.
 * 
 * @code
 *   qset_t *set = qset(10, hash, 0);
 * @endcode
 * 
 * @note
 *   Available options:
 *   - QSET_THREADSAFE - make it thread-safe.
 * 
 */
qset_t *qset(uint64_t num_els, qset_hashfunction hash, int options) {
    if(num_els == 0) {
        errno = EINVAL;
        return NULL;
    }

    qset_t *set = (qset_t *) calloc(1, sizeof(qset_t));
    if (set == NULL) {
        errno = ENOMEM;
        return QSET_MALLERR;
    }
    else {    
        set->nodes = (qset_obj_t**) malloc(num_els * sizeof(qset_obj_t*));
        if (set->nodes == NULL) {
            errno = ENOMEM;
            return QSET_MALLERR;
        }     
        set->num_nodes = num_els;
        for (uint64_t i = 0; i < set->num_nodes; i++) {
            set->nodes[i] = NULL;
        }
        set->used_nodes = 0;    
        set->hash_func = (hash == NULL)? &__default_hash : hash;
    }

    if (options & QSET_THREADSAFE) {
        Q_MUTEX_NEW(set->qmutex, true);
        if (set->qmutex == NULL) {
            free(set);
            errno = ENOMEM;
            return QSET_MALLERR;
        }
    }

    set->add = qset_add;
    set->remove = qset_remove;
    set->contains = qset_contains;
    set->length = qset_length;
    
    set->toarray = qset_toarray;
    set->lock = qset_lock;
    set->unlock = qset_unlock;

    set->clear = qset_clear;
    set->debug = qset_debug;
    set->free = qset_free;

    return set;
}

/**
 * qset->add(): Insert an element at the end of this set.
 * 
 * @param set       qset_t container pointer
 * @param key       a string to be inserted
 * 
 * @return true if successful, otherwise false
 * @retval based on the condition:
 * - successful: QSET_TRUE
 * - non-distinct element: QSET_PRESENT
 * - full: QSET_CIRERR
 * - memory error: QSET_MALLERR     
 * 
 */
extern int qset_add(qset_t *set, const char *key) {
    uint64_t hash = set->hash_func(key);
    return __set_add(set, key, hash);
}
extern int qset_remove(qset_t *set, const char *key) {
    uint64_t index, hash = set->hash_func(key);
    int pos = __get_index(set, key, hash, &index);
    if (pos != QSET_TRUE) {
        return pos;
    }

    __free_index(set, index);
    __relayout_nodes(set, index, 0);

    (set->used_nodes)--;

    return QSET_TRUE;

}
extern int qset_contains(qset_t *set, const char *key) {
    uint64_t index, hash = set->hash_func(key);
    return __get_index(set, key, hash, &index);      
}
extern uint64_t qset_length(qset_t *set) {
    return set->used_nodes;
}

extern qset_t *qset_union(qset_t *a, qset_t *b) {
    qset_t *c = (qset_t *) calloc(1, sizeof(qset_t));
    if (c->used_nodes != 0) {
        return QSET_OCCERR;
    }
    __set_union(a,b);
    return c;
}
extern qset_t *qset_intersection(qset_t *a, qset_t *b) {
    qset_t *c = (qset_t *) calloc(1, sizeof(qset_t));
    if (c->used_nodes != 0) {
        return QSET_OCCERR;
    }
    __set_intersection(a,b);
    return c; 
}
extern qset_t *qset_difference(qset_t *a, qset_t *b) {
    qset_t *c = (qset_t *) calloc(1, sizeof(qset_t));
    if (c->used_nodes != 0) {
        return QSET_OCCERR;
    }
    __set_difference(a,b);
    return c; 
}
extern qset_t *qset_symmetric_difference(qset_t *a, qset_t *b) {
    qset_t *c = (qset_t *) calloc(1, sizeof(qset_t));
    if (c->used_nodes != 0) {
        return QSET_OCCERR;
    }
    __set_symmetric_difference(a,b);
    return c; 

}

extern int qset_is_subset(qset_t *a, qset_t *b) {}
extern int qset_is_superset(qset_t *a, qset_t *b) {}
extern int qset_is_strsubset(qset_t *a, qset_t *b) {}
extern int qset_is_strsuperset(qset_t *a, qset_t *b) {}

extern int qset_cmp(qset_t *a, qset_t *b) {}

extern char **qset_toarray(qset_t *set, uint64_t *setsize) {}
extern void qset_lock(qset_t *set) {}
extern void qset_unlock(qset_t *set) {}

extern void qset_clear(qset_t *set) {}
extern bool qset_debug(qset_t *set, FILE *out) {}
extern void qset_free(qset_t *set) {}

