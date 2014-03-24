/******************************************************************************
 * qLibc
 *
 * Copyright (c) 2010-2014 Seungyoung Kim.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "qlibc.h"

int main(void)
{
    // create a list-table.
    qlisttbl_t *tbl = qlisttbl(QLISTTBL_OPT_THREADSAFE);
    qdlnobj_t obj;
    int i;

    //
    // TEST 1 : adding elements.
    //

    // insert elements (key duplication allowed)
    tbl->put(tbl, "e1", "object1", strlen("object1")+1, false);
    tbl->putstr(tbl, "e2", "object2", false);
    tbl->putstr(tbl, "e3", "object3", false);
    tbl->putstr(tbl, "e4", "object4", false);
    tbl->putint(tbl, "e5", 5, false);

    // print out
    printf("--[Test 1 : adding elements]--\n");
    tbl->debug(tbl, stdout);

    //
    // TEST 2 : many ways to find key.
    //

    printf("\n--[Test 2 : many ways to find key]--\n");
    printf("get('e2') : %s\n", (char *)tbl->get(tbl, "e2", NULL, false));
    printf("getstr('e2') : %s\n", tbl->getstr(tbl, "e2", false));

    char *e2 = tbl->getstr(tbl, "e2", true);
    printf("getstr('e2', newmem) : %s\n", e2);
    free(e2);

    //
    // TEST 3 : getmulti() - fetch all duplicated 'e2' keys.
    //

    printf("\n--[Test 3 : getmulti() - fetch all duplicated 'e2' keys]--\n");
    size_t numobjs = 0;
    qobj_t *objs = tbl->getmulti(tbl, "e2", true, &numobjs);
    printf("getmulti('e2') : %d objects found.\n", (int)numobjs);
    for (i = 0; objs[i].data != NULL; i++) {
        printf("getmulti('e2')[%d]=%s (%zu)\n",
               i, (char *)objs[i].data, objs[i].size);
    }
    tbl->freemulti(objs);

    //
    // TEST 4 : travesal a particular key 'e2'.
    //

    printf("\n--[Test 4 : travesal a particular key 'e2']--\n");
    memset((void *)&obj, 0, sizeof(obj)); // must be cleared before call
    tbl->lock(tbl);
    while (tbl->getnext(tbl, &obj, "e2", false) == true) {
        printf("NAME=%s, DATA=%s, SIZE=%zu\n",
               obj.name, (char *)obj.data, obj.size);
    }
    tbl->unlock(tbl);

    //
    // TEST 5 : travesal a list.
    //

    printf("\n--[Test 5 : travesal a list]--\n");
    printf("list size : %zu elements\n", tbl->size(tbl));
    memset((void *)&obj, 0, sizeof(obj)); // must be cleared before call
    tbl->lock(tbl);
    while (tbl->getnext(tbl, &obj, NULL, true) == true) {
        printf("NAME=%s, DATA=%s, SIZE=%zu\n",
               obj.name, (char *)obj.data, obj.size);
        free(obj.name);
        free(obj.data);
    }
    tbl->unlock(tbl);

    //
    // TEST 6 : changed put direction then add 'e3' and 'e4' element.
    //
    tbl->setputdir(tbl, true);
    tbl->putstr(tbl, "e3", "object6", false);
    tbl->putstr(tbl, "e4", "object7", false);

    // print out
    printf("\n--[Test 6 : changed adding direction then"
           " add 'e3' and 'e4' element]--\n");
    tbl->debug(tbl, stdout);

    //
    // TEST 7 :  add element 'e2' with replace option.
    //
    tbl->putstr(tbl, "e2", "object8", true);

    // print out
    printf("\n--[Test 7 : add element 'e2' with replace option]--\n");
    tbl->debug(tbl, stdout);

    //
    // TEST 8 :  reverse list
    //
    tbl->reverse(tbl);

    // print out
    printf("\n--[Test 8 : reverse]--\n");
    tbl->debug(tbl, stdout);

    //
    // TEST 9 : revert put direction then add some more elements.
    //
    tbl->setputdir(tbl, false);
    tbl->putstr(tbl, "e4", "object9", false);
    tbl->putstr(tbl, "e5", "object10", false);
    tbl->putstr(tbl, "e6", "object11", false);
    tbl->putstr(tbl, "e7", "object12", false);
    tbl->putstr(tbl, "e4", "object13", false);
    tbl->putstr(tbl, "e6", "object14", false);

    // print out
    printf("\n--[Test 9 : revert put direction then add some more"
           " elements.]--\n");
    tbl->debug(tbl, stdout);

    //
    // TEST 10 : turn on setsort() option.
    //
    tbl->setsort(tbl, true, false);

    // print out
    printf("\n--[Test 10 : turn on setsort() option.\n");
    tbl->debug(tbl, stdout);

    //
    // TEST 11 : add new elements 'e1', 'e5' and 'e8'.
    //
    tbl->putstr(tbl, "e1", "object15", false);
    tbl->putint(tbl, "e5", 16, false);
    tbl->putstr(tbl, "e8", "object17", false);

    // print out
    printf("\n--[Test 11 : add new elements 'e1', 'e5' and 'e8'\n");
    tbl->debug(tbl, stdout);

    // free object
    tbl->free(tbl);

    return 0;
}
