
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

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "qlibc.h"

int main(void) {
    qset_t *set = qset(1024, NULL, 0);
    set->add(set, "banana");
    set->add(set, "orange");
    set->add(set, "grape");
    set->add(set, "apple");

    if (set->contains(set, "grape")) {
        printf("Set contains 'grape'!\n");
    }
    else {
        printf("Set doesn't contains 'grape'!\n");
    }

    set->remove(set, "banana");

    if (set->contains(set, "banana")) {
        printf("Set contains 'banana'!\n");
    }
    else {
        printf("Set doesn't contains 'banana'!\n");
    }

    qset_t *A = qset(1024, NULL, 0);
    qset_t *B = qset(1024, NULL, 0);
    A->add(A, "1");
    A->add(A, "2");
    A->add(A, "3");
    A->add(A, "4");

    B->add(B, "3");
    B->add(B, "4");
    B->add(B, "5");
    B->add(B, "6");

    qset_t *_intersection = qset_intersection(A, B); // intersection = {"3", "4"}
    qset_t *_union = qset_union(A,B); // union = {"1", "2", "3", "4", "5", "6"}
    qset_t *_diff = qset_difference(A, B); // diff = {"1", "2"}
    qset_t *_sym_diff = qset_symmetric_difference(A, B) // sym_diff = {"1", "2", "5", "6"}
    
    qset_t *C = qset(1024, NULL, 0);
    qset_t *D = qset(1024, NULL, 0);
    qset_t *E = qset(1024, NULL, 0);
    qset_t *F = qset(1024, NULL, 0);
    C->add(C, "A");
    C->add(C, "B");
    C->add(C, "C");
    C->add(C, "D");
    C->add(C, "E");

    D->add(D, "C");
    D->add(D, "D");
    D->add(D, "E");

    E->add(E, "D");
    E->add(E, "C");
    E->add(E, "E");

    F->add(F, "A");
    F->add(F, "C");
    F->add(F, "B");
    

    bool is_subset = qset_is_subset(C, D); // true
    bool is_superset = qset_is_superset(C, D); // false
    bool is_strsubset = qset_is_strsubset(C, D); // true
    bool is_strsuperset = qset_is_strsuperset(C, D); // false

    qset_cmp_t cmp = qset_cmp(C, D); // QSET_CMP_LGREATER
    cmp = qset_cmp(D, C); // QSET_CMP_RGREATER
    cmp = qset_cmp(D, E); // QSET_CMP_EQ
    cmp = qset_cmp(F, E); // QSET_CMP_NEQ

    set->destroy(set);
    A->destroy(A);
    B->destroy(B);
    C->destroy(C);
    D->destroy(D);
    E->destroy(E);
}
