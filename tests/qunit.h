/******************************************************************************
 * qunit
 *
 * Copyright (c) 2014 Seungyoung Kim.
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
 * qunit C Unit Test framework.
 *
 * @file qunit.h
 */

#ifndef _QUNIT_H
#define _QUNIT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OUTSTREAM    (stdout)
#define PRINT(fmt, args...) do {                                            \
        fprintf(OUTSTREAM, "" fmt, ##args);                                 \
    } while(0)
#define PRINTLN(fmt, args...) do {                                          \
        fprintf(OUTSTREAM, "" fmt "\n", ##args);                            \
    } while(0)

#define QUNIT_START(title)                                                  \
char *_q_title = title;                                                     \
int _q_tot_tests = 0;                                                       \
int _q_tot_failed = 0;                                                      \
int _q_this_failed = 0;                                                     \
int _q_errcnt = 0;                                                          \
int main(int argc, char **argv) {                                           \
    PRINTLN("%s", _q_title);                                                \
    PRINTLN("======================================================================");  \

#define QUNIT_END()                                                         \
    _TEST_RESULT();                                                         \
    PRINTLN("======================================================================");  \
    PRINTLN("%s - %d/%d tests passed.",                                     \
        ((_q_tot_failed == 0) ? "PASS" : "FAIL"),                           \
        (_q_tot_tests - _q_tot_failed), _q_tot_tests);                      \
    return _q_tot_failed;                                                   \
}

#define TEST(name)                                                          \
    _TEST_RESULT();                                                         \
    _q_tot_tests++;                                                         \
    PRINT("* TEST : %s ", name);

#define _TEST_RESULT()                                                      \
    if (_q_tot_tests ) PRINTLN(" %s", (_q_this_failed) ? "FAIL" : "OK");    \
    _q_tot_failed += (_q_this_failed) ? 1 : 0;                              \
    _q_this_failed = 0;

#define ASSERT(expr)                                                        \
    if (! (expr))  {                                                        \
        _q_this_failed++;                                                   \
        PRINTLN("\nAssertion '%s' failed (%s:%d)", #expr, __FILE__, __LINE__); \
    } else {                                                                \
        PRINT(".");                                                         \
    }

#define ASSERT_EQUAL_STR(s1, s2) ASSERT(!strcmp(s1, s2))
#define ASSERT_EQUAL_INT(d1, d2) ASSERT(d1 == d2)

#ifdef __cplusplus
}
#endif

#endif /*_QUNIT_H */
