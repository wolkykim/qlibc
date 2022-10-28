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

QUNIT_START("Test qstack.c");

TEST("Test basic features") {
    const int data[] = {0, 1, 2, 3, 4, 5, 6};
    const char *string = "a test for string";
    int64_t num = 4234080023325;

    qstack_t *stack = qstack(0);
    ASSERT_EQUAL_INT(stack->size(stack), 0);
    
    bool result;
    result = stack->push(stack, (void *)data, sizeof(data));
    ASSERT_EQUAL_BOOL(true, result);
    ASSERT_EQUAL_INT(1, stack->size(stack));
    result = stack->pushstr(stack, string);   
    ASSERT_EQUAL_BOOL(true, result);
    ASSERT_EQUAL_INT(2, stack->size(stack));
    result = stack->pushint(stack, num);
    ASSERT_EQUAL_BOOL(true, result);
    ASSERT_EQUAL_INT(3, stack->size(stack));

    void *pop_data;
    int64_t pop_num = stack->popint(stack);
    ASSERT_EQUAL_INT(pop_num, num);
    void *pop_str = stack->popstr(stack);
    ASSERT_EQUAL_STR(pop_str, string);
    pop_data = stack->pop(stack, NULL);
    ASSERT_EQUAL_MEM(pop_data, data, sizeof(data));

    stack->clear(stack);
    stack->free(stack); 
}

TEST("Test boundary conditions") {
    
    const int array[] = {1, 2, 3, 4, 5, 6};
    const char *string = "ewqljljoaq;vsl23053054302ds;flajewjpeo2353rekffkl;sdk;f";

    /*test when queue is empty*/
    qstack_t *stack = qstack(0);
    ASSERT_EQUAL_INT(0, stack->size(stack));
    void *data;
    char *str;
    int64_t num;
    bool result;
    data = stack->pop(stack, NULL);
    ASSERT_NULL(data);
    str = stack->popstr(stack);
    ASSERT_NULL(str);
    num = stack->popint(stack);
    ASSERT_EQUAL_INT(0, num);
    data = stack->popat(stack, 0, NULL);
    ASSERT_NULL(data);
    data = stack->get(stack, NULL, false);
    ASSERT_NULL(data);
    str = stack->getstr(stack);
    ASSERT_NULL(str);
    num = stack->getint(stack);
    ASSERT_EQUAL_INT(0, num);
    data = stack->getat(stack, 0, NULL, false);
    ASSERT_NULL(data);
    stack->clear(stack);
    stack->free(stack);

    /*test when queue has only one element*/
    stack = qstack(0);

    result = stack->push(stack, (void *)array, sizeof(array));
    ASSERT_EQUAL_BOOL(result, true);
    data = stack->popat(stack, 2, NULL);
    ASSERT_NULL(data);
    data = stack->getat(stack, 2, NULL, false);
    ASSERT_NULL(data);
    data = stack->get(stack, NULL, false);
    ASSERT_NOT_NULL(data);
    ASSERT_EQUAL_MEM(array, data, sizeof(array));
    data = stack->popat(stack, 0, NULL);
    ASSERT_EQUAL_MEM(array, data, sizeof(array));

    result = stack->pushstr(stack, string);
    ASSERT_EQUAL_BOOL(result, true);
    data = stack->getat(stack, 2, NULL, false);
    ASSERT_NULL(data);
    data = stack->getat(stack, 0, NULL, false);
    ASSERT_EQUAL_STR(data, string);
    str = stack->getstr(stack);
    ASSERT_EQUAL_STR(str, string);
    data = stack->popat(stack, 1, NULL);
    ASSERT_NULL(data);
    str = stack->popstr(stack);
    ASSERT_EQUAL_STR(str, string);
    stack->clear(stack);
    stack->free(stack);

    /*test when max size is set*/
    stack = qstack(0);

    stack->setsize(stack, 2);

    result = stack->push(stack, array, sizeof(array));
    ASSERT_EQUAL_BOOL(result, true);
    result = stack->pushstr(stack, string);
    ASSERT_EQUAL_BOOL(result, true);
    result = stack->push(stack, array, sizeof(array));
    ASSERT_EQUAL_BOOL(result, false);
    ASSERT_EQUAL_INT(2, stack->size(stack));
    result = stack->pushstr(stack, string);
    ASSERT_EQUAL_BOOL(result, false);
    ASSERT_EQUAL_INT(2, stack->size(stack));
    stack->clear(stack);
    stack->free(stack);

    /*test when add NULL element into queue*/
    stack = qstack(0);
    result = stack->push(stack, NULL, 0);
    ASSERT_EQUAL_BOOL(result, false);
    ASSERT_EQUAL_INT(0, stack->size(stack));
    result = stack->pushstr(stack, NULL);
    ASSERT_EQUAL_BOOL(result, false);
    ASSERT_EQUAL_INT(0, stack->size(stack));
    stack->clear(stack);
    stack->free(stack);
}

TEST("Test thousands of values: without prefix and postfix") {
    test_thousands_of_values(10000, "", "");
}

TEST("Test thousands of values: with prefix and without postfix") {
    test_thousands_of_values(10000, "1a087a6982371bbfc9d4e14ae76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866", "");
}

TEST("Test thousands of values: without prefix and with postfix") {
    test_thousands_of_values(10000, "", "1a087a6982371bbfc9d4e14ae76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866");
}

TEST("Test thousands of values: with prefix and postfix") {
    test_thousands_of_values(10000, "1a087a6982371bbfc9d4e14ae76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866", "1a087a6982371bbfc9d4e14ae    76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866");
}

QUNIT_END();

void test_thousands_of_values(int num_values, char *prefix, char *postfix) {
    qstack_t *stack = qstack(0);
    ASSERT_EQUAL_INT(0, stack->size(stack));

    // Test strings
    int i;
    for(i = 0; i < num_values; i++) {
        char *value = qstrdupf("value:%s%d%s", prefix, i, postfix);

        stack->pushstr(stack, value);
        ASSERT_EQUAL_INT(i + 1, stack->size(stack));
        free(value);
    }

    i = num_values - 1;
    while(stack->size(stack) != 0) {
        char *value = qstrdupf("value:%s%d%s", prefix, i, postfix);

        char *data = stack->popstr(stack);
        ASSERT_EQUAL_STR(value, data);
        free(value);
        i--;
    }

    /*test arrays*/
    int array[8];
    for(i = 0; i < num_values; i++) {
        int j;
        for(j = 0; j < 8; j++)
        {
            array[j] = 8 * i + 1;
        }

        stack->push(stack, array, sizeof(array));
        ASSERT_EQUAL_INT(i + 1, stack->size(stack));
    }

    void *array_data;
    for(i = 0; i < num_values; i++) {
        int j;
        for(j = 0; j < 8; j++) {
            array[j] = 8 * (num_values - i - 1) + 1;
        }

        array_data = stack->pop(stack, NULL);
        ASSERT_EQUAL_MEM(array_data, array, sizeof(array));
    }

    /*test ints*/
    int64_t k;
    int64_t size;
    for(k = LONG_MIN, size = 0; size < num_values; k++, size++)
    {
        stack->pushint(stack, k);
        ASSERT_EQUAL_INT(size + 1, stack->size(stack));
    }

    for(k = LONG_MIN, size = 0; size < num_values; k++, size++)
    {
        int64_t value = LONG_MIN + num_values - size - 1;
        int64_t num = stack->popint(stack);
        ASSERT_EQUAL_INT(value, num);
    }

    stack->clear(stack);
    stack->free(stack);
}
