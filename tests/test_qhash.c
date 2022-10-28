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

#include "qunit.h"
#include "qlibc.h"

QUNIT_START("Test qhash.c");

TEST("qhashmd5_file(test_qhash_data_1.bin, 0, 0)") {
    unsigned char digest[16];
    bool success = qhashmd5_file("test_qhash_data_1.bin", 0, 0, digest);
    ASSERT_EQUAL_BOOL(true, success);
    char *hash = qhex_encode(digest, 16);
    ASSERT_NOT_NULL(hash);
    ASSERT_EQUAL_STR(hash, "76658de2ac7d406f93dfbe8bb6d9f549");
    free(hash);
}

TEST("qhashmd5_file(test_qhash_data_1.bin, 0, 1)") {
    unsigned char digest[16];
    bool success = qhashmd5_file("test_qhash_data_1.bin", 0, 1, digest);
    ASSERT_EQUAL_BOOL(true, success);
    char *hash = qhex_encode(digest, 16);
    ASSERT_NOT_NULL(hash);
    ASSERT_EQUAL_STR(hash, "0cc175b9c0f1b6a831c399e269772661");
    free(hash);
}

TEST("qhashmd5_file(test_qhash_data_1.bin, 0, 2)") {
    unsigned char digest[16];
    bool success = qhashmd5_file("test_qhash_data_1.bin", 0, 2, digest);
    ASSERT_EQUAL_BOOL(true, success);
    char *hash = qhex_encode(digest, 16);
    ASSERT_NOT_NULL(hash);
    ASSERT_EQUAL_STR(hash, "187ef4436122d1cc2f40dc2b92f0eba0");
    free(hash);
}

TEST("qhashmd5_file(test_qhash_data_1.bin, 0, 3)") {
    unsigned char digest[16];
    bool success = qhashmd5_file("test_qhash_data_1.bin", 0, 3, digest);
    ASSERT_EQUAL_BOOL(true, success);
    char *hash = qhex_encode(digest, 16);
    ASSERT_NOT_NULL(hash);
    ASSERT_EQUAL_STR(hash, "900150983cd24fb0d6963f7d28e17f72");
    free(hash);
}

TEST("qhashmd5_file(test_qhash_data_1.bin, 1, 2)") {
    unsigned char digest[16];
    bool success = qhashmd5_file("test_qhash_data_1.bin", 1, 2, digest);
    ASSERT_EQUAL_BOOL(true, success);
    char *hash = qhex_encode(digest, 16);
    ASSERT_NOT_NULL(hash);
    ASSERT_EQUAL_STR(hash, "5360af35bde9ebd8f01f492dc059593c");
    free(hash);
}

TEST("qhashmd5_file(test_qhash_data_1.bin, 2, 3)") {
    unsigned char digest[16];
    bool success = qhashmd5_file("test_qhash_data_1.bin", 2, 3, digest);
    ASSERT_EQUAL_BOOL(true, success);
    char *hash = qhex_encode(digest, 16);
    ASSERT_NOT_NULL(hash);
    ASSERT_EQUAL_STR(hash, "a256e6b336afdc38c564789c399b516c");
    free(hash);
}

TEST("qhashmd5_file(test_qhash_data_2.bin, 0, 0)") {
    unsigned char digest[16];
    bool success = qhashmd5_file("test_qhash_data_2.bin", 0, 0, digest);
    ASSERT_EQUAL_BOOL(true, success);
    char *hash = qhex_encode(digest, 16);
    ASSERT_NOT_NULL(hash);
    ASSERT_EQUAL_MEM(hash, "8d03ad1bae270828874995868fa74476", 16);
    free(hash);
}

/* test_qhash_data_3.bin is (32*1024) bytes to test for loop edge case */
TEST("qhashmd5_file(test_qhash_data_3.bin, 0, 0)") {
    unsigned char digest[16];
    bool success = qhashmd5_file("test_qhash_data_3.bin", 0, 0, digest);
    ASSERT_EQUAL_BOOL(true, success);
    char *hash = qhex_encode(digest, 16);
    ASSERT_NOT_NULL(hash);
    ASSERT_EQUAL_STR(hash, "df6b8a02c62e7928407e29778acf8b71");
    free(hash);
}

/* test_qhash_data_4.bin is (32*1024)+1 bytes to test for loop edge case */
TEST("qhashmd5_file(test_qhash_data_4.bin, 0, 0)") {
    unsigned char digest[16];
    bool success = qhashmd5_file("test_qhash_data_4.bin", 0, 0, digest);
    ASSERT_EQUAL_BOOL(true, success);
    char *hash = qhex_encode(digest, 16);
    ASSERT_NOT_NULL(hash);
    ASSERT_EQUAL_STR(hash, "0c4f3eb4f6f56fbc958d7c1572ffbbbf");
    free(hash);
}

QUNIT_END();
