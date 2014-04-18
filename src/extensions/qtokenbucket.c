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
 * @file qtokenbucket.c Token Bucket implementation.
 *
 * Current implementation is not thread-safe.
 *
 * More information about token-bucket:
 *   http://en.wikipedia.org/wiki/Token_bucket
 *
 * @code
 *   qtokenbucket_t bucket;
 *   qtokenbucket_init(&bucket, 500, 1000, 1000);
 *   while (1) {
 *     if (qtokenbucket_consume(&bucket, 1) == false) {
 *       // Bucket is empty. Let's wait
 *       usleep(qtokenbucket_waittime(&bucket, 1) * 1000);
 *       continue;
 *     }
 *     // Got a token. Let's do something here.
 *     do_something();
 *   }
 * @endcode
 */

#include "extensions/qtokenbucket.h"

#include <stdio.h>
#include <stdbool.h>
#include <strings.h>
#include <sys/time.h>
#include "utilities/qtime.h"
#include "qinternal.h"

#ifndef _DOXYGEN_SKIP
/* internal functions */
static void refill_tokens(qtokenbucket_t *bucket);
#endif

/**
 * Initialize the token bucket.
 *
 * @param init_tokens
 *      the initial number of tokens.
 * @param max_tokens
 *      maximum number of tokens in the bucket.
 * @param tokens_per_sec
 *      number of tokens to fill per a second.
 */
void qtokenbucket_init(qtokenbucket_t *bucket, int init_tokens, int max_tokens,
                       int tokens_per_sec) {
    bzero(bucket, sizeof(qtokenbucket_t));
    bucket->tokens = init_tokens;
    bucket->max_tokens = max_tokens;
    bucket->tokens_per_sec = tokens_per_sec;
    bucket->last_fill = qtime_current_milli();
}

/**
 * Consume tokens from the bucket.
 *
 * @param bucket tockenbucket object.
 * @param tokens number of tokens to request.
 *
 * @return return true if there are enough tokens, otherwise false.
 */
bool qtokenbucket_consume(qtokenbucket_t *bucket, int tokens) {
    refill_tokens(bucket);
    if (bucket->tokens < tokens) {
        return false;
    }
    bucket->tokens -= tokens;
    return true;
}

/**
 * Get the estimate time until given number of token is ready.
 *
 * @param tokens number of tokens
 *
 * @return estimated milliseconds
 */
long qtokenbucket_waittime(qtokenbucket_t *bucket, int tokens) {
    refill_tokens(bucket);
    if (bucket->tokens >= tokens) {
        return 0;
    }
    int tokens_needed = tokens - (int)bucket->tokens;
    double estimate_milli = (1000 * tokens_needed) / bucket->tokens_per_sec;
    estimate_milli += ((1000 * tokens_needed) % bucket->tokens_per_sec) ? 1 : 0;
    return estimate_milli;
}

#ifndef _DOXYGEN_SKIP
/**
 * Refill tokens.
 */
static void refill_tokens(qtokenbucket_t *bucket) {
    long now = qtime_current_milli();
    if (bucket->tokens < bucket->max_tokens) {
        double new_tokens = (now - bucket->last_fill) * 0.001
                * bucket->tokens_per_sec;
        bucket->tokens =
                ((bucket->tokens + new_tokens) < bucket->max_tokens) ?
                        (bucket->tokens + new_tokens) : bucket->max_tokens;
    }
    bucket->last_fill = now;
}
#endif // _DOXYGEN_SKIP
