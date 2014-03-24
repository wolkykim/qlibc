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
 * @file qhashtbl.c Hash-table container implementation.
 *
 * qhashtbl implements a hashtable, which maps keys to values. Key is a unique
 * string and value is any non-null object. The creator qHashtbl() has one
 * parameters that affect its performance: initial hash range. The hash range
 * is the number of slots(pointers) in the hash table. in the case of a hash
 * collision, a single slots stores multiple elements using linked-list
 * structure, which must be searched sequentially. So lower range than the
 * number of elements decreases the space overhead but increases the number of
 * hash collisions and consequently it increases the time cost to look up an
 * element.
 *
 * @code
 *  [Internal Structure Example for 10-slot hash table]
 *
 *  RANGE    NAMED-OBJECT-LIST
 *  =====    =================
 *  [ 0 ] -> [hash=320,key3=value] -> [hash=210,key5=value] -> [hash=110,...]
 *  [ 1 ] -> [hash=1,key1=value]
 *  [ 2 ]
 *  [ 3 ] -> [hash=873,key4=value]
 *  [ 4 ] -> [hash=2674,key11=value] -> [hash=214,key5=value]
 *  [ 5 ] -> [hash=8545,key10=value]
 *  [ 6 ] -> [hash=9226,key9=value]
 *  [ 7 ]
 *  [ 8 ] -> [hash=8,key6=value] -> [hash=88,key8=value]
 *  [ 9 ] -> [hash=12439,key7=value]
 * @endcode
 *
 * @code
 *  // create a hash-table with 10 hash-index range.
 *  // Please be aware, the hash-index range 10 does not mean the number of
 *  // objects which can be stored. You can put as many as you want but if
 *  // this range is too small, hash conflict will happen and fetch time will
 *  // slightly increase.
 *  qhashtbl_t *tbl = qHashtbl(0, QHASHTBL_OPT_THREADSAFE);
 *
 *  // put objects into table.
 *  tbl->put(tbl, "sample1", "binary", 6);
 *  tbl->putstr(tbl, "sample2", "string");
 *  tbl->putint(tbl, "sample3", 1);
 *
 *  // debug print out
 *  tbl->debug(tbl, stdout, true);
 *
 *  // get objects
 *  void *sample1 = tbl->get(tbl, "sample1", &size, true);
 *  char *sample2 = tbl->getstr(tbl, "sample2", false);
 *  int  sample3  = tbl->getint(tbl, "sample3");
 *
 *  // sample1 is memalloced
 *  if(sample1 != NULL) free(sample1);
 *
 *  // release table
 *  tbl->free(tbl);
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "qlibc.h"
#include "qinternal.h"

#define DEFAULT_INDEX_RANGE (1000)  /*!< default value of hash-index range */

#ifndef _DOXYGEN_SKIP

// member methods
static bool put(qhashtbl_t *tbl, const char *name, const void *data,
                size_t size);
static bool putstr(qhashtbl_t *tbl, const char *name, const char *str);
static bool putstrf(qhashtbl_t *tbl, const char *name, const char *format,
                    ...);
static bool putint(qhashtbl_t *tbl, const char *name, int64_t num);

static void *get(qhashtbl_t *tbl, const char *name, size_t *size, bool newmem);
static char *getstr(qhashtbl_t *tbl, const char *name, bool newmem);
static int64_t getint(qhashtbl_t *tbl, const char *name);

static bool getnext(qhashtbl_t *tbl, qhnobj_t *obj, bool newmem);

static bool remove_(qhashtbl_t *tbl, const char *name);

static size_t size(qhashtbl_t *tbl);
static void clear(qhashtbl_t *tbl);
static bool debug(qhashtbl_t *tbl, FILE *out);

static void lock(qhashtbl_t *tbl);
static void unlock(qhashtbl_t *tbl);

static void free_(qhashtbl_t *tbl);

#endif

/**
 * Initialize hash table.
 *
 * @param range     initial size of index range. Value of 0 will use default value, DEFAULT_INDEX_RANGE;
 * @param options   combination of initialization options.
 *
 * @return a pointer of malloced qhashtbl_t, otherwise returns false
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *  // create a hash-table.
 *  qhashtbl_t *basic_hashtbl = qHashtbl(0, 0);
 *
 *  // create a large hash-table for millions of keys with thread-safe option.
 *  qhashtbl_t *small_hashtbl = qHashtbl(1000000, QHASHTBL_OPT_THREADSAFE);
 * @endcode
 *
 * @note
 *   Setting the right range is a magic.
 *   In practice, pick a value between (total keys / 3) ~ (total keys * 2).
 *   Available options:
 *   - QHASHTBL_OPT_THREADSAFE - make it thread-safe.
 */
qhashtbl_t *qhashtbl(size_t range, int options)
{
    if (range == 0) {
        range = DEFAULT_INDEX_RANGE;
    }

    qhashtbl_t *tbl = (qhashtbl_t *)calloc(1, sizeof(qhashtbl_t));
    if (tbl == NULL) goto malloc_failure;

    // allocate table space
    tbl->slots = (qhnobj_t **)calloc(range, sizeof(qhnobj_t *));
    if (tbl->slots == NULL) goto malloc_failure;

    // handle options.
    if (options & QHASHTBL_OPT_THREADSAFE) {
        Q_MUTEX_NEW(tbl->qmutex, true);
        if (tbl->qmutex == NULL) goto malloc_failure;
    }

    // assign methods
    tbl->put        = put;
    tbl->putstr     = putstr;
    tbl->putstrf    = putstrf;
    tbl->putint     = putint;

    tbl->get        = get;
    tbl->getstr     = getstr;
    tbl->getint     = getint;

    tbl->getnext    = getnext;

    tbl->remove     = remove_;

    tbl->size       = size;
    tbl->clear      = clear;
    tbl->debug      = debug;

    tbl->lock       = lock;
    tbl->unlock     = unlock;

    tbl->free       = free_;

    // set table range.
    tbl->range = range;

    return tbl;

  malloc_failure:
    errno = ENOMEM;
    if (tbl) {
        if (tbl->slots) free(tbl->slots);
        assert(tbl->qmutex == NULL);
    	free_(tbl);
    }
    return NULL;
}

/**
 * qhashtbl->put(): Put a object into this table.
 *
 * @param tbl       qhashtbl_t container pointer.
 * @param name      key name
 * @param data      data object
 * @param size      size of data object
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 */
static bool put(qhashtbl_t *tbl, const char *name, const void *data,
                size_t size)
{
    if (name == NULL || data == NULL) {
        errno = EINVAL;
        return false;
    }

    // get hash integer
    uint32_t hash = qhashmurmur3_32(name, strlen(name));
    int idx = hash % tbl->range;

    lock(tbl);

    // find existence key
    qhnobj_t *obj;
    for (obj = tbl->slots[idx]; obj != NULL; obj = obj->next) {
        if (obj->hash == hash && !strcmp(obj->name, name)) {
            break;
        }
    }

    // duplicate object
    char *dupname = strdup(name);
    void *dupdata = malloc(size);
    if (dupname == NULL || dupdata == NULL) {
        if (dupname != NULL) free(dupname);
        if (dupdata != NULL) free(dupdata);
        unlock(tbl);
        errno = ENOMEM;
        return false;
    }
    memcpy(dupdata, data, size);

    // put into table
    if (obj == NULL) {
        // insert
        obj = (qhnobj_t *)calloc(1, sizeof(qhnobj_t));
        if (obj == NULL) {
            free(dupname);
            free(dupdata);
            unlock(tbl);
            errno = ENOMEM;
            return false;
        }

        if (tbl->slots[idx] != NULL) {
            // insert at the beginning
            obj->next = tbl->slots[idx];
        }
        tbl->slots[idx] = obj;

        // increase counter
        tbl->num++;
    } else {
        // replace
        free(obj->name);
        free(obj->data);
    }

    // set data
    obj->hash = hash;
    obj->name = dupname;
    obj->data = dupdata;
    obj->size = size;

    unlock(tbl);
    return true;
}

/**
 * qhashtbl->putstr(): Put a string into this table.
 *
 * @param tbl       qhashtbl_t container pointer.
 * @param name      key name.
 * @param str       string data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 */
static bool putstr(qhashtbl_t *tbl, const char *name, const char *str)
{
    size_t size = (str != NULL) ? (strlen(str) + 1) : 0;
    return put(tbl, name, str, size);
}

/**
 * qhashtbl->putstrf(): Put a formatted string into this table.
 *
 * @param tbl       qhashtbl_t container pointer.
 * @param name      key name.
 * @param format    formatted string data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 */
static bool putstrf(qhashtbl_t *tbl, const char *name, const char *format, ...)
{
    char *str;
    DYNAMIC_VSPRINTF(str, format);
    if (str == NULL) {
        errno = ENOMEM;
        return false;
    }

    bool ret = putstr(tbl, name, str);
    free(str);
    return ret;
}

/**
 * qhashtbl->putint(): Put a integer into this table as string type.
 *
 * @param tbl       qhashtbl_t container pointer.
 * @param name      key name.
 * @param num       integer data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 *
 * @note
 * The integer will be converted to a string object and stored as string object.
 */
static bool putint(qhashtbl_t *tbl, const char *name, const int64_t num)
{
    char str[20+1];
    snprintf(str, sizeof(str), "%"PRId64, num);
    return putstr(tbl, name, str);
}

/**
 * qhashtbl->get(): Get a object from this table.
 *
 * @param tbl       qhashtbl_t container pointer.
 * @param name      key name.
 * @param size      if not NULL, oject size will be stored.
 * @param newmem    whether or not to allocate memory for the data.
 *
 * @return a pointer of data if the key is found, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *  qhashtbl_t *tbl = qHashtbl(1000);
 *  (...codes...)
 *
 *  // with newmem flag unset
 *  size_t size;
 *  struct myobj *obj = (struct myobj*)tbl->get(tbl, "key_name", &size, false);
 *
 *  // with newmem flag set
 *  size_t size;
 *  struct myobj *obj = (struct myobj*)tbl->get(tbl, "key_name", &size, true);
 *  if(obj != NULL) free(obj);
 * @endcode
 *
 * @note
 *  If newmem flag is set, returned data will be malloced and should be
 *  deallocated by user. Otherwise returned pointer will point internal buffer
 *  directly and should not be de-allocated by user. In thread-safe mode,
 *  newmem flag should be true always.
 */
static void *get(qhashtbl_t *tbl, const char *name, size_t *size, bool newmem)
{
    if (name == NULL) {
        errno = EINVAL;
        return NULL;
    }

    uint32_t hash = qhashmurmur3_32(name, strlen(name));
    int idx = hash % tbl->range;

    lock(tbl);

    // find key
    qhnobj_t *obj;
    for (obj = tbl->slots[idx]; obj != NULL; obj = obj->next) {
        if (obj->hash == hash && !strcmp(obj->name, name)) {
            break;
        }
    }

    void *data = NULL;
    if (obj != NULL) {
        if (newmem == false) {
            data = obj->data;
        } else {
            data = malloc(obj->size);
            if (data == NULL) {
                errno = ENOMEM;
                return NULL;
            }
            memcpy(data, obj->data, obj->size);
        }
        if (size != NULL && data != NULL) *size = obj->size;
    }

    unlock(tbl);

    if (data == NULL) errno = ENOENT;
    return data;
}

/**
 * qhashtbl->getstr(): Finds an object with given name and returns as
 * string type.
 *
 * @param tbl       qhashtbl_t container pointer.
 * @param name      key name
 * @param newmem    whether or not to allocate memory for the data.
 *
 * @return a pointer of data if the key is found, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 *
 * @note
 *  If newmem flag is set, returned data will be malloced and should be
 *  deallocated by user.
 */
static char *getstr(qhashtbl_t *tbl, const char *name, const bool newmem)
{
    return get(tbl, name, NULL, newmem);
}

/**
 * qhashtbl->getint(): Finds an object with given name and returns as
 * integer type.
 *
 * @param tbl       qhashtbl_t container pointer.
 * @param name      key name
 *
 * @return value integer if successful, otherwise(not found) returns 0
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 */
static int64_t getint(qhashtbl_t *tbl, const char *name)
{
    int64_t num = 0;
    char *str = getstr(tbl, name, true);
    if (str != NULL) {
        num = atoll(str);
        free(str);
    }

    return num;
}

/**
 * qhashtbl->getnext(): Get next element.
 *
 * @param tbl       qhashtbl_t container pointer.
 * @param obj       found data will be stored in this object
 * @param newmem    whether or not to allocate memory for the data.
 *
 * @return true if found otherwise returns false
 * @retval errno will be set in error condition.
 *  - ENOENT : No next element.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *  qhashtbl_t *tbl = qHashtbl(1000);
 *  (...add data into list...)
 *
 *  // non-thread usages
 *  qhnobj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  tbl->lock(tbl);
 *  while(tbl->getnext(tbl, &obj, false) == true) {
 *     printf("NAME=%s, DATA=%s, SIZE=%zu\n",
 *     obj.name, (char*)obj.data, obj.size);
 *  }
 *  tbl->unlock(tbl);
 *
 *  // thread model with newmem flag
 *  qhnobj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  tbl->lock(tbl);
 *  while(tbl->getnext(tbl, &obj, true) == true) {
 *     printf("NAME=%s, DATA=%s, SIZE=%zu\n",
 *     obj.name, (char*)obj.data, obj.size);
 *     free(obj.name);
 *     free(obj.data);
 *  }
 *  tbl->unlock(tbl);
 * @endcode
 *
 * @note
 *  obj should be initialized with 0 by using memset() before first call.
 *  If newmem flag is true, user should de-allocate obj.name and obj.data
 *  resources.
 */
static bool getnext(qhashtbl_t *tbl, qhnobj_t *obj, const bool newmem)
{
    if (obj == NULL) {
        errno = EINVAL;
        return NULL;
    }

    lock(tbl);

    bool found = false;

    qhnobj_t *cursor = NULL;
    int idx = 0;
    if (obj->name != NULL) {
        idx = (obj->hash % tbl->range) + 1;
        cursor = obj->next;
    }

    if (cursor != NULL) {
        // has link
        found = true;
    } else {
        // search from next index
        for (; idx < tbl->range; idx++) {
            if (tbl->slots[idx] != NULL) {
                cursor = tbl->slots[idx];
                found = true;
                break;
            }
        }
    }

    if (cursor != NULL) {
        if (newmem == true) {
            obj->name = strdup(cursor->name);
            obj->data = malloc(cursor->size);
            if (obj->name == NULL || obj->data == NULL) {
                DEBUG("getnext(): Unable to allocate memory.");
                if (obj->name != NULL) free(obj->name);
                if (obj->data != NULL) free(obj->data);
                unlock(tbl);
                errno = ENOMEM;
                return false;
            }
            memcpy(obj->data, cursor->data, cursor->size);
            obj->size = cursor->size;
        } else {
            obj->name = cursor->name;
            obj->data = cursor->data;
        }
        obj->hash = cursor->hash;
        obj->size = cursor->size;
        obj->next = cursor->next;

    }

    unlock(tbl);

    if (found == false) errno = ENOENT;

    return found;
}

/**
 * qhashtbl->remove(): Remove an object from this table.
 *
 * @param tbl   qhashtbl_t container pointer.
 * @param name  key name
 *
 * @return true if successful, otherwise(not found) returns false
 * @retval errno will be set in error condition.
 *  - ENOENT : No next element.
 *  - EINVAL : Invalid argument.
 */
static bool remove_(qhashtbl_t *tbl, const char *name)
{
    if (name == NULL) {
        errno = EINVAL;
        return false;
    }

    lock(tbl);

    uint32_t hash = qhashmurmur3_32(name, strlen(name));
    int idx = hash % tbl->range;

    // find key
    bool found = false;
    qhnobj_t *prev = NULL;
    qhnobj_t *obj;
    for (obj = tbl->slots[idx]; obj != NULL; obj = obj->next) {
        if (obj->hash == hash && !strcmp(obj->name, name)) {
            // adjust link
            if (prev == NULL) tbl->slots[idx] = obj->next;
            else prev->next = obj->next;

            // remove
            free(obj->name);
            free(obj->data);
            free(obj);

            found = true;
            tbl->num--;
            break;
        }

        prev = obj;
    }

    unlock(tbl);

    if (found == false) errno = ENOENT;

    return found;
}

/**
 * qhashtbl->size(): Returns the number of keys in this hashtable.
 *
 * @param tbl   qhashtbl_t container pointer.
 *
 * @return number of elements stored
 */
static size_t size(qhashtbl_t *tbl)
{
    return tbl->num;
}

/**
 * qhashtbl->clear(): Clears this hashtable so that it contains no keys.
 *
 * @param tbl   qhashtbl_t container pointer.
 */
void clear(qhashtbl_t *tbl)
{
    lock(tbl);
    int idx;
    for (idx = 0; idx < tbl->range && tbl->num > 0; idx++) {
        if (tbl->slots[idx] == NULL) continue;
        qhnobj_t *obj = tbl->slots[idx];
        tbl->slots[idx] = NULL;
        while (obj != NULL) {
            qhnobj_t *next = obj->next;
            free(obj->name);
            free(obj->data);
            free(obj);
            obj = next;

            tbl->num--;
        }
    }

    unlock(tbl);
}

/**
 * qhashtbl->debug(): Print hash table for debugging purpose
 *
 * @param tbl   qhashtbl_t container pointer.
 * @param out   output stream
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EIO : Invalid output stream.
 */
bool debug(qhashtbl_t *tbl, FILE *out)
{
    if (out == NULL) {
        errno = EIO;
        return false;
    }

    qhnobj_t obj;
    memset((void *)&obj, 0, sizeof(obj)); // must be cleared before call
    lock(tbl);
    while (tbl->getnext(tbl, &obj, false) == true) {
        fprintf(out, "%s=" , obj.name);
        _q_humanOut(out, obj.data, obj.size, MAX_HUMANOUT);
        fprintf(out, " (%zu, hash=%u)\n" , obj.size, obj.hash);
    }
    unlock(tbl);

    return true;
}

/**
 * qhashtbl->lock(): Enter critical section.
 *
 * @param tbl   qhashtbl_t container pointer.
 *
 * @note
 *  From user side, normally locking operation is only needed when traverse
 *  all elements using qhashtbl->getnext().
 *
 * @note
 *  This operation will do nothing if QHASHTBL_OPT_THREADSAFE option was not
 *  given at the initialization time.
 */
static void lock(qhashtbl_t *tbl)
{
    Q_MUTEX_ENTER(tbl->qmutex);
}

/**
 * qhashtbl->unlock(): Leave critical section.
 *
 * @param tbl   qhashtbl_t container pointer.
 *
 * @note
 *  This operation will do nothing if QHASHTBL_OPT_THREADSAFE option was not
 *  given at the initialization time.
 */
static void unlock(qhashtbl_t *tbl)
{
	Q_MUTEX_LEAVE(tbl->qmutex);
}

/**
 * qhashtbl->free(): De-allocate hash table
 *
 * @param tbl   qhashtbl_t container pointer.
 */
void free_(qhashtbl_t *tbl)
{
    lock(tbl);
    clear(tbl);
    if (tbl->slots != NULL) free(tbl->slots);
    unlock(tbl);
    Q_MUTEX_DESTROY(tbl->qmutex);
    free(tbl);
}
