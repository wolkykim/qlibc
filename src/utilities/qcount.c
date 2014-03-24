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
 * @file qcount.c Counter file handling APIs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "qlibc.h"
#include "qinternal.h"

/**
 * Read counter(integer) from file with advisory file locking.
 *
 * @param filepath  file path
 *
 * @return counter value readed from file. in case of failure, returns 0.
 *
 * @code
 *   qcount_save("number.dat", 75);
 *   int count = qcount_read("number.dat");
 * @endcode
 *
 * @code
 *   ---- number.dat ----
 *   75
 *   --------------------
 * @endcode
 */
int64_t qcount_read(const char *filepath)
{
    int fd = open(filepath, O_RDONLY, 0);
    if (fd < 0) return 0;

    char buf[20+1];
    ssize_t readed = read(fd, buf, (sizeof(buf) - 1));
    close(fd);

    int64_t num = 0;
    if (readed > 0) {
        buf[readed] = '\0';
        num = atoll(buf);
    }

    return num;
}

/**
 * Save counter(integer) to file with advisory file locking.
 *
 * @param filepath  file path
 * @param number    counter integer value
 *
 * @return true if successful, otherwise returns false.
 *
 * @code
 *   qcount_save("number.dat", 75);
 * @endcode
 */
bool qcount_save(const char *filepath, int64_t number)
{
    int fd = open(filepath, O_CREAT|O_WRONLY|O_TRUNC, DEF_FILE_MODE);
    if (fd < 0) return false;

    char *str = qstrdupf("%"PRId64, number);
    ssize_t updated = write(fd, str, strlen(str));
    close(fd);

    if (updated > 0) return true;
    return false;
}

/**
 * Increases(or decrease) the counter value as much as specified number
 * with advisory file locking.
 *
 * @param filepath  file path
 * @param number    how much increase or decrease
 *
 * @return updated counter value. in case of failure, returns 0.
 *
 * @code
 *   int count;
 *   count = qcount_update("number.dat", -3);
 * @endcode
 */
int64_t qcount_update(const char *filepath, int64_t number)
{
    int64_t counter = qcount_read(filepath);
    counter += number;
    if (qcount_save(filepath, counter) == true) {
        return counter;
    }
    return 0;
}
