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
 * Copyright (c) 2015 Anthony Tseng - https://github.com/darkdh
 *****************************************************************************/

#include "qunit.h"
#include "qlibc.h"
#include <errno.h>

QUNIT_START("Test qhasharr.c by darkdh");

qhasharr_t *tbl;
char *memory;

TEST("qhasharr()") {
    int memsize = qhasharr_calculate_memsize(5);
    memory = (char*) malloc(memsize * sizeof(char));
    tbl = qhasharr(memory, memsize);
    ASSERT_NOT_NULL(tbl);
    int maxslots, usedslots;
    tbl->size(tbl, &maxslots, &usedslots);
    ASSERT(maxslots > 0);
    ASSERT(usedslots == 0);
}

char *key[] = { "key1", "key2", "key3", "key4", "key5" };
char *value[] = { "data1", "data2", "data3", "data4", "data5" };
#define TARGET_NUM sizeof(key)/sizeof(char*)
int valuesize[TARGET_NUM];
int found[TARGET_NUM];
int i, j;
for (i = 0; i < TARGET_NUM; i++) {
    valuesize[i] = strlen(value[i]);
    found[i] = 0;
}
char *extra_key[] = { "extra1", "extra2", "extra3" };
char *extra_value[] = { "extra_data1", "extra_data2", "extra_data3" };
#define EXTRA_NUM sizeof(extra_key)/sizeof(char*)
int extra_valuesize[EXTRA_NUM];
for (i = 0; i < EXTRA_NUM; i++) {
    extra_valuesize[i] = strlen(extra_value[i]);
}

TEST("put() to full") {
    size_t size;
    for (i = 0; i < TARGET_NUM; i++) {
        ASSERT_TRUE(tbl->put(tbl, key[i], value[i], valuesize[i]));
        char *target_got = (char*) tbl->get(tbl, key[i], &size);
        ASSERT_NOT_NULL(target_got);
        ASSERT_EQUAL_MEM(target_got, value[i], valuesize[i]);
        free(target_got);
    }
    ASSERT_EQUAL_INT(tbl->size(tbl, NULL, NULL), TARGET_NUM);
    ASSERT_FALSE(
            tbl->put(tbl, extra_key[0], extra_value[0], extra_valuesize[0]));
    ASSERT_NULL(tbl->get(tbl, extra_key[0], &size));
    ASSERT_EQUAL_INT(tbl->size(tbl, NULL, NULL), TARGET_NUM);
}

TEST("get() non-exist key") {
    size_t size;
    char *target_got = (char*) tbl->get(tbl, extra_key[0], &size);
    ASSERT_NULL(target_got);
}

TEST("remove() to empty") {
    for (i = 0; i < TARGET_NUM; i++) {
        ASSERT_TRUE(tbl->remove(tbl, key[i]));
    }
    ASSERT_EQUAL_INT(tbl->size(tbl, NULL, NULL), 0);
    ASSERT_FALSE(tbl->remove(tbl, key[0]));
    ASSERT_EQUAL_INT(tbl->size(tbl, NULL, NULL), 0);
}

TEST("add() same key") {
    size_t size;
    ASSERT_TRUE(tbl->put(tbl, key[0], value[0], valuesize[0]));
    ASSERT_TRUE(tbl->put(tbl, key[0], value[1], valuesize[1]));
    char *target_got = (char*) tbl->get(tbl, key[0], &size);
    ASSERT_NOT_NULL(target_got);
    ASSERT_EQUAL_MEM(target_got, value[1], valuesize[1]);
    free(target_got);
    ASSERT_TRUE(tbl->remove(tbl, key[0]));
}

TEST("getnext()") {
    size_t size;
    for (i = 0; i < TARGET_NUM; i++) {
        ASSERT_TRUE(tbl->put(tbl, key[i], value[i], valuesize[i]));
        char *target_got = (char*) tbl->get(tbl, key[i], &size);
        ASSERT_NOT_NULL(target_got);
        ASSERT_EQUAL_MEM(target_got, value[i], valuesize[i]);
        free(target_got);
    }
    ASSERT_EQUAL_INT(tbl->size(tbl, NULL, NULL), TARGET_NUM);
    int idx = 0;
    qhasharr_obj_t obj;
    for (i = 0; i < TARGET_NUM; i++) {
        ASSERT_TRUE(tbl->getnext(tbl, &obj, &idx));
        for (j = 0; j < TARGET_NUM; j++) {
            if (!memcmp(obj.data, value[j], valuesize[j]) && found[j] == 0)
                found[j] = 1;
        }
        free(obj.name);
        free(obj.data);
    }
    ASSERT_FALSE(tbl->getnext(tbl, &obj, &idx));
    for (i = 0; i < TARGET_NUM; i++) {
        ASSERT_EQUAL_INT(found[i], 1);
    }
    ASSERT_EQUAL_INT(tbl->size(tbl, NULL, NULL), TARGET_NUM);
}

// free table reference object.
tbl->free(tbl);

free(memory);

QUNIT_END();
