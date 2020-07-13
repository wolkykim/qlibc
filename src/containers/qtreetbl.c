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
 * @file qtreetbl.c Tree Table container that implements "Left-Leaning Red-Black"
 * BST algorithm.
 *
 * qtreetbl implements a binary search tree that allows efficient in-order traversal
 * with O(log n) search time.
 *
 * The algorithm qtreetbl specifically implements is left-leaning red-black tree
 * algorithm invented in 2008 by Robert Sedgewick. A left-leaning red–black tree
 * is a type of self-balancing binary search tree and it is a variant of the
 * red–black tree which was invented in 1972 by Rudolf Bayer.
 *
 * References:
 * - http://www.cs.princeton.edu/~rs/talks/LLRB/LLRB.pdf
 * - http://www.cs.princeton.edu/~rs/talks/LLRB/RedBlack.pdf
 * - http://en.wikipedia.org/wiki/Left-leaning_red-black_tree
 *
 * Following example code will construct the data structure shown in below
 * diagram. This example is quoted from the inventor's presentation slide
 * p24-p25, http://www.cs.princeton.edu/~rs/talks/LLRB/RedBlack.pdf and
 * used in unit tests for verification purpose.
 *
 *                                (E)
 *                   ______________|______________
 *                  /                             \
 *                 (C)                           (R)
 *                /   \                         /   \
 *            (A,B)   (D)                   (I,N)   (S,X)
 *
 *                   <2-3-4 Tree Data Structure>
 *
 *                                 E
 *                   ______________|______________
 *                  /                             \
 *                 C                               R
 *          _______|_______                 _______|_______
 *         /               \               /               \
 *        B                 D             N                 X
 *      //                              //                //
 *     A*                              I*                S*
 *
 *            Tree Info : tot=10, red=3, black=-7, root=E
 *     Nodes A, I and S are nodes with RED upper link. Others are BLACK
 *
 *            <Left-Leaning Red-Black Tree Data Structure>
 *
 * Red-Black BST algorithm has been one of the famous BST algorithm especially
 * for in-memory operation. It's been using in all around the computing area.
 * I was very impressed about this variant, Left-Leaning version of Red-Black
 * about how it improves the performance and reduces the overall complexity.
 * Since it's relatively new algorithm, there's not many of practically functional
 * working codes yet other than proof of concept kinds. Here's one of fully
 * functional codes and I, Seungyoung Kim, would like to dedicate this code to
 * the genius inventor Robert Sedgewick and to all the great qLibc users.
 * Cheers!
 *
 * Unique features:
 *   - iterator. (have you ever seen iterator implementation in LLRB tree?)
 *   - find nearest key. (have you ever seen this as well? :)
 *   - iteration from given key.
 *   - find min/max key.
 *
 * @code
 *  qtreetbl_t *tbl = qtreetbl(QTREETBL_THREADSAFE);
 *
 *  tbl->put(tbl, "key", "DATA", 4);          // use put_by_obj() for binary keys.
 *  void *data = tbl->get(tbl, "key", false); // use get_by_obj() for binary keys.
 *  tbl->remove(tbl, "key");                  // use remove_by_key() for binary keys.
 *
 *  // iteration example
 *  tbl->lock(tbl);
 *  qtreetbl_obj_t obj = tbl->find_nearest(tbl, "k", 2, false);
 *  while (tbl->getnext(tbl, &obj, false) == true) {
 *    ...
 *  }
 *  tbl->unlock(tbl);
 *
 *  tbl->set_compare(tbl, my_compare_func);
 *  size_t num = tbl->size(tbl);
 *  void *min = tbl->find_min(tbl, &keysize);
 *  qtree_clean();
 *
 *  tbl->free(tbl);
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include "qinternal.h"
#include "utilities/qstring.h"
#include "containers/qtreetbl.h"

#ifndef _DOXYGEN_SKIP

/* internal functions */
static bool is_red(qtreetbl_obj_t *obj);
static qtreetbl_obj_t *flip_color(qtreetbl_obj_t *obj);
static qtreetbl_obj_t *rotate_left(qtreetbl_obj_t *obj);
static qtreetbl_obj_t *rotate_right(qtreetbl_obj_t *obj);
static qtreetbl_obj_t *move_red_left(qtreetbl_obj_t *obj);
static qtreetbl_obj_t *move_red_right(qtreetbl_obj_t *obj);
static qtreetbl_obj_t *fix(qtreetbl_obj_t *obj);
static qtreetbl_obj_t *find_min(qtreetbl_obj_t *obj);
static qtreetbl_obj_t *find_max(qtreetbl_obj_t *obj);
static qtreetbl_obj_t *find_obj(qtreetbl_t *tbl, const void *name,
                                size_t namesize);
static qtreetbl_obj_t *remove_min(qtreetbl_obj_t *obj);
static qtreetbl_obj_t *new_obj(bool red, const void *name, size_t namesize,
                               const void *data, size_t datasize);
static qtreetbl_obj_t *put_obj(qtreetbl_t *tbl, qtreetbl_obj_t *obj,
                               const void *name, size_t namesize,
                               const void *data, size_t datasize);
static qtreetbl_obj_t *remove_obj(qtreetbl_t *tbl, qtreetbl_obj_t *obj,
                                  const void *name, size_t namesize);
static void free_objs(qtreetbl_obj_t *obj);
static void free_obj(qtreetbl_obj_t *obj);
static uint8_t reset_iterator(qtreetbl_t *tbl);

#endif

/**
 * Initialize a tree table.
 *
 * @param options   combination of initialization options.
 *
 * @return a pointer of malloced qtreetbl_t, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *  qtreetbl_t *tbl = qtreetbl(0);
 * @endcode
 *
 * @note
 *   Available options:
 *   - QTREETBL_THREADSAFE - make it thread-safe.
 */
qtreetbl_t *qtreetbl(int options) {
    qtreetbl_t *tbl = (qtreetbl_t *) calloc(1, sizeof(qtreetbl_t));
    if (tbl == NULL)
        goto malloc_failure;

    // handle options.
    if (options & QTREETBL_THREADSAFE) {
        Q_MUTEX_NEW(tbl->qmutex, true);
        if (tbl->qmutex == NULL)
            goto malloc_failure;
    }

    // assign methods
    tbl->set_compare = qtreetbl_set_compare;

    tbl->put = qtreetbl_put;
    tbl->putstr = qtreetbl_putstr;
    tbl->putstrf = qtreetbl_putstrf;
    tbl->put_by_obj = qtreetbl_put_by_obj;

    tbl->get = qtreetbl_get;
    tbl->getstr = qtreetbl_getstr;
    //tbl->getint = qtreetbl_getint;
    tbl->get_by_obj = qtreetbl_get_by_obj;

    tbl->remove = qtreetbl_remove;
    tbl->remove_by_obj = qtreetbl_remove_by_obj;

    tbl->getnext = qtreetbl_getnext;

    tbl->find_min = qtreetbl_find_min;
    tbl->find_max = qtreetbl_find_max;
    tbl->find_nearest = qtreetbl_find_nearest;

    tbl->size = qtreetbl_size;
    tbl->clear = qtreetbl_clear;

    tbl->lock = qtreetbl_lock;
    tbl->unlock = qtreetbl_unlock;

    tbl->free = qtreetbl_free;
    tbl->debug = qtreetbl_debug;

    // Set default comparison function.
    qtreetbl_set_compare(tbl, qtreetbl_byte_cmp);
    reset_iterator(tbl);

    return tbl;

    malloc_failure:
    errno = ENOMEM;
    if (tbl) {
        assert(tbl->qmutex == NULL);
        qtreetbl_free(tbl);
    }
    return NULL;
}

/**
 * qtreetbl->set_compare(): Set user comparator.
 *
 * @param tbl       qtreetbl_t container pointer.
 * @param cmp       a pointer to the user comparator function.
 *
 * @note
 * By default, qtreetbl uses byte comparator that works for
 * both binary type key and string type key. Please refer
 * qtreetbl_byte_cmp() for your idea to make your own comparator,
 */
void qtreetbl_set_compare(
        qtreetbl_t *tbl,
        int (*cmp)(const void *name1, size_t namesize1, const void *name2,
                   size_t namesize2)) {
    tbl->compare = cmp;
}

/**
 * qtreetbl->put(): Put an object into this table with string type key.
 *
 * @param tbl       qtreetbl_t container pointer.
 * @param name      key name
 * @param data      data object
 * @param datasize  size of data object
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 */
bool qtreetbl_put(qtreetbl_t *tbl, const char *name, const void *data,
                  size_t datasize) {
    return qtreetbl_put_by_obj(tbl, name,
                               (name != NULL) ? (strlen(name) + 1) : 0, data,
                               datasize);
}

/**
 * qtreetbl->putstr(): Put a string into this table.
 *
 * @param tbl       qtreetbl container pointer.
 * @param name      key name.
 * @param str       string data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 */
bool qtreetbl_putstr(qtreetbl_t *tbl, const char *name, const char *str) {
    return qtreetbl_put_by_obj(tbl, name,
                               (name != NULL) ? (strlen(name) + 1) : 0, str,
                               (str != NULL) ? (strlen(str) + 1) : 0);
}

/**
 * qtreetbl->putstrf(): Put a formatted string into this table.
 *
 * @param tbl       qtreetbl_t container pointer.
 * @param name      key name.
 * @param format    formatted string data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 */
bool qtreetbl_putstrf(qtreetbl_t *tbl, const char *name, const char *format,
                      ...) {
    char *str;
    DYNAMIC_VSPRINTF(str, format);
    if (str == NULL) {
        errno = ENOMEM;
        return false;
    }

    bool ret = qtreetbl_putstr(tbl, name, str);
    free(str);
    return ret;
}

/**
 * qtreetbl->put_by_obj(): Put an object data into this table with an object name.
 *
 * @param tbl       qtreetbl_t container pointer.
 * @param name      key name
 * @param namesize  key size
 * @param data      data object
 * @param datasize  size of data object
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 *
 * @note
 *  This is the underlying put function which all other put methods use.
 */
bool qtreetbl_put_by_obj(qtreetbl_t *tbl, const void *name, size_t namesize,
                         const void *data, size_t datasize) {
    if (name == NULL || namesize == 0 || data == NULL || datasize == 0) {
        errno = EINVAL;
        return false;
    }

    qtreetbl_lock(tbl);
    errno = 0;
    qtreetbl_obj_t *root = put_obj(tbl, tbl->root, name, namesize, data,
                                   datasize);
    if (root == NULL || errno == ENOMEM) {
        qtreetbl_unlock(tbl);
        return false;
    }
    root->red = false;
    tbl->root = root;
    qtreetbl_unlock(tbl);

    return true;
}

/**
 * qtreetbl->get(): Get an object from this table.
 *
 * @param tbl       qtreetbl_t container pointer.
 * @param name      key name.
 * @param datasize  if not NULL, oject size will be stored.
 * @param newmem    whether or not to allocate memory for the data.
 *
 * @return a pointer of data if the key is found, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *  qtreetbl_t *tbl = qtreetbl(0);
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
void *qtreetbl_get(qtreetbl_t *tbl, const char *name, size_t *datasize,
bool newmem) {
    return qtreetbl_get_by_obj(tbl, name,
                               (name != NULL) ? (strlen(name) + 1) : 0,
                               datasize, newmem);
}

/**
 * qtreetbl->getstr(): Finds an object and returns as string type.
 *
 * @param tbl       qtreetbl_t container pointer.
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
 *  deallocated by user. Otherwise returned pointer will point internal buffer
 *  directly and should not be de-allocated by user. In thread-safe mode,
 *  newmem flag must be set to true always.
 */
char *qtreetbl_getstr(qtreetbl_t *tbl, const char *name, const bool newmem) {
    return qtreetbl_get_by_obj(tbl, name,
                               (name != NULL) ? (strlen(name) + 1) : 0, NULL,
                               newmem);
}

/**
 * qtreetbl->get_by_obj(): Get an object from this table with an object name.
 *
 * @param tbl       qtreetbl_t container pointer.
 * @param name      key name
 * @param namesize  key size
 * @param datasize  if not NULL, oject size will be stored.
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
 *  deallocated by user. Otherwise returned pointer will point internal buffer
 *  directly and should not be de-allocated by user. In thread-safe mode,
 *  newmem flag must be set to true always.
 */
void *qtreetbl_get_by_obj(qtreetbl_t *tbl, const char *name, size_t namesize,
                          size_t *datasize, bool newmem) {
    if (name == NULL || namesize == 0) {
        errno = EINVAL;
        return NULL;
    }

    qtreetbl_lock(tbl);
    qtreetbl_obj_t *obj = find_obj(tbl, name, namesize);
    void *data = NULL;
    if (obj != NULL) {
        data = (newmem) ? qmemdup(obj->data, obj->datasize) : obj->data;
        if (data != NULL && datasize != NULL) {
            *datasize = obj->datasize;
        }
    }
    qtreetbl_unlock(tbl);
    return data;
}

/**
 * qtreetbl->remove(): Remove an object from this table.
 *
 * @param tbl   qtreetbl_t container pointer.
 * @param name  key name
 *
 * @return true if successful, otherwise(not found) returns false
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 */
bool qtreetbl_remove(qtreetbl_t *tbl, const char *name) {
    return qtreetbl_remove_by_obj(tbl, name,
                                  (name != NULL) ? strlen(name) + 1 : 0);
}

/**
 * qtreetbl->remove(): Remove an object from this table with an object name.
 *
 * @param tbl   qtreetbl_t container pointer.
 * @param name  key name
 * @param name  key size
 *
 * @return true if successful, otherwise(not found) returns false
 * @retval errno will be set in error condition.
 *  - ENOENT : No such key found.
 *  - EINVAL : Invalid argument.
 */
bool qtreetbl_remove_by_obj(qtreetbl_t *tbl, const void *name, size_t namesize) {
    if (name == NULL) {
        errno = EINVAL;
        return false;
    }

    qtreetbl_lock(tbl);
    errno = 0;
    tbl->root = remove_obj(tbl, tbl->root, name, namesize);
    if (tbl->root)
        tbl->root->red = false;
    bool removed = (errno != ENOENT) ? true : false;
    qtreetbl_unlock(tbl);

    return removed;
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
 *  [Iteration example from the beginning]
 *  qtreetbl_obj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  tbl->lock(tbl);  // lock it when thread condition is expected
 *  while(tbl->getnext(tbl, &obj, false) == true) {
 *     //
 *     // obj.name     : key data
 *     // obj.namesize : key size
 *     // obj.data     : data
 *     // obj.datasize : data size
 *     //
 *     // Do free obj.name and obj.data if newmem is set to true;
 *  }
 *  tbl->unlock(tbl);
 * @endcode
 *
 * @code
 *  [Iteration example from given point]
 *
 *  qtreetbl_obj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj));
 *  tbl->lock(tbl);  // lock must be raised before find_nearest() call.
 *  qtreetbl_obj_t obj = tbl->find_nearest(tbl, "F", 2, false); // here we go!
 *  while (tbl->getnext(tbl, &obj, false) == true) {  // newmem is false
 *      //If tree has 5 objects, A, C, E, G and I.
 *      //Iteration sequence from nearest "F" will be: E->G->I->A->C
 *  }
 *  tbl->unlock(tbl);
 *
 * @endcode
 *
 * @code
 * [Removal example in iteration loop]
 *   qtreetbl_obj_t obj;
 *   memset((void*) &obj, 0, sizeof(obj));  // start from the minimum.
 *   tbl->lock(tbl);
 *   while (tbl->getnext(tbl, &obj, false) == true) {
 *       if (...condition...) {
 *           char *name = qmemdup(obj.name, obj.namesize);  // keep the name
 *           size_t namesize = obj.namesize;                // for removal argument
 *
 *           tbl->remove_by_obj(tbl, obj.name, obj.namesize);  // remove
 *
 *           obj = tbl->find_nearest(tbl, name, namesize, false); // rewind one step back
 *
 *           free(name);  // clean up
 *       }
 *   }
 *   tbl->unlock(tbl);
 *  @endcode
 *
 * @note
 *  - locking must be provided on user code when thread condition is expected
 *  because entire traversal needs to be running under read-only mode.
 *  - Data insertion or deletion can be made during the traversal, but in that
 *  case iterator doesn't guarantee full sweep and possibly skip some visits.
 *  When deletion happens in getnext() loop, use find_nearest() to rewind the
 *  iterator one step back.
 *  - Object obj should be initialized with 0 by using memset() before first call.
 *  - If newmem flag is true, user should de-allocate obj.name and obj.data
 *  resources.
 */
bool qtreetbl_getnext(qtreetbl_t *tbl, qtreetbl_obj_t *obj, const bool newmem) {
    if (obj == NULL) {
        errno = EINVAL;
        return NULL;
    }

    uint8_t tid = obj->tid;
    if (obj->next == NULL) {  // first time call
        if (tbl->root == NULL) {
            return false;
        }
        // get a new iterator id
        tid = reset_iterator(tbl);;
    }

    qtreetbl_obj_t *cursor = ((obj->next != NULL) ? obj->next : tbl->root);
    while (cursor != NULL) {
        if (cursor->left && cursor->left->tid != tid) {
            cursor->left->next = cursor;
            cursor = cursor->left;
            continue;
        } else if (cursor->tid != tid) {
            cursor->tid = tid;
            *obj = *cursor;
            if (newmem) {
                obj->name = qmemdup(cursor->name, cursor->namesize);
                obj->data = qmemdup(cursor->data, cursor->datasize);
            }
            obj->next = cursor;  // store original address in tree for next iteration
            return true;
        } else if (cursor->right && cursor->right->tid != tid) {
            cursor->right->next = cursor;
            cursor = cursor->right;
            continue;
        }
        cursor = cursor->next;
    }

    // end of travel
    reset_iterator(tbl);  // to allow iteration start over directly from find_nearest()
    return false;
}

/**
 * qtreetbl->find_min(): Find the name of very left object.
 *
 * @param tbl       qtreetbl_t container pointer.
 * @param namesize  if not NULL, the size of key name will be stored.
 *
 * @return malloced memory copying the key name.
 *
 * @note
 *  It's user's responsibility to free the return.
 */
void *qtreetbl_find_min(qtreetbl_t *tbl, size_t *namesize) {
    qtreetbl_lock(tbl);
    qtreetbl_obj_t *obj = find_min(tbl->root);
    if (obj == NULL) {
        errno = ENOENT;
        qtreetbl_unlock(tbl);
        return NULL;
    }

    if (namesize != NULL) {
        *namesize = obj->namesize;
    }
    void *name = qmemdup(obj->name, obj->namesize);
    qtreetbl_unlock(tbl);
    return name;
}

/**
 * qtreetbl->find_max(): Find the name of very right object.
 *
 * @param tbl       qtreetbl_t container pointer.
 * @param namesize  if not NULL, the size of key name will be stored.
 *
 * @return malloced memory copying the key name.
 *
 * @note
 *  It's user's responsibility to free the return.
 */
void *qtreetbl_find_max(qtreetbl_t *tbl, size_t *namesize) {
    qtreetbl_lock(tbl);
    qtreetbl_obj_t *obj = find_max(tbl->root);
    if (obj == NULL) {
        errno = ENOENT;
        qtreetbl_unlock(tbl);
        return NULL;
    }

    if (namesize != NULL) {
        *namesize = obj->namesize;
    }
    void *name = qmemdup(obj->name, obj->namesize);
    qtreetbl_unlock(tbl);
    return name;
}

/**
 * qtreetbl->find_nearest(): Find equal or nearest object.
 *
 * find_nearest() returns matching key or nearest key. If there's
 * no keys in the table. It'll return empty qtreetbl_obj_t object
 * with errno ENOENT.
 *
 * @param tbl       qtreetbl_t container pointer.
 * @param name      key name
 * @param namesize  key size
 * @param newmem    whether or not to allocate memory for the data.
 *
 * @return qtreetbl_obj_t object.
 *
 * @retval errno will be set in error condition.
 *  - ENOENT : No next element.
 *  - EINVAL : Invalid argument.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *  [Some examples here]
 *  Data Set : A B C D E I N R S X
 *  find_nearest("0") => "A" // no smaller key available, so "A"
 *  find_nearest("C") => "C" // matching key found
 *  find_nearest("F") => "E" // "E" is nearest smaller key from "F"
 *  find_nearest("M") => "N" // "N" is nearest smaller key from "M"
 *  find_nearest("Z") => "X" // "X" is nearest smaller key from "Z"
 * @endcode
 *
 * @note
 *  When there's no matching key it look for closest smaller key
 *  in the neighbors. The only exception when it returns bigger key
 *  than given search key is that when there's no smaller keys available
 *  in the table. In such case, it'll return the nearest bigger key.
 */
qtreetbl_obj_t qtreetbl_find_nearest(qtreetbl_t *tbl, const void *name,
                                     size_t namesize, bool newmem) {
    qtreetbl_obj_t retobj;
    memset((void*) &retobj, 0, sizeof(retobj));

    if (name == NULL || namesize == 0) {
        errno = EINVAL;
        return retobj;
    }

    qtreetbl_lock(tbl);
    qtreetbl_obj_t *obj, *lastobj;
    for (obj = lastobj = tbl->root; obj != NULL;) {
        int cmp = tbl->compare(name, namesize, obj->name, obj->namesize);
        if (cmp == 0) {
            break;
        }
        lastobj = obj;
        if (cmp < 0) {
            if (obj->left)
                obj->left->next = obj;
            obj = obj->left;
        } else {
            if (obj->right)
                obj->right->next = obj;
            obj = obj->right;
        }
    }

    if (obj == NULL) {
        for (obj = lastobj;
                obj != NULL
                        && (tbl->compare(name, namesize, obj->name,
                                         obj->namesize) < 0); obj = obj->next)
            ;
        if (obj == NULL) {
            obj = lastobj;
        }
    }

    if (obj) {
        retobj = *obj;
        if (newmem) {
            retobj.name = qmemdup(obj->name, obj->namesize);
            retobj.data = qmemdup(obj->data, obj->datasize);
        }
        // set travel info to be used for iteration in getnext()
        retobj.tid = tbl->tid;
        retobj.next = obj;

    } else {
        errno = ENOENT;
    }

    qtreetbl_unlock(tbl);
    return retobj;
}

/**
 * qtreetbl->size(): Returns the number of keys in the table.
 *
 * @param tbl   qtreetbl_t container pointer.
 *
 * @return number of elements stored
 */
size_t qtreetbl_size(qtreetbl_t *tbl) {
    return tbl->num;
}

/**
 * qtreetbl->clear(): Clears the table so that it contains no keys.
 *
 * @param tbl   qtreetbl_t container pointer.
 */
void qtreetbl_clear(qtreetbl_t *tbl) {
    qtreetbl_lock(tbl);
    free_objs(tbl->root);
    tbl->root = NULL;
    tbl->num = 0;
    qtreetbl_unlock(tbl);
}

/**
 * qtreetbl->lock(): Enter critical section.
 *
 * @param tbl   qtreetbl_t container pointer.
 *
 * @note
 *  From user side, normally locking operation is only needed when traverse
 *  all elements using qtreetbl->getnext().
 *
 * @note
 *  This operation will do nothing if QTREETBL_THREADSAFE option was not
 *  given at the initialization time.
 */
void qtreetbl_lock(qtreetbl_t *tbl) {
    Q_MUTEX_ENTER(tbl->qmutex);
}

/**
 * qtreetbl->unlock(): Leave critical section.
 *
 * @param tbl   qtreetbl_t container pointer.
 *
 * @note
 *  This operation will do nothing if QTREETBL_THREADSAFE option was not
 *  given at the initialization time.
 */
void qtreetbl_unlock(qtreetbl_t *tbl) {
    Q_MUTEX_LEAVE(tbl->qmutex);
}

/**
 * qtreetbl->free(): De-allocate the table
 *
 * @param tbl   qtreetbl_t container pointer.
 */
void qtreetbl_free(qtreetbl_t *tbl) {
    qtreetbl_clear(tbl);
    Q_MUTEX_DESTROY(tbl->qmutex);
    free(tbl);
}

int qtreetbl_byte_cmp(const void *name1, size_t namesize1, const void *name2,
                      size_t namesize2) {
    size_t minsize = (namesize1 < namesize2) ? namesize1 : namesize2;
    int cmp = memcmp(name1, name2, minsize);
    if (cmp != 0 || namesize1 == namesize2)
        return cmp;
    return (namesize1 < namesize2) ? -1 : +1;
}

/**
 * qtreetbl->debug(): Print hash table for debugging purpose
 *
 * @param tbl   qtreetbl_t container pointer.
 * @param out   output stream
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EIO : Invalid output stream.
 */
bool qtreetbl_debug(qtreetbl_t *tbl, FILE *out) {
    if (out == NULL) {
        errno = EIO;
        return false;
    }

    qtreetbl_lock(tbl);

    qtreetbl_unlock(tbl);

    return true;
}

/**
 * Display the tree structure from the node pointed by `obj` and down.
 *
 * @param tbl   A pointer to a node of the tree.
 * @param depth The depth of the node in the tree.
 *              The depth of the tree root is 0.
 *
 */
void print_node(qtreetbl_obj_t *obj, int depth) {
    // Skip tree-leaves
    if (!obj) return;

    // 4-spaces indentation for each node level
    for (int i = 0; i < depth; i++) {
        printf("    ");
    }

    // Print the red-ness of the node
    printf("R=%c W= ", (obj->red) ? 'Y' : 'N');

    // Print the content of the node
    for (int i = 0; i < obj->namesize; i++) {
        printf("%c", ((char *) obj->name)[i]);
    }

    // No more data to print for this node, go to next line.
    printf("\n");

    // Recursive call to display the children
    print_node(obj->left, depth + 1);
    print_node(obj->right, depth + 1);
}

/**
 * Display the tree structure.
 *
 * @param tbl   qtreetbl_t container pointer.
 *
 */
void qtreetbl_print(qtreetbl_t *tbl) {

    qtreetbl_lock(tbl);

    print_node(tbl->root, 0);

    qtreetbl_unlock(tbl);
}

/**
 * Verifies that RULE 4 of the red-black tree is verified for the node pointed
 * by `obj` and all its children.
 *
 * Rule 4 states that no red node shall have a red child.
 *
 * @param tbl A pointer to the tree object.
 * @param obj A pointer to a node of the tree object.
 */
int node_check_rule4(qtreetbl_t *tbl, qtreetbl_obj_t *obj) {
    // RULE 4: No red node has a red child

    if (obj == NULL) return 0;

    if (is_red(obj)) {
        if (is_red(obj->right) || is_red(obj->left)) {
            printf("ERROR: Rule 4 violated.\n");
            printf("Red node with key '");
            // Print the name
            for (int i = 0; i < obj->namesize; i++) {
                printf("%c", ((char *) obj->name)[i]);
            }
            printf("' has at least one red child.\n");
            print_node(tbl->root, 0);

            return 1;
        }
    }

    if (node_check_rule4(tbl, obj->right))
        return 1;
    if (node_check_rule4(tbl, obj->left))
        return 1;
    
    return 0;
}

/**
 * Verifies that RULE 5 of the red-black tree is verified for the node pointed
 * by `obj` and all its children.
 *
 * Rule 5 states that every path from the root of the tree to any leaf of the
 * tree has the same number of black nodes.
 *
 * @param tbl A pointer to the tree object.
 * @param obj A pointer to a node of the tree object.
 */
int node_check_rule5(qtreetbl_t *tbl, qtreetbl_obj_t *obj, int *path_len) {

    if (obj == NULL) {
        *path_len = 0;
    } else {
        int right_path_len;
        if (node_check_rule5(tbl, obj->right, &right_path_len))
            return 1;
        
        int left_path_len;
        if (node_check_rule5(tbl, obj->left, &left_path_len))
            return 1;

        if (right_path_len != left_path_len) {
            printf("ERROR: Rule 5 violated.");
            print_node(tbl->root, 0);
            return 1;
        } else {
            *path_len = right_path_len;

            if (!is_red(obj)) (*path_len)++;
        }
    }

    return 0;
}

/**
 * Verifies that the (some) invariants of the red-black tree are satisfied.
 *
 * @param tbl A pointer to the tree object to check.
 */
int qtreetbl_check(qtreetbl_t *tbl) {

    printf("Checking tree... \n");
    if (tbl == NULL) return 0;

    if (node_check_rule4(tbl, tbl->root)) 
        return 1;
    int path_len = 0;
    if (node_check_rule5(tbl, tbl->root, &path_len))
        return 1;
    
    
    
    print_node(tbl->root, 0);
    
    return 0;
}

#ifndef _DOXYGEN_SKIP

static bool is_red(qtreetbl_obj_t *obj) {
    return (obj != NULL) ? obj->red : false;
}

static qtreetbl_obj_t *flip_color(qtreetbl_obj_t *obj) {
    obj->red = !(obj->red);
    obj->left->red = !(obj->left->red);
    obj->right->red = !(obj->right->red);
    return obj;
}

static qtreetbl_obj_t *rotate_left(qtreetbl_obj_t *obj) {
    qtreetbl_obj_t *x = obj->right;
    obj->right = x->left;
    x->left = obj;
    x->red = x->left->red;
    x->left->red = true;
    return x;
}

static qtreetbl_obj_t *rotate_right(qtreetbl_obj_t *obj) {
    qtreetbl_obj_t *x = obj->left;
    obj->left = x->right;
    x->right = obj;
    x->red = x->right->red;
    x->right->red = true;
    return x;
}

static qtreetbl_obj_t *move_red_left(qtreetbl_obj_t *obj) {
    flip_color(obj);
    if (obj->right && is_red(obj->right->left)) {
        obj->right = rotate_right(obj->right);
        obj = rotate_left(obj);
        flip_color(obj);
    }
    return obj;
}

static qtreetbl_obj_t *move_red_right(qtreetbl_obj_t *obj) {
    flip_color(obj);
    if (obj->left && is_red(obj->left->left)) {
        obj = rotate_right(obj);
        flip_color(obj);
    }
    return obj;
}

static qtreetbl_obj_t *fix(qtreetbl_obj_t *obj) {
    // rotate right red to left
    if (is_red(obj->right)) {
        obj = rotate_left(obj);
    }
    // rotate left red-red to right
    if (obj->left && is_red(obj->left) && is_red(obj->left->left)) {
        obj = rotate_right(obj);
    }
    // split 4-nodes
    if (is_red(obj->left) && is_red(obj->right)) {
        flip_color(obj);
    }
    return obj;
}

static qtreetbl_obj_t *find_min(qtreetbl_obj_t *obj) {
    if (obj == NULL) {
        errno = ENOENT;
        return NULL;
    }

    qtreetbl_obj_t *o;
    for (o = obj; o->left != NULL; o = o->left)
        ;
    return o;
}

static qtreetbl_obj_t *find_max(qtreetbl_obj_t *obj) {
    if (obj == NULL) {
        errno = ENOENT;
        return NULL;
    }

    qtreetbl_obj_t *o;
    for (o = obj; o->right != NULL; o = o->right)
        ;
    return o;
}

static qtreetbl_obj_t *find_obj(qtreetbl_t *tbl, const void *name,
                                size_t namesize) {
    if (name == NULL || namesize == 0) {
        errno = EINVAL;
        return NULL;
    }

    qtreetbl_lock(tbl);
    qtreetbl_obj_t *obj;
    for (obj = tbl->root; obj != NULL;) {
        int cmp = tbl->compare(name, namesize, obj->name, obj->namesize);
        if (cmp == 0) {
            qtreetbl_unlock(tbl);
            return obj;
        }
        obj = (cmp < 0) ? obj->left : obj->right;
    }
    qtreetbl_unlock(tbl);

    errno = ENOENT;
    return NULL;
}

static qtreetbl_obj_t *remove_min(qtreetbl_obj_t *obj) {
    if (obj->left == NULL) {
        // 3-nodes are left-leaning, so this is a leaf.
        free(obj->name);
        free(obj->data);
        return NULL;
    }
    if (!is_red(obj->left) && !is_red(obj->left->left)) {
        obj = move_red_left(obj);
    }
    obj->left = remove_min(obj->left);
    return fix(obj);
}

static qtreetbl_obj_t *new_obj(bool red, const void *name, size_t namesize,
                               const void *data, size_t datasize) {
    qtreetbl_obj_t *obj = (qtreetbl_obj_t *) calloc(1, sizeof(qtreetbl_obj_t));
    void *copyname = qmemdup(name, namesize);
    void *copydata = qmemdup(data, datasize);

    if (obj == NULL || copyname == NULL || copydata == NULL) {
        errno = ENOMEM;
        free(obj);
        free(copyname);
        free(copydata);
        return NULL;
    }

    obj->red = red;
    obj->name = copyname;
    obj->namesize = namesize;
    obj->data = copydata;
    obj->datasize = datasize;

    return obj;
}

static qtreetbl_obj_t *put_obj(qtreetbl_t *tbl, qtreetbl_obj_t *obj,
                               const void *name, size_t namesize,
                               const void *data, size_t datasize) {
    if (obj == NULL) {
        tbl->num++;
        return new_obj(true, name, namesize, data, datasize);
    }

    // split 4-nodes on the way down.
    if (is_red(obj->left) && is_red(obj->right)) {
        flip_color(obj);
    }

    int cmp = tbl->compare(obj->name, obj->namesize, name, namesize);
    if (cmp == 0) {  // existing key found.
        void *copydata = qmemdup(data, datasize);
        if (copydata != NULL) {
            free(obj->data);
            obj->data = copydata;
            obj->datasize = datasize;
        }
    } else if (cmp < 0) {
        obj->right = put_obj(tbl, obj->right, name, namesize, data, datasize);
    } else {
        obj->left = put_obj(tbl, obj->left, name, namesize, data, datasize);
    }

    // fix right-leaning reds on the way up
    if (is_red(obj->right)) {
        obj = rotate_left(obj);
    }

    // fix two reds in a row on the way up
    if (is_red(obj->left) && is_red(obj->left->left)) {
        obj = rotate_right(obj);
    }
    
    return obj;
}

static qtreetbl_obj_t *remove_obj(qtreetbl_t *tbl, qtreetbl_obj_t *obj,
                                  const void *name, size_t namesize) {
    if (obj == NULL) {
        errno = ENOENT;
        return NULL;
    }

    if (tbl->compare(name, namesize, obj->name, obj->namesize) < 0) {  // left
        // move red left
        if (obj->left && (!is_red(obj->left) && !is_red(obj->left->left))) {
            obj = move_red_left(obj);
        }
        // keep going down to the left
        obj->left = remove_obj(tbl, obj->left, name, namesize);
    } else {  // right or equal
        if (is_red(obj->left) && !is_red(obj->right)) {
            obj = rotate_right(obj);
        }
        // remove if equal at the bottom
        if (tbl->compare(name, namesize, obj->name, obj->namesize)
                == 0&& obj->right == NULL) {
            free(obj->name);
            free(obj->data);
            free(obj);
            tbl->num--;
            assert(tbl->num >= 0);
            return NULL;
        }
        // move red right
        if (obj->right != NULL
                && (!is_red(obj->right) && !is_red(obj->right->left))) {
            obj = move_red_right(obj);
        }
        // found in the middle
        if (tbl->compare(name, namesize, obj->name, obj->namesize) == 0) {
            // copy min to this then remove min. What a genius inventor!
            qtreetbl_obj_t *minobj = find_min(obj->right);
            assert(minobj != NULL);
            free(obj->name);
            free(obj->data);
            obj->name = qmemdup(minobj->name, minobj->namesize);
            obj->namesize = minobj->namesize;
            obj->data = qmemdup(minobj->data, minobj->datasize);
            obj->datasize = minobj->datasize;
            obj->right = remove_min(obj->right);
            tbl->num--;
        } else {
            // keep going down to the right
            obj->right = remove_obj(tbl, obj->right, name, namesize);
        }
    }
    // Fix right-leaning red nodes on the way up.
    return fix(obj);
}

static void free_objs(qtreetbl_obj_t *obj) {
    if (obj == NULL) {
        return;
    }
    if (obj->left) {
        free_objs(obj->left);
    }
    if (obj->right) {
        free_objs(obj->right);
    }
    free_obj(obj);
}

static void free_obj(qtreetbl_obj_t *obj) {
    if (obj == NULL) {
        return;
    }
    free(obj->name);
    free(obj->data);
    free(obj);
}

static uint8_t reset_iterator(qtreetbl_t *tbl) {
    return (++tbl->tid);
}

#endif
