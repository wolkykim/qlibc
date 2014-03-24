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
 * @file qqueue.c Queue implementation.
 *
 * qqueue container is a queue implementation. It represents a
 * first-in-first-out(FIFO). It extends qlist container that allow a
 * linked-list to be treated as a queue.
 *
 * @code
 *  [Conceptional Data Structure Diagram]
 *
 *                      top     bottom
 *  DATA POP    <==    [ A ][ B ][ C ]  <== DATA PUSH
 *  (positive index)     0    1    2
 *  (negative index)    -3   -2   -1
 * @endcode
 *
 * @code
 *  // create queue
 *  qqueue_t *queue = qQueue();
 *
 *  // example: integer queue
 *  queue->pushint(queue, 1);
 *  queue->pushint(queue, 2);
 *  queue->pushint(queue, 3);
 *
 *  printf("popint(): %d\n", queue->popint(queue));
 *  printf("popint(): %d\n", queue->popint(queue));
 *  printf("popint(): %d\n", queue->popint(queue));
 *
 *  // example: string queue
 *  queue->pushstr(queue, "A string");
 *  queue->pushstr(queue, "B string");
 *  queue->pushstr(queue, "C string");
 *
 *  char *str = queue->popstr(queue);
 *  printf("popstr(): %s\n", str);
 *  free(str);
 *  str = queue->popstr(queue);
 *  printf("popstr(): %s\n", str);
 *  free(str);
 *  str = queue->popstr(queue);
 *  printf("popstr(): %s\n", str);
 *  free(str);
 *
 *  // example: object queue
 *  queue->push(queue, "A object", sizeof("A object"));
 *  queue->push(queue, "B object", sizeof("B object"));
 *  queue->push(queue, "C object", sizeof("C object"));
 *
 *  void *obj = queue->pop(queue, NULL);
 *  printf("pop(): %s\n", (char*)obj);
 *  free(obj);
 *  str = queue->pop(queue, NULL);
 *  printf("pop(): %s\n", (char*)obj);
 *  free(obj);
 *  str = queue->pop(queue, NULL);
 *  printf("pop(): %s\n", (char*)obj);
 *  free(obj);
 *
 *  // release
 *  queue->free(queue);
 *
 *  [Output]
 *  popint(): 3
 *  popint(): 2
 *  popint(): 1
 *  popstr(): C string
 *  popstr(): B string
 *  popstr(): A string
 *  pop(): C object
 *  pop(): B object
 *  pop(): A object
 * @endcode
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "qlibc.h"
#include "qinternal.h"

/*
 * Member method protos
 */
#ifndef _DOXYGEN_SKIP

static size_t setsize(qqueue_t *queue, size_t max);

static bool push(qqueue_t *queue, const void *data, size_t size);
static bool pushstr(qqueue_t *queue, const char *str);
static bool pushint(qqueue_t *queue, int64_t num);

static void *pop(qqueue_t *queue, size_t *size);
static char *popstr(qqueue_t *queue);
static int64_t  popint(qqueue_t *queue);
static void *popat(qqueue_t *queue, int index, size_t *size);

static void *get(qqueue_t *queue, size_t *size, bool newmem);
static char *getstr(qqueue_t *queue);
static int64_t getint(qqueue_t *queue);
static void *getat(qqueue_t *queue, int index, size_t *size, bool newmem);

static size_t size(qqueue_t *queue);
static void clear(qqueue_t *queue);
static bool debug(qqueue_t *queue, FILE *out);
static void free_(qqueue_t *queue);

#endif

/**
 * Create new queue container
 *
 * @param options   combination of initialization options.
 *
 * @return a pointer of malloced qqueue container, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @code
 *   qqueue_t *queue = qQueue(QQUEUE_OPT_THREADSAFE);
 * @endcode
 *
 * @note
 *   Available options:
 *   - QQUEUE_OPT_THREADSAFE - make it thread-safe.
 */
qqueue_t *qqueue(int options)
{
    qqueue_t *queue = (qqueue_t *)malloc(sizeof(qqueue_t));
    if (queue == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    memset((void *)queue, 0, sizeof(qqueue_t));
    queue->list = qlist(options);
    if (queue->list == NULL) {
        free(queue);
        return NULL;
    }

    // methods
    queue->setsize      = setsize;

    queue->push         = push;
    queue->pushstr      = pushstr;
    queue->pushint      = pushint;

    queue->pop          = pop;
    queue->popstr       = popstr;
    queue->popint       = popint;
    queue->popat        = popat;

    queue->get          = get;
    queue->getstr       = getstr;
    queue->getint       = getint;
    queue->getat        = getat;

    queue->size         = size;
    queue->clear        = clear;
    queue->debug        = debug;
    queue->free         = free_;

    return queue;
}

/**
 * qqueue->setsize(): Sets maximum number of elements allowed in this
 * queue.
 *
 * @param queue qqueue container pointer.
 * @param max   maximum number of elements. 0 means no limit.
 *
 * @return previous maximum number.
 */
static size_t setsize(qqueue_t *queue, size_t max)
{
    return queue->list->setsize(queue->list, max);
}

/**
 * qqueue->push(): Pushes an element onto the top of this queue.
 *
 * @param queue qqueue container pointer.
 * @param data  a pointer which points data memory.
 * @param size  size of the data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EINVAL    : Invalid argument.
 *  - ENOBUFS   : Queue full. Only happens when this queue has set to have
 *                limited number of elements)
 *  - ENOMEM    : Memory allocation failure.
 */
static bool push(qqueue_t *queue, const void *data, size_t size)
{
    return queue->list->addlast(queue->list, data, size);
}

/**
 * qqueue->pushstr(): Pushes a string onto the top of this queue.
 *
 * @param queue qqueue container pointer.
 * @param data  a pointer which points data memory.
 * @param size  size of the data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EINVAL    : Invalid argument.
 *  - ENOBUFS   : Queue full. Only happens when this queue has set to have
 *                limited number of elements.
 *  - ENOMEM    : Memory allocation failure.
 */
static bool pushstr(qqueue_t *queue, const char *str)
{
    if (str == NULL) {
        errno = EINVAL;
        return false;
    }
    return queue->list->addlast(queue->list, str, strlen(str) + 1);
}

/**
 * qqueue->pushint(): Pushes a integer onto the top of this queue.
 *
 * @param queue qqueue container pointer.
 * @param num   integer data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOBUFS   : Queue full. Only happens when this queue has set to have
 *                limited number of elements.
 *  - ENOMEM    : Memory allocation failure.
 */
static bool pushint(qqueue_t *queue, int64_t num)
{
    return queue->list->addlast(queue->list, &num, sizeof(num));
}

/**
 * qqueue->pop(): Removes a element at the top of this queue and returns
 * that element.
 *
 * @param queue qqueue container pointer.
 * @param size  if size is not NULL, element size will be stored.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Queue is empty.
 *  - ENOMEM    : Memory allocation failure.
 */
static void *pop(qqueue_t *queue, size_t *size)
{
    return queue->list->popfirst(queue->list, size);
}

/**
 * qqueue->popstr(): Removes a element at the top of this queue and
 * returns that element.
 *
 * @param queue qqueue container pointer.
 *
 * @return a pointer of malloced string element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Queue is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * The string element should be pushed through pushstr().
 */
static char *popstr(qqueue_t *queue)
{
    size_t strsize;
    char *str = queue->list->popfirst(queue->list, &strsize);
    if (str != NULL) {
        str[strsize - 1] = '\0'; // just to make sure
    }

    return str;
}

/**
 * qqueue->popint(): Removes a integer at the top of this queue and
 * returns that element.
 *
 * @param queue qqueue container pointer.
 *
 * @return an integer value, otherwise returns 0.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Queue is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * The integer element should be pushed through pushint().
 */
static int64_t popint(qqueue_t *queue)
{
    int64_t num = 0;
    int64_t *pnum = queue->list->popfirst(queue->list, NULL);
    if (pnum != NULL) {
        num = *pnum;
        free(pnum);
    }

    return num;
}

/**
 * qqueue->popat(): Returns and remove the element at the specified
 * position in this queue.
 *
 * @param queue qqueue container pointer.
 * @param index index at which the specified element is to be inserted
 * @param size  if size is not NULL, element size will be stored.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ERANGE    : Index out of range.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 *  Negative index can be used for addressing a element from the bottom in this
 *  queue. For example, index -1 will always pop a element which is pushed at
 *  very last time.
 */
static void *popat(qqueue_t *queue, int index, size_t *size)
{
    return queue->list->popat(queue->list, index, size);
}

/**
 * qqueue->get(): Returns an element at the top of this queue without
 * removing it.
 *
 * @param queue     qqueue container pointer.
 * @param size      if size is not NULL, element size will be stored.
 * @param newmem    whether or not to allocate memory for the element.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Queue is empty.
 *  - ENOMEM    : Memory allocation failure.
 */
static void *get(qqueue_t *queue, size_t *size, bool newmem)
{
    return queue->list->getfirst(queue->list, size, newmem);
}

/**
 * qqueue->getstr(): Returns an string at the top of this queue without
 * removing it.
 *
 * @param queue qqueue container pointer.
 *
 * @return a pointer of malloced string element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Queue is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * The string element should be pushed through pushstr().
 */
static char *getstr(qqueue_t *queue)
{
    size_t strsize;
    char *str = queue->list->getfirst(queue->list, &strsize, true);
    if (str != NULL) {
        str[strsize - 1] = '\0'; // just to make sure
    }

    return str;
}

/**
 * qqueue->getint(): Returns an integer at the top of this queue without
 * removing it.
 *
 * @param queue qqueue container pointer.
 *
 * @return an integer value, otherwise returns 0.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Queue is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 *  The integer element should be pushed through pushint().
 */
static int64_t getint(qqueue_t *queue)
{
    int64_t num = 0;
    int64_t *pnum = queue->list->getfirst(queue->list, NULL, true);
    if (pnum != NULL) {
        num = *pnum;
        free(pnum);
    }

    return num;
}

/**
 * qqueue->getat(): Returns an element at the specified position in this
 * queue without removing it.
 *
 * @param queue     qqueue container pointer.
 * @param index     index at which the specified element is to be inserted
 * @param size      if size is not NULL, element size will be stored.
 * @param newmem    whether or not to allocate memory for the element.
 *
 * @return a pointer of element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ERANGE    : Index out of range.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 *  Negative index can be used for addressing a element from the bottom in this
 *  queue. For example, index -1 will always get a element which is pushed at
 *  very last time.
 */
static void *getat(qqueue_t *queue, int index, size_t *size, bool newmem)
{
    return queue->list->getat(queue->list, index, size, newmem);
}

/**
 * qqueue->size(): Returns the number of elements in this queue.
 *
 * @param queue qqueue container pointer.
 *
 * @return the number of elements in this queue.
 */
static size_t size(qqueue_t *queue)
{
    return queue->list->size(queue->list);
}

/**
 * qqueue->clear(): Removes all of the elements from this queue.
 *
 * @param queue qqueue container pointer.
 */
static void clear(qqueue_t *queue)
{
    queue->list->clear(queue->list);
}

/**
 * qqueue->debug(): Print out stored elements for debugging purpose.
 *
 * @param queue     qqueue container pointer.
 * @param out       output stream FILE descriptor such like stdout, stderr.
 *
 * @return true if successful, otherwise returns false.
 */
static bool debug(qqueue_t *queue, FILE *out)
{
    return queue->list->debug(queue->list, out);
}

/**
 * qqueue->free(): Free qqueue_t
 *
 * @param queue qqueue container pointer.
 *
 * @return always returns true.
 */
static void free_(qqueue_t *queue)
{
    queue->list->free(queue->list);
    free(queue);
}
