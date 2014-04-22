#include "qunit.h"
#include "qlibc.h"

QUNIT_START("Test qstring.c");

TEST("qstrtrim()") {
    ASSERT_EQUAL_STR(qstrtrim(strdup("")), "");
    ASSERT_EQUAL_STR(qstrtrim(strdup(" ")), "");
    ASSERT_EQUAL_STR(qstrtrim(strdup("  ")), "");
    ASSERT_EQUAL_STR(qstrtrim(strdup("a")), "a");
    ASSERT_EQUAL_STR(qstrtrim(strdup("a ")), "a");
    ASSERT_EQUAL_STR(qstrtrim(strdup(" a")), "a");
    ASSERT_EQUAL_STR(qstrtrim(strdup(" a ")), "a");
    ASSERT_EQUAL_STR(qstrtrim(strdup("ab")), "ab");
    ASSERT_EQUAL_STR(qstrtrim(strdup("ab ")), "ab");
    ASSERT_EQUAL_STR(qstrtrim(strdup(" ab")), "ab");
    ASSERT_EQUAL_STR(qstrtrim(strdup(" ab ")), "ab");
    ASSERT_EQUAL_STR(qstrtrim(strdup("ab c")), "ab c");
    ASSERT_EQUAL_STR(qstrtrim(strdup("ab c ")), "ab c");
    ASSERT_EQUAL_STR(qstrtrim(strdup(" ab c")), "ab c");
    ASSERT_EQUAL_STR(qstrtrim(strdup(" ab c ")), "ab c");
}

TEST("qstrtrim_head()") {
    ASSERT_EQUAL_STR(qstrtrim_head(strdup("")), "");
    ASSERT_EQUAL_STR(qstrtrim_head(strdup(" ")), "");
    ASSERT_EQUAL_STR(qstrtrim_head(strdup("  ")), "");
    ASSERT_EQUAL_STR(qstrtrim_head(strdup("a")), "a");
    ASSERT_EQUAL_STR(qstrtrim_head(strdup("a ")), "a ");
    ASSERT_EQUAL_STR(qstrtrim_head(strdup(" a")), "a");
    ASSERT_EQUAL_STR(qstrtrim_head(strdup(" a ")), "a ");
}

TEST("qstrtrim_tail()") {
    ASSERT_EQUAL_STR(qstrtrim_tail(strdup("")), "");
    ASSERT_EQUAL_STR(qstrtrim_tail(strdup(" ")), "");
    ASSERT_EQUAL_STR(qstrtrim_tail(strdup("  ")), "");
    ASSERT_EQUAL_STR(qstrtrim_tail(strdup("a")), "a");
    ASSERT_EQUAL_STR(qstrtrim_tail(strdup("a ")), "a");
    ASSERT_EQUAL_STR(qstrtrim_tail(strdup(" a")), " a");
    ASSERT_EQUAL_STR(qstrtrim_tail(strdup(" a ")), " a");
}

QUNIT_END();
