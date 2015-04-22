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
 * @file qgrow.c Grow container that handles growable objects.
 *
 * qgrow container is a grow implementation. It implements a growable array
 * of objects and it extends qlist container that allow a linked-list to be
 * treated as a grow.
 *
 * @code
 *  [Code sample - Object]
 *  qgrow_t *grow = qgrow(QGROW_THREADSAFE);
 *
 *  // add elements
 *  grow->addstr(grow, "AB");      // no need to supply size
 *  grow->addstrf(grow, "%d", 12); // for formatted string
 *  grow->addstr(grow, "CD");
 *
 *  // get the chunk as a string
 *  char *final = grow->tostring(grow);
 *
 *  // print out
 *  printf("Number of elements = %zu\n", grow->size(grow));
 *  printf("Final string = %s\n", final);
 *
 *  // release
 *  free(final);
 *  grow->free(grow);
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
 *  // get new grow
 *  qgrow_t *grow = qgrow();
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
 *    grow->add(grow, (void *)&obj, sizeof(struct sampleobj));
 *  }
 *
 *  // final
 *  struct sampleobj *final;
 *  final = (struct sampleobj *)grow->toarray(grow, NULL);
 *
 *  // print out
 *  printf("Number of Objects = %zu\n", grow->size(grow));
 *  for(i = 0; i < grow->size(grow); i++) {
 *    printf("Object%d %d, %s\n", i+1, final[i].num, final[i].str);
 *  }
 *
 *  // release
 *  free(final);
 *  grow->free(grow);
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
#include "containers/qgrow.h"

/**
 * Initialize grow.
 *
 * @param options   combination of initialization options.
 *
 * @return qgrow_t container pointer.
 * @retval errno will be set in error condition.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @code
 *  // allocate memory
 *  qgrow_t *grow = qgrow(QGROW_THREADSAFE);
 *  grow->free(grow);
 * @endcode
 *
 * @note
 *   Available options:
 *   - QGROW_THREADSAFE - make it thread-safe.
 */
qgrow_t *qgrow(int options) {
    qgrow_t *grow = (qgrow_t *) calloc(1, sizeof(qgrow_t));
    if (grow == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    grow->list = qlist(options);
    if (grow->list == NULL) {
        free(grow);
        errno = ENOMEM;
        return NULL;
    }

    // methods
    grow->add = qgrow_add;
    grow->addstr = qgrow_addstr;
    grow->addstrf = qgrow_addstrf;

    grow->size = qgrow_size;
    grow->datasize = qgrow_datasize;

    grow->toarray = qgrow_toarray;
    grow->tostring = qgrow_tostring;

    grow->clear = qgrow_clear;
    grow->debug = qgrow_debug;
    grow->free = qgrow_free;

    return grow;
}

/**
 * qgrow->add(): Stack object
 *
 * @param grow    qgrow_t container pointer.
 * @param object    a pointer of object data
 * @param size        size of object
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - EINVAL    : Invalid argument.
 *  - ENOMEM    : Memory allocation failure.
 */
bool qgrow_add(qgrow_t *grow, const void *data, size_t size) {
    return grow->list->addlast(grow->list, data, size);
}

/**
 * qgrow->addstr(): Stack string
 *
 * @param grow    qgrow_t container pointer.
 * @param str        a pointer of string
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - EINVAL    : Invalid argument.
 *  - ENOMEM    : Memory allocation failure.
 */
bool qgrow_addstr(qgrow_t *grow, const char *str) {
    return grow->list->addlast(grow->list, str, strlen(str));
}

/**
 * qgrow->addstrf(): Stack formatted string
 *
 * @param grow    qgrow_t container pointer.
 * @param format    string format
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - EINVAL    : Invalid argument.
 *  - ENOMEM    : Memory allocation failure.
 */
bool qgrow_addstrf(qgrow_t *grow, const char *format, ...) {
    char *str;
    DYNAMIC_VSPRINTF(str, format);
    if (str == NULL) {
        errno = ENOMEM;
        return false;
    }

    bool ret = qgrow_addstr(grow, str);
    free(str);

    return ret;
}

/**
 * qgrow->size(): Returns the number of elements in this grow.
 *
 * @param grow    qgrow_t container pointer.
 *
 * @return the number of elements in this grow.
 */
size_t qgrow_size(qgrow_t *grow) {
    return grow->list->size(grow->list);
}

/**
 * qgrow->datasize(): Returns the sum of total element size in this
 * grow.
 *
 * @param grow    qgrow_t container pointer.
 *
 * @return the sum of total element size in this grow.
 */
size_t qgrow_datasize(qgrow_t *grow) {
    return grow->list->datasize(grow->list);
}

/**
 * qgrow->toarray(): Returns the serialized chunk containing all the
 * elements in this grow.
 *
 * @param grow    qgrow_t container pointer.
 * @param size        if size is not NULL, merged object size will be stored.
 *
 * @return a pointer of finally merged elements(malloced), otherwise returns
 *  NULL
 * @retval errno will be set in error condition.
 *  - ENOENT    : empty.
 *  - ENOMEM    : Memory allocation failure.
 */
void *qgrow_toarray(qgrow_t *grow, size_t *size) {
    return grow->list->toarray(grow->list, size);
}

/**
 * qgrow->tostring(): Returns a string representation of this grow,
 * containing string representation of each element.
 *
 * @param grow    qgrow_t container pointer.
 *
 * @return a pointer of finally merged strings(malloced), otherwise returns NULL
 * @retval errno will be set in error condition.
 *  - ENOENT    : empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * Return string is always terminated by '\0'.
 */
char *qgrow_tostring(qgrow_t *grow) {
    return grow->list->tostring(grow->list);
}

/**
 * qgrow->clear(): Removes all of the elements from this grow.
 *
 * @param grow    qgrow_t container pointer.
 */
void qgrow_clear(qgrow_t *grow) {
    grow->list->clear(grow->list);
}

/**
 * qgrow->debug(): Print out stored elements for debugging purpose.
 *
 * @param grow    qgrow_t container pointer.
 * @param out       output stream FILE descriptor such like stdout, stderr.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EIO   : Invalid output stream.
 */
bool qgrow_debug(qgrow_t *grow, FILE *out) {
    return grow->list->debug(grow->list, out);
}

/**
 * qgrow->free(): De-allocate grow
 *
 * @param grow    qgrow_t container pointer.
 */
void qgrow_free(qgrow_t *grow) {
    grow->list->free(grow->list);
    free(grow);
}
