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
 * @file qtokenbucket.h
 */

#ifndef _QTOKENBUCKET_H
#define _QTOKENBUCKET_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* types */
typedef struct qtokenbucket_s qtokenbucket_t;

/* public functions */
extern void qtokenbucket_init(qtokenbucket_t *bucket, int init_tokens,
                              int max_tokens, int tokens_per_sec);
extern bool qtokenbucket_consume(qtokenbucket_t *bucket, int tokens);
extern long qtokenbucket_waittime(qtokenbucket_t *bucket, int tokens);

/**
 * qtokenbucket internal data structure
 */
struct qtokenbucket_s {
    double tokens; /*!< current number of tokens. */
    int max_tokens; /*!< maximum number of tokens. */
    int tokens_per_sec; /*!< fill rate per second. */
    long last_fill; /*!< last refill time in Millisecond. */
};

#ifdef __cplusplus
}
#endif

#endif /*_QTOKENBUCKET_H */
