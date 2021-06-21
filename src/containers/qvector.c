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
/* This code is written and updated by following people and released under
 * the same license as above qLibc license.
 * Copyright (c) 2015 Zhenjiang Xie - https://github.com/Charles0429
 *****************************************************************************/

/**
 * @file qvector.c Vector container implementation.
 *
 * qvector container is a vector container implementation
 * qvector provides uniformly named methods to add, get, pop and remove an
 * element at the beginning and end of the vector.
 *
 * @code
 *  [Conceptional Data Structure Diagram]
 *
 *  DATA             [ C ][ B ][ A ]
 *  (positive index)   0    1    2
 *  (negative index)  -3   -2   -1
 *
 * @encode
 *
 * @code
 *  //create a vector
 *  qvector_t *vector = qvector(QVECTOR_THREADSAFE, 3, sizeof(int));
 *
 *  //insert elements
 *  vector->addlast(vector, 100);
 *  vector->addlast(vector, 101);
 *  vector->addlast(vector, 102);
 *
 *  //get
 *  void *e1 = vector->getfirst(vector, true);   //malloced
 *  void *e3 = vector->getlast(vector, false);   //not malloced
 *  (...omit...)
 *  free(e1);
 *
 *  //pop (get and remove)
 *  void *e2 = vector->popat(vector, 1);    //get malloced copy
 *  (...omit...)
 *  free(e2);
 *
 *  //debug output
 *  vector->debug(vector, stdout, true);
 *
 *  //remove all the elements
 *  vector->clear(vector);
 *
 *  //free vector object
 *  vector->free(vector);
 * @endcode
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "qinternal.h"
#include "containers/qvector.h"

#ifndef _DOXGEN_SKIP

static void *get_at(qvector_t *vector, int index, bool newmem);
static bool remove_at(qvector_t *vector, int index);

#endif

/**
 * Create new qvector_t container
 *
 * @param max       max number of elements
 * @param objsize   size of each element
 * @param options   combination of initialization options.
 *
 * @return a pointer of malloced qvector_t container, otherwise returns NULL
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *  - EINVAL  : Invalid argument.
 *
 * @code
 *  qvector_t *vector = qvector(10, sizeof(int), 0);
 * @endcode
 *
 * @note
 *  Available options:
 *  - QVECTOR_THREADSAFE - make it thread-safe.
 *  - QVECTOR_RESIZE_DOUBLE - double the size when vector is full
 *  - QVECTOR_RESIZE_LINEAR - add the size with initial num when vector is full
 *  - QVECTOR_RESIZE_EXACT - add up as much as needed
 */
qvector_t *qvector(size_t max, size_t objsize, int options) {
    if (objsize == 0) {
        errno = EINVAL;
        return NULL;
    }

    qvector_t *vector = (qvector_t *)calloc(1, sizeof(qvector_t));
    if (vector == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    if (max == 0) {
        vector->data = NULL;
        vector->num = 0;
        vector->max = 0;
        vector->objsize = objsize;
    } else {
        void *data = malloc(max * objsize);
        if(data == NULL) {
            free(vector);
            errno = ENOMEM;
            return NULL;
        }

        vector->data = data;
        vector->num = 0;
        vector->objsize = objsize;
        vector->max = max;
    }

    //handle options
    if (options & QVECTOR_THREADSAFE) {
        Q_MUTEX_NEW(vector->qmutex, true);
        if(vector->qmutex == NULL) {
            free(vector);
            errno = ENOMEM;
            return NULL;
        }
    }

    vector->options = 0;
    if (options & QVECTOR_RESIZE_DOUBLE) {
        vector->options |= QVECTOR_RESIZE_DOUBLE;
    } else if (options & QVECTOR_RESIZE_LINEAR) {
        vector->options |= QVECTOR_RESIZE_LINEAR;
        if (max == 0) {
            vector->initnum = 1;
        } else {
            vector->initnum = max;
        }
    } else {
        vector->options |= QVECTOR_RESIZE_EXACT;
    }

    //member methods
    vector->addfirst = qvector_addfirst;
    vector->addlast = qvector_addlast;
    vector->addat = qvector_addat;

    vector->getfirst = qvector_getfirst;
    vector->getlast = qvector_getlast;
    vector->getat = qvector_getat;

    vector->setfirst = qvector_setfirst;
    vector->setlast = qvector_setlast;
    vector->setat = qvector_setat;

    vector->popfirst = qvector_popfirst;
    vector->poplast = qvector_poplast;
    vector->popat = qvector_popat;

    vector->removefirst = qvector_removefirst;
    vector->removelast = qvector_removelast;
    vector->removeat = qvector_removeat;

    vector->size = qvector_size;
    vector->resize = qvector_resize;

    vector->toarray = qvector_toarray;

    vector->lock = qvector_lock;
    vector->unlock = qvector_unlock;

    vector->clear = qvector_clear;
    vector->debug = qvector_debug;
    vector->free = qvector_free;

    vector->reverse = qvector_reverse;
    vector->getnext = qvector_getnext;

    return vector;
}

/**
 * qvector->addfirst(): Insert a element at the beginning of this vector.
 *
 * @param vector    qvector_t container pointer.
 * @param data      a pointer which points data memory
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *
 * - EINVAL  : Invalid argument.
 * - ENOMEM  : Memory allocation failure.
 *
 * @code
 *  //create a sample object.
 *  struct my_obj obj;
 *
 *  //create a vector and add the sample object.
 *  qvector_t *vector = qvector(0, 1, sizeof(struct my_obj));
 *  vector->addfirst(vector, &obj);
 *
 * @endcode
 */
bool qvector_addfirst(qvector_t *vector, const void *data) {
    return vector->addat(vector, 0, data);
}

/**
 * qvector->addlast(): Insert a element at the end of this vector.
 *
 * @param vector    qvector_t container pointer.
 * @param data      a pointer which points data memory
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *
 * - EINVAL  : Invalid argument.
 * - ENOMEM  : Memory allocation failure.
 *
 * @code
 *  //create a sample object.
 *  struct my_obj obj;
 *
 *  //create a vector and add the sample object.
 *  qvector_t *vector = qvector(0, 1, sizeof(struct my_obj));
 *  vector->addlast(vector, &obj);
 *
 * @endcode
 */
bool qvector_addlast(qvector_t *vector, const void *data) {
    return vector->addat(vector, vector->num, data);
}

/**
 * qvector->addat(): Inserts a element at the specified position in this
 * vector.
 *
 * @param vector    qvector_t container pointer
 * @param index     index at which the specified element is to be inserted
 * @param data      a pointer which points data memory
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in errno condition.
 *
 * - ERANGE  : Index out of range.
 * - EINVAL  : Invalid argument.
 * - ENOMEM  : Memory allocation failure.
 *
 * @code
 *                     first     last      new
 *  Array              [ A ][ B ][ C ]?==?[   ]
 *  (positive index)     0    1    2        3
 *  (negative index)    -3   -2   -1
 *
 * @encode
 *
 * @code
 *  qvector_t *vector = qvector();
 *  vector->addat(vector, 0, &data); //same as addfirst().
 *  vector->addat(vector, 0, &data); //same as addlast().
 *
 * @encode
 *
 * @note
 *  Index starts from 0.
 */
bool qvector_addat(qvector_t *vector, int index, const void *data) {
    //check arguments
    if (data == NULL) {
        errno = EINVAL;
        return false;
    }

    //check index
    if (index < 0) {
        index += vector->num;
    }
    if (index > vector->num) {
        errno = ERANGE;
        return false;
    }

    vector->lock(vector);

    //check whether the vector is full
    if (vector->num >= vector->max) {
        size_t newmax = vector->max + 1;
        if (vector->options & QVECTOR_RESIZE_DOUBLE) {
            newmax = (vector->max + 1) * 2;
        } else if (vector->options & QVECTOR_RESIZE_LINEAR) {
            newmax = vector->max + vector->initnum;
        } else {
            newmax = vector->max + 1;
        }
        bool result = vector->resize(vector, newmax);
        if(result == false)
        {
            vector->unlock(vector);
            errno = ENOMEM;
            return false;
        }
    }

    //shift data from index...(num - 1)  to index + 1...num
    int i;
    for (i = vector->num; i > index; i--) {
        void *dst = (unsigned char *)vector->data + vector->objsize * i;
        void *src = (unsigned char *)vector->data + vector->objsize * (i - 1);

        memcpy(dst, src, vector->objsize);
    }

    void *add = (unsigned char *)vector->data + index * vector->objsize;
    memcpy(add, data, vector->objsize);
    vector->num++;

    vector->unlock(vector);
    return true;
}

/**
 * qvector->getfirst(): Returns the first element in this vector.
 *
 * @param vector    qvector_t container pointer.
 * @param newmem    whether or not to allocate memory for the element.
 *
 * @return a pointer of element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : Vector is empty.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *  size_t size;
 *  void *data = vector->getfirst(vector, true);
 *  if (data != NULL) {
 *      (...omit...)
 *      free(data);
 *  }
 *
 * @endcode
 */
void *qvector_getfirst(qvector_t *vector, bool newmem) {
    return vector->getat(vector, 0, newmem);
}

/**
 * qvector->getlast(): Returns the last element in this vector.
 *
 * @param vector    qvector_t container pointer.
 * @param newmem    whether or not to allocate memory for the element.
 *
 * @return a pointer of element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : Vector is empty.
 *  - ENOMEM : Memory alocation failure.
 *
 * @code
 *  void *data = vector->getlast(vector, true);
 *  if (data != NULL) {
 *      (...omit...)
 *      free(data);
 *  }
 *
 * @endcode
 */
void *qvector_getlast(qvector_t *vector, bool newmem) {
    return vector->getat(vector, -1, newmem);
}

/**
 * qvector->getat(): Returns the element at the specified position in this
 * vector.
 *
 * @param vector    qvector_t container pointer.
 * @param index     index at which the specified element is to access.
 * @param newmem    whether or not to allocate memory for the element.
 *
 * @return a pointer of element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ERANGE : Index out of range.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *                     first     last
 *  Array              [ A ][ B ][ C ]
 *  (positive index)     0    1    2
 *  (negative index)    -1   -2   -3
 *
 * @endcode
 *
 * @note
 *  Index starts from 0.
 */
void *qvector_getat(qvector_t *vector, int index, bool newmem) {
    vector->lock(vector);
    void *data = get_at(vector, index, newmem);
    vector->unlock(vector);

    return data;
}

/**
 * qvector->setfirst(): Set the first element with a new value in this
 * vector.
 *
 * @param vector    qvector_t container pointer.
 * @param data      the pointer of new value.
 *
 * @returns true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOENT : Vector is empty.
 *
 * @code
 *
 *  struct my_obj obj;
 *  //set values to obj;
 *  qvector_t *vector = qvector();
 *  vector->addlast();
 *  vector->setfirst(vector, &obj);
 *
 * @endcode
 */
bool qvector_setfirst(qvector_t *vector, const void *data) {
    return vector->setat(vector, 0, data);
}

/**
 * qvector->setlast(): Set the last element with a new value in this
 * vector.
 *
 * @param vector    qvector_t container pointer.
 * @param data      the pointer of new value.
 *
 * @returns true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOENT : Vector is empty.
 *
 * @code
 *
 *  struct my_obj obj;
 *  //set values to obj;
 *  qvector_t *vector = qvector();
 *  vector->addlast();
 *  vector->setlast(vector, &obj);
 *
 * @endcode
 */
bool qvector_setlast(qvector_t *vector, const void *data) {
    return vector->setat(vector, -1, data);
}

/**
 * qvector->setat(): Set new value to the specified position in this
 * vector.
 *
 * @param vector    qvector_t container pointer
 * @param index     index at which the specifed element is to set
 * @param data      the pointer of new value to be set
 *
 * @return true if successful, otherwise return false.
 * @retval errno will be set in error condition.
 *  - ERANGE : Index out of range.
 *
 * @code
 *
 *  struct my_obj obj;
 *  //set values to obj;
 *  qvector_t *vector = qvector();
 *  vector->addlast();
 *  vector->setat(vector, 0, &obj);
 *
 * @endcode
 */
bool qvector_setat(qvector_t *vector, int index, const void *data) {
    vector->lock(vector);
    void *old_data = get_at(vector, index, false);
    if (old_data == NULL) {
        return false;
    }
    memcpy(old_data, data, vector->objsize);
    vector->unlock(vector);

    return true;
}

/**
 * qvector->popfirst(): Returns and remove the first element in this vector.
 *
 * @param vector    qvector_t container pointer.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : Vector is empty.
 *  - ENOMEM : Memory allocation failure.
 */
void *qvector_popfirst(qvector_t *vector) {
    return vector->popat(vector, 0);
}

/**
 * qvector->poplast(): Returns the last element of this vector.
 *
 * @param vector    qvector_t container pointer.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : Vector is empty.
 *  - ENOMEM : Memeory allocation failure.
 */
void *qvector_poplast(qvector_t *vector) {
    return vector->popat(vector, -1);
}

/**
 * qvector->popat(): Returns and remove the element at specified
 * position in this vector.
 *
 * @param vector    qvector_t container pointer.
 * @param index     index at which the specified element is to be poped.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT : Vector is empty.
 *  - ERANGE : Index out of range.
 *  - ENOMEM : Mmemory allocation failure.
 *
 * @code
 *                      first     last
 *  Array               [ A ][ B ][ C ]
 *  (positive index)      1    2    3
 *  (negative index)     -1   -2   -3
 *
 * @endcode
 *
 * @note
 *  Index starts from 0.
 */
void *qvector_popat(qvector_t *vector, int index) {
    vector->lock(vector);
    void *data = get_at(vector, index, true);
    if (data == NULL) {
        return NULL;
    }

    bool result = remove_at(vector, index);
    if (result == false) {
        free(data);
        vector->unlock(vector);
        return NULL;
    }
    vector->num--;

    vector->unlock(vector);
    return data;
}

/**
 * qvector->removefirst(): Removes the first element in this vector.
 *
 * @param vector    qvector_t container pointer.
 *
 * @return true, otherwise returns false.
 * @retval errno will be set in error condition.
 * - ENOENT : Vector is empty.
 * - ERANGE : Index out of range.
 */
bool qvector_removefirst(qvector_t *vector) {
    return vector->removeat(vector, 0);
}

/**
 * qvector->removelast(): Removes the last element in this vector.
 *
 * @param vector    qvector_t container pointer.
 *
 * @return true, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOENT : Vector is empty.
 *  - ERANGE : Index out of range.
 */
bool qvector_removelast(qvector_t *vector) {
    return vector->removeat(vector, -1);
}

/**
 * qvector->removeat(): Removes the element at the specified position in this vector.
 *
 * @param vector    qvector_t container pointer.
 * @param index     index at which the specified element is to be removed.
 *
 * @return true, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOENT : Vector is empty.
 *  - ERANGE : Index out of range.
 */
bool qvector_removeat(qvector_t *vector, int index) {
    vector->lock(vector);
    bool result = remove_at(vector, index);
    if (result) {
        vector->num--;
    }

    vector->unlock(vector);

    return result;
}

/**
 * qvector->size(): Get the number of elements in this vector.
 *
 * @param vector    qvector_t container pointer.
 *
 * @return the number of elements in this vector.
 */
size_t qvector_size(qvector_t *vector) {
    return vector->num;
}

/**
 * qvector->lock(): Enters critical section.
 *
 * @param vector    qvector_t container pointer.
 *
 * @note
 *  From user side, normally locking operation is only needed when traverse all
 *  elements using qvector->getnext().
 */
void qvector_lock(qvector_t *vector) {
    Q_MUTEX_ENTER(vector->qmutex);
}

/**
 * qvector->unlock(): Leaves critical section.
 *
 * @param vector    qvector_t container pointer.
 */
void qvector_unlock(qvector_t *vector) {
    Q_MUTEX_LEAVE(vector->qmutex);
}

/**
 * qvector->clear(): Remove all the elemnts in this vector.
 *
 * @param vector    qvector_t container pointer.
 */
void qvector_clear(qvector_t *vector) {
    vector->lock(vector);
    vector->num = 0;
    vector->unlock(vector);
}

/**
 * qvector->free(): Free this vector.
 *
 * @param vector    qvector_t container pointer.
 */
void qvector_free(qvector_t *vector) {
    vector->clear(vector);
    Q_MUTEX_DESTROY(vector->qmutex);

    if(vector->data != NULL) {
        free(vector->data);
    }

    free(vector);
}

/**
 * qvector->debug(): Prints out stored elements for debugging purpose.
 *
 * @param vector    qvector_t container pointer.
 * @param out       output stream FILE descriptor such like stdout, stderr.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EIO : Invalid output stream.
 */
bool qvector_debug(qvector_t *vector, FILE *out) {
    if (out == NULL) {
        errno = EIO;
        return false;
    }

    vector->lock(vector);
    int i;
    for (i = 0; i < vector->num; i++) {
        void *data = (unsigned char  *)vector->data + i * vector->objsize;
        fprintf(out, "%d=", i);
        _q_textout(out, data, vector->objsize, MAX_HUMANOUT);
        fprintf(out, " (%zu)\n", vector->objsize);
    }
    vector->unlock(vector);

    return true;
}

/**
 * qvector->resize(): Changes the allocated memory space size.
 *
 * @param vector    qvector_t container pointer.
 * @param newsize   the new max number of elements.
 *
 * @retval errno will be set in error condition.
 *  - ENOMEM : Memory allocation failure.
 *
 * @code
 *  //create a sample object.
 *  struct my_obj obj;
 *
 *  //create a vector which allocates 4 * sizeof(obj) memory
 *  qvector_t *vector = qvector(0, 4, sizeof(struct my_obj));
 *  //expand the memory space of vector to 8 * sizeof(obj)
 *  vector->resize(vector, 8);
 *
 * @endcode
 */
bool qvector_resize(qvector_t *vector, size_t newmax) {
    vector->lock(vector);

    if (newmax == 0) {
        free(vector->data);
        vector->data = NULL;
        vector->max = 0;
        vector->num = 0;
        vector->objsize = 0;

        vector->unlock(vector);
        return true;
    }

    void *newdata = realloc(vector->data, newmax * vector->objsize);
    if (newdata == NULL) {
        errno = ENOMEM;
        vector->unlock(vector);
        return false;
    }

    vector->data = newdata;
    vector->max = newmax;
    if (vector->num > newmax) {
        vector->num = newmax;
    }

    vector->unlock(vector);
    return true;
}

/**
 * qvector->toarray(): Returns an array contains all the elements in this vector.
 * @param vector    qvector_t container pointer.
 * @param size      if size is not NULL, the number of elements will be stored.
 *
 * @return a malloced pointer, otherwise return NULL.
 * @retval errno wil be set in error condition.
 *  - ENOENT : Vector is empty.
 *  - ENOMEM : Memory allocation failure.
 */
void *qvector_toarray(qvector_t *vector, size_t *size) {
    if (vector->num <= 0) {
        if (size != NULL) {
            *size = 0;
        }
        errno = ENOENT;
        return NULL;
    }

    vector->lock(vector);

    void *array = malloc(vector->num * vector->objsize);
    if (array == NULL) {
        vector->unlock(vector);
        errno = ENOMEM;
        return NULL;
    }

    memcpy(array, vector->data, vector->num * vector->objsize);

    if (size != NULL) {
        *size = vector->num;
    }

    vector->unlock(vector);
    return array;
}

/**
 * qvector->reverse(): Reverse the order of element in this vector.
 *
 * @param vector    qvector_t container pointer.
 *
 * @retval will be set in error condition.
 *  - ENOMEM : Memory allocations failure.
 */
void qvector_reverse(qvector_t *vector) {
    vector->lock(vector);

    if (vector->num <= 1) {
        vector->unlock(vector);
        return;
    }

    int i;
    int j;
    void *tmp = malloc(vector->objsize);
    if (tmp == NULL) {
        errno = ENOMEM;
        return;
    }

    for (i = 0, j = vector->num - 1; i < j; i++, j--) {
        void *data1 = (unsigned char *)vector->data + i * vector->objsize;
        void *data2 = (unsigned char *)vector->data + j * vector->objsize;

        memcpy(tmp, data1, vector->objsize);
        memcpy(data1, data2, vector->objsize);
        memcpy(data2, tmp, vector->objsize);
    }
    free(tmp);

    vector->unlock(vector);
}

/**
 * qvector->getnext(): Get next element in this vector.
 *
 * @param vector    qvector_t container pointer.
 * @param obj       found data will be stored in this structure.
 * @param newmem    whether or not to allocate memory for element.
 *
 * @return true if found, otherwise return fasle.
 * @retval errno will be set in error condition.
 *  - ENOENT : No next element.
 *  - ENOMEM : Memory allocation failure.
 *
 * @note
 *  obj should be initialized with 0 by using memset() by the first call.
 *  If newmem flag is true, user should de-allocate obj.data resources.
 *
 * @code
 *  qvector_t *vector = qvector();
 *  (...add data into vector...)
 *
 *  qvector_obj_t obj;
 *  memset((void *)&obj, 0, sizeof(obj));
 *  vector->lock(vector);
 *  while(vector->getnext(vector, &obj, false) == true) {
 *      printf("DATA=%s\n", obj.data);
 *  }
 *  vector->unlock(vector);
 * @endcode
 */
bool qvector_getnext(qvector_t *vector, qvector_obj_t *obj, bool newmem) {
    if (obj == NULL) {
        return false;
    }
    vector->lock(vector);

    if (obj->index >= vector->num) {
        errno = ENOENT;
        obj->data = NULL;
        vector->unlock(vector);
        return false;
    }

    void *data = (unsigned char *)vector->data + (obj->index) * vector->objsize;
    if (newmem) {
        void *dump = malloc(vector->objsize);
        if (dump == NULL ) {
            errno = ENOMEM;
            obj->data = NULL;
            vector->unlock(vector);
            return false;
        }
        memcpy(dump, data, vector->objsize);
        obj->data = dump;
    }
    else
    {
        obj->data = data;
    }

    obj->index++;
    vector->unlock(vector);
    return true;
}

#ifndef _DOXYGEN_SKIP

static void *get_at(qvector_t *vector, int index, bool newmem) {
    if (index < 0) {
        index += vector->num;
    }
    if (index >= vector->num) {
        if (vector->num == 0) {
            errno = ENOENT;
            return NULL;
        } else {
            errno = ERANGE;
            return NULL;
        }
    }

    void *src_data = (unsigned char *)vector->data + index * vector->objsize;
    if (newmem) {
        void *dump_data = malloc(vector->objsize);
        if (dump_data == NULL) {
            errno = ENOMEM;
            return NULL;
        } else {
            memcpy(dump_data, src_data, vector->objsize);
            return dump_data;
        }
    } else {
        return src_data;
    }
}

static bool remove_at(qvector_t *vector, int index) {
    if (index < 0) {
        index += vector->num;
    }
    if (index >= vector->num) {
        if (vector->num == 0) {
            errno = ENOENT;
            return false;
        } else {
            errno = ERANGE;
            return false;
        }
    }

    int i;
    for (i = index + 1; i < vector->num; i++) {
        void *src = (unsigned char *)vector->data + i * vector->objsize;
        void *dst = (unsigned char *)vector->data + (i - 1) * vector->objsize;

        memcpy(dst, src, vector->objsize);
    }

    return true;
}

#endif
