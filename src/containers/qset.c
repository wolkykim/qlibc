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

/**
 * @file qset.c Mathematical Set Implementation
 * qset container is a mathemetical set implementation.
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "qinternal.h"
#include "containers/qset.h"


#ifndef _DOXYGEN_SKIP

static U64 __default_hash(const char *key);
static bool __get_index(qset_t *set, const char *key, size_t hash, size_t *index);
static bool __assign_node(qset_t *set, const char *key, size_t hash, size_t index);
static void __free_index(qset_t *set, size_t index);
static bool __set_contains(qset_t *set, const char *key, size_t hash);
static bool __set_add(qset_t *set, const char *key, size_t hash);
static void __relayout_nodes(qset_t *set, size_t start, short end_on_null);

#endif

/**
 * Create new qset_t container
 * 
 * @param size num of elements
 * @param hash hash function callback
 * @param options combination of initialization options.
 * 
 * @return a pointer of malloced qset_t container, otherwise returns NULL
 * @retval errno will be set in error condition.
 *   - QSET_MEMERR  : Malloc failure.
 *   - QSET_INVAL  : Invalid argument.
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
qset_t *qset(size_t size, qset_hashfunction_t hash, int options) {
    if(size == 0) {
        errno = QSET_INVAL;
        return NULL;
    }
    qset_t *set = (qset_t *) calloc(1, sizeof(qset_t));
    if (set == NULL) {
        errno = QSET_MEMERR;
        return NULL;
    }
    else {    
        set->nodes = (qset_obj_t**) malloc(size * sizeof(qset_obj_t*));
        if (set->nodes == NULL) {
            errno = QSET_MEMERR;
            return NULL;
        }     
        set->num_nodes = size;
        for (size_t i = 0; i < set->num_nodes; i++) {
            set->nodes[i] = NULL;
        }
        set->used_nodes = 0;    
        set->hash_func = (hash == NULL)? &__default_hash : hash;
    }

    if (options & QSET_THREADSAFE) {
        Q_MUTEX_NEW(set->qmutex, true);
        if (set->qmutex == NULL) {
            errno = QSET_MEMERR;
            free(set);
            return NULL;
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
    set->free = qset_free;

    return set;
}

/**
 * @brief Insert an element at the end of this set.
 * 
 * @param set       qset_t container pointer
 * @param key       a string to be inserted
 * 
 * @return true if successful, otherwise refer to Return values
 * @retval based on the condition:
 * - non-distinct element: QSET_PRESENT
 * - full: QSET_CIRERR
 * - memory error: QSET_MEMERR     
 * 
 * CAUTION: QSET_CIRERR actually doesn't exist, but still there for security purpose.
 * 
 */
bool qset_add(qset_t *set, const char *key) {
    size_t hash = set->hash_func(key);
    return __set_add(set, key, hash);
}
/**
 * @brief Remove an element at the end of this set.
 * 
 * @param set qset_t container pointer
 * @param key a string to be removed
 * 
 * @return if removed, returns true, false if otherwise.
 */
bool qset_remove(qset_t *set, const char *key) {
    size_t index, hash = set->hash_func(key);
    bool pos = __get_index(set, key, hash, &index);
    if (!pos) {
        return pos;
    }
    qset_lock(set);
    __free_index(set, index);
    __relayout_nodes(set, index, 0);

    (set->used_nodes)--;
    qset_unlock(set);
    return true;
}
/**
 * @brief Check if key in set
 * 
 * @param set qset_t container pointer
 * @param key a string to be searched
 * 
 * @return true if successful, otherwise refer to Return values
 * 
 * @retval based on the condition:
 * - false if not found
 * - QSET_CIRERR of set is full and not found 
 * 
 */
bool qset_contains(qset_t *set, const char *key) {
    size_t index, hash = set->hash_func(key);
    return __get_index(set, key, hash, &index);      
}
/**
 * @brief Get the length of the set
 * 
 * @param set  qset_t container pointer
 * @return the length in unsigned int 64 bit
 */
size_t qset_length(qset_t *set) {
    return set->used_nodes;
}

size_t qset_size(qset_t *set) {
    return set->num_nodes;
}
/**
 * @brief The union of two sets a and b is the set that its element are either a or b. 
 * 
 * @param a lhs
 * @param b rhs
 * @return a new qset_t from the union of a and b
 */
qset_t *qset_union(qset_t *a, qset_t *b) {
    qset_t *c = (qset_t *) calloc(1, sizeof(qset_t));
    if (c == NULL) {
        errno = QSET_MEMERR;
        return NULL;
    }
    for (size_t i = 0; i < a->num_nodes; ++i) {
        if (a->nodes[i] != NULL) {
            __set_add(c, a->nodes[i]->key, a->nodes[i]->hash);
        }
    }
    for (size_t i = 0; i < b->num_nodes; ++i) {
        if (b->nodes[i] != NULL) {
            __set_add(c, b->nodes[i]->key, b->nodes[i]->hash);
        }
    }
    return c;
}
/**
 * @brief The intersection of two sets a and b is the set that its element are in both a or b. 
 * 
 * @param a lhs
 * @param b rhs
 * @return a new qset_t from the intersection of a and b
 */
qset_t *qset_intersection(qset_t *a, qset_t *b) {
    qset_t *c = (qset_t *) calloc(1, sizeof(qset_t));
    if (c == NULL) {
        errno = QSET_MEMERR;
        return NULL;
    }
    for (size_t i = 0; i < a->num_nodes; ++i) {
        if (a->nodes[i] == NULL) {
            if (__set_contains(b, a->nodes[i]->key, a->nodes[i]->hash)) {
                __set_add(c, a->nodes[i]->key, a->nodes[i]->hash);
            }
        } 
    }
    return c; 
}
/**
 * @brief The difference of two sets a and b is the set that consists the elements of A and not B.
 * 
 * @param a lhs
 * @param b rhs
 * @return a new qset_t from the difference of a and b
 */
qset_t *qset_difference(qset_t *a, qset_t *b) {
    qset_t *c = (qset_t *) calloc(1, sizeof(qset_t));
    if (c == NULL) {
        errno = QSET_MEMERR;
        return NULL;
    }
    for (size_t i = 0; i < a->num_nodes; ++i) {
        if (a->nodes[i] != NULL) {
            if (__set_contains(b, a->nodes[i]->key, a->nodes[i]->hash)) {
               __set_add(c, a->nodes[i]->key, a->nodes[i]->hash);
            }
        }
    }
    return c; 
}
/**
 * @brief The symmetric difference of two sets a and b is the set that consists the elements of A and B but not both.
 * 
 * @param a lhs
 * @param b rhs
 * @return a new qset_t from the difference of a and b
 */
qset_t *qset_symmetric_difference(qset_t *a, qset_t *b) {
    qset_t *c = (qset_t *) calloc(1, sizeof(qset_t));
    if (c == NULL) {
        errno = QSET_MEMERR;
        return NULL;
    }
    for (size_t i = 0; i < a->num_nodes; ++i) {
        if (a->nodes[i] != NULL) {
            if (__set_contains(b, a->nodes[i]->key, a->nodes[i]->hash)) {
                __set_add(c, a->nodes[i]->key, a->nodes[i]->hash);
            }
        }
    }


    for (size_t i = 0; i < b->num_nodes; ++i) {
        if (b->nodes[i] != NULL) {
            if (__set_contains(a, b->nodes[i]->key, b->nodes[i]->hash)) {
                __set_add(c, b->nodes[i]->key, b->nodes[i]->hash);
            }
        }
    }
    return c; 
}
/**
 * @brief a is a subset of b if every element of a is contained inside b
 * 
 * @param a left hand side
 * @param b right hand side
 * @return true if a is a subset of b, false if otherwise
 */
bool qset_is_subset(qset_t *a, qset_t *b) {
    for (size_t i; i < a->num_nodes;i++) {
        if (a->nodes[i] != NULL) {
            if (!__set_contains(b, a->nodes[i]->key, a->nodes[i]->hash)) {
                return false;
            }
        }
    }

    return true;
}
/**
 * @brief a is a superset of b if every element of b is contained inside a
 * 
 * @param a left hand side
 * @param b right hand side
 * @return true if a is a superset of b, false if otherwise
 */
bool qset_is_superset(qset_t *a, qset_t *b) {
    return qset_is_subset(b, a);
}
/**
 * @brief a is a strict subset of b if all elements of a in b but both two sets can't be equal
 * 
 * @param a left hand side
 * @param b right hand side
 * @return true if a is a strict subset of b, false if otherwise
 */
bool qset_is_strsubset(qset_t *a, qset_t *b) {
    if (a->used_nodes >= b->used_nodes) {
        return false;
    }

    return qset_is_subset(a, b);
}
/**
 * @brief a is a strict superset of a if all elements of b in a but both two sets can't be equal
 * 
 * @param a left hand side
 * @param b right hand side
 * @return true if a is a strict superset of b, false if otherwise
 */
bool qset_is_strsuperset(qset_t *a, qset_t *b) {
    return qset_is_strsubset(b,a);
}

/**
 * @brief comparing two sets a and b based on their size and their keys
 * 
 * @param a left hand side
 * @param b right hand side
 * @return 
 *  if both sets have the different size:
 *      - QSET_RGREATER if a < b
 *      - QSET_LGREATER if a > b
 *  if both sets have the same size:
 *      - QSET_NEQ if elements are different
 *      - QSET_EQ if both sets have same elements
 */
qset_cmp_t qset_cmp(qset_t *a, qset_t *b) {
    if (a->used_nodes < b->used_nodes) {
        return QSET_CMP_RGREATER;
    }
    else if (b->used_nodes < a->used_nodes) {
        return QSET_CMP_LGREATER;
    }

    for (size_t i = 0; i < a->used_nodes; ++i) {
        if (a->nodes[i] != NULL) {
            if (!qset_contains(b, a->nodes[i]->key)) {
                return QSET_CMP_NEQ;
            }
        }
    }

    return QSET_CMP_EQ;
}

/**
 * @brief convert qset_t object to an array of stirng
 * 
 * @param set qset_t container pointer
 * @param set_size if size is not NULL, the number of elements will be stored
 * @return an array of string if succeed, otherwise return NULL
 * @retval errno will be set in error condition.
 *   - QSET_EMPTY: Set is empty.
 *   - QSET_MEMERR: Malloc error.
 */
char **qset_toarray(qset_t *set, size_t *set_size) {
    if (set->used_nodes <= 0) {
        if (set_size != NULL) {
            *set_size = 0;
        }
        errno = QSET_EMPTY;
        return NULL;
    }
    qset_lock(set);
    char **results = (char **)calloc(set->used_nodes + 1, sizeof(char *));
    if (results != NULL) {
        errno = QSET_MEMERR;
        return NULL;
    }
    size_t i, j = 0;
    size_t len;
    for (i = 0; i < set->num_nodes; ++i) {
        if (set->nodes[i] != NULL) {
            len = strlen(set->nodes[i]->key);
            results[j] = (char *)calloc(len + 1, sizeof(char));
            memcpy(results[j], set->nodes[i]->key, len);
            ++j;
        }
    }
    qset_unlock(set);
    return results;
}
/**
 * @brief Enters critical section
 * 
 * @param set qset_t container pointer.
 * 
 */
void qset_lock(qset_t *set) {
    Q_MUTEX_ENTER(set->qmutex);
}
/**
 * @brief Leaves critical section
 * 
 * @param set qset_t container pointer.
 * 
 */
void qset_unlock(qset_t *set) {
    Q_MUTEX_LEAVE(set->qmutex);
}

/**
 * @brief Remove all elements in this set
 * 
 * @param set qset_t container pointer
 */
void qset_clear(qset_t *set) {
    qset_lock(set);
    for (size_t i = 0; i < set->num_nodes; ++i) {
        if (set->nodes[i] != NULL) {
            __free_index(set, i);
        }
    }
    set->used_nodes = 0;
    qset_unlock(set);
}

/**
 * @brief Prints out stored elements for debugging purpose. (Not implemented yet)
 * 
 * @param set qset_t container pointer
 * @param out output stream FILE descriptor such like stdout, stderr
 * @return true if successful, otherwise returns false
 * @return errno will be set in error condition.
 *   - EIO : Invalid output stream
 */
bool qset_debug(qset_t *set, FILE *out) {
    if (out == NULL) {
        errno = EIO;
        return false;
    }

    set->lock(set);
    //TODO: IMPLEMENT THIS
    set->unlock(set);

    return true;
}

/**
 * @brief Free this set
 * 
 * @param set qset_t container pointer
 */
void qset_free(qset_t *set) {
    qset_lock(set);
    qset_clear(set);
    free(set->nodes);
    set->num_nodes = 0;
    set->used_nodes = 0;
    set->hash_func = NULL;
    qset_unlock(set);
}

#ifndef _DOXYGEN_SKIP

static U64 __default_hash(const char *key) {
    U64 i, len = strlen(key);
    U64 h = 14695981039346656037ULL; 
    for (i = 0; i < len; ++i) {
        h = h ^ (unsigned char) key[i];
        h = h * 1099511628211ULL; 
    }
    return h;
}
static bool __get_index(qset_t *set, const char *key, size_t hash, size_t *index) {
    size_t i, idx;
    idx = hash % set->num_nodes;
    i = idx;
    size_t len = strlen(key);
    qset_lock(set);
    while (1) {
        if (set->nodes[i] == NULL) {
            *index = i;
            qset_unlock(set);
            return false; 
        } else if (hash == set->nodes[i]->hash && len == strlen(set->nodes[i]->key) && strncmp(key, set->nodes[i]->key, len) == 0) {
            *index = i;
            qset_unlock(set);
            return true;
        }
        ++i;
        if (i == set->num_nodes)
            i = 0;
        if (i == idx) 
            errno = QSET_CIRERR;
            qset_unlock(set);
            return false;
    }
}
static bool __assign_node(qset_t *set, const char *key, size_t hash, size_t index) {
    size_t len = strlen(key);
    qset_lock(set);
    set->nodes[index] = (qset_obj_t*)malloc(sizeof(qset_obj_t));
    set->nodes[index]->key = (char*)calloc(len + 1, sizeof(char));
    memcpy(set->nodes[index]->key, key, len);
    set->nodes[index]->hash = hash;
    qset_unlock(set);
    return true;
}
static void __free_index(qset_t *set, size_t index) {
    qset_lock(set);
    free(set->nodes[index]->key);
    free(set->nodes[index]);
    set->nodes[index] = NULL;
    qset_unlock(set);
}
static bool __set_contains(qset_t *set, const char *key, size_t hash) {
    size_t index;
    return __get_index(set, key, hash, &index);
}
static bool __set_add(qset_t *set, const char *key, size_t hash) {
    size_t index;
    qset_lock(set);
    if (__set_contains(set, key, hash))
        errno = QSET_PRESENT;
        return false;

    if ((float)set->used_nodes / set->num_nodes > MAX_FULLNESS_PERCENT) {
        size_t size = set->num_nodes * 2; 
        qset_obj_t** tmp = (qset_obj_t**)realloc(set->nodes, size * sizeof(qset_obj_t*));
        if (tmp == NULL || set->nodes == NULL) 
            errno = QSET_MEMERR;
            return false;

        set->nodes = tmp;
        size_t i, orig_size = set->num_nodes;
        for (i = orig_size; i < size; ++i)
            set->nodes[i] = NULL;

        set->num_nodes = size;
        
        __relayout_nodes(set, 0, 1);
    }
    bool res = __get_index(set, key, hash, &index);
    if (res == false) { 
        __assign_node(set, key, hash, index);
        ++set->used_nodes;
        return true;
    }
    qset_unlock(set);
    return res;
}
static void __relayout_nodes(qset_t *set, size_t start, short end_on_null) {
    size_t index = 0, i;
    qset_lock(set);
    for (i = start; i < set->num_nodes; ++i) {
        if(set->nodes[i] != NULL) {
            __get_index(set, set->nodes[i]->key, set->nodes[i]->hash, &index);
            if (i != index) { // we are moving this node
                __assign_node(set, set->nodes[i]->key, set->nodes[i]->hash, index);
                __free_index(set, i);
            }
        } else if (end_on_null == 0 && i != start) {
            break;
        }
    }
    qset_unlock(set);
}

#endif