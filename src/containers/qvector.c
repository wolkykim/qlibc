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
 * @file qvector.c Vector implementation.
 *
 * qvector container is a vector implementation. It implements a growable array
 * of objects and it extends qlist container that allow a linked-list to be
 * treated as a vector.
 *
 * @code
 *  [Code sample - Object]
 *  qvector_t *vector = qvector(QVECTOR_THREADSAFE);
 *
 *  // add elements
 *  vector->addstr(vector, "AB");      // no need to supply size
 *  vector->addstrf(vector, "%d", 12); // for formatted string
 *  vector->addstr(vector, "CD");
 *
 *  // get the chunk as a string
 *  char *final = vector->tostring(vector);
 *
 *  // print out
 *  printf("Number of elements = %zu\n", vector->size(vector));
 *  printf("Final string = %s\n", final);
 *
 *  // release
 *  free(final);
 *  vector->free(vector);
 *
 *  [Result]
 *  Number of elements = 3
 *  Final string = AB12CD
 * @endcode
 *
 * @code
 *  [Code sample - Object]
 *  // sample object
 *  struct sampleobj {
 *    int num;
 *    char str[10];
 *  };
 *
 *  // get new vector
 *  qvector_t *vector = qvector();
 *
 *  // add objects
 *  int i;
 *  struct sampleobj obj;
 *  for(i = 0; i < 3; i++) {
 *    // filling object with sample data
 *    obj.num  = i;
 *    sprintf(obj.str, "hello%d", i);
 *
 *    // stack
 *    vector->add(vector, (void *)&obj, sizeof(struct sampleobj));
 *  }
 *
 *  // final
 *  struct sampleobj *final;
 *  final = (struct sampleobj *)vector->toarray(vector, NULL);
 *
 *  // print out
 *  printf("Number of Objects = %zu\n", vector->size(vector));
 *  for(i = 0; i < vector->size(vector); i++) {
 *    printf("Object%d %d, %s\n", i+1, final[i].num, final[i].str);
 *  }
 *
 *  // release
 *  free(final);
 *  vector->free(vector);
 *
 *  [Result]
 *  Number of Objects = 3
 *  Object1 0, hello0
 *  Object2 1, hello1
 *  Object3 2, hello2
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "qinternal.h"
#include "containers/qvector.h"

/*
 * Member method protos
 */
#ifndef _DOXYGEN_SKIP
static bool add(qvector_t *vector, const void *object, size_t size);
static bool addstr(qvector_t *vector, const char *str);
static bool addstrf(qvector_t *vector, const char *format, ...);
static void *toarray(qvector_t *vector, size_t *size);
static char *tostring(qvector_t *vector);
static size_t size(qvector_t *vector);
static size_t datasize(qvector_t *vector);
static void clear(qvector_t *vector);
static bool debug(qvector_t *vector, FILE *out);
static void free_(qvector_t *vector);
#endif

/**
 * Initialize vector.
 *
 * @param options   combination of initialization options.
 *
 * @return qvector_t container pointer.
 * @retval errno will be set in error condition.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @code
 *  // allocate memory
 *  qvector_t *vector = qvector(QVECTOR_THREADSAFE);
 *  vector->free(vector);
 * @endcode
 *
 * @note
 *   Available options:
 *   - QVECTOR_THREADSAFE - make it thread-safe.
 */
qvector_t *qvector(int options) {
    qvector_t *vector = (qvector_t *) calloc(1, sizeof(qvector_t));
    if (vector == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    vector->list = qlist(options);
    if (vector->list == NULL) {
        free(vector);
        errno = ENOMEM;
        return NULL;
    }

    // methods
    vector->add = add;
    vector->addstr = addstr;
    vector->addstrf = addstrf;

    vector->toarray = toarray;
    vector->tostring = tostring;

    vector->size = size;
    vector->datasize = datasize;
    vector->clear = clear;
    vector->debug = debug;
    vector->free = free_;

    return vector;
}

/**
 * qvector->add(): Stack object
 *
 * @param vector    qvector_t container pointer.
 * @param object    a pointer of object data
 * @param size        size of object
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - EINVAL    : Invalid argument.
 *  - ENOMEM    : Memory allocation failure.
 */
static bool add(qvector_t *vector, const void *data, size_t size) {
    return vector->list->addlast(vector->list, data, size);
}

/**
 * qvector->addstr(): Stack string
 *
 * @param vector    qvector_t container pointer.
 * @param str        a pointer of string
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - EINVAL    : Invalid argument.
 *  - ENOMEM    : Memory allocation failure.
 */
static bool addstr(qvector_t *vector, const char *str) {
    return vector->list->addlast(vector->list, str, strlen(str));
}

/**
 * qvector->addstrf(): Stack formatted string
 *
 * @param vector    qvector_t container pointer.
 * @param format    string format
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - EINVAL    : Invalid argument.
 *  - ENOMEM    : Memory allocation failure.
 */
static bool addstrf(qvector_t *vector, const char *format, ...) {
    char *str;
    DYNAMIC_VSPRINTF(str, format);
    if (str == NULL) {
        errno = ENOMEM;
        return false;
    }

    bool ret = addstr(vector, str);
    free(str);

    return ret;
}

/**
 * qvector->toarray(): Returns the serialized chunk containing all the
 * elements in this vector.
 *
 * @param vector    qvector_t container pointer.
 * @param size        if size is not NULL, merged object size will be stored.
 *
 * @return a pointer of finally merged elements(malloced), otherwise returns
 *  NULL
 * @retval errno will be set in error condition.
 *  - ENOENT    : Vector is empty.
 *  - ENOMEM    : Memory allocation failure.
 */
static void *toarray(qvector_t *vector, size_t *size) {
    return vector->list->toarray(vector->list, size);
}

/**
 * qvector->tostring(): Returns a string representation of this vector,
 * containing string representation of each element.
 *
 * @param vector    qvector_t container pointer.
 *
 * @return a pointer of finally merged strings(malloced), otherwise returns NULL
 * @retval errno will be set in error condition.
 *  - ENOENT    : Vector is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * Return string is always terminated by '\0'.
 */
static char *tostring(qvector_t *vector) {
    return vector->list->tostring(vector->list);
}

/**
 * qvector->size(): Returns the number of elements in this vector.
 *
 * @param vector    qvector_t container pointer.
 *
 * @return the number of elements in this vector.
 */
static size_t size(qvector_t *vector) {
    return vector->list->size(vector->list);
}

/**
 * qvector->datasize(): Returns the sum of total element size in this
 * vector.
 *
 * @param vector    qvector_t container pointer.
 *
 * @return the sum of total element size in this vector.
 */
static size_t datasize(qvector_t *vector) {
    return vector->list->datasize(vector->list);
}

/**
 * qvector->clear(): Removes all of the elements from this vector.
 *
 * @param vector    qvector_t container pointer.
 */
static void clear(qvector_t *vector) {
    vector->list->clear(vector->list);
}

/**
 * qvector->debug(): Print out stored elements for debugging purpose.
 *
 * @param vector    qvector_t container pointer.
 * @param out       output stream FILE descriptor such like stdout, stderr.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EIO   : Invalid output stream.
 */
static bool debug(qvector_t *vector, FILE *out) {
    return vector->list->debug(vector->list, out);
}

/**
 * qvector->free(): De-allocate vector
 *
 * @param vector    qvector_t container pointer.
 */
static void free_(qvector_t *vector) {
    vector->list->free(vector->list);
    free(vector);
}
