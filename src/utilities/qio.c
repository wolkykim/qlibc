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
 * @file qio.c I/O handling APIs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include "qlibc.h"
#include "qinternal.h"

#define MAX_IOSEND_SIZE     (32 * 1024)

/**
 * Test & wait until the file descriptor has readable data.
 *
 * @param fd        file descriptor
 * @param timeoutms wait timeout milliseconds. 0 for no wait,
 *                  -1 for infinite wait
 *
 * @return 1 if readable, 0 on timeout, -1 if an error occurred.
 *
 * @note
 *  The argument timeoutms can be used to set maximum wait time for a socket
 *  descriptor.
 */
int qio_wait_readable(int fd, int timeoutms)
{
    struct pollfd fds[1];

    fds[0].fd = fd;
    fds[0].events = POLLIN;

    int pollret = poll(fds, 1, timeoutms);
    if (pollret == 0) {
        errno = ETIMEDOUT;
        return 0;
    } else if (pollret < 0) {
        return -1;
    }

    if (fds[0].revents & POLLIN) return 1;
    return -1;
}

/**
 * Test & wait until the file descriptor is ready for writing.
 *
 * @param fd        file descriptor
 * @param timeoutms wait timeout milliseconds. 0 for no wait,
 *                  -1 for infinite wait
 *
 * @return 1 if writable, 0 on timeout, -1 if an error occurred.
 */
int qio_wait_writable(int fd, int timeoutms)
{
    struct pollfd fds[1];

    fds[0].fd = fd;
    fds[0].events = POLLOUT;

    int pollret = poll(fds, 1, timeoutms);
    if (pollret == 0) {
        errno = ETIMEDOUT;
        return 0;
    } else if (pollret < 0) {
        return -1;
    }

    if (fds[0].revents & POLLOUT) return 1;
    return -1;
}

/**
 * Read from a file descriptor.
 *
 * @param fd        file descriptor
 * @param buf       data buffer pointer to write to
 * @param nbytes    the number of bytes to read from file descriptor & write
 *                  into buffer
 * @param timeoutms wait timeout milliseconds. 0 for no wait, -1 for infinite
 *                  wait
 *
 * @return the number of bytes read if successful, 0 on timeout, -1 for error.
 */
ssize_t qio_read(int fd, void *buf, size_t nbytes, int timeoutms)
{
    if (nbytes == 0) return 0;

    ssize_t total  = 0;
    while (total < nbytes) {
        if (timeoutms >= 0 && qio_wait_readable(fd, timeoutms) <= 0) break;

        ssize_t rsize = read(fd, buf + total, nbytes - total);
        if (rsize <= 0) {
            if (errno == EAGAIN || errno == EINPROGRESS) {
                // possible with non-block io
                usleep(1);
                continue;
            }
            break;
        }
        total += rsize;
    }

    if (total > 0) return total;
    else if (errno == ETIMEDOUT) return 0;
    return -1;
}

/**
 * Write to a file descriptor.
 *
 * @param fd        file descriptor
 * @param buf       data buffer pointer to read from
 * @param nbytes    the number of bytes to write to file descriptor & read
 *                  from buffer
 * @param timeoutms wait timeout milliseconds. 0 for no wait, -1 for infinite
 *                  wait
 *
 * @return the number of bytes written if successful, 0 on timeout,
 *         -1 for error.
 */
ssize_t qio_write(int fd, const void *buf, size_t nbytes, int timeoutms)
{
    if (nbytes == 0) return 0;

    ssize_t total  = 0;
    while (total < nbytes) {
        if (timeoutms >= 0 && qio_wait_writable(fd, timeoutms) <= 0) break;
        ssize_t wsize = write(fd, buf + total, nbytes - total);
        if (wsize <= 0) {
            if (errno == EAGAIN || errno == EINPROGRESS) {
                // possible with non-block io
                usleep(1);
                continue;
            }
            break;
        }
        total += wsize;
    }

    if (total > 0) return total;
    else if (errno == ETIMEDOUT) return 0;
    return -1;
}

/**
 * Transfer data between file descriptors
 *
 * @param outfd       output file descriptor
 * @param infd        input file descriptor
 * @param nbytes      the number of bytes to copy between file descriptors.
 *                    0 means transfer until end of infd.
 * @param timeoutms   wait timeout milliseconds. 0 for no wait, -1 for infinite
 *                    wait
 *
 * @return the number of bytes transferred if successful, 0 on timeout,
 *         -1 for error.
 */
off_t qio_send(int outfd, int infd, off_t nbytes, int timeoutms)
{
    if (nbytes == 0) return 0;

    unsigned char buf[MAX_IOSEND_SIZE];

    off_t total = 0; // total size sent
    while (total < nbytes) {
        size_t chunksize; // this time sending size
        if (nbytes - total <= sizeof(buf)) chunksize = nbytes - total;
        else chunksize = sizeof(buf);

        // read
        ssize_t rsize = qio_read(infd, buf, chunksize, timeoutms);
        DEBUG("read %zd", rsize);
        if (rsize <= 0) break;

        // write
        ssize_t wsize = qio_write(outfd, buf, rsize, timeoutms);
        DEBUG("write %zd", wsize);
        if (wsize <= 0) break;

        total += wsize;
        if (rsize != wsize) {
            DEBUG("size mismatch. read:%zd, write:%zd", rsize, wsize);
            break;
        }
    }

    if (total > 0) return total;
    else if (errno == ETIMEDOUT) return 0;
    return -1;
}

/**
 * Read a line from a file descriptor into the buffer pointed to until either a
 * terminating newline or EOF. New-line characters(CR, LF ) will not be stored
 * into buffer.
 *
 * @param fd      file descriptor
 * @param buf     data buffer pointer
 * @param bufsize     buffer size
 * @param timeoutms   wait timeout milliseconds. 0 for no wait, -1 for infinite
 *                    wait
 *
 * @return the number of bytes read if successful, 0 on timeout, -1 for error.
 *
 * @note
 *  Be sure the return value does not mean the length of actual stored data.
 *  It means how many bytes are readed from the file descriptor,
 *  so the new-line characters will be counted, but not be stored.
 */
ssize_t qio_gets(int fd, char *buf, size_t bufsize, int timeoutms)
{
    if (bufsize <= 1) return -1;

    ssize_t readcnt = 0;
    char *ptr;
    for (ptr = buf; readcnt < (bufsize - 1); ptr++) {
        ssize_t rsize = qio_read(fd, ptr, 1, timeoutms);
        if (rsize != 1) {
            if (errno == EAGAIN || errno == EINPROGRESS) {
                // possible with non-block io
                usleep(1);
                continue;
            }
            break;
        }

        readcnt++;
        if (*ptr == '\r') ptr--;
        else if (*ptr == '\n') break;
    }

    *ptr = '\0';

    if (readcnt > 0) return readcnt;
    else if (errno == ETIMEDOUT) return 0;
    return -1;
}

/**
 * Writes the string and a trailing newline to file descriptor.
 *
 * @param fd        file descriptor
 * @param str       string pointer
 * @param timeoutms wait timeout milliseconds. 0 for no wait, -1 for infinite
 *                  wait
 *
 * @return the number of bytes written including trailing newline characters
 *         if successful, 0 for timeout and -1 for errors.
 */
ssize_t qio_puts(int fd, const char *str, int timeoutms)
{
    size_t strsize = strlen(str);
    char *newstr = (char *)malloc(strsize + 1 + 1);
    if (newstr == NULL) return -1;
    strncpy(newstr, str, strsize);
    newstr[strsize] = '\n';
    newstr[strsize + 1] = '\0';
    ssize_t ret = qio_write(fd, newstr, strsize + 1, timeoutms);
    free(newstr);
    return ret;
}

/**
 * Formatted output to a file descriptor
 *
 * @param fd        file descriptor
 * @param timeoutms wait timeout milliseconds. 0 for no wait, -1 for infinite
 *                  wait
 * @param format    format string
 *
 * @return the number of bytes written including trailing newline characters
 *         if successful, 0 for timeout and -1 for errors.
 */
ssize_t qio_printf(int fd, int timeoutms, const char *format, ...)
{
    char *buf;
    DYNAMIC_VSPRINTF(buf, format);
    if (buf == NULL) return -1;

    ssize_t ret = qio_write(fd, buf, strlen(buf), timeoutms);
    free(buf);

    return ret;
}
