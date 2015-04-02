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
 * @file qlist.c Doubly Linked-list implementation.
 *
 * qlist container is a doubly Linked-List implementation.
 * qlist provides uniformly named methods to add, get, pop and remove an
 * element at the beginning and end of the list. These operations allow qlist
 * to be used as a stack, queue, or double-ended queue.
 *
 * @code
 *  [Conceptional Data Structure Diagram]
 *
 *  last~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+
 *                                                          |
 *          +-----------+  doubly  +-----------+  doubly  +-|---------+
 *  first~~~|~>   0   <~|~~~~~~~~~~|~>   1   <~|~~~~~~~~~~|~>   N     |
 *          +-----|-----+  linked  +-----|-----+  linked  +-----|-----+
 *                |                      |                      |
 *          +-----v---------------+      |                +-----v-----+
 *          | DATA A              |      |                | DATA N    |
 *          +---------------------+      |                +-----------+
 *                 +---------------------v------------------+
 *                 | DATA B                                 |
 *                 +----------------------------------------+
 * @endcode
 *
 * @code
 *  // create a list.
 *  qlist_t *list = qlist(QLIST_THREADSAFE);
 *
 *  // insert elements
 *  list->addlast(list, "e1", sizeof("e1"));
 *  list->addlast(list, "e2", sizeof("e2"));
 *  list->addlast(list, "e3", sizeof("e3"));
 *
 *  // get
 *  char *e1 = (char*)list->getfirst(list, NULL, true));    // malloced
 *  char *e3  = (char*)list->getat(list, -1, NULL, false)); // no malloc
 *  (...omit...)
 *  free(e1);
 *
 *  // pop (get and remove)
 *  char *e2 = (char*)list->popat(list, 1, NULL)); // get malloced copy
 *  (...omit...)
 *  free(e2);
 *
 *  // debug output
 *  list->debug(list, stdout, true);
 *
 *  // traversal
 *  qlist_obj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  list->lock(list);
 *  while (list->getnext(list, &obj, false) == true) {
 *    printf("DATA=%s, SIZE=%zu\n", (char*)obj.data, obj.size);
 *  }
 *  list->unlock(list);
 *
 *  // free object
 *  list->free(list);
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "qinternal.h"
#include "containers/qlist.h"

#ifndef _DOXYGEN_SKIP

static void *get_at(qlist_t *list, int index, size_t *size, bool newmem, bool remove);
static qlist_obj_t *get_obj(qlist_t *list, int index);
static bool remove_obj(qlist_t *list, qlist_obj_t *obj);

#endif

/**
 * Create new qlist_t linked-list container
 *
 * @param options   combination of initialization options.
 *
 * @return a pointer of malloced qlist_t container, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  -ENOMEM : Memory allocation failure.
 *
 * @code
 *  qlist_t *list = qlist(0);
 * @endcode
 *
 * @note
 *   Available options:
 *   - QLIST_THREADSAFE - make it thread-safe.
 */
qlist_t *qlist(int options) {
    qlist_t *list = (qlist_t *) calloc(1, sizeof(qlist_t));
    if (list == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    // handle options.
    if (options & QLIST_THREADSAFE) {
        Q_MUTEX_NEW(list->qmutex, true);
        if (list->qmutex == NULL) {
            errno = ENOMEM;
            free(list);
            return NULL;
        }
    }

    // member methods
    list->setsize = qlist_setsize;

    list->addfirst = qlist_addfirst;
    list->addlast = qlist_addlast;
    list->addat = qlist_addat;

    list->getfirst = qlist_getfirst;
    list->getlast = qlist_getlast;
    list->getat = qlist_getat;
    list->getnext = qlist_getnext;

    list->popfirst = qlist_popfirst;
    list->poplast = qlist_poplast;
    list->popat = qlist_popat;

    list->removefirst = qlist_removefirst;
    list->removelast = qlist_removelast;
    list->removeat = qlist_removeat;

    list->reverse = qlist_reverse;
    list->clear = qlist_clear;

    list->size = qlist_size;
    list->datasize = qlist_datasize;

    list->toarray = qlist_toarray;
    list->tostring = qlist_tostring;
    list->debug = qlist_debug;

    list->lock = qlist_lock;
    list->unlock = qlist_unlock;

    list->free = qlist_free;

    return list;
}

/**
 * qlist->setsize(): Limit maximum number of elements allowed in this list.
 *
 * @param list  qlist_t container pointer.
 * @param max   maximum number of elements. 0 means no limit.
 *
 * @return previous maximum number.
 *
 * @note
 *  The default maximum number of elements is unlimited.
 */
size_t qlist_setsize(qlist_t *list, size_t max) {
    qlist_lock(list);
    size_t old = list->max;
    list->max = max;
    qlist_unlock(list);
    return old;
}

/**
 * qlist->addfirst(): Inserts a element at the beginning of this list.
 *
 * @param list  qlist_t container pointer.
 * @param data  a pointer which points data memory.
 * @param size  size of the data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOBUFS : List full. Only happens when this list has set to have limited
 *              number of elements.
 *  - EINVAL  : Invalid argument.
 *  - ENOMEM  : Memory allocation failure.
 *
 * @code
 *  // create a sample object.
 *  struct my_obj obj;
 *
 *  // create a list and add the sample object.
 *  qlist_t *list = qlist();
 *  list->addfirst(list, &obj, sizeof(struct my_obj));
 * @endcode
 */
bool qlist_addfirst(qlist_t *list, const void *data, size_t size) {
    return qlist_addat(list, 0, data, size);
}

/**
 * qlist->addlast(): Appends a element to the end of this list.
 *
 * @param list  qlist_t container pointer.
 * @param data  a pointer which points data memory.
 * @param size  size of the data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOBUFS : List full. Only happens when this list has set to have limited
 *              number of elements.
 *  - EINVAL  : Invalid argument.
 *  - ENOMEM  : Memory allocation failure.
 */
bool qlist_addlast(qlist_t *list, const void *data, size_t size) {
    return qlist_addat(list, -1, data, size);
}

/**
 * qlist->addat(): Inserts a element at the specified position in this
 * list.
 *
 * @param list   qlist_t container pointer.
 * @param index  index at which the specified element is to be inserted.
 * @param data   a pointer which points data memory.
 * @param size   size of the data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOBUFS : List full. Only happens when this list has set to have limited
 *              number of elements.
 *  - ERANGE  : Index out of range.
 *  - EINVAL  : Invalid argument.
 *  - ENOMEM  : Memory allocation failure.
 *
 * @code
 *                     first           last      new
 *  Linked-list        [ A ]<=>[ B ]<=>[ C ]?==?[   ]
 *  (positive index)     0       1       2        3
 *  (negative index)    -3      -2      -1
 * @endcode
 *
 * @code
 *  qlist_t *list = qlist();
 *  list->addat(list, 0, &obj, sizeof(obj));  // same as addfirst().
 *  list->addat(list, -1, &obj, sizeof(obj)); // same as addlast().
 * @endcode
 *
 * @note
 *  Index starts from 0.
 */
bool qlist_addat(qlist_t *list, int index, const void *data, size_t size) {
    // check arguments
    if (data == NULL || size <= 0) {
        errno = EINVAL;
        return false;
    }

    qlist_lock(list);

    // check maximum number of allowed elements if set
    if (list->max > 0 && list->num >= list->max) {
        errno = ENOBUFS;
        qlist_unlock(list);
        return false;
    }

    // adjust index
    if (index < 0)
        index = (list->num + index) + 1;  // -1 is same as addlast()
    if (index < 0 || index > list->num) {
        // out of bound
        qlist_unlock(list);
        errno = ERANGE;
        return false;
    }

    // duplicate object
    void *dup_data = malloc(size);
    if (dup_data == NULL) {
        qlist_unlock(list);
        errno = ENOMEM;
        return false;
    }
    memcpy(dup_data, data, size);

    // make new object list
    qlist_obj_t *obj = (qlist_obj_t *) malloc(sizeof(qlist_obj_t));
    if (obj == NULL) {
        free(dup_data);
        qlist_unlock(list);
        errno = ENOMEM;
        return false;
    }
    obj->data = dup_data;
    obj->size = size;
    obj->prev = NULL;
    obj->next = NULL;

    // make link
    if (index == 0) {
        // add at first
        obj->next = list->first;
        if (obj->next != NULL)
            obj->next->prev = obj;
        list->first = obj;
        if (list->last == NULL)
            list->last = obj;
    } else if (index == list->num) {
        // add after last
        obj->prev = list->last;
        if (obj->prev != NULL)
            obj->prev->next = obj;
        list->last = obj;
        if (list->first == NULL)
            list->first = obj;
    } else {
        // add at the middle of list
        qlist_obj_t *tgt = get_obj(list, index);
        if (tgt == NULL) {
            // should not be happened.
            free(dup_data);
            free(obj);
            qlist_unlock(list);
            errno = EAGAIN;
            return false;
        }

        // insert obj
        tgt->prev->next = obj;
        obj->prev = tgt->prev;
        obj->next = tgt;
        tgt->prev = obj;
    }

    list->datasum += size;
    list->num++;

    qlist_unlock(list);

    return true;
}

/**
 * qlist->getfirst(): Returns the first element in this list.
 *
 * @param list    qlist_t container pointer.
 * @param size    if size is not NULL, element size will be stored.
 * @param newmem  whether or not to allocate memory for the element.
 *
 * @return a pointer of element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : List is empty.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *  size_t size;
 *  void *data = list->getfirst(list, &size, true);
 *  if (data != NULL) {
 *    (...omit...)
 *    free(data);
 *  }
 * @endcode
 */
void *qlist_getfirst(qlist_t *list, size_t *size, bool newmem) {
    return qlist_getat(list, 0, size, newmem);
}

/**
 * qlist->getlast(): Returns the last element in this list.
 *
 * @param list    qlist_t container pointer.
 * @param size    if size is not NULL, element size will be stored.
 * @param newmem  whether or not to allocate memory for the element.
 *
 * @return a pointer of element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *        ENOENT : List is empty.
 *        ENOMEM : Memory allocation failure.
 */
void *qlist_getlast(qlist_t *list, size_t *size, bool newmem) {
    return qlist_getat(list, -1, size, newmem);
}

/**
 * qlist->getat(): Returns the element at the specified position in this
 * list.
 *
 * @param list    qlist_t container pointer.
 * @param index   index at which the specified element is to be inserted
 * @param size    if size is not NULL, element size will be stored.
 * @param newmem  whether or not to allocate memory for the element.
 *
 * @return a pointer of element, otherwise returns NULL.
 * @retval errno
 * @retval errno will be set in error condition.
 *  -ERANGE : Index out of range.
 *  -ENOMEM : Memory allocation failure.
 *
 * @code
 *                     first           last
 *  Linked-list        [ A ]<=>[ B ]<=>[ C ]
 *  (positive index)     0       1       2
 *  (negative index)    -3      -2      -1
 * @endcode
 *
 * @note
 *  Negative index can be used for addressing a element from the end in this
 *  stack. For example, index -1 is same as getlast() and index 0 is same as
 *  getfirst();
 */
void *qlist_getat(qlist_t *list, int index, size_t *size, bool newmem) {
    return get_at(list, index, size, newmem, false);
}

/**
 * qlist->popfirst(): Returns and remove the first element in this list.
 *
 * @param list  qlist_t container pointer.
 * @param size  if size is not NULL, element size will be stored.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  -ENOENT : List is empty.
 *  -ENOMEM : Memory allocation failure.
 */
void *qlist_popfirst(qlist_t *list, size_t *size) {
    return qlist_popat(list, 0, size);
}

/**
 * qlist->getlast(): Returns and remove the last element in this list.
 *
 * @param list  qlist_t container pointer.
 * @param size  if size is not NULL, element size will be stored.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  -ENOENT : List is empty.
 *  -ENOMEM : Memory allocation failure.
 */
void *qlist_poplast(qlist_t *list, size_t *size) {
    return qlist_popat(list, -1, size);
}

/**
 * qlist->popat(): Returns and remove the element at the specified
 * position in this list.
 *
 * @param list   qlist_t container pointer.
 * @param index  index at which the specified element is to be inserted
 * @param size   if size is not NULL, element size will be stored.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  -ERANGE : Index out of range.
 *  -ENOMEM : Memory allocation failure.
 *
 * @code
 *                     first           last
 *  Linked-list        [ A ]<=>[ B ]<=>[ C ]
 *  (positive index)     0       1       2
 *  (negative index)    -3      -2      -1
 * @endcode
 *
 * @note
 *  Negative index can be used for addressing a element from the end in this
 *  stack. For example, index -1 is same as poplast() and index 0 is same as
 *  popfirst();
 */
void *qlist_popat(qlist_t *list, int index, size_t *size) {
    return get_at(list, index, size, true, true);
}

/**
 * qlist->removefirst(): Removes the first element in this list.
 *
 * @param list  qlist_t container pointer.
 *
 * @return a number of removed objects.
 * @retval errno will be set in error condition.
 *  -ENOENT : List is empty.
 */
bool qlist_removefirst(qlist_t *list) {
    return qlist_removeat(list, 0);
}

/**
 * qlist->removelast(): Removes the last element in this list.
 *
 * @param list  qlist_t container pointer.
 *
 * @return a number of removed objects.
 * @retval errno will be set in error condition.
 *  -ENOENT : List is empty.
 */
bool qlist_removelast(qlist_t *list) {
    return qlist_removeat(list, -1);
}

/**
 * qlist->removeat(): Removes the element at the specified position in
 * this list.
 *
 * @param list   qlist_t container pointer.
 * @param index  index at which the specified element is to be removed.
 *
 * @return a number of removed objects.
 * @retval errno will be set in error condition.
 *  -ERANGE : Index out of range.
 */
bool qlist_removeat(qlist_t *list, int index) {
    qlist_lock(list);

    // get object pointer
    qlist_obj_t *obj = get_obj(list, index);
    if (obj == NULL) {
        qlist_unlock(list);
        return false;
    }

    bool ret = remove_obj(list, obj);

    qlist_unlock(list);

    return ret;
}

/**
 * qlist->getnext(): Get next element in this list.
 *
 * @param list    qlist_t container pointer.
 * @param obj     found data will be stored in this structure
 * @param newmem  whether or not to allocate memory for the element.
 *
 * @return true if found otherwise returns false
 * @retval errno will be set in error condition.
 *  -ENOENT : No next element.
 *  -ENOMEM : Memory allocation failure.
 *
 * @note
 *  obj should be initialized with 0 by using memset() before first call.
 *  If newmem flag is true, user should de-allocate obj.name and obj.data
 *  resources.
 *
 * @code
 *  qlist_t *list = qlist();
 *  (...add data into list...)
 *
 *  qlist_obj_t obj;
 *  memset((void*)&obj, 0, sizeof(obj)); // must be cleared before call
 *  list->lock(list);   // can be omitted in single thread model.
 *  while (list->getnext(list, &obj, false) == true) {
 *   printf("DATA=%s, SIZE=%zu\n", (char*)obj.data, obj.size);
 *  }
 *  list->unlock(list); // release lock.
 * @endcode
 */
bool qlist_getnext(qlist_t *list, qlist_obj_t *obj, bool newmem) {
    if (obj == NULL)
        return false;

    qlist_lock(list);

    qlist_obj_t *cont = NULL;
    if (obj->size == 0)
        cont = list->first;
    else
        cont = obj->next;

    if (cont == NULL) {
        errno = ENOENT;
        qlist_unlock(list);
        return false;
    }

    bool ret = false;
    while (cont != NULL) {
        if (newmem == true) {
            obj->data = malloc(cont->size);
            if (obj->data == NULL)
                break;

            memcpy(obj->data, cont->data, cont->size);
        } else {
            obj->data = cont->data;
        }
        obj->size = cont->size;
        obj->prev = cont->prev;
        obj->next = cont->next;

        ret = true;
        break;
    }

    qlist_unlock(list);
    return ret;
}

/**
 * qlist->size(): Returns the number of elements in this list.
 *
 * @param list  qlist_t container pointer.
 *
 * @return the number of elements in this list.
 */
size_t qlist_size(qlist_t *list) {
    return list->num;
}

/**
 * qlist->size(): Returns the sum of total element size.
 *
 * @param list  qlist_t container pointer.
 *
 * @return the sum of total element size.
 */
size_t qlist_datasize(qlist_t *list) {
    return list->datasum;
}

/**
 * qlist->reverse(): Reverse the order of elements.
 *
 * @param list  qlist_t container pointer.
 */
void qlist_reverse(qlist_t *list) {
    qlist_lock(list);
    qlist_obj_t *obj;
    for (obj = list->first; obj;) {
        qlist_obj_t *next = obj->next;
        obj->next = obj->prev;
        obj->prev = next;
        obj = next;
    }

    obj = list->first;
    list->first = list->last;
    list->last = obj;

    qlist_unlock(list);
}

/**
 * qlist->clear(): Removes all of the elements from this list.
 *
 * @param list  qlist_t container pointer.
 */
void qlist_clear(qlist_t *list) {
    qlist_lock(list);
    qlist_obj_t *obj;
    for (obj = list->first; obj;) {
        qlist_obj_t *next = obj->next;
        free(obj->data);
        free(obj);
        obj = next;
    }

    list->num = 0;
    list->datasum = 0;
    list->first = NULL;
    list->last = NULL;
    qlist_unlock(list);
}

/**
 * qlist->toarray(): Returns the serialized chunk containing all the
 * elements in this list.
 *
 * @param list  qlist_t container pointer.
 * @param size  if size is not NULL, chunk size will be stored.
 *
 * @return a malloced pointer,
 *  otherwise(if there is no data to merge) returns NULL.
 * @retval errno will be set in error condition.
 *  -ENOENT : List is empty.
 *  -ENOMEM : Memory allocation failure.
 */
void *qlist_toarray(qlist_t *list, size_t *size) {
    if (list->num <= 0) {
        if (size != NULL)
            *size = 0;
        errno = ENOENT;
        return NULL;
    }

    qlist_lock(list);

    void *chunk = malloc(list->datasum);
    if (chunk == NULL) {
        qlist_unlock(list);
        errno = ENOMEM;
        return NULL;
    }
    void *dp = chunk;

    qlist_obj_t *obj;
    for (obj = list->first; obj; obj = obj->next) {
        memcpy(dp, obj->data, obj->size);
        dp += obj->size;
    }
    qlist_unlock(list);

    if (size != NULL)
        *size = list->datasum;
    return chunk;
}

/**
 * qlist->tostring(): Returns a string representation of this list,
 * containing string representation of each element.
 *
 * @param list  qlist_t container pointer.
 *
 * @return a malloced pointer,
 *  otherwise(if there is no data to merge) returns NULL.
 * @retval errno will be set in error condition.
 *  -ENOENT : List is empty.
 *  -ENOMEM : Memory allocation failure.
 *
 * @note
 *  Return string is always terminated by '\0'.
 */
char *qlist_tostring(qlist_t *list) {
    if (list->num <= 0) {
        errno = ENOENT;
        return NULL;
    }

    qlist_lock(list);

    void *chunk = malloc(list->datasum + 1);
    if (chunk == NULL) {
        qlist_unlock(list);
        errno = ENOMEM;
        return NULL;
    }
    void *dp = chunk;

    qlist_obj_t *obj;
    for (obj = list->first; obj; obj = obj->next) {
        size_t size = obj->size;
        // do not copy tailing '\0'
        if (*(char *) (obj->data + (size - 1)) == '\0')
            size -= 1;
        memcpy(dp, obj->data, size);
        dp += size;
    }
    *((char *) dp) = '\0';
    qlist_unlock(list);

    return (char *) chunk;
}

/**
 * qlist->debug(): Prints out stored elements for debugging purpose.
 *
 * @param list  qlist_t container pointer.
 * @param out   output stream FILE descriptor such like stdout, stderr.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  -EIO  : Invalid output stream.
 */
bool qlist_debug(qlist_t *list, FILE *out) {
    if (out == NULL) {
        errno = EIO;
        return false;
    }

    qlist_lock(list);
    qlist_obj_t *obj;
    int i;
    for (i = 0, obj = list->first; obj; obj = obj->next, i++) {
        fprintf(out, "%d=", i);
        _q_textout(out, obj->data, obj->size, MAX_HUMANOUT);
        fprintf(out, " (%zu)\n", obj->size);
    }
    qlist_unlock(list);

    return true;
}

/**
 * qlist->lock(): Enters critical section.
 *
 * @param list  qlist_t container pointer.
 *
 * @note
 *  From user side, normally locking operation is only needed when traverse all
 *  elements using qlist->getnext().
 */
void qlist_lock(qlist_t *list) {
    Q_MUTEX_ENTER(list->qmutex);
}

/**
 * qlist->unlock(): Leaves critical section.
 *
 * @param list  qlist_t container pointer.
 */
void qlist_unlock(qlist_t *list) {
    Q_MUTEX_LEAVE(list->qmutex);
}

/**
 * qlist->free(): Free qlist_t.
 *
 * @param list  qlist_t container pointer.
 */
void qlist_free(qlist_t *list) {
    qlist_clear(list);
    Q_MUTEX_DESTROY(list->qmutex);

    free(list);
}

#ifndef _DOXYGEN_SKIP

static void *get_at(qlist_t *list, int index, size_t *size, bool newmem,
bool remove) {
    qlist_lock(list);

    // get object pointer
    qlist_obj_t *obj = get_obj(list, index);
    if (obj == NULL) {
        qlist_unlock(list);
        return false;
    }

    // copy data
    void *data;
    if (newmem == true) {
        data = malloc(obj->size);
        if (data == NULL) {
            qlist_unlock(list);
            errno = ENOMEM;
            return false;
        }
        memcpy(data, obj->data, obj->size);
    } else {
        data = obj->data;
    }
    if (size != NULL)
        *size = obj->size;

    // remove if necessary
    if (remove == true) {
        if (remove_obj(list, obj) == false) {
            if (newmem == true)
                free(data);
            data = NULL;
        }
    }

    qlist_unlock(list);

    return data;
}

static qlist_obj_t *get_obj(qlist_t *list, int index) {
    // index adjustment
    if (index < 0)
        index = list->num + index;
    if (index >= list->num) {
        errno = ERANGE;
        return NULL;
    }

    // detect faster scan direction
    bool backward;
    qlist_obj_t *obj;
    int listidx;
    if (index < list->num / 2) {
        backward = false;
        obj = list->first;
        listidx = 0;
    } else {
        backward = true;
        obj = list->last;
        listidx = list->num - 1;
    }

    // find object
    while (obj != NULL) {
        if (listidx == index)
            return obj;

        if (backward == false) {
            obj = obj->next;
            listidx++;
        } else {
            obj = obj->prev;
            listidx--;
        }
    }

    // never reach here
    errno = ENOENT;
    return NULL;
}

static bool remove_obj(qlist_t *list, qlist_obj_t *obj) {
    if (obj == NULL)
        return false;

    // chain prev and next elements
    if (obj->prev == NULL)
        list->first = obj->next;
    else
        obj->prev->next = obj->next;
    if (obj->next == NULL)
        list->last = obj->prev;
    else
        obj->next->prev = obj->prev;

    // adjust counter
    list->datasum -= obj->size;
    list->num--;

    // release obj
    free(obj->data);
    free(obj);

    return true;
}

#endif /* _DOXYGEN_SKIP */
