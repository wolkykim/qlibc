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
 * qfile header file.
 *
 * @file qfile.h
 */

#ifndef _QFILE_H
#define _QFILE_H

#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool qfile_lock(int fd);
extern bool qfile_unlock(int fd);
extern bool qfile_exist(const char *filepath);
extern void *qfile_load(const char *filepath, size_t *nbytes);
extern void *qfile_read(FILE *fp, size_t *nbytes);
extern ssize_t qfile_save(const char *filepath, const void *buf, size_t size,
                          bool append);
extern bool qfile_mkdir(const char *dirpath, mode_t mode, bool recursive);

extern char *qfile_get_name(const char *filepath);
extern char *qfile_get_dir(const char *filepath);
extern char *qfile_get_ext(const char *filepath);
extern off_t qfile_get_size(const char *filepath);

extern bool qfile_check_path(const char *path);
extern char *qfile_correct_path(char *path);
extern char *qfile_abspath(char *buf, size_t bufsize, const char *path);

#ifdef __cplusplus
}
#endif

#endif /*_QFILE_H */
