qLibc Unit Tests
================

## How to Run Unit Tests.
If you have already run `cmake` and `make`, you will only need to run `ctest`
```
$ ctest --output-on-failure
Test project /my-projects/qlibc-build
    Start 1: test_qcount
1/2 Test #1: test_qcount ......................   Passed    0.00 sec
    Start 2: test_qstring
2/2 Test #2: test_qstring .....................   Passed    0.00 sec
```
When a test fails, using the `--output-on-failure` option, you will see the
specifics of the test that failed:
```
Test project /my-projects/qlibc-build
    Start 1: test_qcount
1/2 Test #1: test_qcount ......................***Failed    0.00 sec
Test qcount.c
======================================================================
* TEST : qcount_read() 
Assertion 'result == expected' failed (/my-projects/qlibc/tests/test_qcount.c:13)
 FAIL
======================================================================
FAIL - 0/1 tests passed.

    Start 2: test_qstring
2/2 Test #2: test_qstring .....................   Passed    0.00 sec

50% tests passed, 1 tests failed out of 2

Total Test time (real) =   0.01 sec

The following tests FAILED:
	  1 - test_qcount (Failed)
Errors while running CTest
```

If you'd like to see all of the output (not just failures), you can use the
verbose command:
```
$ ctest --verbose
Test project /my-projects/qlibc-build
test 1
    Start 1: test_qcount

1: Test command: /my-projects/qlibc-build/tests/test_qcount
1: Test timeout computed to be: 9.99988e+06
1: Test qcount.c
1: ======================================================================
1: * TEST : qcount_read() . OK
1: ======================================================================
1: PASS - 1/1 tests passed.
1/2 Test #1: test_qcount ......................   Passed    0.00 sec
test 2
    Start 2: test_qstring

2: Test command: /my-projects/qlibc-build/tests/test_qstring
2: Test timeout computed to be: 9.99988e+06
2: Test qstring.c
2: ======================================================================
2: * TEST : qstrtrim() ............... OK
2: * TEST : qstrtrim_head() ....... OK
2: * TEST : qstrtrim_tail() ....... OK
2: ======================================================================
2: PASS - 3/3 tests passed.
2/2 Test #2: test_qstring .....................   Passed    0.00 sec

100% tests passed, 0 tests failed out of 2

Total Test time (real) =   0.01 sec
```

## How to Write Unit Tests

We need your help to write unit tests. Please refer `test_qstring.c` as a sample.

```
#include "qunit.h"
#include "qlibc.h"

QUNIT_START("Test title");

TEST("name1") {
    ASSERT(1 == 1);
}

TEST("name") {
    ASSERT_EQUAL_STR("abc", "abc");
}

TEST("name3") {
    ASSERT_EQUAL_INT(3, 3);
}

QUNIT_END();
```
