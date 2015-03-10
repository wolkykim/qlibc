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

#include "qunit.h"
#include "qlibc.h"

QUNIT_START("Test qhashtbl.c");

#define KEY1      "testkey1"
#define KEY2      "testkey2"
#define VALUE_STR "value1"
TEST("Test basic but complete") {
    const char *KEYS[] = {"testkey0", "testkey1" };
    const char *VALUES[] = { "value0", "value1" };

    qhashtbl_t *tbl = qhashtbl(0, 0);
    ASSERT_EQUAL_INT(0, tbl->size(tbl));

    tbl->putstr(tbl, KEYS[0], VALUES[0]);
    ASSERT_EQUAL_INT(1, tbl->size(tbl));
    ASSERT_EQUAL_STR(VALUES[0], tbl->getstr(tbl, KEYS[0], false));

    tbl->putstr(tbl, KEYS[1], VALUES[1]);
    ASSERT_EQUAL_INT(2, tbl->size(tbl));
    ASSERT_EQUAL_STR(VALUES[1], tbl->getstr(tbl, KEYS[1], false));

    tbl->remove(tbl, KEYS[0]);
    ASSERT_EQUAL_INT(1, tbl->size(tbl));
    ASSERT_EQUAL_PT(NULL, tbl->getstr(tbl, KEYS[0], false));
    ASSERT_EQUAL_STR(VALUES[1], tbl->getstr(tbl, KEYS[1], false));

    tbl->clear(tbl);
    ASSERT_EQUAL_INT(0, tbl->size(tbl));

    tbl->free(tbl);
}

TEST("Test thousands of keys insertion and removal") {
    const int NUM_KEYS = 10000;

    qhashtbl_t *tbl = qhashtbl(0, 0);
    ASSERT_EQUAL_INT(0, tbl->size(tbl));

    int i;
    for (i = 0; i < NUM_KEYS; i++) {
        char *key = qstrdupf("key%d", i);
        char *value = qstrdupf("value%d", i);

        tbl->putstr(tbl, key, value);
        ASSERT_EQUAL_INT(i + 1, tbl->size(tbl));
        ASSERT_EQUAL_STR(value, tbl->getstr(tbl, key, false));

        free(key);
        free(value);
    }

    for (i = NUM_KEYS - 1; i >= 0; i--) {
        char *key = qstrdupf("key%d", i);

        tbl->remove(tbl, key);
        ASSERT_EQUAL_INT(i, tbl->size(tbl));
        ASSERT_EQUAL_PT(NULL, tbl->getstr(tbl, key, false));

        free(key);
    }

    ASSERT_EQUAL_INT(0, tbl->size(tbl));
    tbl->free(tbl);
}

QUNIT_END();
