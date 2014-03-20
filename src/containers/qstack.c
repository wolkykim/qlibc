/******************************************************************************
 * qLibc - http://www.qdecoder.org
 *
 * Copyright (c) 2010-2012 Seungyoung Kim.
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
 ******************************************************************************
 * $Id: qstack.c 81 2012-04-12 01:01:34Z seungyoung.kim $
 ******************************************************************************/

/**
 * @file qstack.c Stack implementation.
 *
 * qstack container is a stack implementation. It represents a
 * last-in-first-out(LIFO). It extends qlist container that allow a linked-list
 * to be treated as a stack.
 *
 * @code
 *  [Conceptional Data Structure Diagram]
 *
 *                       top     bottom
 *  DATA PUSH/POP <==> [ C ][ B ][ A ]
 *  (positive index)     0    1    2
 *  (negative index)    -3   -2   -1
 * @endcode
 *
 * @code
 *  // create stack
 *  qstack_t *stack = qstack();
 *
 *  // example: integer stack
 *  stack->pushint(stack, 1);
 *  stack->pushint(stack, 2);
 *  stack->pushint(stack, 3);
 *
 *  printf("popint(): %d\n", stack->popint(stack));
 *  printf("popint(): %d\n", stack->popint(stack));
 *  printf("popint(): %d\n", stack->popint(stack));
 *
 *  // example: string stack
 *  stack->pushstr(stack, "A string");
 *  stack->pushstr(stack, "B string");
 *  stack->pushstr(stack, "C string");
 *
 *  char *str = stack->popstr(stack);
 *  printf("popstr(): %s\n", str);
 *  free(str);
 *  str = stack->popstr(stack);
 *  printf("popstr(): %s\n", str);
 *  free(str);
 *  str = stack->popstr(stack);
 *  printf("popstr(): %s\n", str);
 *  free(str);
 *
 *  // example: object stack
 *  stack->push(stack, "A object", sizeof("A object"));
 *  stack->push(stack, "B object", sizeof("B object"));
 *  stack->push(stack, "C object", sizeof("C object"));
 *
 *  void *obj = stack->pop(stack, NULL);
 *  printf("pop(): %s\n", (char*)obj);
 *  free(obj);
 *  str = stack->pop(stack, NULL);
 *  printf("pop(): %s\n", (char*)obj);
 *  free(obj);
 *  str = stack->pop(stack, NULL);
 *  printf("pop(): %s\n", (char*)obj);
 *  free(obj);
 *
 *  // release
 *  stack->free(stack);
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
 *
 * @note
 *  Use "--enable-threadsafe" configure script option to use under
 *  multi-threaded environments.
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

static size_t setsize(qstack_t *stack, size_t max);

static bool push(qstack_t *stack, const void *data, size_t size);
static bool pushstr(qstack_t *stack, const char *str);
static bool pushint(qstack_t *stack, int64_t num);

static void *pop(qstack_t *stack, size_t *size);
static char *popstr(qstack_t *stack);
static int64_t popint(qstack_t *stack);
static void *popat(qstack_t *stack, int index, size_t *size);

static void *get(qstack_t *stack, size_t *size, bool newmem);
static char *getstr(qstack_t *stack);
static int64_t getint(qstack_t *stack);
static void *getat(qstack_t *stack, int index, size_t *size, bool newmem);

static size_t size(qstack_t *stack);
static void clear(qstack_t *stack);
static bool debug(qstack_t *stack, FILE *out);
static void free_(qstack_t *stack);

#endif

/**
 * Create a new stack container
 *
 * @return a pointer of malloced qstack_t container, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @code
 *   qstack_t *stack = qstack();
 * @endcode
 */
qstack_t *qstack(void)
{
    qstack_t *stack = (qstack_t *)malloc(sizeof(qstack_t));
    if (stack == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    memset((void *)stack, 0, sizeof(qstack_t));
    stack->list = qlist();
    if (stack->list == NULL) {
        free(stack);
        return NULL;
    }

    // methods
    stack->setsize      = setsize;

    stack->push         = push;
    stack->pushstr      = pushstr;
    stack->pushint      = pushint;

    stack->pop          = pop;
    stack->popstr       = popstr;
    stack->popint       = popint;
    stack->popat        = popat;

    stack->get          = get;
    stack->getstr       = getstr;
    stack->getint       = getint;
    stack->getat        = getat;

    stack->size         = size;
    stack->clear        = clear;
    stack->debug        = debug;
    stack->free         = free_;

    return stack;
}

/**
 * qstack->setsize(): Sets maximum number of elements allowed in this
 * stack.
 *
 * @param stack qstack container pointer.
 * @param max   maximum number of elements. 0 means no limit.
 *
 * @return previous maximum number.
 */
static size_t setsize(qstack_t *stack, size_t max)
{
    return stack->list->setsize(stack->list, max);
}

/**
 * qstack->push(): Pushes an element onto the top of this stack.
 *
 * @param stack qstack container pointer.
 * @param data  a pointer which points data memory.
 * @param size  size of the data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EINVAL    : Invalid argument.
 *  - ENOBUFS   : Stack full. Only happens when this stack has set to have
 *                limited number of elements)
 *  - ENOMEM    : Memory allocation failure.
 */
static bool push(qstack_t *stack, const void *data, size_t size)
{
    return stack->list->addfirst(stack->list, data, size);
}

/**
 * qstack->pushstr(): Pushes a string onto the top of this stack.
 *
 * @param stack qstack container pointer.
 * @param data  a pointer which points data memory.
 * @param size  size of the data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - EINVAL    : Invalid argument.
 *  - ENOBUFS   : Stack full. Only happens when this stack has set to have
 *                limited number of elements.
 *  - ENOMEM    : Memory allocation failure.
 */
static bool pushstr(qstack_t *stack, const char *str)
{
    if (str == NULL) {
        errno = EINVAL;
        return false;
    }
    return stack->list->addfirst(stack->list, str, strlen(str) + 1);
}

/**
 * qstack->pushint(): Pushes a integer onto the top of this stack.
 *
 * @param stack qstack container pointer.
 * @param num   integer data.
 *
 * @return true if successful, otherwise returns false.
 * @retval errno will be set in error condition.
 *  - ENOBUFS   : Stack full. Only happens when this stack has set to have
 *                limited number of elements.
 *  - ENOMEM    : Memory allocation failure.
 */
static bool pushint(qstack_t *stack, int64_t num)
{
    return stack->list->addfirst(stack->list, &num, sizeof(int));
}

/**
 * qstack->pop(): Removes a element at the top of this stack and returns
 * that element.
 *
 * @param stack qstack container pointer.
 * @param size  if size is not NULL, element size will be stored.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Stack is empty.
 *  - ENOMEM    : Memory allocation failure.
 */
static void *pop(qstack_t *stack, size_t *size)
{
    return stack->list->popfirst(stack->list, size);
}

/**
 * qstack->popstr(): Removes a element at the top of this stack and
 * returns that element.
 *
 * @param stack qstack container pointer.
 *
 * @return a pointer of malloced string element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Stack is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * The string element should be pushed through pushstr().
 */
static char *popstr(qstack_t *stack)
{
    size_t strsize;
    char *str = stack->list->popfirst(stack->list, &strsize);
    if (str != NULL) {
        str[strsize - 1] = '\0'; // just to make sure
    }

    return str;
}

/**
 * qstack->popint(): Removes a integer at the top of this stack and
 * returns that element.
 *
 * @param stack qstack container pointer.
 *
 * @return an integer value, otherwise returns 0.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Stack is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * The integer element should be pushed through pushint().
 */
static int64_t popint(qstack_t *stack)
{
    int64_t num = 0;
    int64_t *pnum = stack->list->popfirst(stack->list, NULL);
    if (pnum != NULL) {
        num = *pnum;
        free(pnum);
    }

    return num;
}

/**
 * qstack->popat(): Returns and remove the element at the specified
 * position in this stack.
 *
 * @param stack qstack container pointer.
 * @param index index at which the specified element is to be inserted
 * @param size  if size is not NULL, element size will be stored.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ERANGE    : Index out of range.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 *  Negative index can be used for addressing a element from the bottom in
 *  this stack. For example, index -1 will always pop a element which is pushed
 *  at very first time.
 */
static void *popat(qstack_t *stack, int index, size_t *size)
{
    return stack->list->popat(stack->list, index, size);
}

/**
 * qstack->get(): Returns an element at the top of this stack without
 * removing it.
 *
 * @param stack     qstack container pointer.
 * @param size      if size is not NULL, element size will be stored.
 * @param newmem    whether or not to allocate memory for the element.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Stack is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @return a pointer of malloced element, otherwise returns NULL.
 */
static void *get(qstack_t *stack, size_t *size, bool newmem)
{
    return stack->list->getfirst(stack->list, size, newmem);
}

/**
 * qstack->getstr(): Returns an string at the top of this stack without
 * removing it.
 *
 * @param stack qstack container pointer.
 *
 * @return a pointer of malloced string element, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Stack is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * The string element should be pushed through pushstr().
 */
static char *getstr(qstack_t *stack)
{
    size_t strsize;
    char *str = stack->list->getfirst(stack->list, &strsize, true);
    if (str != NULL) {
        str[strsize - 1] = '\0'; // just to make sure
    }

    return str;
}

/**
 * qstack->getint(): Returns an integer at the top of this stack without
 * removing it.
 *
 * @param stack qstack container pointer.
 *
 * @return an integer value, otherwise returns 0.
 * @retval errno will be set in error condition.
 *  - ENOENT    : Stack is empty.
 *  - ENOMEM    : Memory allocation failure.
 *
 * @note
 * The integer element should be pushed through pushint().
 */
static int64_t getint(qstack_t *stack)
{
    int64_t num = 0;
    int64_t *pnum = stack->list->getfirst(stack->list, NULL, true);
    if (pnum != NULL) {
        num = *pnum;
        free(pnum);
    }

    return num;
}

/**
 * qstack->getat(): Returns an element at the specified position in this
 * stack without removing it.
 *
 * @param stack     qstack container pointer.
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
 * Negative index can be used for addressing a element from the bottom in this
 * stack. For example, index -1 will always get a element which is pushed at
 * very first time.
 */
static void *getat(qstack_t *stack, int index, size_t *size, bool newmem)
{
    return stack->list->getat(stack->list, index, size, newmem);
}

/**
 * qstack->size(): Returns the number of elements in this stack.
 *
 * @param stack qstack container pointer.
 *
 * @return the number of elements in this stack.
 */
static size_t size(qstack_t *stack)
{
    return stack->list->size(stack->list);
}

/**
 * qstack->clear(): Removes all of the elements from this stack.
 *
 * @param stack qstack container pointer.
 */
static void clear(qstack_t *stack)
{
    stack->list->clear(stack->list);
}

/**
 * qstack->debug(): Print out stored elements for debugging purpose.
 *
 * @param stack     qstack container pointer.
 * @param out       output stream FILE descriptor such like stdout, stderr.
 *
 * @return true if successful, otherwise returns false.
 */
static bool debug(qstack_t *stack, FILE *out)
{
    return stack->list->debug(stack->list, out);
}

/**
 * qstack->free(): Free qstack_t
 *
 * @param stack qstack container pointer.
 *
 * @return always returns true.
 */
static void free_(qstack_t *stack)
{
    stack->list->free(stack->list);
    free(stack);
}
