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

static bool drawtree(qtreetbl_t *tbl);
static void test_thousands_of_keys(int num_keys, char *key_postfix,
                                   char *value_postfix);

QUNIT_START("Test qtreetbl.c");

/* Test growth of tree
 *
 * Example from the slide p24-p25.
 * http://www.cs.princeton.edu/~rs/talks/LLRB/RedBlack.pdf
 *
 * Key insertion sequence : A S E R C D I N B X
 * After all the insertions, the data structure must be like this.
 *
 *                                 E
 *                   ______________|______________
 *                  /                             \
 *                 C                               R
 *          _______|_______                 _______|_______
 *         /               \               /               \
 *        B                 D             N                 X
 *      //                              //                //
 *    [A]                             [I]               [S]
 *
 * The nodes A I and S are nodes with RED upper link. Others are BLACK.
 */
TEST("Test growth of tree") {
    const char *KEY[] = { "A", "S", "E", "R", "C", "D", "I", "N", "B", "X", "" };
    qtreetbl_t *tbl = qtreetbl(0);

    int i;
    for (i = 0; KEY[i][0] != '\0'; i++) {
        tbl->putstr(tbl, KEY[i], "DATA");
    }

    printf("\n");
    drawtree(tbl);

    ASSERT(((char*)tbl->root->name)[0] == 'E');
    ASSERT((tbl->root->red) == false);
    ASSERT(((char*)tbl->root->left->name)[0] == 'C');
    ASSERT((tbl->root->left->red) == false);
    ASSERT(((char*)tbl->root->right->name)[0] == 'R');
    ASSERT((tbl->root->right->red) == false);
    ASSERT(((char*)tbl->root->left->left->name)[0] == 'B');
    ASSERT((tbl->root->left->left->red) == false);
    ASSERT(((char*)tbl->root->left->right->name)[0] == 'D');
    ASSERT((tbl->root->left->right->red) == false);
    ASSERT(((char*)tbl->root->right->left->name)[0] == 'N');
    ASSERT((tbl->root->right->left->red) == false);
    ASSERT(((char*)tbl->root->right->right->name)[0] == 'X');
    ASSERT((tbl->root->right->right->red) == false);
    ASSERT(((char*)tbl->root->left->left->left->name)[0] == 'A');
    ASSERT((tbl->root->left->left->left->red) == true);
    ASSERT(((char*)tbl->root->right->left->left->name)[0] == 'I');
    ASSERT((tbl->root->right->left->left->red) == true);
    ASSERT(((char*)tbl->root->right->right->left->name)[0] == 'S');
    ASSERT((tbl->root->right->right->left->red) == true);

    tbl->free(tbl);
}

TEST("Test basic but complete") {
    const char *KEY[] = { "A", "S", "E", "R", "C", "D", "I", "N", "B", "X", "" };

    qtreetbl_t *tbl = qtreetbl(0);
    ASSERT_EQUAL_INT(0, tbl->size(tbl));

    // insert
    int i;
    for (i = 0; KEY[i][0] != '\0'; i++) {
        tbl->putstr(tbl, KEY[i], KEY[i]);
        ASSERT_EQUAL_STR(KEY[i], tbl->getstr(tbl, KEY[i], false));
    }

    // verify
    for (i = 0; KEY[i][0] != '\0'; i++) {
        ASSERT_EQUAL_STR(KEY[i], tbl->getstr(tbl, KEY[i], false));
    }

    // not found case
    ASSERT_NULL(tbl->getstr(tbl, "_NOT_EXIST_", false));  // not found

    // delete
    for (i = 0; KEY[i][0] != '\0'; i++) {
        ASSERT_EQUAL_BOOL(true, tbl->remove(tbl, KEY[i]));
    }
    ASSERT_EQUAL_INT(0, tbl->size(tbl));

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

TEST("Test put_by_obj() / get_by_obj()") {
    qtreetbl_t *tbl = qtreetbl(0);
    ASSERT_EQUAL_BOOL(true, tbl->put_by_obj(tbl, "bin_name", 8, "bin_data", 8));
    size_t datasize = 0;
    void *data = tbl->get_by_obj(tbl, "bin_name", 8, &datasize, false);
    ASSERT(data != NULL && datasize == 8 && !memcmp(data, "bin_data", datasize));
    data = tbl->get_by_obj(tbl, "bin_name", 8, &datasize, true);
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

    qtreetbl_obj_t obj = tbl->find_nearest(tbl, "0", 2, false);
    ASSERT_NULL(obj.name);
    ASSERT_EQUAL_INT(errno, ENOENT);

    int i;
    for (i = 0; KEY[i][0] != '\0'; i++) {
        tbl->putstr(tbl, KEY[i], KEY[i]);
    }

    obj = tbl->find_nearest(tbl, "0", 2, false);
    ASSERT_EQUAL_STR("A", (char*)obj.name);
    obj = tbl->find_nearest(tbl, "F", 2, false);
    ASSERT_EQUAL_STR("E", (char*)obj.name);
    obj = tbl->find_nearest(tbl, "J", 2, false);
    ASSERT_EQUAL_STR("I", (char*)obj.name);
    obj = tbl->find_nearest(tbl, "O", 2, false);
    ASSERT_EQUAL_STR("N", (char*)obj.name);
    obj = tbl->find_nearest(tbl, "T", 2, false);
    ASSERT_EQUAL_STR("S", (char*)obj.name);
    obj = tbl->find_nearest(tbl, "Z", 2, false);
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

TEST("Test deletion in getnext() loop") {
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

            // 2. Remove the object
            tbl->remove_by_obj(tbl, obj.name, obj.namesize);  // remove

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

TEST("Test thousands of keys put/delete: short key + short value") {
    test_thousands_of_keys(10000, "", "");
}

TEST("Test thousands of keys put/delete: short key + long value") {
    test_thousands_of_keys(
            10000,
            "",
            "1a087a6982371bbfc9d4e14ae76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866");
}

TEST("Test thousands of keys put/delete: long key + short value") {
    test_thousands_of_keys(
            10000,
            "1a087a6982371bbfc9d4e14ae76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866",
            "");
}

TEST("Test thousands of keys put/delete: long key + long value") {
    test_thousands_of_keys(
            10000,
            "1a087a6982371bbfc9d4e14ae76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866",
            "1a087a6982371bbfc9d4e14ae76e05ddd784a5d9c6b0fc9e6cd715baab66b90987b2ee054764e58fc04e449dfa060a68398601b64cf470cb6f0a260ec6539866");
}

TEST("Test rule 4 and 5") {
    qtreetbl_t *tbl = qtreetbl(0);
    
    size_t siz = sizeof(int);
    int val = 0;
    
    tbl->put(tbl, "0 0 0 0 0 0 0 0 0 0 0 0", &val, siz);
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    tbl->remove(tbl, "0 0 0 0 0 0 0 0 0 0 0 0");
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    tbl->put(tbl, "0 0 0 0 0 0 0 0 0 0 0 0", &val, siz); 
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    
    tbl->put(tbl, "0 0 0 0 0 0 0 0 0 0 1 0", &val, siz); 
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    tbl->remove(tbl, "0 0 0 0 0 0 0 0 0 0 1 0");  
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    tbl->put(tbl, "0 0 0 0 0 0 0 0 0 0 1 0", &val, siz); 
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));

    tbl->put(tbl, "0 0 0 0 0 0 0 0 0 1 1 0", &val, siz); 
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    
    tbl->put(tbl, "0 0 0 0 0 0 0 0 0 0 1 1", &val, siz); 
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    tbl->remove(tbl, "0 0 0 0 0 0 0 0 0 0 1 1");
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    tbl->put(tbl, "0 0 0 0 0 0 0 0 0 0 1 1", &val, siz); 
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    
    tbl->put(tbl, "0 0 1 0 0 0 0 0 0 0 1 1", &val, siz);
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    
    tbl->put(tbl, "0 0 0 0 0 0 1 0 0 0 1 1", &val, siz); 
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    
    tbl->put(tbl, "0 0 0 0 0 0 0 0 0 1 1 1", &val, siz);
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    
    tbl->remove(tbl, "0 0 0 0 0 0 0 0 0 1 1 0");
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    
    tbl->put(tbl, "0 0 0 0 0 0 0 0 0 1 1 0", &val, siz);
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    
    tbl->put(tbl, "1 0 0 0 0 0 0 0 0 1 1 0", &val, siz);
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    
    tbl->put(tbl, "0 0 0 0 0 0 0 0 1 1 1 0", &val, siz);
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));
    tbl->remove(tbl, "0 0 0 0 0 0 0 0 1 1 1 0");
    ASSERT_EQUAL_INT(0, qtreetbl_check(tbl));

    tbl->free(tbl);
}

QUNIT_END();

static void test_thousands_of_keys(int num_keys, char *key_postfix,
                                   char *value_postfix) {
    qtreetbl_t *tbl = qtreetbl(0);
    ASSERT_EQUAL_INT(0, tbl->size(tbl));

    int i;
    for (i = 0; i < num_keys; i++) {
        char *key = qstrdupf("key%d%s", i, key_postfix);
        char *value = qstrdupf("value%d%s", i, value_postfix);

        ASSERT_EQUAL_BOOL(true, tbl->putstr(tbl, key, value));
        ASSERT_EQUAL_INT(i + 1, tbl->size(tbl));
        ASSERT_EQUAL_STR(value, tbl->getstr(tbl, key, false));

        free(key);
        free(value);
    }

    for (i--; i >= 0; i--) {
        char *key = qstrdupf("key%d%s", i, key_postfix);
        ASSERT_EQUAL_BOOL(true, tbl->remove(tbl, key));
        ASSERT_EQUAL_INT(i, tbl->size(tbl));
        ASSERT_NULL(tbl->getstr(tbl, key, false));
        free(key);
    }

    ASSERT_EQUAL_INT(0, tbl->size(tbl));
    tbl->free(tbl);
}

#define PARENT(i) ((i-1) / 2)
#define LINE_WIDTH 70
static bool drawtree(qtreetbl_t *tbl) {
    tbl->lock(tbl);
    qtreetbl_obj_t nullobj;
    nullobj.name = strdup(" ");
    nullobj.red = false;

    qqueue_t *q = qqueue(0);
    q->push(q, tbl->root, sizeof(qtreetbl_obj_t));

    int i, j, k, pos, x = 1, level = 0;
    int redcnt = 0;
    int print_pos[tbl->size(tbl) * 2];
    for (print_pos[0] = 0, i = 0, j = 1; q->size(q) > 0; i++, j++) {
        qtreetbl_obj_t *obj = q->pop(q, NULL);
        if (obj == NULL) {
            break;
        }

        pos = print_pos[PARENT(i)]
                + (i % 2 ? -1 : 1) * (LINE_WIDTH / (pow(2, level + 1)) + 1);

        for (k = 0; k < pos - x; k++) {
            printf("%c", i == 0 || i % 2 ? ' ' : '`');
        }
        printf("%c%s%c",
            (obj->red) ? '[' : ' ',
            (char*)obj->name,
            (obj->red) ? ']' : ' ');
        if (obj->red) {
            redcnt++;
        }

        print_pos[i] = x = pos + 1;
        x += 2;
        if (j == pow(2, level)) {
            printf("\n");
            level++;
            x = 1;
            j = 0;
        }

        if (((char*)obj->name)[0] != ' ') {
            q->push(q, (obj->left) ? obj->left : &nullobj,
                    sizeof(qtreetbl_obj_t));
            q->push(q, (obj->right) ? obj->right : &nullobj,
                    sizeof(qtreetbl_obj_t));
        }
        free(obj);
    }
    q->free(q);
    printf("\n");

    tbl->unlock(tbl);

    printf("\n           Tree Info : #nodes=%d, #red=%d, #black=-%d, root=%s\n",
        (int)tbl->size(tbl), redcnt, ((int)tbl->size(tbl) - redcnt),
        (char*)tbl->root->name);

    return true;
}
