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
 * Defines basic object types that are commonly used in various containers.
 *
 * @file qtype.h
 */

#ifndef _QTYPE_H
#define _QTYPE_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* types */
typedef struct qmutex_s qmutex_t;    /*!< qlibc pthread mutex type*/
typedef struct qobj_s qobj_t;        /*!< object type*/
typedef struct qnobj_s qnobj_t;      /*!< named-object type*/
typedef struct qdlobj_s qdlobj_t;    /*!< doubly-linked-object type*/
typedef struct qdlnobj_s qdlnobj_t;  /*!< doubly-linked-named-object type*/
typedef struct qhnobj_s qhnobj_t;    /*!< hashed-named-object type*/

/**
 * qlibc pthread mutex data structure.
 */
struct qmutex_s {
    pthread_mutex_t mutex;  /*!< pthread mutex */
    pthread_t owner;        /*!< mutex owner thread id */
    int count;              /*!< recursive lock counter */
};

/**
 * object data structure.
 */
struct qobj_s {
    void *data;         /*!< data */
    size_t size;        /*!< data size */
    uint8_t type;       /*!< data type */
};

/**
 * named-object data structure.
 */
struct qnobj_s {
    char *name;         /*!< object name */
    void *data;         /*!< data */
    size_t size;        /*!< data size */
};

/**
 * doubly-linked-object data structure.
 */
struct qdlobj_s {
    void *data;         /*!< data */
    size_t size;        /*!< data size */

    qdlobj_t *prev;     /*!< previous link */
    qdlobj_t *next;     /*!< next link */
};

/**
 * doubly-linked-named-object data structure.
 */
struct qdlnobj_s {
    uint32_t hash;      /*!< 32bit-hash value of object name */
    char *name;         /*!< object name */
    void *data;         /*!< data */
    size_t size;        /*!< data size */

    qdlnobj_t *prev;    /*!< previous link */
    qdlnobj_t *next;    /*!< next link */
};

/**
 * hashed-named-object data structure.
 */
struct qhnobj_s {
    uint32_t hash;      /*!< 32bit-hash value of object name */
    char *name;         /*!< object name */
    void *data;         /*!< data */
    size_t size;        /*!< data size */

    qhnobj_t *next;     /*!< for chaining next collision object */
};

#ifdef __cplusplus
}
#endif

#endif /*_QTYPE_H */

