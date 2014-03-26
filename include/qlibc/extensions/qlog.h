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
 ******************************************************************************/

/**
 * Rotating file logger.
 *
 * This is a qLibc extension implementing application level auto-rotating file
 * logger.
 *
 * @file qlog.h
 */

#ifndef _QLOG_H
#define _QLOG_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "containers/qtype.h"

#ifdef __cplusplus
extern "C" {
#endif

/* types */
typedef struct qlog_s qlog_t;

/* constants */
#define QLOG_OPT_THREADSAFE  (0x01)
#define QLOG_OPT_FLUSH       (0x01 << 1)

/* public functions */
extern qlog_t *qlog(const char *filepathfmt, mode_t mode, int rotateinterval, int options);

/**
 * qlog structure object structure
 */
struct qlog_s {
    /* encapsulated member functions */
    bool (*write) (qlog_t *log, const char *str);
    bool (*writef) (qlog_t *log, const char *format, ...);
    bool (*duplicate) (qlog_t *log, FILE *outfp, bool flush);
    bool (*flush) (qlog_t *log);
    void (*free) (qlog_t *log);

    /* private variables - do not access directly */
    qmutex_t *qmutex;  /*!< activated if compiled with --enable-threadsafe */

    char filepathfmt[PATH_MAX]; /*!< file file naming format like
                                     /somepath/daily-%Y%m%d.log */
    char filepath[PATH_MAX];    /*!< generated system path of log file */
    FILE *fp;           /*!< file pointer of logpath */
    mode_t mode;        /*!< file mode */
    int rotateinterval; /*!< log file will be rotate in this interval seconds */
    int nextrotate;  /*!< next rotate universal time, seconds */
    bool logflush;   /*!< flag for immediate flushing */

    FILE *outfp;    /*!< stream pointer for duplication */
    bool outflush;  /*!< flag for immediate flushing for duplicated stream */
};

#ifdef __cplusplus
}
#endif

#endif /*_QLOG_H */
