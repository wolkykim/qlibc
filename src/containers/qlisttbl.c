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
 * @file qlisttbl.c Linked-list-table implementation.
 *
 * qlisttbl container is a Linked-List-Table implementation.
 * Which maps keys to values. Key is a string and value is any non-null object.
 * These elements are stored sequentially in Doubly-Linked-List data structure.
 *
 * Compared to Hash-Table, List-Table is efficient when you need to keep
 * duplicated keys since Hash-Table only keep unique keys. Of course, qlisttbl
 * supports both unique keys and key duplication.
 *
 * @code
 *  [Conceptional Data Structure Diagram]
 *
 *  last~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
 *                                                          |
 *          +-----------+  doubly  +-----------+  doubly  +-v---------+
 *  first~~~|~>   0   <~|~~~~~~~~~~|~>   1   <~|~~~~~~~~~~|~>   N     |
 *          +--|-----|--+  linked  +--|-----|--+  linked  +--|-----|--+
 *             |     |                |     |                |     |
 *       +-----v-+ +-v-------+        |     |          +-----v-+ +-v-------+
 *       | KEY A | | VALUE A |        |     |          | KEY N | | VALUE N |
 *       +-------+ +---------+        |     |          +-------+ +---------+
 *                              +-----v-+ +-v-------+
 *                              | KEY B | | VALUE B |
 *                              +-------+ +---------+
 * @endcode
 *
 * @code
 *  // create a list table.
 *  qlisttbl_t *tbl = qlisttbl(QLISTTBL_THREADSAFE);
 *
 *  // insert elements (key duplication allowed)
 *  tbl->put(tbl, "e1", "a", strlen("e1")+1); // equal to putstr();
 *  tbl->putstr(tbl, "e2", "b");
 *  tbl->putstr(tbl, "e2", "c");
 *  tbl->putstr(tbl, "e3", "d");
 *
 *  // debug output
 *  tbl->debug(tbl, stdout, true);
 *
 *  // get element
 *  printf("get('e2') : %s\n", (char*)tbl->get(tbl, "e2", NULL, false));
 *  printf("getstr('e2') : %s\n", tbl->getstr(tbl, "e2", false));
 *
 *  // get all matching elements
 *  qlisttbl_data_t *objs = tbl->getmulti(tbl, "e2", true, NULL);
 *  for (i = 0; objs[i].data != NULL; i++) {
 *      printf("getmulti('e2')[%d] : %s (size=%d)\n",
 *              i, (char *)objs[i].data, (int)objs[i].size);
 *  }
 *  tbl->freemulti(objs);
 *
 *  // find every 'e2' elements
 *  qlisttbl_obj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  while(tbl->getnext(tbl, &obj, "e2", false) == true) {
 *    printf("NAME=%s, DATA=%s, SIZE=%zu\n",
 *           obj.name, (char*)obj.data, obj.size);
 *  }
 *
 *  // free object
 *  tbl->free(tbl);
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include "qinternal.h"
#include "utilities/qencode.h"
#include "utilities/qfile.h"
#include "utilities/qhash.h"
#include "utilities/qio.h"
#include "utilities/qstring.h"
#include "utilities/qtime.h"
#include "containers/qlisttbl.h"

#ifndef _DOXYGEN_SKIP

static qlisttbl_obj_t *newobj(const char *name, const void *data, size_t size);
static bool insertobj(qlisttbl_t *tbl, qlisttbl_obj_t *obj);
static qlisttbl_obj_t *findobj(qlisttbl_t *tbl, const char *name, qlisttbl_obj_t *retobj);

static bool namematch(qlisttbl_obj_t *obj, const char *name, uint32_t hash);
static bool namecasematch(qlisttbl_obj_t *obj, const char *name, uint32_t hash);

#endif

/**
 * Create a new Q_LIST linked-list container
 *
 * @param options   combination of initialization options.
 *
 * @return a pointer of malloced qlisttbl_t structure in case of successful,
 *  otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *  qlisttbl_t *tbl = qlisttbl(0);
 *  qlisttbl_t *threadsafe_tbl = qlisttbl(QLISTTBL_THREADSAFE);
 * @endcode
 *
 * @note
 *   Available options:
 *   - QLISTTBL_THREADSAFE - make it thread-safe.
 *   - QLISTTBL_UNIQUE     - keys are all unique. replace same key
 *   - QLISTTBL_CASEINSENSITIVE  - key is case insensitive
 *   - QLISTTBL_INSERTTOP        - insert new key at the top
 *   - QLISTTBL_LOOKUPFORWARD    - find key from the top
 */
qlisttbl_t *qlisttbl(int options)
{
    qlisttbl_t *tbl = (qlisttbl_t *)calloc(1, sizeof(qlisttbl_t));
    if (tbl == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    // assign member methods.
    tbl->put        = qlisttbl_put;
    tbl->putstr     = qlisttbl_putstr;
    tbl->putstrf    = qlisttbl_putstrf;
    tbl->putint     = qlisttbl_putint;

    tbl->get        = qlisttbl_get;
    tbl->getstr     = qlisttbl_getstr;
    tbl->getint     = qlisttbl_getint;

    tbl->getmulti   = qlisttbl_getmulti;
    tbl->freemulti  = qlisttbl_freemulti;

    tbl->remove     = qlisttbl_remove;
    tbl->removeobj  = qlisttbl_removeobj;

    tbl->getnext    = qlisttbl_getnext;

    tbl->size       = qlisttbl_size;
    tbl->sort       = qlisttbl_sort;
    tbl->clear      = qlisttbl_clear;
    tbl->save       = qlisttbl_save;
    tbl->load       = qlisttbl_load;

    tbl->debug      = qlisttbl_debug;

    tbl->lock       = qlisttbl_lock;
    tbl->unlock     = qlisttbl_unlock;

    tbl->free       = qlisttbl_free;

    // assign private methods.
    tbl->namematch  = namematch;
    tbl->namecmp    = strcmp;

    // handle options.
    if (options & QLISTTBL_THREADSAFE) {
        Q_MUTEX_NEW(tbl->qmutex, true);
        if (tbl->qmutex == NULL) {
            errno = ENOMEM;
            free(tbl);
            return NULL;
        }
    }
    if (options & QLISTTBL_UNIQUE) {
        tbl->unique = true;
    }
    if (options & QLISTTBL_CASEINSENSITIVE) {
        tbl->namematch = namecasematch;
        tbl->namecmp = strcasecmp;
    }
    if (options & QLISTTBL_INSERTTOP) {
      tbl->inserttop = true;
    }
    if (options & QLISTTBL_LOOKUPFORWARD) {
      tbl->lookupforward = true;
    }

    return tbl;
}

/**
 * qlisttbl->put(): Put an element to this table.
 *
 * @param tbl       qlisttbl container pointer.
 * @param name      element name.
 * @param data      a pointer which points data memory.
 * @param size      size of the data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *  - EINVAL : Invalid argument.
 *
 * @code
 *  struct my_obj {
 *    int a, b, c;
 *  };
 *
 *  // create a sample object.
 *  struct my_obj obj;
 *
 *  // create a table and add a sample object.
 *  qlisttbl_t *tbl = qlisttbl();
 *  tbl->put(tbl, "obj1", &obj, sizeof(struct my_obj));
 * @endcode
 *
 * @note
 *  The default behavior is adding an object at the end of this table
 *  unless QLISTTBL_INSERTTOP option was given.
 */
bool qlisttbl_put(qlisttbl_t *tbl, const char *name, const void *data, size_t size)
{
    // make new object table
    qlisttbl_obj_t *obj = newobj(name, data, size);
    if (obj == NULL) {
        return false;
    }

    // lock table
    qlisttbl_lock(tbl);

    // if unique flag is set, remove same key
    if (tbl->unique == true) qlisttbl_remove(tbl, name);

    // insert into table
    if (tbl->num == 0) {
        obj->prev = NULL;
        obj->next = NULL;
    } else {
        if (tbl->inserttop == false) {
            obj->prev = tbl->last;
            obj->next = NULL;
        } else {
            obj->prev = NULL;
            obj->next = tbl->first;
        }
    }
    insertobj(tbl, obj);

    // unlock table
    qlisttbl_unlock(tbl);

    return true;
}

/**
 * qlisttbl->putstr(): Put a string into this table.
 *
 * @param tbl       qlisttbl container pointer.
 * @param name      element name.
 * @param str       string data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *  - EINVAL : Invalid argument.
 */
bool qlisttbl_putstr(qlisttbl_t *tbl, const char *name, const char *str)
{
    size_t size = (str) ? (strlen(str) + 1) : 0;
    return qlisttbl_put(tbl, name, (const void *)str, size);
}

/**
 * qlisttbl->putstrf(): Put a formatted string into this table.
 *
 * @param tbl       qlisttbl container pointer.
 * @param name      element name.
 * @param format    formatted value string.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *  - EINVAL : Invalid argument.
 */
bool qlisttbl_putstrf(qlisttbl_t *tbl, const char *name, const char *format, ...)
{
    char *str;
    DYNAMIC_VSPRINTF(str, format);
    if (str == NULL) {
        errno = ENOMEM;
        return false;
    }

    bool ret = qlisttbl_putstr(tbl, name, str);
    free(str);

    return ret;
}

/**
 * qlisttbl->putInt(): Put an integer into this table as string type.
 *
 * @param tbl       qlisttbl container pointer.
 * @param name      element name.
 * @param num       number data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *  - EINVAL : Invalid argument.
 *
 * @note
 *  The integer will be converted to a string object and stored as a string
 *  object.
 */
bool qlisttbl_putint(qlisttbl_t *tbl, const char *name, int64_t num)
{
    char str[20+1];
    snprintf(str, sizeof(str), "%"PRId64, num);
    return qlisttbl_putstr(tbl, name, str);
}

/**
 * qlisttbl->get(): Finds an object with given name.
 *
 * If there are duplicate keys in the table, this will return the first matched
 * one from the bottom (or the top if QLISTTBL_LOOKUPFORWARD option was given).
 * So if there are duplicated keys, it'll return recently inserted one.
 *
 * @param tbl       qlisttbl container pointer.
 * @param name      element name.
 * @param size      if size is not NULL, data size will be stored.
 * @param newmem    whether or not to allocate memory for the data.
 *
 * @return a pointer of data if key is found, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *  qlisttbl_t *tbl = qlisttbl();
 *  (...codes...)
 *
 *  // with newmem flag unset
 *  size_t size;
 *  void *data = tbl->get(tbl, "key_name", &size, false);
 *
 *  // with newmem flag set
 *  size_t size;
 *  void *data = tbl->get(tbl, "key_name", &size, true);
 *  (...does something with data...)
 *  if(data != NULL) free(data);
 * @endcode
 *
 * @note
 *  If newmem flag is set, returned data will be malloced and should be
 *  deallocated by user. Otherwise returned pointer will point internal data
 *  buffer directly and should not be de-allocated by user. In thread-safe mode,
 *  always set newmem flag as true to make sure it works in race condition
 *  situation.
 */
void *qlisttbl_get(qlisttbl_t *tbl, const char *name, size_t *size, bool newmem)
{
    if (name == NULL) {
        errno = EINVAL;
        return NULL;
    }

    qlisttbl_lock(tbl);
    void *data = NULL;
    qlisttbl_obj_t *obj = findobj(tbl, name, NULL);
    if (obj != NULL) {
        // get data
        if (newmem == true) {
            data = malloc(obj->size);
            if (data == NULL) {
                errno = ENOMEM;
                qlisttbl_unlock(tbl);
                return NULL;
            }
            memcpy(data, obj->data, obj->size);
        } else {
            data = obj->data;
        }

        // set size
        if (size != NULL) *size = obj->size;
    }
   qlisttbl_unlock(tbl);

    if (data == NULL) {
        errno = ENOENT;
    }

    return data;
}

/**
 * qlisttbl->getstr():  Finds an object with given name and returns as
 * string type.
 *
 * @param tbl qlisttbl container pointer.
 * @param name element name.
 * @param newmem whether or not to allocate memory for the data.
 *
 * @return a pointer of data if key is found, otherwise returns NULL.
  * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
*/
char *qlisttbl_getstr(qlisttbl_t *tbl, const char *name, bool newmem)
{
    return (char *)qlisttbl_get(tbl, name, NULL, newmem);
}

/**
 * qlisttbl->getint():  Finds an object with given name and returns as
 * integer type.
 *
 * @param tbl qlisttbl container pointer.
 * @param name element name.
 *
 * @return an integer value of the integer object, otherwise returns 0.
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 */
int64_t qlisttbl_getint(qlisttbl_t *tbl, const char *name)
{
    int64_t num = 0;
    char *str = qlisttbl_getstr(tbl, name, true);
    if (str != NULL) {
        num = atoll(str);
        free(str);
    }
    return num;
}

/**
 * qlisttbl->getmulti(): Finds all objects with given name and return a array
 * of objects.
 *
 * If there are duplicate keys in the table, this will return all
 * the matched ones. The order of objects in return depends on setnextdir()
 * setting. By default, the order is same(forward) as listed in the table.
 *
 * @param tbl       qlisttbl container pointer.
 * @param name      element name.
 * @param newmem    whether or not to allocate memory for the data.
 * @param numobjs   the nuber of objects returned will be stored. (can be NULL)
 *
 * @return a pointer of data if key is found, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 *
 * @note
 *  The returned array of qlisttbl_data_t should be released by freemulti() call
 *  after use. Even you call getmulti() with newmem set false, freemulti() should
 *  be called all the times, so the object array itself can be released.
 *
 * @code
 *  size_t numobjs = 0;
 *  qlisttbl_data_t *objs = tbl->getmulti(tbl, "e2", true, &numobjs);
 *  for (i = 0; objs[i].data != NULL; i++) {
 *      printf("DATA=%s, SIZE=%zu\n", i, (char *)objs[i].data, objs[i].size);
 *  }
 *  tbl->freemulti(objs);
 * @endcode
 */
qlisttbl_data_t *qlisttbl_getmulti(qlisttbl_t *tbl, const char *name, bool newmem,
                                   size_t *numobjs)
{
    qlisttbl_data_t *objs = NULL;  // objects container
    size_t allocobjs = 0;  // allocated number of objs
    size_t numfound = 0;  // number of keys found

    qlisttbl_obj_t obj;
    memset((void *)&obj, 0, sizeof(obj)); // must be cleared before call
    qlisttbl_lock(tbl);
    while (tbl->getnext(tbl, &obj, name, newmem) == true) {
        numfound++;

        // allocate object array.
        if (numfound >= allocobjs) {
            if (allocobjs == 0) allocobjs = 10;  // start from 10
            else allocobjs *= 2;  // double size
            objs = (qlisttbl_data_t *)realloc(objs, sizeof(qlisttbl_data_t) * allocobjs);
            if (objs == NULL) {
                DEBUG("qlisttbl->getmulti(): Memory reallocation failure.");
                errno = ENOMEM;
                break;
            }
        }

        // copy reference
        qlisttbl_data_t *newobj = &objs[numfound - 1];
        newobj->data = obj.data;
        newobj->size = obj.size;
        newobj->type = (newmem == false) ? 1 : 2;

        // release resource
        if (newmem == true) {
            if (obj.name != NULL) free(obj.name);
        }

        // clear next block
        newobj = &objs[numfound];
        memset((void *)newobj, '\0', sizeof(qlisttbl_data_t));
        newobj->type = 0;  // mark, end of objects
    }
    qlisttbl_unlock(tbl);

    // return found counter
    if (numobjs != NULL) {
        *numobjs = numfound;
    }

    if (numfound == 0) {
        errno = ENOENT;
    }

    return objs;
}

/**
 * qlisttbl->freemulti(): Deallocate object array returned by getmulti().
 *
 * @param objs      pointer of array of qlisttbl_data_t.
 *
 * @code
 *  qlisttbl_data_t *objs = tbl->getmulti(tbl, "newmem is true", true, &numobjs);
 *  tbl->freemulti(objs);  // frees allocated objects and object array itself.
 *
 *  qlisttbl_data_t *objs = tbl->getmulti(tbl, "even newmem is false", false, &numobjs);
 *  tbl->freemulti(objs);  // frees object array itself.
 *
 * @endcode
 */
void qlisttbl_freemulti(qlisttbl_data_t *objs)
{
    if (objs == NULL) return;

    qlisttbl_data_t *obj;
    for (obj = &objs[0]; obj->type == 2; obj++) {
        if (obj->data != NULL) free(obj->data);
    }

    free(objs);
}

/**
 * qlisttbl->remove(): Remove matched objects with given name.
 *
 * @param tbl   qlisttbl container pointer.
 * @param name  element name.
 *
 * @return a number of removed objects.
 */
size_t qlisttbl_remove(qlisttbl_t *tbl, const char *name)
{
    if (name == NULL) return false;

    size_t numremoved = 0;

    qlisttbl_obj_t obj;
    memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
    qlisttbl_lock(tbl);
    while(qlisttbl_getnext(tbl, &obj, name, false) == true) {
        qlisttbl_removeobj(tbl, &obj);
        numremoved++;
    }
    qlisttbl_unlock(tbl);

    return numremoved;
}

/**
 * qlisttbl->removeobj(): Remove objects with given object pointer.
 *
 * This call is useful when you want to remove an element while traversing a
 * table using getnext(). So the address pointed by obj maybe different than
 * the actual object in a table, but it's ok because we'll recalculate the
 * actual object address by referring it's links.
 *
 * @param tbl   qlisttbl container pointer.
 * @param name  element name.
 *
 * @return true if succeed on deletion, false if failed.
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *
 * @code
 *  qlisttbl_obj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  tbl->lock(tbl);
 *  while(tbl->getnext(tbl, &obj, NULL, true) == true) {
 *    tbl->removeobj(tbl, &obj);  // this will remove all entries in the table
 *  }
 *  tbl->unlock(tbl);
 * @endcode
 */
bool qlisttbl_removeobj(qlisttbl_t *tbl, const qlisttbl_obj_t *obj)
{
    if (obj == NULL) return false;

    qlisttbl_lock(tbl);

    // copy chains
    qlisttbl_obj_t *prev = obj->prev;
    qlisttbl_obj_t *next = obj->next;

    // find this object
    qlisttbl_obj_t *this = NULL;
    if (prev != NULL) this = prev->next;
    else if (next != NULL) this = next->prev;
    else this = tbl->first;  // table has only one object.

    // double check
    if (this == NULL) {
        qlisttbl_unlock(tbl);
        DEBUG("qlisttbl->removeobj(): Can't veryfy object.");
        errno = ENOENT;
        return false;
    }

    // adjust chain links
    if (prev == NULL) tbl->first = next; // if the object is first one
    else prev->next = next;  // not the first one

    if (next == NULL) tbl->last = prev; // if the object is last one
    else next->prev = prev;  // not the first one

    // adjust counter
    tbl->num--;

    qlisttbl_unlock(tbl);

    // free object
    free(this->name);
    free(this->data);
    free(this);

    return true;
}

/**
 * qlisttbl->getnext(): Get next element.
 *
 * Default searching direction is backward, from the bottom to top
 * unless QLISTTBL_LOOKUPFORWARD option was specified.
 *
 * @param tbl       qlisttbl container pointer.
 * @param obj       found data will be stored in this object
 * @param name      element name., if key name is NULL search every objects in
 *                  the table.
 * @param newmem    whether or not to allocate memory for the data.
 *
 * @return true if found otherwise returns false
 * @retval errno will be set in error condition.
 *  - ENOENT : No next element.
 *  - ENOMEM : Memory allocation failure.
 *
 * @note
 *  The obj should be initialized with 0 by using memset() before first call.
 *  If newmem flag is true, user should de-allocate obj.name and obj.data
 *  resources.
 *
 * @code
 *  qlisttbl_t *tbl = qlisttbl();
 *  (...add data into table...)
 *
 *  // non-thread usages
 *  qlisttbl_obj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  while(tbl->getnext(tbl, &obj, NULL, false) == true) {
 *    printf("NAME=%s, DATA=%s, SIZE=%zu\n",
 *           obj.name, (char*)obj.data, obj.size);
 *  }
 *
 *  // thread model with particular key search
 *  qlisttbl_obj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  tbl->lock(tbl);
 *  while(tbl->getnext(tbl, &obj, "key_name", true) == true) {
 *    printf("NAME=%s, DATA=%s, SIZE=%zu\n",
 *           obj.name, (char*)obj.data, obj.size);
 *    free(obj.name);
 *    free(obj.data);
 *  }
 *  tbl->unlock(tbl);
 * @endcode
 */
bool qlisttbl_getnext(qlisttbl_t *tbl, qlisttbl_obj_t *obj, const char *name,
                             bool newmem)
{
    if (obj == NULL) return NULL;

    qlisttbl_lock(tbl);

    qlisttbl_obj_t *cont = NULL;
    if (obj->size == 0) {  // first time call
        if (name == NULL) {  // full scan
            cont = (tbl->lookupforward) ? tbl->first : tbl->last;
        } else {  // name search
            cont = findobj(tbl, name, NULL);
        }
    } else {  // next call
        cont = (tbl->lookupforward) ? obj->next : obj->prev;
    }

    if (cont == NULL) {
        errno = ENOENT;
        qlisttbl_unlock(tbl);
        return false;
    }

    uint32_t hash = (name != NULL) ? qhashmurmur3_32(name, strlen(name)) : 0;

    bool ret = false;
    while (cont != NULL) {
        if (name == NULL || tbl->namematch(cont, name, hash) == true) {
            if (newmem == true) {
                obj->name = strdup(cont->name);
                obj->data = malloc(cont->size);
                if (obj->name == NULL || obj->data == NULL) {
                    if (obj->name != NULL) free(obj->name);
                    if (obj->data != NULL) free(obj->data);
                    obj->name = NULL;
                    obj->data = NULL;
                    errno = ENOMEM;
                    break;
                }
                memcpy(obj->data, cont->data, cont->size);
            } else {
                obj->name = cont->name;
                obj->data = cont->data;
            }
            obj->hash = cont->hash;
            obj->size = cont->size;
            obj->prev = cont->prev;
            obj->next = cont->next;

            ret = true;
            break;
        }

        cont = (tbl->lookupforward) ? cont->next : cont->prev;
    }
    qlisttbl_unlock(tbl);

    if (ret == false) {
        errno = ENOENT;
    }

    return ret;
}

/**
 * qlisttbl->size(): Returns the number of elements in this table.
 *
 * @param tbl qlisttbl container pointer.
 *
 * @return the number of elements in this table.
 */
size_t qlisttbl_size(qlisttbl_t *tbl)
{
    return tbl->num;
}

/**
 * qlisttbl->sort(): Sort keys in this table.
 *
 * @param tbl           qlisttbl container pointer.
 *
 * @note
 *  It will sort the table in ascending manner, if you need descending order somehow,
 *  lookup-backword option will do the job without changing the order in the table.
 *
 * @code
 *  The appearence order of duplicated keys will be preserved in a sored table.
 *  See how duplicated key 'b' is ordered in below example
 *
 *  [Unsorted]   [sorted ASC]    [sorted DES]
 *    d = 1          a = 2           d = 1
 *    a = 2          b = 3           c = 5
 *    b = 3          b = 4           b = 3
 *    b = 4          b = 6           b = 4
 *    c = 5          c = 5           b = 6
 *    b = 6          d = 1           a = 2
 * @endcode
 */
void qlisttbl_sort(qlisttbl_t *tbl)
{
    // run bubble sort
    qlisttbl_lock(tbl);
    qlisttbl_obj_t *obj1, *obj2;
    qlisttbl_obj_t tmpobj;
    int n, n2, i;
    for (n = tbl->num; n > 0;) {
        n2 = 0;
        for (i = 0, obj1 = tbl->first; i < (n - 1); i++, obj1 = obj1->next) {
            obj2 = obj1->next;  // this can't be null.
            if (tbl->namecmp(obj1->name, obj2->name) > 0) {
                // swapping contents is faster than adjusting links.
                tmpobj = *obj1;
                obj1->hash = obj2->hash;
                obj1->name = obj2->name;
                obj1->data = obj2->data;
                obj1->size = obj2->size;
                obj2->hash = tmpobj.hash;
                obj2->name = tmpobj.name;
                obj2->data = tmpobj.data;
                obj2->size = tmpobj.size;

                n2 = i + 1;
            }
        }
        n = n2;  // skip sorted tailing elements
    }
    qlisttbl_unlock(tbl);
}

/**
 * qlisttbl->clear(): Removes all of the elements from this table.
 *
 * @param tbl qlisttbl container pointer.
 */
void qlisttbl_clear(qlisttbl_t *tbl)
{
    qlisttbl_lock(tbl);
    qlisttbl_obj_t *obj;
    for (obj = tbl->first; obj != NULL;) {
        qlisttbl_obj_t *next = obj->next;
        free(obj->name);
        free(obj->data);
        free(obj);
        obj = next;
    }

    tbl->num = 0;
    tbl->first = NULL;
    tbl->last = NULL;
    qlisttbl_unlock(tbl);
}

/**
 * qlisttbl->save(): Save qlisttbl as plain text format
 * Dumping direction is always forward(from the top to the bottom) to preserve
 * the order when we load the file again.
 *
 * @param tbl       qlisttbl container pointer.
 * @param filepath  save file path
 * @param sepchar   separator character between name and value. normally '=' is
 *                  used.
 * @param encode    flag for encoding data . false can be used if the all data
 *                  are string or integer type and has no new line. otherwise
 *                  true must be set.
 *
 * @return true if successful, otherwise returns false.
 */
bool qlisttbl_save(qlisttbl_t *tbl, const char *filepath, char sepchar,
                   bool encode)
{
    if (filepath == NULL) {
        errno = EINVAL;
        return false;
    }

    int fd;
    if ((fd = open(filepath, O_CREAT|O_WRONLY|O_TRUNC, (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH))) < 0) {
        DEBUG("qlisttbl->save(): Can't open file %s", filepath);
        return false;
    }

    char *gmtstr = qtime_gmt_str(0);
    qio_printf(fd, -1, "# %s %s\n", filepath, gmtstr);
    free(gmtstr);

    qlisttbl_lock(tbl);
    qlisttbl_obj_t *obj;
    for (obj = tbl->first; obj; obj = obj->next) {
        char *encval;
        if (encode == true) encval = qurl_encode(obj->data, obj->size);
        else encval = obj->data;
        qio_printf(fd, -1, "%s%c%s\n", obj->name, sepchar, encval);
        if (encode == true) free(encval);
    }
    qlisttbl_unlock(tbl);

    close(fd);
    return true;
}

/**
 * qlisttbl->load(): Load and append entries from given filepath
 * Loading direction is always appending at the bottom of the table to preserve
 * the order as it was.
 *
 * @param tbl       qlisttbl container pointer.
 * @param filepath  save file path
 * @param sepchar   separator character between name and value. normally '=' is
 *                  used
 * @param decode    flag for decoding data
 *
 * @return the number of loaded entries, otherwise returns -1.
 */
ssize_t qlisttbl_load(qlisttbl_t *tbl, const char *filepath, char sepchar,
                      bool decode)
{
    // load file
    char *str = qfile_load(filepath, NULL);
    if (str == NULL) return -1;

    // parse
    qlisttbl_lock(tbl);
    char *offset, *buf;
    int cnt = 0;
    for (offset = str; *offset != '\0'; ) {
        // get one line into buf
        for (buf = offset; *offset != '\n' && *offset != '\0'; offset++);
        if (*offset != '\0') {
            *offset = '\0';
            offset++;
        }
        qstrtrim(buf);

        // skip blank or comment line
        if ((buf[0] == '#') || (buf[0] == '\0')) continue;

        // parse
        char *data = strdup(buf);
        char *name  = _q_makeword(data, sepchar);
        qstrtrim(data);
        qstrtrim(name);
        if (decode == true) qurl_decode(data);

        // add to the table.
        qlisttbl_put(tbl, name, data, strlen(data) + 1);

        free(name);
        free(data);
    }
    qlisttbl_unlock(tbl);
    free(str);

    return cnt;
}

/**
 * qlisttbl->debug(): Print out stored elements for debugging purpose.
 *
 * @param tbl qlisttbl container pointer.
 * @param out output stream FILE descriptor such like stdout, stderr.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EIO : Invalid output stream.
 */
bool qlisttbl_debug(qlisttbl_t *tbl, FILE *out)
{
    if (out == NULL) {
        errno = EIO;
        return false;
    }

    qlisttbl_lock(tbl);
    qlisttbl_obj_t *obj;
    for (obj = tbl->first; obj; obj = obj->next) {
        fprintf(out, "%s=" , obj->name);
        _q_textout(out, obj->data, obj->size, MAX_HUMANOUT);
        fprintf(out, " (%zu, %08x)\n" , obj->size, obj->hash);
    }
    qlisttbl_unlock(tbl);

    return true;
}

/**
 * qlisttbl->lock(): Enter critical section.
 *
 * @param tbl qlisttbl container pointer.
 *
 * @note
 *  Normally explicit locking is only needed when traverse all the
 *  elements with qlisttbl->getnext().
 */
void qlisttbl_lock(qlisttbl_t *tbl)
{
    Q_MUTEX_ENTER(tbl->qmutex);
}

/**
 * qlisttbl->unlock(): Leave critical section.
 *
 * @param tbl qlisttbl container pointer.
 */
void qlisttbl_unlock(qlisttbl_t *tbl)
{
    Q_MUTEX_LEAVE(tbl->qmutex);
}

/**
 * qlisttbl->free(): Free qlisttbl_t
 *
 * @param tbl qlisttbl container pointer.
 */
void qlisttbl_free(qlisttbl_t *tbl)
{
    qlisttbl_clear(tbl);
    Q_MUTEX_DESTROY(tbl->qmutex);
    free(tbl);
}

#ifndef _DOXYGEN_SKIP

// lock must be obtained from caller
static qlisttbl_obj_t *newobj(const char *name, const void *data, size_t size)
{
    if (name == NULL || data == NULL || size <= 0) {
        errno = EINVAL;
        return false;
    }

    // make a new object
    char *dup_name = strdup(name);
    void *dup_data = malloc(size);
    qlisttbl_obj_t *obj = (qlisttbl_obj_t *)malloc(sizeof(qlisttbl_obj_t));
    if (dup_name == NULL || dup_data == NULL || obj == NULL) {
        if (dup_name != NULL) free(dup_name);
        if (dup_data != NULL) free(dup_data);
        if (obj != NULL) free(obj);
        errno = ENOMEM;
        return NULL;
    }
    memcpy(dup_data, data, size);
    memset((void *)obj, '\0', sizeof(qlisttbl_obj_t));

    // obj->hash = qhashmurmur3_32(dup_name);
    obj->name = dup_name;
    obj->data = dup_data;
    obj->size = size;

    return obj;
}

// lock must be obtained from caller
static bool insertobj(qlisttbl_t *tbl, qlisttbl_obj_t *obj)
{
    // update hash
    obj->hash = qhashmurmur3_32(obj->name, strlen(obj->name));

    qlisttbl_obj_t *prev = obj->prev;
    qlisttbl_obj_t *next = obj->next;

    if (prev == NULL) tbl->first = obj;
    else prev->next = obj;

    if (next == NULL) tbl->last = obj;
    else next->prev = obj;

     // increase counter
    tbl->num++;

    return true;
}

// lock must be obtained from caller
static qlisttbl_obj_t *findobj(qlisttbl_t *tbl, const char *name, qlisttbl_obj_t *retobj)
{
    if (retobj != NULL) {
        memset((void *)retobj, '\0', sizeof(qlisttbl_obj_t));
    }

    if (name == NULL || tbl->num == 0) {
        errno = ENOENT;
        return NULL;
    }

    uint32_t hash = qhashmurmur3_32(name, strlen(name));
    qlisttbl_obj_t *obj = (tbl->lookupforward) ? tbl->first : tbl->last;
    while (obj != NULL) {
        // name string will be compared only if the hash matches.
        if (tbl->namematch(obj, name, hash) == true) {
           if (retobj != NULL) {
                *retobj = *obj;
            }
            return obj;
        }
        obj = (tbl->lookupforward)? obj->next : obj->prev;
    }

    // not found, set prev and next chain.
    if (retobj != NULL) {
        if (tbl->inserttop) {
            retobj->prev = NULL;
            retobj->next = tbl->first;
        } else {
            retobj->prev = tbl->last;
            retobj->next = NULL;
        }
    }
    errno = ENOENT;

    return NULL;
}

// key comp
static bool namematch(qlisttbl_obj_t *obj, const char *name, uint32_t hash)
{
    if ((obj->hash == hash) && !strcmp(obj->name, name)) {
        return true;
    }
    return false;
}

static bool namecasematch(qlisttbl_obj_t *obj, const char *name, uint32_t hash)
{
    if (!strcasecmp(obj->name, name)) {
        return true;
    }
    return false;
}

#endif /* _DOXYGEN_SKIP */
