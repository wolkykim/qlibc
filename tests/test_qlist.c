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

void test_thousands_of_values(int num_values, char *prefix, char *postfix);

QUNIT_START("Test qlist.c");

TEST("Test basic features") {
    const char *values[] =
            { "value1",
                    "value2_long_value-fef6bd00f77aef990a6d62969fee0cb904d052665a1dcf10492156124fafc59769e91d1a06ec1215e435e29ef43de177f6f2a5e035860e702c82e08084950313",
                    "value3_long_value-fef6bd00f77aef990a6d62969fee0cb904d052665a1dcf10492156124fafc59769e91d1a06ec1215e435e29ef43de177f6f2a5e035860e702c82e08084950313", };
    qlist_t *list = qlist(0);
    ASSERT_EQUAL_INT(0, list->size(list));

    void *data;
    list->addfirst(list, values[0], strlen(values[0]) + 1);
    ASSERT_EQUAL_INT(1, list->size(list));
    data = list->getfirst(list, NULL, false);
    ASSERT_EQUAL_STR(values[0], (char*)data);

    list->addlast(list, values[2], strlen(values[2]) + 1);
    ASSERT_EQUAL_INT(2, list->size(list));
    data = list->getfirst(list, NULL, false);
    ASSERT_EQUAL_STR(values[0], (char*)data);
    data = list->getlast(list, NULL, false);
    ASSERT_EQUAL_STR(values[2], (char*)data);
    
    list->addat(list, 1, values[1], strlen(values[1]) + 1);
    ASSERT_EQUAL_INT(3, list->size(list));
    data = list->getfirst(list, NULL, false);
    ASSERT_EQUAL_STR(values[0], (char*)data);
    data = list->getat(list, 1, NULL, 0);
    ASSERT_EQUAL_STR(values[1], (char*)data);
    data = list->getlast(list, NULL, false);
    ASSERT_EQUAL_STR(values[2], (char*)data);
    
    data = list->popat(list, 1, NULL);
    ASSERT_EQUAL_INT(2, list->size(list));
    ASSERT_EQUAL_STR(values[1], (char*)data);
    free(data);
    data = list->popfirst(list, NULL);
    ASSERT_EQUAL_INT(1, list->size(list));
    ASSERT_EQUAL_STR(values[0], (char*)data);
    free(data);
    data = list->poplast(list, NULL);
    ASSERT_EQUAL_INT(0, list->size(list));
    ASSERT_EQUAL_STR(values[2], (char*)data);
    free(data);
}

TEST("Test boundary conditions") {
    const char *values[] = { "value0" };
    
    /*test when list is empty*/
    qlist_t *list = qlist(0);
    bool result;
    void *data;
    qlist_obj_t obj;
    result = list->addat(list, 2, values[0], strlen(values[0]) + 1);
    ASSERT_EQUAL_BOOL(result, false);
    data = list->getfirst(list, NULL, false);
    ASSERT_NULL(data);
    data = list->getlast(list, NULL, false);
    ASSERT_NULL(data);
    data = list->getat(list, 2, NULL, false);
    ASSERT_NULL(data);
    data = list->popfirst(list, NULL);
    ASSERT_NULL(data);
    data = list->poplast(list, NULL);
    ASSERT_NULL(data);
    data = list->popat(list, 2, NULL);
    ASSERT_NULL(data);
    result = list->removefirst(list);
    ASSERT_EQUAL_BOOL(result, false);
    result = list->removelast(list);
    ASSERT_EQUAL_BOOL(result, false);
    result = list->removeat(list, 2);
    ASSERT_EQUAL_BOOL(result, false);
    result = list->getnext(list, NULL, false);
    ASSERT_EQUAL_BOOL(result, false);
    memset((void *) &obj, 0, sizeof(obj));
    result = list->getnext(list, &obj, false);
    ASSERT_EQUAL_BOOL(result, false);
    list->free(list);

    /*test when list contains 1 elements*/
    list = qlist(0);
    list->addfirst(list, values[0], strlen(values[0]) + 1);

    data = list->getfirst(list, NULL, false);
    ASSERT_EQUAL_STR(values[0], (char*)data);
    data = list->getlast(list, NULL, false);
    ASSERT_EQUAL_STR(values[0], (char*)data);
    data = list->getat(list, 2, NULL, false);
    ASSERT_NULL(data);

    data = list->popfirst(list, NULL);
    ASSERT_EQUAL_STR(values[0], (char*)data);
    free(data);
    ASSERT_EQUAL_INT(0, list->size(list));

    list->addfirst(list, values[0], strlen(values[0]) + 1);
    data = list->popat(list, 2, NULL);
    ASSERT_NULL(data);
    data = list->popat(list, 0, NULL);
    ASSERT_EQUAL_STR(values[0], (char*)data);
    free(data);

    list->addfirst(list, values[0], strlen(values[0]) + 1);
    result = list->removefirst(list);
    ASSERT_EQUAL_BOOL(result, true);
    ASSERT_EQUAL_INT(0, list->size(list));

    list->addfirst(list, values[0], strlen(values[0]) + 1);
    result = list->removeat(list, 2);
    ASSERT_EQUAL_BOOL(result, false);
    result = list->removeat(list, 0);
    ASSERT_EQUAL_BOOL(result, true);
    ASSERT_EQUAL_INT(0, list->size(list));

    list->addfirst(list, values[0], strlen(values[0]) + 1);
    list->reverse(list);
    data = list->getfirst(list, NULL, false);
    ASSERT_EQUAL_STR(values[0], (char*)data);
    list->clear(list);
    ASSERT_EQUAL_INT(0, list->size(list));
    list->free(list);

    /*test while max size of list is set*/
    list = qlist(0);
    list->setsize(list, 2);
    result = list->addfirst(list, values[0], strlen(values[0]) + 1);
    ASSERT_EQUAL_BOOL(result, true);
    result = list->addfirst(list, values[0], strlen(values[0]) + 1);
    ASSERT_EQUAL_BOOL(result, true);
    result = list->addfirst(list, values[0], strlen(values[0]) + 1);
    ASSERT_EQUAL_BOOL(result, false);
    ASSERT_EQUAL_INT(2, list->size(list));
    list->clear(list);
    list->free(list);

    /*test when add NULL element into list*/
    list = qlist(0);
    result = list->addfirst(list, NULL, 0);
    ASSERT_EQUAL_BOOL(result, false);
    list->free(list);
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
    qlist_t *list = qlist(0);

    ASSERT_EQUAL_INT(0, list->size(list));

    int i;
    unsigned long long datasize = 0;
    unsigned long long strsize = 0;
    for (i = 0; i < num_values; i++) {
        char *value = qstrdupf("value%s%d%s", prefix, i, postfix);
        int dsize = strlen(value) + 1;
        int ssize = strlen(value);

        list->addlast(list, value, dsize);
        ASSERT_EQUAL_INT(i + 1, list->size(list));

        datasize += dsize;
        strsize += ssize;
        free(value);
    }
    ASSERT_EQUAL_INT(datasize, list->datasize(list));

    /*test iteration*/
    qlist_obj_t obj;
    memset((void *) &obj, 0, sizeof(obj));
    i = 0;
    while (list->getnext(list, &obj, true)) {
        char *value = qstrdupf("value%s%d%s", prefix, i, postfix);

        ASSERT_EQUAL_STR(obj.data, value);
        free(value);
        free(obj.data);
        i++;
    }

    /* test reverse() */
    list->reverse(list);
    i = num_values - 1;
    memset((void *) &obj, 0, sizeof(obj));
    while (list->getnext(list, &obj, false)) {
        char *value = qstrdupf("value%s%d%s", prefix, i, postfix);

        ASSERT_EQUAL_STR(obj.data, value);
        free(value);
        i--;
    }

    /* test toarray() and tostring() */
    list->reverse(list);
    void *all_data = malloc(datasize);
    char *all_str = malloc(strsize + 1);
    void *dstart = all_data;
    void *sstart = all_str;
    strcpy(sstart, "\0");

    i = 0;
    memset((void *) &obj, 0, sizeof(obj));
    while (list->getnext(list, &obj, false)) {
        char *value = qstrdupf("value%s%d%s", prefix, i, postfix);
        int dsize = strlen(value) + 1;

        memcpy(dstart, value, dsize);
        strcat(sstart, value);

        dstart += dsize;
        free(value);
        i++;
    }

    void *to_array = list->toarray(list, NULL);
    char *to_string = list->tostring(list);

    ASSERT_EQUAL_MEM(all_data, to_array, datasize);
    ASSERT_EQUAL_STR(all_str, to_string);

    free(all_data);
    free(all_str);
    free(to_array);
    free(to_string);

    list->clear(list);
    list->free(list);
}
