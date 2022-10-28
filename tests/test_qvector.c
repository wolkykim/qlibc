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

void test_thousands_of_values(int num_values, int options, char *prefix, char *postfix);

QUNIT_START("Test qvector.c");

TEST("Test basic features") {
    const int values[] = {0, 1, 2};

    qvector_t *vector = qvector(3, sizeof(int), 0);
    ASSERT_EQUAL_INT(0, vector->size(vector));

    void *data;
    bool result = vector->addfirst(vector, &values[0]);
    ASSERT_EQUAL_BOOL(result, true);
    ASSERT_EQUAL_INT(1, vector->size(vector));
    data = vector->getfirst(vector, false);
    ASSERT_EQUAL_INT(values[0], *((int *)data));

    vector->addlast(vector, values + 2);
    ASSERT_EQUAL_INT(2, vector->size(vector));
    data = vector->getfirst(vector, false);
    ASSERT_EQUAL_INT(values[0], *((int *)data));
    data = vector->getlast(vector, false);
    ASSERT_EQUAL_INT(values[2], *((int *)data));
    
    vector->addat(vector, 1, values + 1);
    ASSERT_EQUAL_INT(3, vector->size(vector));
    data = vector->getfirst(vector, false);
    ASSERT_EQUAL_INT(values[0], *((int *)data));
    data = vector->getat(vector, 1, false);
    ASSERT_EQUAL_INT(values[1], *((int *)data));
    data = vector->getlast(vector, false);
    ASSERT_EQUAL_INT(values[2], *((int *)data));
    
    data = vector->popat(vector, 1);
    ASSERT_EQUAL_INT(2, vector->size(vector));
    ASSERT_EQUAL_INT(values[1], *((int *)data));
    free(data);
    data = vector->popfirst(vector);
    ASSERT_EQUAL_INT(1, vector->size(vector));
    ASSERT_EQUAL_INT(values[0], *((int *)data));
    free(data);
    data = vector->poplast(vector);
    ASSERT_EQUAL_INT(0, vector->size(vector));
    ASSERT_EQUAL_INT(values[2], *((int *)data));
    free(data);
    vector->free(vector);
}

TEST("Test boundary conditions") {
    int values[] = {1000, 1001, 1002};
    
    /*test when vector is empty*/
    qvector_t *vector = qvector(1, sizeof(int), 0);
    bool result;
    void *data;
    qvector_obj_t obj;
    result = vector->addat(vector, 2, &values[0]);
    ASSERT_EQUAL_BOOL(result, false);
    data = vector->getfirst(vector, false);
    ASSERT_NULL(data);
    data = vector->getlast(vector, false);
    ASSERT_NULL(data);
    data = vector->getat(vector, 2, false);
    ASSERT_NULL(data);
    data = vector->popfirst(vector);
    ASSERT_NULL(data);
    data = vector->poplast(vector);
    ASSERT_NULL(data);
    data = vector->popat(vector, 2);
    ASSERT_NULL(data);
    result = vector->removefirst(vector);
    ASSERT_EQUAL_BOOL(result, false);
    result = vector->removelast(vector);
    ASSERT_EQUAL_BOOL(result, false);
    result = vector->removeat(vector, 2);
    ASSERT_EQUAL_BOOL(result, false);
    result = vector->getnext(vector, NULL, false);
    ASSERT_EQUAL_BOOL(result, false);
    memset((void *) &obj, 0, sizeof(obj));
    result = vector->getnext(vector, &obj, false);
    ASSERT_EQUAL_BOOL(result, false);
    vector->free(vector);

    /*test when vector contains 1 elements*/
    vector = qvector(1, sizeof(int), 0);
    vector->addfirst(vector, &values[0]);

    data = vector->getfirst(vector, false);
    ASSERT_EQUAL_INT(values[0], *((int*)data));
    data = vector->getlast(vector, false);
    ASSERT_EQUAL_INT(values[0], *((int*)data));
    data = vector->getat(vector, 2, false);
    ASSERT_NULL(data);
    vector->setat(vector, 0, &values[2]);
    data = vector->getfirst(vector, false);
    ASSERT_EQUAL_INT(values[2], *((int*)data));
    vector->setfirst(vector, &values[1]);
    data = vector->getfirst(vector, false);
    ASSERT_EQUAL_INT(values[1], *((int*)data));
    vector->setlast(vector, &values[2]);
    data = vector->getfirst(vector, false);
    ASSERT_EQUAL_INT(values[2], *((int*)data));
    data = vector->popfirst(vector);
    ASSERT_EQUAL_INT(values[2], *((int*)data));
    free(data);
    ASSERT_EQUAL_INT(0, vector->size(vector));

    vector->addfirst(vector, &values[0]);
    data = vector->popat(vector, 2);
    ASSERT_NULL(data);
    data = vector->popat(vector, 0);
    ASSERT_EQUAL_INT(values[0], *((int*)data));
    free(data);

    vector->addfirst(vector, &values[0]);
    result = vector->removefirst(vector);
    ASSERT_EQUAL_BOOL(result, true);
    ASSERT_EQUAL_INT(0, vector->size(vector));

    vector->addfirst(vector, &values[0]);
    result = vector->removeat(vector, 2);
    ASSERT_EQUAL_BOOL(result, false);
    result = vector->removeat(vector, 0);
    ASSERT_EQUAL_BOOL(result, true);
    ASSERT_EQUAL_INT(0, vector->size(vector));

    vector->addfirst(vector, &values[0]);
    vector->reverse(vector);
    data = vector->getfirst(vector, false);
    ASSERT_EQUAL_INT(values[0], *((int*)data));
    data = vector->popfirst(vector);
    ASSERT_EQUAL_INT(values[0], *((int*)data));
    free(data);
    ASSERT_EQUAL_INT(0, vector->size(vector));

    vector->addfirst(vector, &values[0]);
    data = vector->popat(vector, 2);
    ASSERT_NULL(data);
    data = vector->popat(vector, 0);
    ASSERT_EQUAL_INT(values[0], *((int*)data));
    free(data);

    vector->addfirst(vector, &values[0]);
    result = vector->removefirst(vector);
    ASSERT_EQUAL_BOOL(result, true);
    ASSERT_EQUAL_INT(0, vector->size(vector));

    vector->addfirst(vector, &values[0]);
    result = vector->removeat(vector, 2);
    ASSERT_EQUAL_BOOL(result, false);
    result = vector->removeat(vector, 0);
    ASSERT_EQUAL_BOOL(result, true);
    ASSERT_EQUAL_INT(0, vector->size(vector));

    /*test when add NULL element into vector*/
    vector = qvector(1, sizeof(int), 0);
    result = vector->addfirst(vector, 0);
    ASSERT_EQUAL_BOOL(result, false);
    vector->free(vector);
}

TEST("Test thousands of values: without prefix and postfix") {
    test_thousands_of_values(10000, 0, "", "");
}

TEST("Test thousands of values: with prefix and without postfix") {
    test_thousands_of_values(
            10000,
            QVECTOR_RESIZE_DOUBLE,
            "1a087a6982371bbfc9d4e14ae76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866",
            "");
}

TEST("Test thousands of values: without prefix and with postfix") {
    test_thousands_of_values(
            10000,
            QVECTOR_RESIZE_LINEAR,
            "",
            "1a087a6982371bbfc9d4e14ae76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866");
}

TEST("Test thousands of values: with prefix and postfix") {
    test_thousands_of_values(
            10000,
            QVECTOR_RESIZE_EXACT,
            "1a087a6982371bbfc9d4e14ae76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866",
            "1a087a6982371bbfc9d4e14ae    76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866");
}

TEST("Test resize") {
    qvector_t *vector = qvector(10, sizeof(int), 0);
    ASSERT_EQUAL_INT(0, vector->size(vector));

    int i;
    for (i=0; i < 5; i++) {
        vector->addlast(vector, &i);
    }
    ASSERT_EQUAL_INT(5, vector->size(vector));

    for (i=0; i < vector->size(vector); i++) {
        ASSERT_EQUAL_INT(i, *(int *)vector->getat(vector, i, false));;
    }

    vector->resize(vector, 3);
    ASSERT_EQUAL_INT(3, vector->size(vector));

    for (i=0; i < vector->size(vector); i++) {
        ASSERT_EQUAL_INT(i, *(int *)vector->getat(vector, i, false));;
    }

    vector->free(vector);
}

QUNIT_END();

void test_thousands_of_values(int num_values, int options, char *prefix, char *postfix) {
    struct test_obj {
        char *prefix;
        int value;
        char *postfix;
    };

    qvector_t *vector = qvector(0, sizeof(struct test_obj), options);
    ASSERT_EQUAL_INT(0, vector->size(vector));

    int i;
    struct test_obj obj_value;
    for (i = 0; i < num_values; i++) {
        obj_value.prefix = qstrdupf("%s", prefix);
        obj_value.postfix = qstrdupf("%s", postfix);
        obj_value.value = i;

        bool result = vector->addlast(vector, &obj_value);
        ASSERT_EQUAL_BOOL(result, true);
        ASSERT_EQUAL_INT(i + 1, vector->size(vector));
    }

    /* test iteration */
    qvector_obj_t obj;
    memset((void *) &obj, 0, sizeof(obj));
    i = 0;
    while (vector->getnext(vector, &obj, true)) {
        struct test_obj value;
        value.prefix = qstrdupf("%s", prefix);
        value.postfix = qstrdupf("%s", postfix);
        value.value = i;
        ASSERT_EQUAL_STR(((struct test_obj *)obj.data)->prefix, value.prefix);
        ASSERT_EQUAL_INT(((struct test_obj *)obj.data)->value, value.value);
        ASSERT_EQUAL_STR(((struct test_obj *)obj.data)->postfix, value.postfix);

        free(value.prefix);
        free(value.postfix);
        free(obj.data);
        i++;
    }

    /* test reverse() */
    vector->reverse(vector);
    i = num_values - 1;
    memset((void *) &obj, 0, sizeof(obj));
    while (vector->getnext(vector, &obj, false)) {
        struct test_obj value;
        value.prefix = qstrdupf("%s", prefix);
        value.postfix = qstrdupf("%s", postfix);
        value.value = i;

        ASSERT_EQUAL_STR(((struct test_obj *)obj.data)->prefix, value.prefix);
        ASSERT_EQUAL_INT(((struct test_obj *)obj.data)->value, value.value);
        ASSERT_EQUAL_STR(((struct test_obj *)obj.data)->postfix, value.postfix);

        free(value.prefix);
        free(value.postfix);

        i--;
    }

    /* test toarray() */
    vector->reverse(vector);
    void *to_array = vector->toarray(vector, NULL);
    i = 0;
    memset((void *) &obj, 0, sizeof(obj));
    while (vector->getnext(vector, &obj, false)) {
        struct test_obj value;
        value.prefix = qstrdupf("%s", prefix);
        value.postfix = qstrdupf("%s", postfix);
        value.value = i;

        ASSERT_EQUAL_STR(((struct test_obj *)obj.data)->prefix, value.prefix);
        ASSERT_EQUAL_INT(((struct test_obj *)obj.data)->value, value.value);
        ASSERT_EQUAL_STR(((struct test_obj *)obj.data)->postfix, value.postfix);

        free(value.prefix);
        free(value.postfix);
        free(((struct test_obj *)obj.data)->prefix);
        free(((struct test_obj *)obj.data)->postfix);

        i++;
    }

    free(to_array);

    vector->free(vector);
}
