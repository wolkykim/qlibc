qLibc Unit Tests
================

# How to run unit tests.

```
$ make run
Test qstring.c
======================================================================
* TEST : qstrtrim() ............... OK
* TEST : qstrtrim_head() ....... OK
* TEST : qstrtrim_tail() ....... OK
======================================================================
PASS - 3/3 tests passed.
```

# How to write unit tests

We need your help on writing unit tests. Please refer test_qstring.c as a sample.

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
