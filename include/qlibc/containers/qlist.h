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
 * Doubly Linked-list container.
 *
 * @file qlist.h
 */

#ifndef QLIST_H
#define QLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* types */
typedef struct qlist_s qlist_t;
typedef struct qlist_obj_s qlist_obj_t;

enum {
	QLIST_THREADSAFE = (0x01)  /*!< make it thread-safe */
};

/* member functions
 *
 * All the member functions can be accessed in both ways:
 *  - tbl->addlast(tbl, ...);  // easier to switch the container type to other kinds.
 *  - qlist_addlast(tbl, ...); // where avoiding pointer overhead is preferred.
 */
extern qlist_t *qlist(int options); /*!< qlist constructor */
extern size_t qlist_setsize(qlist_t *list, size_t max);

extern bool qlist_addfirst(qlist_t *list, const void *data, size_t size);
extern bool qlist_addlast(qlist_t *list, const void *data, size_t size);
extern bool qlist_addat(qlist_t *list, int index, const void *data, size_t size);

extern void *qlist_getfirst(qlist_t *list, size_t *size, bool newmem);
extern void *qlist_getlast(qlist_t *list, size_t *size, bool newmem);
extern void *qlist_getat(qlist_t *list, int index, size_t *size, bool newmem);

extern void *qlist_popfirst(qlist_t *list, size_t *size);
extern void *qlist_poplast(qlist_t *list, size_t *size);
extern void *qlist_popat(qlist_t *list, int index, size_t *size);

extern bool qlist_removefirst(qlist_t *list);
extern bool qlist_removelast(qlist_t *list);
extern bool qlist_removeat(qlist_t *list, int index);

extern bool qlist_getnext(qlist_t *list, qlist_obj_t *obj, bool newmem);

extern size_t qlist_size(qlist_t *list);
extern size_t qlist_datasize(qlist_t *list);
extern void qlist_reverse(qlist_t *list);
extern void qlist_clear(qlist_t *list);

extern void *qlist_toarray(qlist_t *list, size_t *size);
extern char *qlist_tostring(qlist_t *list);
extern bool qlist_debug(qlist_t *list, FILE *out);

extern void qlist_lock(qlist_t *list);
extern void qlist_unlock(qlist_t *list);

extern void qlist_free(qlist_t *list);

/**
 * qlist container object
 */
struct qlist_s {
	/* encapsulated member functions */
	size_t (*setsize)(qlist_t *list, size_t max);

	bool (*addfirst)(qlist_t *list, const void *data, size_t size);
	bool (*addlast)(qlist_t *list, const void *data, size_t size);
	bool (*addat)(qlist_t *list, int index, const void *data, size_t size);
    
	void *(*getfirst)(qlist_t *list, size_t *size, bool newmem);
	void *(*getlast)(qlist_t *list, size_t *size, bool newmem);
	void *(*getat)(qlist_t *list, int index, size_t *size, bool newmem);

	void *(*popfirst)(qlist_t *list, size_t *size);
	void *(*poplast)(qlist_t *list, size_t *size);
	void *(*popat)(qlist_t *list, int index, size_t *size);

	bool (*removefirst)(qlist_t *list);
	bool (*removelast)(qlist_t *list);
	bool (*removeat)(qlist_t *list, int index);

	bool (*getnext)(qlist_t *list, qlist_obj_t *obj, bool newmem);

	void (*reverse)(qlist_t *list);
	void (*clear)(qlist_t *list);

	size_t (*size)(qlist_t *list);
	size_t (*datasize)(qlist_t *list);

	void *(*toarray)(qlist_t *list, size_t *size);
	char *(*tostring)(qlist_t *list);
	bool (*debug)(qlist_t *list, FILE *out);

	void (*lock)(qlist_t *list);
	void (*unlock)(qlist_t *list);

	void (*free)(qlist_t *list);

	/* private variables - do not access directly */
	void *qmutex;     /*!< initialized when QLIST_OPT_THREADSAFE is given */
	size_t num;           /*!< number of elements */
	size_t max;           /*!< maximum number of elements. 0 means no limit */
	size_t datasum;       /*!< total sum of data size, does not include name size */

	qlist_obj_t *first;   /*!< first object pointer */
	qlist_obj_t *last;    /*!< last object pointer */
};

/**
 * qlist node data structure.
 */
struct qlist_obj_s {
	void *data;          /*!< data */
	size_t size;         /*!< data size */

	qlist_obj_t *prev;   /*!< previous link */
	qlist_obj_t *next;   /*!< next link */
};

#ifdef __cplusplus
}
#endif

#endif /* QLIST_H */
