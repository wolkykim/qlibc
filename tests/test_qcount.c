#include "qunit.h"
#include "qlibc.h"

QUNIT_START("Test qcount.c");

TEST("qcount_read()") {
    // Setup
    // Read fixture
    const char *filepath = "./fixtures/number.dat";
    int expected = 75;
    int result   = qcount_read(filepath);

    ASSERT_EQUAL_INT(result, expected);
}

/*
TEST("qstrtrim()") {
    ASSERT_EQUAL_STR(qstrtrim(strdup("")), "");
    ASSERT_EQUAL_STR(qstrtrim(strdup(" ab c")), "ab c");
    ASSERT_EQUAL_STR(qstrtrim(strdup(" ab c ")), "ab c");
}
*/

QUNIT_END();
