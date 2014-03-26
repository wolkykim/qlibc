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
 * Static Hash Table container that works in preallocated fixed size memory.
 *
 * @file qhasharr.h
 */

#ifndef _QHASHARR_H
#define _QHASHARR_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "qtype.h"

#ifdef __cplusplus
extern "C" {
#endif

/* tunable knobs */
#define _Q_HASHARR_KEYSIZE (16)    /*!< knob for maximum key size. */
#define _Q_HASHARR_VALUESIZE (32)  /*!< knob for maximum data size in a slot. */

/* types */
typedef struct qhasharr_slot_s qhasharr_slot_t;
typedef struct qhasharr_data_s qhasharr_data_t;
typedef struct qhasharr_s qhasharr_t;

/* public functions */
extern qhasharr_t *qhasharr(void *memory, size_t memsize);
extern size_t qhasharr_calculate_memsize(int max);

/**
 * qhasharr internal data slot structure
 */
struct qhasharr_slot_s {
    short  count;   /*!< hash collision counter. 0 indicates empty slot,
                     -1 is used for collision resolution, -2 is used for
                     indicating linked block */
    uint32_t  hash; /*!< key hash. we use FNV32 */

    uint8_t size;   /*!< value size in this slot*/
    int link;       /*!< next link */

    union {
        /*!< key/value data */
        struct _Q_HASHARR_SLOT_KEYVAL {
            unsigned char value[_Q_HASHARR_VALUESIZE];  /*!< value */

            char key[_Q_HASHARR_KEYSIZE];  /*!< key string, can be cut */
            uint16_t  keylen;              /*!< original key length */
            unsigned char keymd5[16];      /*!< md5 hash of the key */
        } pair;

        /*!< extended data block, used only when the count value is -2 */
        struct _Q_HASHARR_SLOT_EXT {
            unsigned char value[sizeof(struct _Q_HASHARR_SLOT_KEYVAL)];
        } ext;
    } data;
};

/**
 * qhasharr memory structure
 */
struct qhasharr_data_s {
    int maxslots;       /*!< number of maximum slots */
    int usedslots;      /*!< number of used slots */
    int num;            /*!< number of stored keys */
    qhasharr_slot_t *slots;  /*!< data area pointer */
};

/**
 * qhasharr container object
 */
struct qhasharr_s {
    /* encapsulated member functions */
    bool (*put) (qhasharr_t *tbl, const char *key, const void *value,
                 size_t size);
    bool (*putstr) (qhasharr_t *tbl, const char *key, const char *str);
    bool (*putstrf) (qhasharr_t *tbl, const char *key, const char *format, ...);
    bool (*putint) (qhasharr_t *tbl, const char *key, int64_t num);

    void *(*get) (qhasharr_t *tbl, const char *key, size_t *size);
    char *(*getstr) (qhasharr_t *tbl, const char *key);
    int64_t (*getint) (qhasharr_t *tbl, const char *key);
    bool (*getnext) (qhasharr_t *tbl, qnobj_t *obj, int *idx);

    bool (*remove) (qhasharr_t *tbl, const char *key);

    int  (*size) (qhasharr_t *tbl, int *maxslots, int *usedslots);
    void (*clear) (qhasharr_t *tbl);
    bool (*debug) (qhasharr_t *tbl, FILE *out);

    void (*free) (qhasharr_t *tbl);

    /* private variables */
    qhasharr_data_t *data;
};

#ifdef __cplusplus
}
#endif

#endif /*_QHASHARR_H */

