qLibc Unit Tests
================

# How to run unit tests.

```
$ make test
Test qstring.c
======================================================================
* TEST : qstrtrim() ............... OK (15 assertions)
* TEST : qstrtrim_head() ....... OK (7 assertions)
* TEST : qstrtrim_tail() ....... OK (7 assertions)
======================================================================
PASS - 3/3 tests passed.

Test qhashtbl.c
======================================================================
* TEST : Test basic but complete ......... OK (9 assertions)
* TEST : Test thousands of keys insertion and removal ..... OK (40002 assertions)
======================================================================
PASS - 2/2 tests passed.
```

# How to write unit tests

We need your help to write unit tests. Please refer test_qstring.c and qunit.h for your inspiration.

```C
#include "qunit.h"
#include "qlibc.h"

QUNIT_START("Test title");

TEST("Test name1") {
    ASSERT_EQUAL_STR("abc", "abc");
    ASSERT_EQUAL_INT(8, 8);
}

TEST("Test name2") {
    ASSERT_EQUAL_PT(NULL == NULL);
}

QUNIT_END();
```
