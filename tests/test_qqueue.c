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

#include "qunit.h"
#include "qlibc.h"
#include "limits.h"

void test_thousands_of_values(int num_values, char *prefix, char *postfix);

QUNIT_START("Test qqueue.c");

TEST("Test basic features") {
    const int data[] = { 0, 1, 2, 3, 4, 5, 6 };
    const char *string = "a test for string";
    int64_t num = 4234080023325;

    qqueue_t *queue = qqueue(0);
    ASSERT_EQUAL_INT(queue->size(queue), 0);
    
    bool result;
    result = queue->push(queue, (void*) data, sizeof(data));
    ASSERT_EQUAL_BOOL(true, result);
    ASSERT_EQUAL_INT(1, queue->size(queue));
    result = queue->pushstr(queue, string);
    ASSERT_EQUAL_BOOL(true, result);
    ASSERT_EQUAL_INT(2, queue->size(queue));
    result = queue->pushint(queue, num);
    ASSERT_EQUAL_BOOL(true, result);
    ASSERT_EQUAL_INT(3, queue->size(queue));

    void *pop_data;
    pop_data = queue->pop(queue, NULL);
    ASSERT_EQUAL_MEM(pop_data, data, sizeof(data));
    void *pop_str = queue->popstr(queue);
    ASSERT_EQUAL_STR(pop_str, string);
    int64_t pop_num = queue->popint(queue);
    ASSERT_EQUAL_INT(pop_num, num);

    queue->clear(queue);
    queue->free(queue);
}

TEST("Test boundary conditions") {

    const int array[] = { 1, 2, 3, 4, 5, 6 };
    const char *string =
            "ewqljljoaq;vsl23053054302ds;flajewjpeo2353rekffkl;sdk;f";
    
    /*test when queue is empty*/
    qqueue_t *queue = qqueue(0);
    ASSERT_EQUAL_INT(0, queue->size(queue));
    void *data;
    char *str;
    int64_t num;
    bool result;
    data = queue->pop(queue, NULL);
    ASSERT_NULL(data);
    str = queue->popstr(queue);
    ASSERT_NULL(str);
    num = queue->popint(queue);
    ASSERT_EQUAL_INT(0, num);
    data = queue->popat(queue, 0, NULL);
    ASSERT_NULL(data);
    data = queue->get(queue, NULL, false);
    ASSERT_NULL(data);
    str = queue->getstr(queue);
    ASSERT_NULL(str);
    num = queue->getint(queue);
    ASSERT_EQUAL_INT(0, num);
    data = queue->getat(queue, 0, NULL, false);
    ASSERT_NULL(data);
    queue->clear(queue);
    queue->free(queue);

    /*test when queue has only one element*/
    queue = qqueue(0);

    result = queue->push(queue, (void*) array, sizeof(array));
    ASSERT_EQUAL_BOOL(result, true);
    data = queue->popat(queue, 2, NULL);
    ASSERT_NULL(data);
    data = queue->getat(queue, 2, NULL, false);
    ASSERT_NULL(data);
    data = queue->get(queue, NULL, false);
    ASSERT_NOT_NULL(data);
    ASSERT_EQUAL_MEM(array, data, sizeof(array));
    data = queue->popat(queue, 0, NULL);
    ASSERT_EQUAL_MEM(array, data, sizeof(array));

    result = queue->pushstr(queue, string);
    ASSERT_EQUAL_BOOL(result, true);
    data = queue->getat(queue, 2, NULL, false);
    ASSERT_NULL(data);
    data = queue->getat(queue, 0, NULL, false);
    ASSERT_EQUAL_STR(data, string);
    str = queue->getstr(queue);
    ASSERT_EQUAL_STR(str, string);
    data = queue->popat(queue, 1, NULL);
    ASSERT_NULL(data);
    str = queue->popstr(queue);
    ASSERT_EQUAL_STR(str, string);
    queue->clear(queue);
    queue->free(queue);

    /*test when max size is set*/
    queue = qqueue(0);

    queue->setsize(queue, 2);

    result = queue->push(queue, array, sizeof(array));
    ASSERT_EQUAL_BOOL(result, true);
    result = queue->pushstr(queue, string);
    ASSERT_EQUAL_BOOL(result, true);
    result = queue->push(queue, array, sizeof(array));
    ASSERT_EQUAL_BOOL(result, false);
    ASSERT_EQUAL_INT(2, queue->size(queue));
    result = queue->pushstr(queue, string);
    ASSERT_EQUAL_BOOL(result, false);
    ASSERT_EQUAL_INT(2, queue->size(queue));
    queue->clear(queue);
    queue->free(queue);

    /*test when add NULL element into queue*/
    queue = qqueue(0);
    result = queue->push(queue, NULL, 0);
    ASSERT_EQUAL_BOOL(result, false);
    ASSERT_EQUAL_INT(0, queue->size(queue));
    result = queue->pushstr(queue, NULL);
    ASSERT_EQUAL_BOOL(result, false);
    ASSERT_EQUAL_INT(0, queue->size(queue));
    queue->clear(queue);
    queue->free(queue);
}

TEST("Test thousands of values: without prefix and postfix") {
    test_thousands_of_values(10000, "", "");
}

TEST("Test thousands of values: with prefix and without postfix") {
    test_thousands_of_values(
            10000,
            "1a087a6982371bbfc9d4e14ae76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866",
            "");
}

TEST("Test thousands of values: without prefix and with postfix") {
    test_thousands_of_values(
            10000,
            "",
            "1a087a6982371bbfc9d4e14ae76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866");
}

TEST("Test thousands of values: with prefix and postfix") {
    test_thousands_of_values(
            10000,
            "1a087a6982371bbfc9d4e14ae76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866",
            "1a087a6982371bbfc9d4e14ae    76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866");
}

QUNIT_END();

void test_thousands_of_values(int num_values, char *prefix, char *postfix) {
    qqueue_t *queue = qqueue(0);
    ASSERT_EQUAL_INT(0, queue->size(queue));

    /* test strings */
    int i;
    for (i = 0; i < num_values; i++) {
        char *value = qstrdupf("value:%s%d%s", prefix, i, postfix);

        queue->pushstr(queue, value);
        ASSERT_EQUAL_INT(i + 1, queue->size(queue));
        free(value);
    }

    i = 0;
    while (queue->size(queue) != 0) {
        char *value = qstrdupf("value:%s%d%s", prefix, i, postfix);

        char *data = queue->popstr(queue);
        ASSERT_EQUAL_STR(value, data);
        free(value);
        i++;
    }

    /* test arrays */
    int array[8];
    for (i = 0; i < num_values; i++) {
        int j;
        for (j = 0; j < 8; j++) {
            array[j] = 8 * i + 1;
        }

        queue->push(queue, array, sizeof(array));
        ASSERT_EQUAL_INT(i + 1, queue->size(queue));
    }

    void *array_data;
    for (i = 0; i < num_values; i++) {
        int j;
        for (j = 0; j < 8; j++) {
            array[j] = 8 * i + 1;
        }

        array_data = queue->pop(queue, NULL);
        ASSERT_EQUAL_MEM(array_data, array, sizeof(array));
    }

    /* test ints */
    int64_t k;
    int64_t size;
    for (k = LONG_MIN, size = 0; size < num_values; k++, size++) {
        queue->pushint(queue, k);
        ASSERT_EQUAL_INT(size + 1, queue->size(queue));
    }

    for (k = LONG_MIN, size = 0; size < num_values; k++, size++) {
        int64_t num = queue->popint(queue);
        ASSERT_EQUAL_INT(k, num);
    }

    queue->clear(queue);
    queue->free(queue);
}
