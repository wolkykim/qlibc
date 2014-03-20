/******************************************************************************
 * qLibc - http://www.qdecoder.org
 *
 * Copyright (c) 2010-2012 Seungyoung Kim.
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
 ******************************************************************************
 * $Id: qlibc.h 55 2012-04-03 03:15:00Z seungyoung.kim $
 ******************************************************************************/

/**
 * HTTP client.
 *
 * This is a qLibc extension implementing HTTP client.
 *
 * @file qhttpclient.h
 */

#ifndef _QHTTPCLIENT_H
#define _QHTTPCLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "qlibc.h"

/* types */
typedef struct qhttpclient_s  qhttpclient_t;

/* public functions */
extern qhttpclient_t *qhttpclient(const char *hostname, int port);

/**
 * qhttpclient structure
 */

struct qhttpclient_s {
    /* capsulated member functions */
    bool (*setssl) (qhttpclient_t *client);
    void (*settimeout) (qhttpclient_t *client, int timeoutms);
    void (*setkeepalive) (qhttpclient_t *client, bool keepalive);
    void (*setuseragent) (qhttpclient_t *client, const char *useragent);

    bool (*open) (qhttpclient_t *client);

    bool (*head) (qhttpclient_t *client, const char *uri, int *rescode,
                  qlisttbl_t *reqheaders, qlisttbl_t *resheaders);
    bool (*get) (qhttpclient_t *client, const char *uri, int fd,
                 off_t *savesize, int *rescode,
                 qlisttbl_t *reqheaders, qlisttbl_t *resheaders,
                 bool (*callback) (void *userdata, off_t recvbytes),
                 void *userdata);
    bool (*put) (qhttpclient_t *client, const char *uri, int fd,
                 off_t length, int *retcode, qlisttbl_t *userheaders,
                 qlisttbl_t *resheaders,
                 bool (*callback) (void *userdata, off_t sentbytes),
                 void *userdata);
    void *(*cmd) (qhttpclient_t *client,
                  const char *method, const char *uri,
                  void *data, size_t size, int *rescode,
                  size_t *contentslength,
                  qlisttbl_t *reqheaders, qlisttbl_t *resheaders);

    bool (*sendrequest) (qhttpclient_t *client, const char *method,
                         const char *uri, qlisttbl_t *reqheaders);
    int (*readresponse) (qhttpclient_t *client, qlisttbl_t *resheaders,
                         off_t *contentlength);

    ssize_t (*gets) (qhttpclient_t *client, char *buf, size_t bufsize);
    ssize_t (*read) (qhttpclient_t *client, void *buf, size_t nbytes);
    ssize_t (*write) (qhttpclient_t *client, const void *buf,
                      size_t nbytes);
    off_t (*recvfile) (qhttpclient_t *client, int fd, off_t nbytes);
    off_t (*sendfile) (qhttpclient_t *client, int fd, off_t nbytes);

    bool (*close) (qhttpclient_t *client);
    void (*free) (qhttpclient_t *client);

    /* private variables - do not access directly */
    int socket;  /*!< socket descriptor */
    void *ssl;   /*!< will be used if SSL has been enabled at compile time */

    struct sockaddr_in addr;
    char *hostname;
    int port;

    int timeoutms;    /*< wait timeout miliseconds*/
    bool keepalive;   /*< keep-alive flag */
    char *useragent;  /*< user-agent name */

    bool connclose;   /*< response keep-alive flag for a last request */
};

#ifdef __cplusplus
}
#endif

#endif /*_QHTTPCLIENT_H */
