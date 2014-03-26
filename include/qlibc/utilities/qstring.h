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

/**
 * qstring header file.
 *
 * @file qstring.h
 */

#ifndef _QSTRING_H
#define _QSTRING_H


#include <stdlib.h>
#include <stdbool.h>
#include "../containers/qlist.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char *qstrtrim(char *str);
extern char *qstrtrim_head(char *str);
extern char *qstrtrim_tail(char *str);
extern char *qstrunchar(char *str, char head, char tail);
extern char *qstrreplace(const char *mode, char *srcstr, const char *tokstr,
                         const char *word);
extern char *qstrcpy(char *dst, size_t size, const char *src);
extern char *qstrncpy(char *dst, size_t size, const char *src, size_t nbytes);
extern char *qstrdupf(const char *format, ...);
extern char *qstrdup_between(const char *str, const char *start,
                             const char *end);
extern char *qstrcatf(char *str, const char *format, ...);
extern char *qstrgets(char *buf, size_t size, char **offset);
extern char *qstrrev(char *str);
extern char *qstrupper(char *str);
extern char *qstrlower(char *str);
extern char *qstrtok(char *str, const char *delimiters, char *retstop,
                     int *offset);
extern qlist_t *qstrtokenizer(const char *str, const char *delimiters);
extern char *qstrunique(const char *seed);
extern char *qstr_comma_number(int number);
extern bool qstrtest(int (*testfunc)(int), const char *str);
extern bool qstr_is_email(const char *email);
extern bool qstr_is_ip4addr(const char *str);
extern char *qstr_conv_encoding(const char *fromstr, const char *fromcode,
                                const char *tocode, float mag);

#ifdef __cplusplus
}
#endif

#endif /*_QSTRING_H */
