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

#include <math.h>
#include <errno.h>
#include "qunit.h"
#include "qlibc.h"

static void ASSERT_TREE_CHECK(qtreetbl_t *tbl, bool verbose);
static bool print_tree(qtreetbl_t *tbl);
int uint32_cmp(const void *name1, size_t namesize1, const void *name2, size_t namesize2);
static void perf_test(uint32_t keys[], int num_keys);

QUNIT_START("Test qtreetbl.c");

/* Test growth of tree
 *
 * Example taken from the inventor's presentation slide p24-p25.
 * https://sedgewick.io/wp-content/uploads/2022/03/2008-09LLRB.pdf
 *
 * Key insertion sequence : A S E R C D I N B X
 *
 * @code
 *     ┌── X
 *     │   └──[S]
 * ┌── R
 * │   └── N
 * │       └──[I]
 * E
 * │   ┌── D
 * └── C
 *     └── B
 *         └──[A]
 * @endcode
 *
 * The nodes A, I and S are Red. Others are Black.
 */
TEST("Test growth of tree / A S E R C D I N B X") {
    const char *KEY[] = { "A", "S", "E", "R", "C", "D", "I", "N", "B", "X", "" };
    qtreetbl_t *tbl = qtreetbl(0);

    DISABLE_PROGRESS_DOT();
    int i;
    for (i = 0; KEY[i][0] != '\0'; i++) {
        tbl->putstr(tbl, KEY[i], "");
        ASSERT_TREE_CHECK(tbl, true);
    }
    ENABLE_PROGRESS_DOT();

    ASSERT_EQUAL_STR((char*)tbl->root->name, "E");
    ASSERT((tbl->root->red) == false);
    ASSERT_EQUAL_STR((char*)tbl->root->left->name, "C");
    ASSERT((tbl->root->left->red) == false);
    ASSERT_EQUAL_STR((char*)tbl->root->right->name, "R");
    ASSERT((tbl->root->right->red) == false);
    ASSERT_EQUAL_STR((char*)tbl->root->left->left->name, "B");
    ASSERT((tbl->root->left->left->red) == false);
    ASSERT_EQUAL_STR((char*)tbl->root->left->right->name, "D");
    ASSERT((tbl->root->left->right->red) == false);
    ASSERT_EQUAL_STR((char*)tbl->root->right->left->name, "N");
    ASSERT((tbl->root->right->left->red) == false);
    ASSERT_EQUAL_STR((char*)tbl->root->right->right->name, "X");
    ASSERT((tbl->root->right->right->red) == false);
    ASSERT_EQUAL_STR((char*)tbl->root->left->left->left->name, "A");
    ASSERT((tbl->root->left->left->left->red) == true);
    ASSERT_EQUAL_STR((char*)tbl->root->right->left->left->name, "I");
    ASSERT((tbl->root->right->left->left->red) == true);
    ASSERT_EQUAL_STR((char*)tbl->root->right->right->left->name, "S");
    ASSERT((tbl->root->right->right->left->red) == true);

    tbl->free(tbl);
}

/*
 * Example taken from
 * https://media.geeksforgeeks.org/wp-content/uploads/LLRB-Example-001.jpg
 *
 * Key insertion sequence : 10 20 30 40 50 25
 *
 * @code
 * ┌── 50
 * 40
 * │   ┌── 30
 * │   │   └──[25]
 * └──[20]
 *     └── 10
 * @endcode
 *
 * The nodes 20 and 25 are Red. Others are Black.
 */
TEST("Test insertion / 10 20 30 40 50 25") {
    const char *KEY[] = { "10", "20", "30", "40", "50", "25", "" };
    qtreetbl_t *tbl = qtreetbl(0);

    DISABLE_PROGRESS_DOT();
    int i;
    for (i = 0; KEY[i][0] != '\0'; i++) {
        tbl->putstr(tbl, KEY[i], "");
        ASSERT_TREE_CHECK(tbl, true);
    }
    ENABLE_PROGRESS_DOT();

    ASSERT_EQUAL_STR((char*)tbl->root->name, "40");
    ASSERT((tbl->root->red) == false);
    ASSERT_EQUAL_STR((char*)tbl->root->left->name, "20");
    ASSERT((tbl->root->left->red) == true);
    ASSERT_EQUAL_STR((char*)tbl->root->right->name, "50");
    ASSERT((tbl->root->right->red) == false);
    ASSERT_EQUAL_STR((char*)tbl->root->left->left->name, "10");
    ASSERT((tbl->root->left->left->red) == false);
    ASSERT_EQUAL_STR((char*)tbl->root->left->right->name, "30");
    ASSERT((tbl->root->left->right->red) == false);
    ASSERT_EQUAL_STR((char*)tbl->root->left->right->left->name, "25");
    ASSERT((tbl->root->left->right->left->red) == true);

    tbl->free(tbl);
}

TEST("Test tree with deletion / 0 1 2 3 4 5 6 7 8 9 0") {
    const char *KEY[] = {
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ""
    };
    qtreetbl_t *tbl = qtreetbl(0);

    DISABLE_PROGRESS_DOT();
    int i;
    for (i = 0; KEY[i][0] != '\0'; i++) {
        tbl->putstr(tbl, KEY[i], "");
        ASSERT_TREE_CHECK(tbl, true);
    }
    for (i = 0; KEY[i][0] != '\0'; i++) {
        tbl->remove(tbl, KEY[i]);
        ASSERT_TREE_CHECK(tbl, true);
    }
    ENABLE_PROGRESS_DOT();

    tbl->free(tbl);
}

/*
 * Example taken from Stroh Snow's finding about red property violation case.
 * https://github.com/wolkykim/qlibc/pull/106#issuecomment-1646521205
 */
TEST("Test tree with deletion / Stroh Snow's test for red property violation") {
    const char *KEY[] = {
        "J", "E", "O", "C", "L", "H", "Q", "B", "G", "K", "P", "D", "I",
        "N", "S", "A", "M", "F", "R",""
    };
    const char *KEY2[] = { "A", "M", "" };

    qtreetbl_t *tbl = qtreetbl(0);

    DISABLE_PROGRESS_DOT();
    int i;
    printf("\n\nInserting keys:");
    for (i = 0; KEY[i][0] != '\0'; i++) {
        printf(" %s", KEY[i]);
        tbl->putstr(tbl, KEY[i], "");
        ASSERT_TREE_CHECK(tbl, false);
    }

    ASSERT_TREE_CHECK(tbl, true);

    for (i = 0; KEY2[i][0] != '\0'; i++) {
        printf("\nKey deleted: %s", KEY2[i]);
        tbl->remove(tbl, KEY2[i]);
        ASSERT_TREE_CHECK(tbl, true);
    }

    ENABLE_PROGRESS_DOT();
    tbl->free(tbl);
}

TEST("Test basics") {
    const char *KEY[] = { "A", "S", "E", "R", "C", "D", "I", "N", "B", "X", "" };

    qtreetbl_t *tbl = qtreetbl(0);
    ASSERT_EQUAL_INT(0, tbl->size(tbl));

    // insert
    int i;
    for (i = 0; KEY[i][0] != '\0'; i++) {
        tbl->putstr(tbl, KEY[i], KEY[i]);
        ASSERT_EQUAL_STR(KEY[i], tbl->getstr(tbl, KEY[i], false));
        ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    }
    ASSERT_EQUAL_INT(i, tbl->size(tbl));

    // verify
    for (i = 0; KEY[i][0] != '\0'; i++) {
        ASSERT_EQUAL_STR(KEY[i], tbl->getstr(tbl, KEY[i], false));
    }

    // not found case
    ASSERT_NULL(tbl->getstr(tbl, "_NOT_EXIST_", false));  // not found

    // delete
    for (i = 0; KEY[i][0] != '\0'; i++) {
        ASSERT_EQUAL_BOOL(true, tbl->remove(tbl, KEY[i]));
        ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    }
    ASSERT_EQUAL_INT(0, tbl->size(tbl));

    tbl->free(tbl);
}

TEST("Test duplicated key insertions()") {
    qtreetbl_t *tbl = qtreetbl(0);
    ASSERT_EQUAL_INT(0, tbl->size(tbl));

    ASSERT_EQUAL_INT(0, tbl->size(tbl));
    for (int i = 1; i < 100; i++ ) {
        char *key = qstrdupf("K%03d", i);
        tbl->putstr(tbl, key, "");
        ASSERT_EQUAL_INT(i, tbl->size(tbl));
        tbl->putstr(tbl, key, "");
        ASSERT_EQUAL_INT(i, tbl->size(tbl));
        free(key);
    }

    tbl->free(tbl);
}

TEST("Test clear()") {
    const char *KEY[] = { "A", "S", "E", "R", "C", "D", "I", "N", "B", "X", "" };
    qtreetbl_t *tbl = qtreetbl(0);
    int i;
    for (i = 0; KEY[i][0] != '\0'; i++) {
        tbl->putstr(tbl, KEY[i], KEY[i]);
    }
    ASSERT_EQUAL_INT(i, tbl->size(tbl));
    tbl->clear(tbl);
    ASSERT_EQUAL_INT(0, tbl->size(tbl));
    tbl->free(tbl);
}

TEST("Test putobj() / getobj()") {
    qtreetbl_t *tbl = qtreetbl(0);
    ASSERT_EQUAL_BOOL(true, tbl->putobj(tbl, "bin_name", 8, "bin_data", 8));
    size_t datasize = 0;
    void *data = tbl->getobj(tbl, "bin_name", 8, &datasize, false);
    ASSERT(data != NULL && datasize == 8 && !memcmp(data, "bin_data", datasize));
    data = tbl->getobj(tbl, "bin_name", 8, &datasize, true);
    ASSERT(data != NULL && datasize == 8 && !memcmp(data, "bin_data", datasize));
    free(data);
    tbl->free(tbl);
}

TEST("Test putstrf() / getstr()") {
    qtreetbl_t *tbl = qtreetbl(0);
    ASSERT_EQUAL_BOOL(true, tbl->putstrf(tbl, "name", "my_%d_%s", 8, "data"));
    ASSERT_EQUAL_STR("my_8_data", tbl->getstr(tbl, "name", false));
    tbl->free(tbl);
}

TEST("Test find_nearest()") {
    const char *KEY[] = { "A", "S", "E", "R", "C", "D", "I", "N", "B", "X", "" };
    qtreetbl_t *tbl = qtreetbl(0);

    qtreetbl_obj_t obj;
    obj = tbl->find_nearest(tbl, "0", sizeof("0"), false);
    ASSERT_NULL(obj.name);
    ASSERT_EQUAL_INT(errno, ENOENT);

    int i;
    for (i = 0; KEY[i][0] != '\0'; i++) {
        tbl->putstr(tbl, KEY[i], KEY[i]);
    }

    obj = tbl->find_nearest(tbl, "0", sizeof("0"), false);
    ASSERT_EQUAL_STR("A", (char*)obj.name);
    obj = tbl->find_nearest(tbl, "F", sizeof("F"), false);
    ASSERT_EQUAL_STR("E", (char*)obj.name);
    obj = tbl->find_nearest(tbl, "J", sizeof("J"), false);
    ASSERT_EQUAL_STR("I", (char*)obj.name);
    obj = tbl->find_nearest(tbl, "O", sizeof("O"), false);
    ASSERT_EQUAL_STR("N", (char*)obj.name);
    obj = tbl->find_nearest(tbl, "T", sizeof("T"), false);
    ASSERT_EQUAL_STR("S", (char*)obj.name);
    obj = tbl->find_nearest(tbl, "Z", sizeof("Z"), false);
    ASSERT_EQUAL_STR("X", (char*)obj.name);

    tbl->free(tbl);
}

TEST("Test getnext()") {
    const char *KEY[] = { "A", "S", "E", "R", "C", "D", "I", "N", "B", "X", "" };
    qtreetbl_t *tbl = qtreetbl(0);
    int i;
    for (i = 0; KEY[i][0] != '\0'; i++) {
        tbl->putstr(tbl, KEY[i], KEY[i]);
    }

    char buf[1024] = "";
    qtreetbl_obj_t obj;
    memset((void*) &obj, 0, sizeof(obj));  // must be cleared before call
    tbl->lock(tbl);  // lock it when thread condition is expected
    while (tbl->getnext(tbl, &obj, false) == true) {  // newmem is false
        printf(">%s", (char*)obj.name);
        qstrcatf(buf, "%s", (char*)obj.name);
    }
    tbl->unlock(tbl);
    ASSERT_EQUAL_STR("ABCDEINRSX", buf);

    tbl->free(tbl);
}

TEST("Test getnext() from find_nearest(N)") {
    const char *KEY[] = { "A", "S", "E", "R", "C", "D", "I", "N", "B", "X", "" };
    qtreetbl_t *tbl = qtreetbl(0);
    int i;
    for (i = 0; KEY[i][0] != '\0'; i++) {
        tbl->putstr(tbl, KEY[i], KEY[i]);
    }

    char buf[1024] = "";
    tbl->lock(tbl);  // lock it when thread condition is expected
    qtreetbl_obj_t obj = tbl->find_nearest(tbl, "N", 2, false);
    while (tbl->getnext(tbl, &obj, false) == true) {  // newmem is false
        printf(">%s", (char*)obj.name);
        qstrcatf(buf, "%s", (char*)obj.name);
    }
    tbl->unlock(tbl);
    ASSERT_EQUAL_STR("INRSXABCDE", buf);

    tbl->free(tbl);
}

TEST("Test remove() in getnext() loop") {
    const char *KEY[] = { "A", "S", "E", "R", "C", "D", "I", "N", "B", "X", "" };
    qtreetbl_t *tbl = qtreetbl(0);
    int i;
    for (i = 0; KEY[i][0] != '\0'; i++) {
        tbl->putstr(tbl, KEY[i], KEY[i]);
    }

    char buf[1024] = "";

    // set iterator
    qtreetbl_obj_t obj;
    memset((void*) &obj, 0, sizeof(obj));

    tbl->lock(tbl);  // lock it when thread condition is expected
    while (tbl->getnext(tbl, &obj, false) == true) {
        printf(">%s", (char*)obj.name);
        if (!memcmp(obj.data, "B", 2) || !memcmp(obj.data, "S", 2)) {
            printf("*");
            qstrcatf(buf, "%s", (char*)obj.name);

            // 1. Keep the key name
            char *name = qmemdup(obj.name, obj.namesize);
            size_t namesize = obj.namesize;

            // 2. Delete the object
            tbl->removeobj(tbl, obj.name, obj.namesize);

            // 3. Rewind iterator one step back.
            // Note that this allows the iterator to continue traveling but
            // it doesn't guarantee it will visit all the nodes. Depends on the location
            // of removed node in the tree and due to tree rotations, iterator might
            // move to next branch recognizing it finished that branch. So, in real practice,
            // you'd prefer to run the scan again once this iteration finishes until
            // there's no objects in condition left to ensure
            obj = tbl->find_nearest(tbl, name, namesize, false);

            // clean up
            free(name);
        }
    }
    tbl->unlock(tbl);
    ASSERT_EQUAL_STR("BS", buf);

    tbl->free(tbl);
}

TEST("Test integrity of tree structure") {
    int num_keys = 10000;

    qtreetbl_t *tbl = qtreetbl(0);
    ASSERT_EQUAL_INT(0, tbl->size(tbl));

    int i;
    for (i = 0; i < num_keys; i++) {
        char *key = qstrdupf("K%05d", i);
        char *value = qstrdupf("V%05d", i);
        ASSERT_EQUAL_BOOL(true, tbl->putstr(tbl, key, value));
        ASSERT_EQUAL_STR(value, tbl->getstr(tbl, key, false));
        free(key);
        free(value);
        ASSERT_EQUAL_INT(i + 1, tbl->size(tbl));
        ASSERT_TREE_CHECK(tbl, false);
    }
    ASSERT_EQUAL_INT(num_keys, tbl->size(tbl));

    for (i--; i >= 0; i--) {
        char *key = qstrdupf("K%05d", i);
        ASSERT_EQUAL_BOOL(true, tbl->remove(tbl, key));
        ASSERT_NULL(tbl->getstr(tbl, key, false));
        free(key);
        ASSERT_EQUAL_INT(i, tbl->size(tbl));
        ASSERT_TREE_CHECK(tbl, false);
    }

    ASSERT_EQUAL_INT(0, tbl->size(tbl));
    tbl->free(tbl);
}

TEST("Test integrity of tree structure with random keys") {
    qtreetbl_t *tbl = qtreetbl(0);
    int num_loop = 30000;
    int key_range = 100; // random key range
    int fillup_max_percent = 50; // 0~50% from the key range
    int delete_percent = 50; // delete 50% keys in the table
    int insert_cnt = 0, remove_cnt = 0;

    #define GET_RAND_NUM() (rand() % key_range)
    srand((unsigned) time(NULL));
    for (int i = 0; i < num_loop; i++) {
        // insert some keys until total number of keys reaches enough
        int fill_upto = GET_RAND_NUM() * fillup_max_percent / 100;
        while (tbl->size(tbl) < fill_upto) {
            char *key = qstrdupf("K%05d", GET_RAND_NUM());
            if (tbl->getstr(tbl, key, false) != NULL) {
                free(key);
                continue;
            }
            ASSERT_EQUAL_BOOL(true, tbl->putstr(tbl, key, ""));
            free(key);
            insert_cnt++;
            ASSERT_TREE_CHECK(tbl, false);
        }

        // delete keys until total number of keys drops enough
        int delete_upto = tbl->size(tbl) * delete_percent / 100;
        while (tbl->size(tbl) > delete_upto) {
            char *key = qstrdupf("K%05d", GET_RAND_NUM());
            if (tbl->getstr(tbl, key, false) == NULL) {
                free(key);
                continue;
            }
            ASSERT_EQUAL_BOOL(true, tbl->remove(tbl, key));
            free(key);
            remove_cnt++;
            ASSERT_TREE_CHECK(tbl, false);
        }
    }
    printf("\n  #loop %d, #tot_insert %d, #tot_delete %d\n", num_loop, insert_cnt, remove_cnt);
    tbl->free(tbl);
}

TEST("Test tree performance / 1 million keys / random") {
    int num_keys = 1000000;
    uint32_t keys[num_keys];
    for (int i = 0; i < num_keys; i++) {
        keys[i] = qhashmurmur3_32(&i, sizeof(i));
    }
    perf_test(keys, num_keys);
}

TEST("Test tree performance / 1 million keys / ascending") {
    int num_keys = 1000000;
    uint32_t keys[num_keys];
    for (int i = 0; i < num_keys; i++) {
        keys[i] = i;
    }
    perf_test(keys, num_keys);
}

TEST("Test tree performance / 1 million keys / descending") {
    int num_keys = 1000000;
    uint32_t keys[num_keys];
    for (uint32_t i = num_keys; i > 0; i--) {
        keys[i] = (uint32_t)i;
    }
    perf_test(keys, num_keys);
}

TEST("Test tree performance / 1 million keys / 4 ascending + 1 random mix") {
    int num_keys = 1000000;
    uint32_t keys[num_keys];
    for (int i = 0; i < num_keys; i++) {
        keys[i] = (i % 3 == 0) ? (qhashmurmur3_32(&i, sizeof(i)) % (10 * num_keys)) : i * 10;
    }
    perf_test(keys, num_keys);
}

TEST("Test tree performance / 1 million keys / 2 ascending + 1 random mix") {
    int num_keys = 1000000;
    uint32_t keys[num_keys];
    for (int i = 0; i < num_keys; i++) {
        keys[i] = (i % 3 == 0) ? (qhashmurmur3_32(&i, sizeof(i)) % (10 * num_keys)) : i * 10;
    }
    perf_test(keys, num_keys);
}

QUNIT_END();

static void ASSERT_TREE_CHECK(qtreetbl_t *tbl, bool verbose) {
    int ret = qtreetbl_check(tbl);
    if (ret != 0) {
        printf("\n\nVIOLATION of property %d found.\n", ret);
        verbose = true;
        ASSERT_EQUAL_INT(0, ret);
    }

    if (verbose) {
        printf("\n");
        print_tree(tbl);
    }
}

static bool print_tree(qtreetbl_t *tbl) {
    tbl->debug(tbl, stdout);

    int redcnt = 0;
    qtreetbl_obj_t obj;
    memset((void*) &obj, 0, sizeof(obj));
    while (tbl->getnext(tbl, &obj, false) == true) {
        if (obj.red) {
            redcnt++;
        }
    }

    printf("(#nodes=%d, #red=%d, #black=%d)\n",
        (int)tbl->size(tbl), redcnt, ((int)tbl->size(tbl) - redcnt));
    return true;
}

int uint32_cmp(const void *name1, size_t namesize1, const void *name2, size_t namesize2) {
    return (*(uint32_t *)name1 == *(uint32_t *)name2) ? 0 :
        (*(uint32_t *)name1 < *(uint32_t *)name2) ? -1 : +1;
}


extern uint32_t _q_treetbl_flip_color_cnt;
extern uint32_t _q_treetbl_rotate_left_cnt;
extern uint32_t _q_treetbl_rotate_right_cnt;
static void perf_test(uint32_t keys[], int num_keys) {
    DISABLE_PROGRESS_DOT();
    qtreetbl_t *tbl = qtreetbl(0);
    long t;

    // set integer comparator
    tbl->set_compare(tbl, uint32_cmp);

    // insert
    _q_treetbl_flip_color_cnt = 0, _q_treetbl_rotate_left_cnt = 0, _q_treetbl_rotate_right_cnt = 0;
    ASSERT_EQUAL_INT(0, tbl->size(tbl));
    TIMER_START(t);
    for (int i = 0; i < num_keys; i++) {
        ASSERT_EQUAL_BOOL(true, tbl->putobj(tbl, &(keys[i]), sizeof(uint32_t), NULL, 0));
    }
    TIMER_STOP(t);
    ASSERT(tbl->size(tbl) > 0);
    ASSERT_TREE_CHECK(tbl, false);
    printf("\n  Insert %d keys: %ldms", num_keys, t);
    printf(" - flip %.2f, rotate %.2f (L %.2f, R %.2f)",
        (float)_q_treetbl_flip_color_cnt / num_keys,
        (float)(_q_treetbl_rotate_left_cnt + _q_treetbl_rotate_right_cnt) / num_keys,
        (float)_q_treetbl_rotate_left_cnt / num_keys,
        (float)_q_treetbl_rotate_right_cnt / num_keys);

    // find
    _q_treetbl_flip_color_cnt = 0, _q_treetbl_rotate_left_cnt = 0, _q_treetbl_rotate_right_cnt = 0;
    TIMER_START(t);
    for (int i = 0; i < num_keys; i++) {
        errno = 0;
        tbl->getobj(tbl, &(keys[i]), sizeof(uint32_t), NULL, false);
        ASSERT_EQUAL_INT(errno, 0);
    }
    TIMER_STOP(t);
    printf("\n  Lookup %d keys: %ldms", num_keys, t);
    printf(" - flip %.2f, rotate %.2f (L %.2f, R %.2f)",
        (float)_q_treetbl_flip_color_cnt / num_keys,
        (float)(_q_treetbl_rotate_left_cnt + _q_treetbl_rotate_right_cnt) / num_keys,
        (float)_q_treetbl_rotate_left_cnt / num_keys,
        (float)_q_treetbl_rotate_right_cnt / num_keys);

    // remove
    _q_treetbl_flip_color_cnt = 0, _q_treetbl_rotate_left_cnt = 0, _q_treetbl_rotate_right_cnt = 0;
    TIMER_START(t);
    for (int i = 0; i < num_keys; i++) {
        tbl->removeobj(tbl, &(keys[i]), sizeof(uint32_t));
    }
    TIMER_STOP(t);
    ASSERT_EQUAL_INT(0, tbl->size(tbl));
    printf("\n  Delete %d keys: %ldms", num_keys, t);
    printf(" - flip %.2f, rotate %.2f (L %.2f, R %.2f)",
        (float)_q_treetbl_flip_color_cnt / num_keys,
        (float)(_q_treetbl_rotate_left_cnt + _q_treetbl_rotate_right_cnt) / num_keys,
        (float)_q_treetbl_rotate_left_cnt / num_keys,
        (float)_q_treetbl_rotate_right_cnt / num_keys);
    printf("\n");

    tbl->free(tbl);
    ENABLE_PROGRESS_DOT();
}
