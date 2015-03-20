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
 * Tree Table container that implements "Left-Leaning Red-Black" Binary Search Tree algorithm.
 *
 * @file qtreetbl.h
 */

#ifndef QTREETBL_H
#define QTREETBL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* types */
typedef struct qtreetbl_s qtreetbl_t;
typedef struct qtreetbl_obj_s qtreetbl_obj_t;

/* public functions */
enum {
    QTREETBL_THREADSAFE = (0x01) /*!< make it thread-safe */
};

extern qtreetbl_t *qtreetbl(int options); /*!< qtreetbl constructor */

/* member functions
 *
 * All the member functions can be accessed in both ways:
 *  - tbl->put(tbl, ...);      // easier to switch the container type to other kinds.
 *  - qtreetbl_put(tbl, ...);  // where avoiding pointer overhead is preferred.
 */
extern void qtreetbl_set_compare(
        qtreetbl_t *tbl,
        int (*cmp)(const void *name1, size_t namesize1, const void *name2,
                   size_t namesize2));

extern bool qtreetbl_put(qtreetbl_t *tbl, const char *name, const void *data,
                         size_t datasize);
extern bool qtreetbl_putstr(qtreetbl_t *tbl, const char *name, const char *str);
extern bool qtreetbl_putstrf(qtreetbl_t *tbl, const char *name,
                             const char *format, ...);
extern bool qtreetbl_put_by_obj(qtreetbl_t *tbl, const void *name,
                                size_t namesize, const void *data,
                                size_t datasize);

extern void *qtreetbl_get(qtreetbl_t *tbl, const char *name, size_t *datasize,
bool newmem);
extern char *qtreetbl_getstr(qtreetbl_t *tbl, const char *name,
                             const bool newmem);
extern void *qtreetbl_get_by_obj(qtreetbl_t *tbl, const char *name,
                                 size_t namesize, size_t *datasize, bool newmem);

extern bool qtreetbl_remove(qtreetbl_t *tbl, const char *name);
extern bool qtreetbl_remove_by_obj(qtreetbl_t *tbl, const void *name,
                                   size_t namesize);

extern bool qtreetbl_getnext(qtreetbl_t *tbl, qtreetbl_obj_t *obj,
                             const bool newmem);

extern void *qtreetbl_find_min(qtreetbl_t *tbl, size_t *namesize);
extern void *qtreetbl_find_max(qtreetbl_t *tbl, size_t *namesize);
extern qtreetbl_obj_t qtreetbl_find_nearest(qtreetbl_t *tbl, const void *name,
                                            size_t namesize, bool newmem);

extern size_t qtreetbl_size(qtreetbl_t *tbl);
extern void qtreetbl_clear(qtreetbl_t *tbl);

extern void qtreetbl_lock(qtreetbl_t *tbl);
extern void qtreetbl_unlock(qtreetbl_t *tbl);

extern void qtreetbl_free(qtreetbl_t *tbl);

extern int qtreetbl_byte_cmp(const void *name1, size_t namesize1,
                             const void *name2, size_t namesize2);
extern bool qtreetbl_debug(qtreetbl_t *tbl, FILE *out);

/**
 * qtreetbl container object
 */
struct qtreetbl_s {
    /* encapsulated member functions */
    void (*set_compare)(
            qtreetbl_t *tbl,
            int (*cmp)(const void *name1, size_t namesize1, const void *name2,
                       size_t namesize2));
    bool (*put)(qtreetbl_t *tbl, const char *name, const void *data,
                size_t size);
    bool (*putstr)(qtreetbl_t *tbl, const char *name, const char *str);
    bool (*putstrf)(qtreetbl_t *tbl, const char *name, const char *format, ...);
    bool (*put_by_obj)(qtreetbl_t *tbl, const void *name, size_t namesize,
                       const void *data, size_t datasize);

    void *(*get)(qtreetbl_t *tbl, const char *name, size_t *datasize,
    bool newmem);
    char *(*getstr)(qtreetbl_t *tbl, const char *name, bool newmem);
    void *(*get_by_obj)(qtreetbl_t *tbl, const char *name, size_t namesize,
                        size_t *datasize, bool newmem);

    bool (*remove)(qtreetbl_t *tbl, const char *name);
    bool (*remove_by_obj)(qtreetbl_t *tbl, const void *name, size_t namesize);

    bool (*getnext)(qtreetbl_t *tbl, qtreetbl_obj_t *obj, const bool newmem);

    void *(*find_min)(qtreetbl_t *tbl, size_t *namesize);
    void *(*find_max)(qtreetbl_t *tbl, size_t *namesize);
    qtreetbl_obj_t (*find_nearest)(qtreetbl_t *tbl, const void *name,
                                   size_t namesize, bool newmem);

    size_t (*size)(qtreetbl_t *tbl);
    void (*clear)(qtreetbl_t *tbl);
    bool (*debug)(qtreetbl_t *tbl, FILE *out);

    void (*lock)(qtreetbl_t *tbl);
    void (*unlock)(qtreetbl_t *tbl);

    void (*free)(qtreetbl_t *tbl);

    /* private member functions */
    int (*compare)(const void *name1, size_t namesize1, const void *name2,
                   size_t namesize2);

    /* private variables - do not access directly */
    void *qmutex;           /*!< initialized when QTREETBL_THREADSAFE is given */
    qtreetbl_obj_t *root;   /*!< root node */
    size_t num;             /*!< number of objects */
    uint8_t tid;            /*!< travel id sequencer */
};

/**
 * qtreetbl object data structure
 */
struct qtreetbl_obj_s {
    void *name;         /*!< name of key */
    size_t namesize;    /*!< name size */
    void *data;         /*!< data */
    size_t datasize;    /*!< data size */

    bool red;           /*!< true if upper link is red */
    qtreetbl_obj_t *left;   /*!< left node */
    qtreetbl_obj_t *right;  /*!< right node */

    qtreetbl_obj_t *next;   /*!< temporary use for tree traversal */
    uint8_t tid;            /*!< temporary use for tree traversal */
};

#ifdef __cplusplus
}
#endif

#endif /* QTREETBL_H */
