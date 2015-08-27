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
 * @file qhashtbl.c Hash-table container implementation.
 *
 * qhashtbl implements a hash table, which maps keys to values. Key is a unique
 * string and value is any non-null object. The creator qhashtbl() has a
 * parameter that affect its performance: initial hash range. The hash range
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
 *  qhashtbl_t *tbl = qhashtbl(0, QHASHTBL_THREADSAFE);
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
 *  free(sample1);
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
#include "qinternal.h"
#include "utilities/qhash.h"
#include "containers/qhashtbl.h"

#define DEFAULT_INDEX_RANGE (1000)  /*!< default value of hash-index range */

/**
 * Initialize hash table.
 *
 * @param range     initial size of index range. Value of 0 will use default value, DEFAULT_INDEX_RANGE;
 * @param options   combination of initialization options.
 *
 * @return a pointer of malloced qhashtbl_t, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *  // create a hash-table.
 *  qhashtbl_t *basic_hashtbl = qhashtbl(0, 0);
 *
 *  // create a large hash-table for millions of keys with thread-safe option.
 *  qhashtbl_t *small_hashtbl = qhashtbl(1000000, QHASHTBL_THREADSAFE);
 * @endcode
 *
 * @note
 *   Setting the right range is a magic.
 *   In practice, pick a value between (total keys / 3) ~ (total keys * 2).
 *   Available options:
 *   - QHASHTBL_THREADSAFE - make it thread-safe.
 */
qhashtbl_t *qhashtbl(size_t range, int options) {
    if (range == 0) {
        range = DEFAULT_INDEX_RANGE;
    }

    qhashtbl_t *tbl = (qhashtbl_t *) calloc(1, sizeof(qhashtbl_t));
    if (tbl == NULL)
        goto malloc_failure;

    // allocate table space
    tbl->slots = (qhashtbl_obj_t **) calloc(range, sizeof(qhashtbl_obj_t *));
    if (tbl->slots == NULL)
        goto malloc_failure;

    // handle options.
    if (options & QHASHTBL_THREADSAFE) {
        Q_MUTEX_NEW(tbl->qmutex, true);
        if (tbl->qmutex == NULL)
            goto malloc_failure;
    }

    // assign methods
    tbl->put = qhashtbl_put;
    tbl->putstr = qhashtbl_putstr;
    tbl->putstrf = qhashtbl_putstrf;
    tbl->putint = qhashtbl_putint;

    tbl->get = qhashtbl_get;
    tbl->getstr = qhashtbl_getstr;
    tbl->getint = qhashtbl_getint;

    tbl->remove = qhashtbl_remove;

    tbl->getnext = qhashtbl_getnext;

    tbl->size = qhashtbl_size;
    tbl->clear = qhashtbl_clear;
    tbl->debug = qhashtbl_debug;

    tbl->lock = qhashtbl_lock;
    tbl->unlock = qhashtbl_unlock;

    tbl->free = qhashtbl_free;

    // set table range.
    tbl->range = range;

    return tbl;

    malloc_failure:
    errno = ENOMEM;
    if (tbl) {
        assert(tbl->qmutex == NULL);
        qhashtbl_free(tbl);
    }
    return NULL;
}

/**
 * qhashtbl->put(): Put an object into this table.
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
bool qhashtbl_put(qhashtbl_t *tbl, const char *name, const void *data,
                size_t size) {
    if (name == NULL || data == NULL) {
        errno = EINVAL;
        return false;
    }

    // get hash integer
    uint32_t hash = qhashmurmur3_32(name, strlen(name));
    int idx = hash % tbl->range;

    qhashtbl_lock(tbl);

    // find existence key
    qhashtbl_obj_t *obj;
    for (obj = tbl->slots[idx]; obj != NULL; obj = obj->next) {
        if (obj->hash == hash && !strcmp(obj->name, name)) {
            break;
        }
    }

    // duplicate object
    char *dupname = strdup(name);
    void *dupdata = malloc(size);
    if (dupname == NULL || dupdata == NULL) {
        free(dupname);
        free(dupdata);
        qhashtbl_unlock(tbl);
        errno = ENOMEM;
        return false;
    }
    memcpy(dupdata, data, size);

    // put into table
    if (obj == NULL) {
        // insert
        obj = (qhashtbl_obj_t *) calloc(1, sizeof(qhashtbl_obj_t));
        if (obj == NULL) {
            free(dupname);
            free(dupdata);
            qhashtbl_unlock(tbl);
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

    qhashtbl_unlock(tbl);
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
bool qhashtbl_putstr(qhashtbl_t *tbl, const char *name, const char *str) {
    return qhashtbl_put(tbl, name, str, (str != NULL) ? (strlen(str) + 1) : 0);
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
bool qhashtbl_putstrf(qhashtbl_t *tbl, const char *name, const char *format, ...) {
    char *str;
    DYNAMIC_VSPRINTF(str, format);
    if (str == NULL) {
        errno = ENOMEM;
        return false;
    }

    bool ret = qhashtbl_putstr(tbl, name, str);
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
bool qhashtbl_putint(qhashtbl_t *tbl, const char *name, const int64_t num) {
    char str[20 + 1];
    snprintf(str, sizeof(str), "%"PRId64, num);
    return qhashtbl_putstr(tbl, name, str);
}

/**
 * qhashtbl->get(): Get an object from this table.
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
 *  qhashtbl_t *tbl = qhashtbl(0, 0);
 *  (...codes...)
 *
 *  // with newmem flag unset
 *  size_t size;
 *  void *data = (struct myobj*)tbl->get(tbl, "key_name", &size, false);
 *
 *  // with newmem flag set
 *  size_t size;
 *  void *data  = (struct myobj*)tbl->get(tbl, "key_name", &size, true);
 *  free(data);
 * @endcode
 *
 * @note
 *  If newmem flag is set, returned data will be malloced and should be
 *  deallocated by user. Otherwise returned pointer will point internal buffer
 *  directly and should not be de-allocated by user. In thread-safe mode,
 *  newmem flag must be set to true always.
 */
void *qhashtbl_get(qhashtbl_t *tbl, const char *name, size_t *size, bool newmem) {
    if (name == NULL) {
        errno = EINVAL;
        return NULL;
    }

    uint32_t hash = qhashmurmur3_32(name, strlen(name));
    int idx = hash % tbl->range;

    qhashtbl_lock(tbl);

    // find key
    qhashtbl_obj_t *obj;
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
        if (size != NULL && data != NULL)
            *size = obj->size;
    }

    qhashtbl_unlock(tbl);

    if (data == NULL)
        errno = ENOENT;
    return data;
}

/**
 * qhashtbl->getstr(): Finds an object and returns as string type.
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
char *qhashtbl_getstr(qhashtbl_t *tbl, const char *name, const bool newmem) {
    return qhashtbl_get(tbl, name, NULL, newmem);
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
int64_t qhashtbl_getint(qhashtbl_t *tbl, const char *name) {
    int64_t num = 0;
    char *str = qhashtbl_getstr(tbl, name, true);
    if (str != NULL) {
        num = atoll(str);
        free(str);
    }

    return num;
}

/**
 * qhashtbl->remove(): Remove an object from this table.
 *
 * @param tbl   qhashtbl_t container pointer.
 * @param name  key name
 *
 * @return true if successful, otherwise(not found) returns false
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 */
bool qhashtbl_remove(qhashtbl_t *tbl, const char *name) {
    if (name == NULL) {
        errno = EINVAL;
        return false;
    }

    qhashtbl_lock(tbl);

    uint32_t hash = qhashmurmur3_32(name, strlen(name));
    int idx = hash % tbl->range;

    // find key
    bool found = false;
    qhashtbl_obj_t *prev = NULL;
    qhashtbl_obj_t *obj;
    for (obj = tbl->slots[idx]; obj != NULL; obj = obj->next) {
        if (obj->hash == hash && !strcmp(obj->name, name)) {
            // adjust link
            if (prev == NULL)
                tbl->slots[idx] = obj->next;
            else
                prev->next = obj->next;

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

    qhashtbl_unlock(tbl);

    if (found == false)
        errno = ENOENT;

    return found;
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
 *  qhashtbl_t *tbl = qhashtbl(0, 0);
 *  (...add data into list...)
 *
 *  qhashtbl_obj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  tbl->lock(tbl);  // lock it when thread condition is expected
 *  while(tbl->getnext(tbl, &obj, false) == true) {  // newmem is false
 *     printf("NAME=%s, DATA=%s, SIZE=%zu\n",
 *     obj.name, (char*)obj.data, obj.size);
 *     // do free obj.name and obj.data if newmem was set to true;
 *  }
 *  tbl->unlock(tbl);
 * @endcode
 *
 * @note
 *  locking must be provided on user code when all element scan must be
 *  guaranteed where multiple threads concurrently update the table.
 *  It's ok not to lock the table on the user code even in thread condition,
 *  when concurreny is importand and all element scan in a path doesn't need
 *  to be guaranteed. In this case, new data inserted during the traversal
 *  will be show up in this scan or next scan. Make sure newmem flag is set
 *  if deletion is expected during the scan.
 *  Object obj should be initialized with 0 by using memset() before first call.
 */
bool qhashtbl_getnext(qhashtbl_t *tbl, qhashtbl_obj_t *obj, const bool newmem) {
    if (obj == NULL) {
        errno = EINVAL;
        return NULL;
    }

    qhashtbl_lock(tbl);

    bool found = false;

    qhashtbl_obj_t *cursor = NULL;
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
                free(obj->name);
                free(obj->data);
                qhashtbl_unlock(tbl);
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

    qhashtbl_unlock(tbl);

    if (found == false)
        errno = ENOENT;

    return found;
}

/**
 * qhashtbl->size(): Returns the number of keys in this hashtable.
 *
 * @param tbl   qhashtbl_t container pointer.
 *
 * @return number of elements stored
 */
size_t qhashtbl_size(qhashtbl_t *tbl) {
    return tbl->num;
}

/**
 * qhashtbl->clear(): Clears this hashtable so that it contains no keys.
 *
 * @param tbl   qhashtbl_t container pointer.
 */
void qhashtbl_clear(qhashtbl_t *tbl) {
    qhashtbl_lock(tbl);
    int idx;
    for (idx = 0; idx < tbl->range && tbl->num > 0; idx++) {
        if (tbl->slots[idx] == NULL)
            continue;
        qhashtbl_obj_t *obj = tbl->slots[idx];
        tbl->slots[idx] = NULL;
        while (obj != NULL) {
            qhashtbl_obj_t *next = obj->next;
            free(obj->name);
            free(obj->data);
            free(obj);
            obj = next;

            tbl->num--;
        }
    }

    qhashtbl_unlock(tbl);
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
bool qhashtbl_debug(qhashtbl_t *tbl, FILE *out) {
    if (out == NULL) {
        errno = EIO;
        return false;
    }

    qhashtbl_obj_t obj;
    memset((void *) &obj, 0, sizeof(obj));  // must be cleared before call
    qhashtbl_lock(tbl);
    while (tbl->getnext(tbl, &obj, false) == true) {
        fprintf(out, "%s=", obj.name);
        _q_textout(out, obj.data, obj.size, MAX_HUMANOUT);
        fprintf(out, " (%zu, hash=%u)\n", obj.size, obj.hash);
    }
    qhashtbl_unlock(tbl);

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
 *  This operation will do nothing if QHASHTBL_THREADSAFE option was not
 *  given at the initialization time.
 */
void qhashtbl_lock(qhashtbl_t *tbl) {
    Q_MUTEX_ENTER(tbl->qmutex);
}

/**
 * qhashtbl->unlock(): Leave critical section.
 *
 * @param tbl   qhashtbl_t container pointer.
 *
 * @note
 *  This operation will do nothing if QHASHTBL_THREADSAFE option was not
 *  given at the initialization time.
 */
void qhashtbl_unlock(qhashtbl_t *tbl) {
    Q_MUTEX_LEAVE(tbl->qmutex);
}

/**
 * qhashtbl->free(): De-allocate hash table
 *
 * @param tbl   qhashtbl_t container pointer.
 */
void qhashtbl_free(qhashtbl_t *tbl) {
    qhashtbl_lock(tbl);
    qhashtbl_clear(tbl);
    free(tbl->slots);
    qhashtbl_unlock(tbl);
    Q_MUTEX_DESTROY(tbl->qmutex);
    free(tbl);
}
