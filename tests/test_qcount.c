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

QUNIT_END();
