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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include "qlibc.h"
#include "qinternal.h"

// Change two hex character to one hex value.
char _q_x2c(char hex_up, char hex_low)
{
    char digit;

    digit = 16 * (hex_up >= 'A' ? ((hex_up & 0xdf) - 'A') + 10 : hex_up - '0');
    digit += (hex_low >= 'A' ? ((hex_low & 0xdf) - 'A') + 10 : hex_low - '0');

    return digit;
}

char *_q_makeword(char *str, char stop)
{
    char *word;
    int  len, i;

    for (len = 0; ((str[len] != stop) && (str[len])); len++);
    word = (char *)malloc(sizeof(char) * (len + 1));
    if (word == NULL) return NULL;

    for (i = 0; i < len; i++) word[i] = str[i];
    word[i] = '\0';

    if (str[len]) len++;
    for (i = len; str[i]; i++) str[i - len] = str[i];
    str[i - len] = '\0';

    return word;
}

void _q_humanOut(FILE *fp, void *data, size_t size, size_t max)
{
    size_t i;
    for (i = 0; i < size && i < max; i++) {
        int c = ((char *)data)[i];
        if (isprint(c) == 0) c = '?';
        fputc(c, fp);
    }

    if (size > max) fputs("...", fp);
}
