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

#include "qlibc.h"

// This includes all extension header files.
#include "qlibcext.h"
// In case, you want to include only particular extensions, replace with this.
//#include "extensions/qconfig.h"

#define CONF_PATH   "config.conf"

int main(void) {
#ifdef DISABLE_QCONFIG
    printf("qconfig extension is disabled at compile time.\n");
    return 1;
#else
    qlisttbl_t *tbl = qlisttbl(QLISTTBL_OPT_UNIQUEKEY);
    if (qconfig_parse_file(tbl, CONF_PATH, '=') == NULL) {
        printf("Failed to open '" CONF_PATH "'.\n");
        return -1;
    }
    tbl->debug(tbl, stdout);

    return 0;
#endif
}
