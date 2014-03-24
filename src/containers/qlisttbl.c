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
 * @file qlisttbl.c Linked-list-table implementation.
 *
 * qlisttbl container is a Linked-List-Table implementation.
 * Which maps keys to values. Key is a string and value is any non-null object.
 * These elements are stored sequentially in Doubly-Linked-List data structure.
 *
 * Compared to Hash-Table, List-Table is efficient when you need to keep
 * duplicated keys since Hash-Table only keep unique keys. Of course, qlisttbl
 * supports both behavior storing unique key or allowing key duplication.
 *
 * qlisttbl also provides sorting feature. It supports a table stays in sorted
 * and also one-time sorting. When a table is set to be sorted, it can speed up
 * it's lookup performance by skipping unnecessary table scans. This option is
 * disabled by default so it should be explictly turned on.
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
 *  qlisttbl_t *tbl = qlisttbl(QLISTTBL_OPT_THREADSAFE);
 *  tbl->setsort(tbl, true, false);  // set table to be kept sorted
 *
 *  // insert elements (key duplication allowed)
 *  tbl->put(tbl, "e1", "a", strlen("e1")+1, false); // equal to putstr();
 *  tbl->putstr(tbl, "e2", "b", false);
 *  tbl->putstr(tbl, "e2", "c", false);
 *  tbl->putstr(tbl, "e3", "d", false);
 *
 *  // debug output
 *  tbl->debug(tbl, stdout, true);
 *
 *  // get element
 *  printf("get('e2') : %s\n", (char*)tbl->get(tbl, "e2", NULL, false));
 *  printf("getstr('e2') : %s\n", tbl->getstr(tbl, "e2", false));
 *
 *  // get all matching elements
 *  qobj_t *objs = tbl->getmulti(tbl, "e2", true, NULL);
 *  for (i = 0; objs[i].data != NULL; i++) {
 *      printf("getmulti('e2')[%d] : %s (size=%d)\n",
 *              i, (char *)objs[i].data, (int)objs[i].size);
 *  }
 *  tbl->freemulti(objs);
 *
 *  // find every 'e2' elements
 *  qdlnobj_t obj;
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
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include "qlibc.h"
#include "qinternal.h"

/*
 * Member method protos
 */
#ifndef _DOXYGEN_SKIP

static bool setcase(qlisttbl_t *tbl, bool insensitive);
static int setsort(qlisttbl_t *tbl, bool sort, bool descending);
static bool setputdir(qlisttbl_t *tbl, bool before);
static bool setgetdir(qlisttbl_t *tbl, bool first);
static bool setnextdir(qlisttbl_t *tbl, bool backward);

static bool put(qlisttbl_t *tbl, const char *name, const void *data,
                size_t size, bool unique);
static bool putstr(qlisttbl_t *tbl, const char *name, const char *str,
                   bool unique);
static bool putstrf(qlisttbl_t *tbl, bool unique, const char *name,
                    const char *format, ...);
static bool putint(qlisttbl_t *tbl, const char *name, int64_t num, bool unique);

static void *get(qlisttbl_t *tbl, const char *name, size_t *size, bool newmem);
static char *getstr(qlisttbl_t *tbl, const char *name, bool newmem);
static int64_t getint(qlisttbl_t *tbl, const char *name);

static qobj_t *getmulti(qlisttbl_t *tbl, const char *name, bool newmem,
                        size_t *numobjs);
static void freemulti(qobj_t *objs);
static bool getnext(qlisttbl_t *tbl, qdlnobj_t *obj, const char *name,
                    bool newmem);

static size_t remove_(qlisttbl_t *tbl, const char *name);
static bool removeobj(qlisttbl_t *tbl, const qdlnobj_t *obj);

static size_t size(qlisttbl_t *tbl);
static void sort_(qlisttbl_t *tbl, bool descending);
static void reverse(qlisttbl_t *tbl);
static void clear(qlisttbl_t *tbl);

static bool save(qlisttbl_t *tbl, const char *filepath, char sepchar,
                 bool encode);
static ssize_t load(qlisttbl_t *tbl, const char *filepath, char sepchar,
                    bool decode);
static bool debug(qlisttbl_t *tbl, FILE *out);

static void lock(qlisttbl_t *tbl);
static void unlock(qlisttbl_t *tbl);

static void free_(qlisttbl_t *tbl);

/* internal functions */
static bool _put(qlisttbl_t *tbl, const char *name, const void *data,
                 size_t size, bool unique, bool first);
static void *_get(qlisttbl_t *tbl, const char *name, size_t *size, bool newmem,
                  bool first);

static qdlnobj_t *_createobj(const char *name, const void *data, size_t size);
static bool _insertobj(qlisttbl_t *tbl, qdlnobj_t *obj);
static qdlnobj_t *_findobj(qlisttbl_t *tbl, const char *name,
                           bool first,
                           qdlnobj_t *retobj);

static bool _namematch(qdlnobj_t *obj, const char *name, uint32_t hash);
static bool _namecasematch(qdlnobj_t *obj, const char *name, uint32_t hash);
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
 *  qlisttbl_t *threadsafe_tbl = qlisttbl(QLISTTBL_OPT_THREADSAFE);
 * @endcode
 *
 * @note
 *   Available options:
 *   - QLISTTBL_OPT_THREADSAFE - make it thread-safe.
 */
qlisttbl_t *qlisttbl(int options)
{
    qlisttbl_t *tbl = (qlisttbl_t *)calloc(1, sizeof(qlisttbl_t));
    if (tbl == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    // handle options.
    if (options & QLISTTBL_OPT_THREADSAFE) {
        Q_MUTEX_NEW(tbl->qmutex, true);
        if (tbl->qmutex == NULL) {
            errno = ENOMEM;
            free(tbl);
            return NULL;
        }
    }

    // member methods
    tbl->setcase    = setcase;
    tbl->setsort    = setsort;
    tbl->setputdir  = setputdir;
    tbl->setgetdir  = setgetdir;
    tbl->setnextdir  = setnextdir;

    tbl->put        = put;
    tbl->putstr     = putstr;
    tbl->putstrf    = putstrf;
    tbl->putint     = putint;

    tbl->get        = get;
    tbl->getstr     = getstr;
    tbl->getint     = getint;

    tbl->getmulti   = getmulti;
    tbl->freemulti  = freemulti;

    tbl->getnext    = getnext;

    tbl->remove     = remove_;
    tbl->removeobj  = removeobj;


    tbl->size       = size;
    tbl->sort       = sort_;
    tbl->reverse    = reverse;
    tbl->clear      = clear;

    tbl->save       = save;
    tbl->load       = load;
    tbl->debug      = debug;

    tbl->lock       = lock;
    tbl->unlock     = unlock;

    tbl->free       = free_;

    // private methods
    tbl->namematch  = _namematch;
    tbl->namecmp    = strcmp;

    // these are the defaults - we don't need to call them but just to be clear.
    //setcase(tbl, false);     // case sensitive
    //setsort(tbl, false);     // no sort
    //setputdir(tbl, false);   // append at bottom of list
    //setgetdir(tbl, false);   // return last object for duplicated keys.
    //setnextdir(tbl, false);  // forward

    return tbl;
}

/**
 * qlisttbl->setcase(): Sets case sensitivity for key lookup.
 * When this option is turned on, hash comparison will be disabled and
 * each key will be compared. So lookup up speed will be relatively slower.
 * If setsort() is on, table will be resorted to sort keys in case insensitive
 * order.
 *
 * @param tbl           qlisttbl container pointer.
 * @param insensitive   false for case-sensitive, true for case-insensitive.
 *
 * @return previous setting
 */
static bool setcase(qlisttbl_t *tbl, bool insensitive)
{
    if (tbl->lookupcase == insensitive) {
        return tbl->lookupcase;
    }

    lock(tbl);
    bool prevcase = tbl->lookupcase;
    tbl->lookupcase = insensitive;

    // set new compfunc.
    if (insensitive == false) {
        tbl->namematch = _namematch;
        tbl->namecmp = strcmp;
    } else {
        tbl->namematch = _namecasematch;
        tbl->namecmp = strcasecmp;
    }

    // resort if sort option is on.
    if (tbl->sortflag != 0) {
        if (tbl->sortflag == 1) sort_(tbl, false);
        else if (tbl->sortflag == 2) sort_(tbl, true);
    }
    unlock(tbl);

    return prevcase;
}

/**
 * qlisttbl->setsort(): Sets sort flag to keep table sorted with it's keys.
 * Table will be maintained in sorted manner by it's keys. New elements will
 * be also inserted at corresponding chain, so the table will be kept in sorted
 * order.
 *
 * @param tbl           qlisttbl container pointer.
 * @param sort          sort flag. false(default) no sort, true keep it sorted.
 * @param descending    sorting order. false for ascending, true for descending.
 *
 * @return previous setting. 0: disabled, 1: ascending, 2: descending.
 */
static int setsort(qlisttbl_t *tbl, bool sort, bool descending)
{
    lock(tbl);
    int prevflag = tbl->sortflag;

    if (sort == false) {
        tbl->sortflag = 0;
    } else {
        if (descending == false) tbl->sortflag = 1;  // ascending
        else tbl->sortflag = 2;  // descending

        // sort table
        if (prevflag != tbl->sortflag) {
            sort_(tbl, descending);
        }
    }
    unlock(tbl);

    return prevflag;
}

/**
 * qlisttbl->setputdir(): Sets adding direction(at last of first).
 * The default direction is adding new element at the end of table.
 *
 * @param tbl       qlisttbl container pointer.
 * @param before    direction flag. false(default) for adding at the end of
 *                  this table, true for adding at the beginning of this table.
 *
 * @return previous direction.
 */
static bool setputdir(qlisttbl_t *tbl, bool before)
{
    bool prevdir = tbl->putdir;
    tbl->putdir = before;

    return prevdir;
}

/**
 * qlisttbl->setgetdir(): Sets lookup direction(backward or forward).
 * The default direction is backward(from the bottom to the top), so if
 * there are duplicated keys then later added one will be picked up first.
 *
 * @param tbl       qlisttbl container pointer.
 * @param first     direction flag. false(default) for searching from the
 *                  bottom of this table. true for searching from the top of
 *                  this table.
 *
 * @return previous direction.
 */
static bool setgetdir(qlisttbl_t *tbl, bool first)
{
    bool prevdir = tbl->getdir;
    tbl->getdir = first;

    return prevdir;
}

/**
 * qlisttbl->setnextdir(): Sets table traversal direction(forward or
 * backward). The default direction is forward(from the top to the bottom).
 *
 * @param tbl       qlisttbl container pointer.
 * @param backward  direction flag. false(default) for traversal from the top
 *                  of this table, true for searching from the bottom of this
 *                  table.
 *
 * @return previous direction.
 */
static bool setnextdir(qlisttbl_t *tbl, bool backward)
{
    bool prevdir = tbl->nextdir;
    tbl->nextdir = backward;

    return prevdir;
}

/**
 * qlisttbl->put(): Put an element to this table.
 * Default behavior is adding an element at the end of this table unless changed
 * by setputdir().
 *
 * @param tbl       qlisttbl container pointer.
 * @param name      element name.
 * @param data      a pointer which points data memory.
 * @param size      size of the data.
 * @param unique    set true to remove existing elements of same name if exists,
 *                  false for adding.
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
 *  tbl->put(tbl, "obj1", &obj, sizeof(struct my_obj), false);
 * @endcode
 *
 * @note
 *  The default behavior is adding an object at the end of this table unless
 *  it's changed by setputdir().
 */
static bool put(qlisttbl_t *tbl, const char *name, const void *data,
                size_t size, bool unique)
{
    return _put(tbl, name, data, size, unique, tbl->putdir);
}

/**
 * qlisttbl->putstr(): Put a string into this table.
 *
 * @param tbl       qlisttbl container pointer.
 * @param name      element name.
 * @param str       string data.
 * @param unique    set true to remove existing elements of same name if exists,
 *                  false for adding.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *  - EINVAL : Invalid argument.
 *
 * @note
 *  The default behavior is adding object at the end of this table unless it's
 *  changed by calling setputdir().
 */
static bool putstr(qlisttbl_t *tbl, const char *name, const char *str,
                   bool unique)
{
    size_t size = (str!=NULL) ? (strlen(str) + 1) : 0;
    return put(tbl, name, (const void *)str, size, unique);
}

/**
 * qlisttbl->putstrf(): Put a formatted string into this table.
 *
 * @param tbl       qlisttbl container pointer.
 * @param unique    set true to remove existing elements of same name if exists,
 *                  false for adding.
 * @param name      element name.
 * @param format    formatted value string.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *  - EINVAL : Invalid argument.
 *
 * @note
 *  The default behavior is adding object at the end of this table unless it's
 *  changed by calling setputdir().
 */
static bool putstrf(qlisttbl_t *tbl, bool unique, const char *name,
                    const char *format, ...)
{
    char *str;
    DYNAMIC_VSPRINTF(str, format);
    if (str == NULL) {
        errno = ENOMEM;
        return false;
    }

    bool ret = putstr(tbl, name, str, unique);
    free(str);

    return ret;
}

/**
 * qlisttbl->putInt(): Put an integer into this table as string type.
 *
 * @param tbl       qlisttbl container pointer.
 * @param name      element name.
 * @param num       number data.
 * @param unique    set true to remove existing elements of same name if exists,
 *                  false for adding.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *  - EINVAL : Invalid argument.
 *
 * @note
 *  The integer will be converted to a string object and stored as a string
 *  object. The default behavior is adding object at the end of this table
 *  unless it's changed by calling setputdir().
 */
static bool putint(qlisttbl_t *tbl, const char *name, int64_t num, bool unique)
{
    char str[20+1];
    snprintf(str, sizeof(str), "%"PRId64, num);
    return putstr(tbl, name, str, unique);
}

/**
 * qlisttbl->get(): Finds an object with given name.
 * If there are duplicate keys in the table, this will return the first matched
 * one from the top or bottom depending on setgetdir() setting. By default,
 * it'll return the first matched one from the bottom of table. So in case,
 * there are duplicated keys, it'll return the newly inserted one unless
 * setputdir() is set in the other way.
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
static void *get(qlisttbl_t *tbl, const char *name, size_t *size, bool newmem)
{
    return _get(tbl, name, size, newmem, tbl->getdir);
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
static char *getstr(qlisttbl_t *tbl, const char *name, bool newmem)
{
    return (char *)get(tbl, name, NULL, newmem);
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
static int64_t getint(qlisttbl_t *tbl, const char *name)
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
 * qlisttbl->getmulti(): Finds all objects with given name and return a array
 * of objects. If there are duplicate keys in the table, this will return all
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
 *  getmulti() returns faster if table is set to be sorted by setsort() option.
 *
 * @note
 *  The returned array of qobj_t should be released by freemulti() call after
 *  use. Even you call getmulti() with newmem set false, freemulti() should
 *  be called all the times, so the object array itself can be released.
 *
 * @code
 *  size_t numobjs = 0;
 *  qobj_t *objs = tbl->getmulti(tbl, "e2", true, &numobjs);
 *  for (i = 0; objs[i].data != NULL; i++) {
 *      printf("DATA=%s, SIZE=%zu\n", i, (char *)objs[i].data, objs[i].size);
 *  }
 *  tbl->freemulti(objs);
 * @endcode
 */
static qobj_t *getmulti(qlisttbl_t *tbl, const char *name, bool newmem,
                        size_t *numobjs)
{
    qobj_t *objs = NULL;  // objects container
    size_t allocobjs = 0;  // allocated number of objs
    size_t numfound = 0;  // number of keys found

    qdlnobj_t obj;
    memset((void *)&obj, 0, sizeof(obj)); // must be cleared before call
    lock(tbl);
    while (tbl->getnext(tbl, &obj, name, newmem) == true) {
        numfound++;

        // allocate object array.
        if (numfound >= allocobjs) {
            if (allocobjs == 0) allocobjs = 10;  // start from 10
            else allocobjs *= 2;  // double size
            objs = (qobj_t *)realloc(objs, sizeof(qobj_t) * allocobjs);
            if (objs == NULL) {
                DEBUG("qlisttbl->getmulti(): Memory reallocation failure.");
                errno = ENOMEM;
                break;
            }
        }

        // copy reference
        qobj_t *newobj = &objs[numfound - 1];
        newobj->data = obj.data;
        newobj->size = obj.size;
        newobj->type = (newmem == false) ? 1 : 2;

        // release resource
        if (newmem == true) {
            if (obj.name != NULL) free(obj.name);
        }

        // clear next block
        newobj = &objs[numfound];
        memset((void *)newobj, '\0', sizeof(qobj_t));
        newobj->type = 0;  // mark, end of objects
    }
    unlock(tbl);

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
 * @param objs      pointer of array of qobj_t.
 *
 * @code
 *  qobj_t *objs = tbl->getmulti(tbl, "newmem is true", true, &numobjs);
 *  tbl->freemulti(objs);  // frees allocated objects and object array itself.
 *
 *  qobj_t *objs = tbl->getmulti(tbl, "even newmem is false", false, &numobjs);
 *  tbl->freemulti(objs);  // frees object array itself.
 *
 * @endcode
 */
static void freemulti(qobj_t *objs)
{
    if (objs == NULL) return;

    qobj_t *obj;
    for (obj = &objs[0]; obj->type == 2; obj++) {
        if (obj->data != NULL) free(obj->data);
    }

    free(objs);
}

/**
 * qlisttbl->getnext(): Get next element.
 * Default searching direction is forward, from the top to to bottom,
 * unless it's changed by setnextdir().
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
 *  qdlnobj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  while(tbl->getnext(tbl, &obj, NULL, false) == true) {
 *    printf("NAME=%s, DATA=%s, SIZE=%zu\n",
 *           obj.name, (char*)obj.data, obj.size);
 *  }
 *
 *  // thread model with particular key search
 *  qdlnobj_t obj;
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
static bool getnext(qlisttbl_t *tbl, qdlnobj_t *obj, const char *name,
                    bool newmem)
{
    if (obj == NULL) return NULL;

    lock(tbl);

    qdlnobj_t *cont = NULL;
    if (obj->size == 0) {  // first time call
        if (name == NULL) {  // full scan
            if (tbl->nextdir == false) cont = tbl->first;
            else cont = tbl->last;
        } else {  // name search
            cont = _findobj(tbl, name, !(tbl->nextdir), NULL);
        }
    } else {  // next call
        if (tbl->nextdir == false) cont = obj->next;
        else cont = obj->prev;
    }

    if (cont == NULL) {
        errno = ENOENT;
        unlock(tbl);
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

        if (name != NULL && tbl->sortflag != 0) break;
        if (tbl->nextdir == false) cont = cont->next;
        else cont = cont->prev;
    }
    unlock(tbl);

    if (ret == false) {
        errno = ENOENT;
    }

    return ret;
}

/**
 * qlisttbl->remove(): Remove matched objects with given name.
 *
 * @param tbl   qlisttbl container pointer.
 * @param name  element name.
 *
 * @return a number of removed objects.
 */
static size_t remove_(qlisttbl_t *tbl, const char *name)
{
    if (name == NULL) return false;

    size_t numremoved = 0;

    qdlnobj_t obj;
    memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
    lock(tbl);
    while(getnext(tbl, &obj, name, false) == true) {
        removeobj(tbl, &obj);
        numremoved++;
    }
    unlock(tbl);

    return numremoved;
}

/**
 * qlisttbl->removeobj(): Remove objects with given object pointer.
 * This call is useful when you want to remove an element while traversing a
 * table using getnext(). So the address pointed by obj maybe different than
 * the actual object in a table, but it's ok becuase it'll recalculate actual
 * object address by referring it's links to previous and next objects.
 *
 * @param tbl   qlisttbl container pointer.
 * @param name  element name.
 *
 * @return true if succeed on deletion, false if failed.
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *
 * @code
 *  qdlnobj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  tbl->lock(tbl);
 *  while(tbl->getnext(tbl, &obj, NULL, true) == true) {
 *    tbl->removeobj(tbl, &obj);  // this will remove all entries in the table
 *  }
 *  tbl->unlock(tbl);
 * @endcode
 */
static bool removeobj(qlisttbl_t *tbl, const qdlnobj_t *obj)
{
    if (obj == NULL) return false;

    lock(tbl);

    // copy chains
    qdlnobj_t *prev = obj->prev;
    qdlnobj_t *next = obj->next;

    // find this object
    qdlnobj_t *this = NULL;
    if (prev != NULL) this = prev->next;
    else if (next != NULL) this = next->prev;
    else this = tbl->first;  // table has only one object.

    // double check
    if (this == NULL) {
        unlock(tbl);
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

    unlock(tbl);

    // free object
    free(this->name);
    free(this->data);
    free(this);

    return true;
}

/**
 * qlisttbl->size(): Returns the number of elements in this table.
 *
 * @param tbl qlisttbl container pointer.
 *
 * @return the number of elements in this table.
 */
static size_t size(qlisttbl_t *tbl)
{
    return tbl->num;
}

/**
 * qlisttbl->sort(): Sort keys in this table.
 *
 * @param tbl           qlisttbl container pointer.
 * @param descending    sorting order. false for ascending, true for descending.
 *
 * @note
 *  This is for one time sort, to keep a table sorted, set it on by setsort().
 *  Sorting algorithm here we use is Bubble-sort algorithm.
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
static void sort_(qlisttbl_t *tbl, bool descending)
{
    // run bubble sort
    lock(tbl);
    qdlnobj_t *obj1, *obj2;
    qdlnobj_t tmpobj;
    int n, n2, i;
    int adjustcmp = (descending == false) ? 1 : -1;
    for (n = tbl->num; n > 0;) {
        n2 = 0;
        for (i = 0, obj1 = tbl->first; i < (n - 1); i++, obj1 = obj1->next) {
            obj2 = obj1->next;  // this can't be null.
            if ((tbl->namecmp(obj1->name, obj2->name) * adjustcmp) > 0) {
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
    unlock(tbl);
}

/**
 * qlisttbl->reverse(): Reverse the order of elements.
 *
 * @param tbl qlisttbl container pointer.
 *
 * @return true if successful otherwise returns false.
 *
 * @note
 *  If setsort() option is set, this will also revert the sorting order
 *  from ascending to descending or from descending to ascending.
 */
static void reverse(qlisttbl_t *tbl)
{
    lock(tbl);
    qdlnobj_t *obj;
    for (obj = tbl->first; obj;) {
        qdlnobj_t *next = obj->next;
        obj->next = obj->prev;
        obj->prev = next;
        obj = next;
    }

    obj = tbl->first;
    tbl->first = tbl->last;
    tbl->last = obj;

    // change sort type
    if (tbl->sortflag == 1) tbl->sortflag = 2;
    else if (tbl->sortflag == 2) tbl->sortflag = 1;

    unlock(tbl);
}

/**
 * qlisttbl->clear(): Removes all of the elements from this table.
 *
 * @param tbl qlisttbl container pointer.
 */
static void clear(qlisttbl_t *tbl)
{
    lock(tbl);
    qdlnobj_t *obj;
    for (obj = tbl->first; obj != NULL;) {
        qdlnobj_t *next = obj->next;
        free(obj->name);
        free(obj->data);
        free(obj);
        obj = next;
    }

    tbl->num = 0;
    tbl->first = NULL;
    tbl->last = NULL;
    unlock(tbl);
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
static bool save(qlisttbl_t *tbl, const char *filepath, char sepchar,
                 bool encode)
{
    if (filepath == NULL) {
        errno = EINVAL;
        return false;
    }

    int fd;
    if ((fd = open(filepath, O_CREAT|O_WRONLY|O_TRUNC, DEF_FILE_MODE)) < 0) {
        DEBUG("qlisttbl->save(): Can't open file %s", filepath);
        return false;
    }

    char *gmtstr = qtime_gmt_str(0);
    qio_printf(fd, -1, "# Generated by " _Q_PRGNAME " at %s.\n", gmtstr);
    qio_printf(fd, -1, "# %s\n", filepath);
    free(gmtstr);

    lock(tbl);
    qdlnobj_t *obj;
    for (obj = tbl->first; obj; obj = obj->next) {
        char *encval;
        if (encode == true) encval = qurl_encode(obj->data, obj->size);
        else encval = obj->data;
        qio_printf(fd, -1, "%s%c%s\n", obj->name, sepchar, encval);
        if (encode == true) free(encval);
    }
    unlock(tbl);

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
static ssize_t load(qlisttbl_t *tbl, const char *filepath, char sepchar,
                    bool decode)
{
    // load file
    char *str = qfile_load(filepath, NULL);
    if (str == NULL) return -1;

    // parse
    lock(tbl);
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

        // append at the bottom of table.
        _put(tbl, name, data, strlen(data) + 1, false, false);

        free(name);
        free(data);
    }
    unlock(tbl);
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
static bool debug(qlisttbl_t *tbl, FILE *out)
{
    if (out == NULL) {
        errno = EIO;
        return false;
    }

    lock(tbl);
    qdlnobj_t *obj;
    for (obj = tbl->first; obj; obj = obj->next) {
        fprintf(out, "%s=" , obj->name);
        _q_humanOut(out, obj->data, obj->size, MAX_HUMANOUT);
        fprintf(out, " (%zu,%4x)\n" , obj->size, obj->hash);
    }
    unlock(tbl);

    return true;
}

/**
 * qlisttbl->lock(): Enter critical section.
 *
 * @param tbl qlisttbl container pointer.
 *
 * @note
 *  From user side, normally locking operation is only needed when traverse all
 *  elements using qlisttbl->getnext().
 */
static void lock(qlisttbl_t *tbl)
{
    Q_MUTEX_ENTER(tbl->qmutex);
}

/**
 * qlisttbl->unlock(): Leave critical section.
 *
 * @param tbl qlisttbl container pointer.
 */
static void unlock(qlisttbl_t *tbl)
{
    Q_MUTEX_LEAVE(tbl->qmutex);
}

/**
 * qlisttbl->free(): Free qlisttbl_t
 *
 * @param tbl qlisttbl container pointer.
 */
static void free_(qlisttbl_t *tbl)
{
    clear(tbl);
    Q_MUTEX_DESTROY(tbl->qmutex);
    free(tbl);
}

#ifndef _DOXYGEN_SKIP

static bool _put(qlisttbl_t *tbl, const char *name, const void *data,
                 size_t size, bool unique, bool first)
{
    // make new object table
    qdlnobj_t *obj = _createobj(name, data, size);
    if (obj == NULL) {
        return false;
    }

    // lock table
    lock(tbl);

    // if unique flag is set, remove same key
    if (unique == true) remove_(tbl, name);

    // insert into table
    if (tbl->num == 0) {
        obj->prev = NULL;
        obj->next = NULL;
    } else {
        if (tbl->sortflag == 0) {  // unsorted table
            if (first == true) {
                obj->prev = NULL;
                obj->next = tbl->first;
            } else {
                obj->prev = tbl->last;
                obj->next = NULL;
            }
        } else {  // sorted table
            qdlnobj_t posobj;
            qdlnobj_t *matchobj = _findobj(tbl, name, first, &posobj);
            if (matchobj == NULL) {
                obj->prev = posobj.prev;
                obj->next = posobj.next;
            } else if (first == true) {
                obj->prev = posobj.prev;
                obj->next = matchobj;
            } else {
                obj->prev = matchobj;
                obj->next = posobj.next;
            }
        }
    }
    _insertobj(tbl, obj);

    // unlock table
    unlock(tbl);

    return true;
}

static void *_get(qlisttbl_t *tbl, const char *name, size_t *size, bool newmem,
                  bool first)
{
    if (name == NULL) {
        errno = EINVAL;
        return NULL;
    }

    lock(tbl);
    void *data = NULL;
    qdlnobj_t *obj = _findobj(tbl, name, first, NULL);
    if (obj != NULL) {
        // get data
        if (newmem == true) {
            data = malloc(obj->size);
            if (data == NULL) {
                errno = ENOMEM;
                unlock(tbl);
                return NULL;
            }
            memcpy(data, obj->data, obj->size);
        } else {
            data = obj->data;
        }

        // set size
        if (size != NULL) *size = obj->size;
    }
   unlock(tbl);

    if (data == NULL) {
        errno = ENOENT;
    }

    return data;
}

// lock must be obtained from caller
static qdlnobj_t *_createobj(const char *name, const void *data, size_t size)
{
    if (name == NULL || data == NULL || size <= 0) {
        errno = EINVAL;
        return false;
    }

    // make a new object
    char *dup_name = strdup(name);
    void *dup_data = malloc(size);
    qdlnobj_t *obj = (qdlnobj_t *)malloc(sizeof(qdlnobj_t));
    if (dup_name == NULL || dup_data == NULL || obj == NULL) {
        if (dup_name != NULL) free(dup_name);
        if (dup_data != NULL) free(dup_data);
        if (obj != NULL) free(obj);
        errno = ENOMEM;
        return NULL;
    }
    memcpy(dup_data, data, size);
    memset((void *)obj, '\0', sizeof(qdlnobj_t));

    // obj->hash = qhashmurmur3_32(dup_name);
    obj->name = dup_name;
    obj->data = dup_data;
    obj->size = size;

    return obj;
}

// lock must be obtained from caller
static bool _insertobj(qlisttbl_t *tbl, qdlnobj_t *obj)
{
    // update hash
    obj->hash = qhashmurmur3_32(obj->name, strlen(obj->name));

    qdlnobj_t *prev = obj->prev;
    qdlnobj_t *next = obj->next;

    if (prev == NULL) tbl->first = obj;
    else prev->next = obj;

    if (next == NULL) tbl->last = obj;
    else next->prev = obj;

     // increase counter
    tbl->num++;

    return true;
}

// lock must be obtained from caller
static qdlnobj_t *_findobj(qlisttbl_t *tbl, const char *name,
                           bool first,
                           qdlnobj_t *retobj)
{
    if (retobj != NULL) {
        memset((void *)retobj, '\0', sizeof(qdlnobj_t));
    }

    if (name == NULL || tbl->num == 0) {
        errno = ENOENT;
        return NULL;
    }

    uint32_t hash = qhashmurmur3_32(name, strlen(name));
    qdlnobj_t *obj;
    if (first == true) obj = tbl->first;
    else obj = tbl->last;
    while (obj != NULL) {
        // name string will be compared only if the hash matches.
        if (tbl->namematch(obj, name, hash) == true) {
           if (retobj != NULL) {
                *retobj = *obj;
            }
            return obj;
        }

        if (first == true) obj = obj->next;
        else obj = obj->prev;
    }

    // not found, set prev and next chain.
    if (retobj != NULL) {
        if (tbl->putdir == false) {  // bottom
            retobj->prev = tbl->last;
            retobj->next = NULL;
        } else {  // top
            retobj->prev = NULL;
            retobj->next = tbl->first;
        }
    }
    errno = ENOENT;

    return NULL;
}

// key comp
static bool _namematch(qdlnobj_t *obj, const char *name, uint32_t hash)
{
    if ((obj->hash == hash) && !strcmp(obj->name, name)) {
        return true;
    }
    return false;
}

static bool _namecasematch(qdlnobj_t *obj, const char *name, uint32_t hash)
{
    if (!strcasecmp(obj->name, name)) {
        return true;
    }
    return false;
}

#endif /* _DOXYGEN_SKIP */
