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
 * @file qhttpclient.c HTTP client object.
 *
 * qhttpclient implements HTTP client.
 *
 * Example code for simple HTTP GET operation.
 *
 * @code
 *  #define REMOTE_URL  "/robots.txt"
 *  #define SAVEFILE    "/tmp/robots.txt"
 *
 *  int main(void) {
 *    // create new HTTP client
 *    qhttpclient_t *httpclient = qhttpclient("https://secure.qdecoder.org", 0);
 *    if(httpclient == NULL) return -1;
 *
 *    // open file for writing
 *    int nFd = open(SAVEFILE, O_CREAT | O_TRUNC | O_WRONLY, 0644);
 *    if(nFd < 0) {
 *      httpclient->free(httpclient);
 *      return -1;
 *    }
 *
 *    // container for storing response headers for debugging purpose
 *    qlisttbl_t *resheaders = qlisttbl(QLISTTBL_UNIQUE | QLISTTBL_CASEINSENSITIVE);
 *
 *    // download
 *    off_t nSavesize = 0;
 *    int nRescode = 0;
 *    bool bRet = httpclient->get(httpclient, REMOTE_URL, nFd, &nSavesize,
 *                                &nRescode, NULL, resheaders, NULL, NULL);
 *
 *    // close file
 *    close(nFd);
 *
 *    // print out debugging info
 *    printf("%s %d, %d bytes saved\n", (bRet?"Success":"Failed"), nRescode,
 *                                       (int)nSavesize);
 *    resheaders->debug(resheaders, stdout);
 *
 *    // de-allocate HTTP client object
 *    httpclient->free(httpclient);
 *
 *    return (bRet ? 0 : -1);
 *  }
 *
 *  [Output]
 *  Success 200, 30 bytes saved
 *  Date=Fri, 11 Feb 2011 23:40:50 GMT? (30)
 *  Server=Apache? (7)
 *  Last-Modified=Sun, 15 Mar 2009 11:43:07 GMT? (30)
 *  ETag="2e5c9d-1e-46526d665c8c0"? (26)
 *  Accept-Ranges=bytes? (6)
 *  Content-Length=30? (3)
 *  Cache-Control=max-age=604800? (15)
 *  Expires=Fri, 18 Feb 2011 23:40:50 GMT? (30)
 *  Connection=close? (6)
 *  Content-Type=text/plain? (11)
 * @endcode
 *
 * Example code for multiple PUT operation using same keep-alive connection.
 *
 * @code
 *  // create new HTTP client
 *  qhttpclient_t *httpclient = qhttpclient("www.qdecoder.org", 80);
 *  if(httpclient == NULL) return;
 *
 *  // set options
 *  httpclient->setkeepalive(httpclient, true);
 *
 *  // make a connection
 *  if(httpclient->open(httpclient) == false) return;
 *
 *  // upload files
 *  httpclient->put(httpclient, ...);
 *  httpclient->put(httpclient, ...); // will be done within same connection.
 *
 *  // close connection - not necessary if we call free() just after this.
 *  httpclient->close(httpclient);
 *
 *  // de-allocate HTTP client object
 *  httpclient->free(httpclient);
 * @endcode
 */

#ifndef DISABLE_QHTTPCLIENT

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#ifdef  ENABLE_OPENSSL
#include "openssl/ssl.h"
#include "openssl/err.h"
#endif

#include "qinternal.h"
#include "utilities/qio.h"
#include "utilities/qstring.h"
#include "utilities/qsocket.h"
#include "containers/qlisttbl.h"
#include "containers/qvector.h"
#include "extensions/qhttpclient.h"

#ifndef _DOXYGEN_SKIP

static bool open_(qhttpclient_t *client);
static bool setssl(qhttpclient_t *client);
static void settimeout(qhttpclient_t *client, int timeoutms);
static void setkeepalive(qhttpclient_t *client, bool keepalive);
static void setuseragent(qhttpclient_t *client, const char *agentname);

static bool head(qhttpclient_t *client, const char *uri, int *rescode,
                 qlisttbl_t *reqheaders, qlisttbl_t *resheaders);
static bool get(qhttpclient_t *client, const char *uri, int fd, off_t *savesize,
                int *rescode, qlisttbl_t *reqheaders, qlisttbl_t *resheaders,
                bool (*callback)(void *userdata, off_t recvbytes),
                void *userdata);
static bool put(qhttpclient_t *client, const char *uri, int fd, off_t length,
                int *rescode, qlisttbl_t *reqheaders, qlisttbl_t *resheaders,
                bool (*callback)(void *userdata, off_t sentbytes),
                void *userdata);
static void *cmd(qhttpclient_t *client, const char *method, const char *uri,
                 void *data, size_t size, int *rescode, size_t *contentslength,
                 qlisttbl_t *reqheaders, qlisttbl_t *resheaders);

static bool sendrequest(qhttpclient_t *client, const char *method,
                        const char *uri, qlisttbl_t *reqheaders);
static int readresponse(qhttpclient_t *client, qlisttbl_t *resheaders,
                        off_t *contentlength);

static ssize_t gets_(qhttpclient_t *client, char *buf, size_t bufsize);
static ssize_t read_(qhttpclient_t *client, void *buf, size_t nbytes);
static ssize_t write_(qhttpclient_t *client, const void *buf, size_t nbytes);
static off_t recvfile(qhttpclient_t *client, int fd, off_t nbytes);
static off_t sendfile_(qhttpclient_t *client, int fd, off_t nbytes);

static bool _close(qhttpclient_t *client);
static void _free(qhttpclient_t *client);

// internal usages
static bool _set_socket_option(int socket);
static bool _parse_uri(const char *uri, bool *protocol, char *hostname,
                       size_t namesize, int *port);

#endif

//
// HTTP RESPONSE CODE
//
#define HTTP_NO_RESPONSE                (0)
#define HTTP_CODE_CONTINUE              (100)
#define HTTP_CODE_OK                    (200)
#define HTTP_CODE_CREATED               (201)
#define HTTP_CODE_NO_CONTENT            (204)
#define HTTP_CODE_MULTI_STATUS          (207)
#define HTTP_CODE_MOVED_TEMPORARILY     (302)
#define HTTP_CODE_NOT_MODIFIED          (304)
#define HTTP_CODE_BAD_REQUEST           (400)
#define HTTP_CODE_FORBIDDEN             (403)
#define HTTP_CODE_NOT_FOUND             (404)
#define HTTP_CODE_METHOD_NOT_ALLOWED    (405)
#define HTTP_CODE_REQUEST_TIME_OUT      (408)
#define HTTP_CODE_REQUEST_URI_TOO_LONG  (414)
#define HTTP_CODE_INTERNAL_SERVER_ERROR (500)
#define HTTP_CODE_NOT_IMPLEMENTED       (501)
#define HTTP_CODE_SERVICE_UNAVAILABLE   (503)

#define HTTP_PROTOCOL_11                "HTTP/1.1"

//
// TCP SOCKET DEFINITION
//
#define SET_TCP_LINGER_TIMEOUT  (15)   /*< linger seconds, 0 for disable */
#define SET_TCP_NODELAY         (1)    /*< 0 for disable */
#define MAX_SHUTDOWN_WAIT       (100)  /*< maximum shutdown wait, unit is ms */
#define MAX_ATOMIC_DATA_SIZE    (32 * 1024)  /*< maximum sending bytes */

#ifdef  ENABLE_OPENSSL
struct SslConn {
    SSL *ssl;
    SSL_CTX *ctx;
};
#endif

/**
 * Initialize & create new HTTP client.
 *
 * @param destname  remote address, one of IP address, FQDN domain name and URI.
 * @param port      remote port number. (can be 0 when destname is URI)
 *
 * @return HTTP client object if succcessful, otherwise returns NULL.
 *
 * @code
 *   qhttpclient_t *client = qhttpclient("1.2.3.4", 80);
 *   qhttpclient_t *client = qhttpclient("www.qdecoder.org", 80);
 *   qhttpclient_t *client = qhttpclient("http://www.qdecoder.org", 0);
 *   qhttpclient_t *client = qhttpclient("http://www.qdecoder.org:80", 0);
 *   qhttpclient_t *client = qhttpclient("https://www.qdecoder.org", 0);
 *   qhttpclient_t *client = qhttpclient("https://www.qdecoder.org:443", 0);
 * @endcode
 *
 * @note
 *  Keep-alive feature is turned off by default. Turn it on by calling
 *  setkeepalive(). If destname is URI string starting with
 *  "https://", setssl() will be called internally.
 */
qhttpclient_t *qhttpclient(const char *destname, int port) {
    bool ishttps = false;
    char hostname[256];
    if (port == 0 || strstr(hostname, "://") != NULL) {
        if (_parse_uri(destname, &ishttps, hostname, sizeof(hostname), &port)
                == false) {
            DEBUG("Can't parse URI %s", destname);
            return NULL;
        }

        DEBUG("https: %d, hostname: %s, port:%d\n", ishttps, hostname, port);
    } else {
        qstrcpy(hostname, sizeof(hostname), destname);
    }

    // get remote address
    struct sockaddr_in addr;
    if (qsocket_get_addr(&addr, hostname, port) == false) {
        return NULL;
    }

    // allocate  object
    qhttpclient_t *client = (qhttpclient_t *) malloc(sizeof(qhttpclient_t));
    if (client == NULL)
        return NULL;
    memset((void *) client, 0, sizeof(qhttpclient_t));

    // initialize object
    client->socket = -1;

    memcpy((void *) &client->addr, (void *) &addr, sizeof(client->addr));
    client->hostname = strdup(hostname);
    client->port = port;

    // member methods
    client->setssl = setssl;
    client->settimeout = settimeout;
    client->setkeepalive = setkeepalive;
    client->setuseragent = setuseragent;

    client->open = open_;

    client->head = head;
    client->get = get;
    client->put = put;
    client->cmd = cmd;

    client->sendrequest = sendrequest;
    client->readresponse = readresponse;

    client->gets = gets_;
    client->read = read_;
    client->write = write_;
    client->recvfile = recvfile;
    client->sendfile = sendfile_;

    client->close = _close;
    client->free = _free;

    // init client
    settimeout(client, 0);
    setkeepalive(client, false);
    setuseragent(client, QHTTPCLIENT_NAME);
    if (ishttps == true)
        setssl(client);

    return client;
}

/**
 * qhttpclient->setssl(): Sets connection to HTTPS connection
 *
 * @param client    qhttpclient object pointer
 *
 * @code
 *   httpclient->setssl(httpclient);
 * @endcode
 */
static bool setssl(qhttpclient_t *client) {
#ifdef  ENABLE_OPENSSL
    static bool initialized = false;

    if (client->socket >= 0) {
        // must be set before making a connection.
        return false;
    }
    
    // init openssl
    if (initialized == false) {
      initialized = true;
      SSL_load_error_strings();
      SSL_library_init();
    }

    // allocate ssl structure
    if (client->ssl == NULL) {
        client->ssl = malloc(sizeof(struct SslConn));
        if (client->ssl == NULL) return false;
        memset(client->ssl, 0, sizeof(struct SslConn));
    }

    return true;
#else
    return false;
#endif
}

/**
 * qhttpclient->settimeout(): Sets connection wait timeout.
 *
 * @param client    qhttpclient object pointer
 * @param timeoutms timeout mili-seconds. 0 for system defaults
 *
 * @code
 *   httpclient->settimeout(httpclient, 0);    // default
 *   httpclient->settimeout(httpclient, 5000); // 5 seconds
 * @endcode
 */
static void settimeout(qhttpclient_t *client, int timeoutms) {
    if (timeoutms <= 0)
        timeoutms = -1;
    client->timeoutms = timeoutms;
}

/**
 * qhttpclient->setkeepalive(): Sets KEEP-ALIVE feature on/off.
 *
 * @param client    qhttpclient object pointer
 * @param keepalive true to set keep-alive on, false to set keep-alive off
 *
 * @code
 *   httpclient->setkeepalive(httpclient, true);  // keep-alive on
 *   httpclient->setkeepalive(httpclient, false); // keep-alive off
 * @endcode
 */
static void setkeepalive(qhttpclient_t *client, bool keepalive) {
    client->keepalive = keepalive;
}

/**
 * qhttpclient->setuseragent(): Sets user-agent string.
 *
 * @param client    qhttpclient object pointer
 * @param useragent user-agent string
 *
 * @code
 *   httpclient->setuseragent(httpclient, "MyAgent/1.0");
 * @endcode
 */
static void setuseragent(qhttpclient_t *client, const char *useragent) {
    if (client->useragent != NULL)
        free(client->useragent);
    client->useragent = strdup(useragent);
}

/**
 * qhttpclient->open(): Opens a connection to the remote host.
 *
 * @param client    qhttpclient object pointer
 *
 * @return true if successful, otherwise returns false
 *
 * @note
 *  Don't need to open a connection unless you definitely need to do this,
 *  because qhttpclient open a connection automatically when it's needed.
 *  This function also can be used to veryfy a connection failure with remote
 *  host.
 *
 * @code
 *   if(httpclient->open(httpclient) == false) return;
 * @endcode
 */
static bool open_(qhttpclient_t *client) {
    if (client->socket >= 0) {
        // check if connection is still alive
        if (qio_wait_writable(client->socket, 0) > 0)
            return true;
        _close(client);
    }

    // create new socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        DEBUG("sockfd creation failed.");
        return false;
    }

    // set to non-block socket if timeout is set
    int sockflag = 0;
    if (client->timeoutms > 0) {
        sockflag = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, sockflag | O_NONBLOCK);
    }

    // try to connect
    int status = connect(sockfd, (struct sockaddr *) &client->addr,
                         sizeof(client->addr));
    if (status < 0
            && (errno != EINPROGRESS
                    || qio_wait_writable(sockfd, client->timeoutms) <= 0)) {
        DEBUG("connection failed. (%d)", errno);
        close(sockfd);
        return false;
    }

    // restore to block socket
    if (client->timeoutms > 0) {
        fcntl(sockfd, F_SETFL, sockflag);
    }

    // store socket descriptor
    client->socket = sockfd;

    // set socket option
    _set_socket_option(sockfd);

#ifdef ENABLE_OPENSSL
    // set SSL option
    if (client->ssl != NULL) {
        // get ssl context using SSL 2 or 3
        struct SslConn *ssl = client->ssl;
        ssl->ctx = SSL_CTX_new(SSLv23_client_method());
        if (ssl->ctx == NULL) {
            DEBUG("OpenSSL: %s", ERR_reason_error_string(ERR_get_error()));
            _close(client);
            return false;
        }

        // get ssl handle
        ssl->ssl = SSL_new(ssl->ctx);
        if (ssl->ssl == NULL) {
            DEBUG("OpenSSL: %s", ERR_reason_error_string(ERR_get_error()));
            _close(client);
            return false;
        }

        // map ssl handle with socket
        if (SSL_set_fd(ssl->ssl, client->socket) != 1) {
            DEBUG("OpenSSL: %s", ERR_reason_error_string(ERR_get_error()));
            _close(client);
            return false;
        }

        // set options
        SSL_set_connect_state(ssl->ssl);

        // handshake
        if (SSL_connect(ssl->ssl) != 1) {
            DEBUG("OpenSSL: %s", ERR_reason_error_string(ERR_get_error()));
            _close(client);
            return false;
        }

        DEBUG("ssl initialized");
    }
#endif

    return true;
}

/**
 * qhttpclient->head(): Sends a HEAD request.
 *
 * @param client        qhttpclient object pointer.
 * @param uri           URL encoded remote URI for downloading file.
 *                      ("/path" or "http://.../path")
 * @param rescode       if not NULL, remote response code will be stored.
 *                      (can be NULL)
 * @param reqheaders    qlisttbl_t pointer which contains additional user
 *                      request headers. (can be NULL)
 * @param resheaders    qlisttbl_t pointer for storing response headers.
 *                      (can be NULL)
 *
 * @return true if successful(got 200 response), otherwise returns false
 *
 * @code
 *   main() {
 *     // create new HTTP client
 *     qhttpclient_t *httpclient = qhttpclient("http://www.qdecoder.org", 0);
 *     if(httpclient == NULL) return;
 *
 *     // set additional custom headers
 *     qlisttbl_t *reqheaders = qlisttbl(QLISTTBL_UNIQUE | QLISTTBL_CASEINSENSITIVE);
 *     qlisttbl_t *resheaders = qlisttbl(QLISTTBL_UNIQUE | QLISTTBL_CASEINSENSITIVE);
 *
 *     // send HEAD request
 *     int nRescode = 0;
 *     char *pszEncPath = qEncodeUrl("/img/qdecoder.png");
 *     bool bRet = httpclient->head(httpclient, pszEncPath, &nRescode,
 *                                 reqheaders, resheaders);
 *     free(pszEncPath);
 *
 *     // to print out request, response headers
 *     reqheaders->debug(reqheaders, stdout);
 *     resheaders->debug(resheaders, stdout);
 *
 *     // check results
 *     if(bRet == false) {
 *       ...(error occured)...
 *     }
 *
 *     // free resources
 *     httpclient->free(httpclient);
 *     reqheaders->free(reqheaders);
 *     resheaders->free(resheaders);
 *   }
 * @endcode
 */
static bool head(qhttpclient_t *client, const char *uri, int *rescode,
                 qlisttbl_t *reqheaders, qlisttbl_t *resheaders) {

    // reset rescode
    if (rescode != NULL)
        *rescode = 0;

    // generate request headers if necessary
    bool freeReqHeaders = false;
    if (reqheaders == NULL) {
        reqheaders = qlisttbl(
                QLISTTBL_UNIQUE | QLISTTBL_CASEINSENSITIVE);
        freeReqHeaders = true;
    }

    // add additional headers
    reqheaders->putstr(reqheaders, "Accept", "*/*");

    // send request
    bool sendret = sendrequest(client, "HEAD", uri, reqheaders);
    if (freeReqHeaders == true)
        reqheaders->free(reqheaders);
    if (sendret == false) {
        _close(client);
        return false;
    }

    // read response
    off_t clength = 0;
    int resno = readresponse(client, resheaders, &clength);
    if (rescode != NULL)
        *rescode = resno;

    // throw out content
    if (clength > 0) {
        if (read_(client, NULL, clength) != clength) {
            _close(client);
        }
    }

    // close connection if required
    if (client->keepalive == false || client->connclose == true) {
        _close(client);
    }

    if (resno == HTTP_CODE_OK)
        return true;
    return false;
}

/**
 * qhttpclient->get(): Downloads a file from the remote host using GET
 * method.
 *
 * @param client    qhttpclient object pointer.
 * @param uri       URL encoded remote URI for downloading file.
 *                  ("/path" or "http://.../path")
 * @param fd        opened file descriptor for writing.
 * @param savesize  if not NULL, the length of stored bytes will be stored.
 *                  (can be NULL)
 * @param rescode   if not NULL, remote response code will be stored.
 *                  (can be NULL)
 * @param reqheaders    qlisttbl_t pointer which contains additional user
 *                      request headers. (can be NULL)
 * @param resheaders    qlisttbl_t pointer for storing response headers.
 *                      (can be NULL)
 * @param callback  set user call-back function. (can be NULL)
 * @param userdata  set user data for call-back. (can be NULL)
 *
 * @return true if successful(200 OK), otherwise returns false
 *
 * @code
 *   struct userdata {
 *     ...
 *   };
 *
 *   static bool callback(void *userdata, off_t sentbytes) {
 *     struct userdata *pMydata = (struct userdata*)userdata;
 *     ...(codes)...
 *     if(need_to_cancel) return false; // stop file uploading immediately
 *     return true;
 *   }
 *
 *   main() {
 *     // create new HTTP client
 *     qhttpclient_t *httpclient = qhttpclient("http://www.qdecoder.org", 0);
 *     if(httpclient == NULL) return;
 *
 *     // open file
 *     int nFd = open("/tmp/test.data", O_WRONLY | O_CREAT, 0644);
 *
 *     // set additional custom headers
 *     qlisttbl_t *reqheaders = qlisttbl(QLISTTBL_UNIQUE | QLISTTBL_CASEINSENSITIVE);
 *     qlisttbl_t *resheaders = qlisttbl(QLISTTBL_UNIQUE | QLISTTBL_CASEINSENSITIVE);
 *
 *     // set userdata
 *     struct userdata mydata;
 *     ...(codes)...
 *
 *     // send file
 *     int nRescode = 0;
 *     off_t nSavesize = 0;
 *     char *pszEncPath = qEncodeUrl("/img/qdecoder.png");
 *     bool bRet = httpclient->get(httpclient, pszEncPath, nFd, &nSavesize,
 *                                 &nRescode,
 *                                 reqheaders, resheaders,
 *                                 callback, (void*)&mydata);
 *     free(pszEncPath);
 *
 *     // to print out request, response headers
 *     reqheaders->debug(reqheaders, stdout);
 *     resheaders->debug(resheaders, stdout);
 *
 *     // check results
 *     if(bRet == false) {
 *       ...(error occured)...
 *     }
 *
 *     // free resources
 *     httpclient->free(httpclient);
 *     reqheaders->free(reqheaders);
 *     resheaders->free(resheaders);
 *     close(nFd);
 *   }
 * @endcode
 *
 * @note
 *  The call-back function will be called peridically whenever it send data as
 *  much as MAX_ATOMIC_DATA_SIZE. To stop uploading, return false in the
 *  call-back function, then PUT process will be stopped immediately.
 *  If a connection was not opened, it will open a connection automatically.
 *
 * @note
 *  The "rescode" will be set if it received any response code from a remote
 *  server even though it returns false.
 */
static bool get(qhttpclient_t *client, const char *uri, int fd, off_t *savesize,
                int *rescode, qlisttbl_t *reqheaders, qlisttbl_t *resheaders,
                bool (*callback)(void *userdata, off_t recvbytes),
                void *userdata) {

    // reset rescode
    if (rescode != NULL)
        *rescode = 0;
    if (savesize != NULL)
        *savesize = 0;

    // generate request headers if necessary
    bool freeReqHeaders = false;
    if (reqheaders == NULL) {
        reqheaders = qlisttbl(
                QLISTTBL_UNIQUE | QLISTTBL_CASEINSENSITIVE);
        freeReqHeaders = true;
    }

    // add additional headers
    reqheaders->putstr(reqheaders, "Accept", "*/*");

    // send request
    bool sendret = sendrequest(client, "GET", uri, reqheaders);
    if (freeReqHeaders == true)
        reqheaders->free(reqheaders);
    if (sendret == false) {
        _close(client);
        return false;
    }

    // read response
    off_t clength = 0;
    int resno = readresponse(client, resheaders, &clength);
    if (rescode != NULL)
        *rescode = resno;

    // check response code
    if (resno != HTTP_CODE_OK) {
        // throw out content
        if (clength > 0) {
            if (read_(client, NULL, clength) != clength) {
                _close(client);
            }
        }

        // close connection if required
        if (client->keepalive == false || client->connclose == true) {
            _close(client);
        }
        return false;
    }

    // start retrieving data
    off_t recv = 0;
    if (callback != NULL && callback(userdata, recv) == false) {
        _close(client);
        return false;
    }

    if (clength > 0) {
        while (recv < clength) {
            unsigned int recvsize;  // this time receive size
            if (clength - recv < MAX_ATOMIC_DATA_SIZE) {
                recvsize = clength - recv;
            } else {
                recvsize = MAX_ATOMIC_DATA_SIZE;
            }

            ssize_t ret = recvfile(client, fd, recvsize);
            if (ret <= 0)
                break;  // Connection closed by peer
            recv += ret;
            if (savesize != NULL)
                *savesize = recv;

            if (callback != NULL) {
                if (callback(userdata, recv) == false) {
                    _close(client);
                    return false;
                }
            }
        }

        if (recv != clength) {
            _close(client);
            return false;
        }

    } else if (clength == -1) {  // chunked
        bool completed = false;
        do {
            // read chunk size
            char buf[64];
            if (gets_(client, buf, sizeof(buf)) <= 0)
                break;

            // parse chunk size
            unsigned int recvsize;  // this time chunk size
            sscanf(buf, "%x", &recvsize);
            if (recvsize == 0) {
                // end of transfer
                completed = true;
            } else if (recvsize < 0) {
                // parsing failure
                break;
            }

            // save chunk
            if (recvsize > 0) {
                ssize_t ret = recvfile(client, fd, recvsize);
                if (ret != recvsize)
                    break;
                recv += ret;
                DEBUG("%zd %zd", recv, ret);
                if (savesize != NULL)
                    *savesize = recv;
            }

            // read tailing CRLF
            if (gets_(client, buf, sizeof(buf)) <= 0)
                break;

            // call back
            if (recvsize > 0 && callback != NULL
                    && callback(userdata, recv) == false) {
                _close(client);
                return false;
            }
        } while (completed == false);

        if (completed == false) {
            DEBUG("Broken pipe. %jd/chunked, errno=%d", recv, errno);
            _close(client);
            return false;
        }
    }

    // close connection
    if (client->keepalive == false || client->connclose == true) {
        _close(client);
    }

    return true;
}

/**
 * qhttpclient->put(): Uploads a file to the remote host using PUT method.
 *
 * @param client    qhttpclient object pointer.
 * @param uri       remote URL for uploading file.
 *                  ("/path" or "http://.../path")
 * @param fd        opened file descriptor for reading.
 * @param length    send size.
 * @param rescode   if not NULL, remote response code will be stored.
 *                  (can be NULL)
 * @param reqheaders    qlisttbl_t pointer which contains additional user
 *                      request headers. (can be NULL)
 * @param resheaders    qlisttbl_t pointer for storing response headers.
 *                      (can be NULL)
 * @param callback  set user call-back function. (can be NULL)
 * @param userdata  set user data for call-back. (can be NULL)
 *
 * @return true if successful(201 Created), otherwise returns false
 *
 * @code
 *   struct userdata {
 *     ...
 *   };
 *
 *   static bool callback(void *userdata, off_t sentbytes) {
 *     struct userdata *pMydata = (struct userdata*)userdata;
 *     ...(codes)...
 *     if(need_to_cancel) return false; // stop file uploading immediately
 *     return true;
 *   }
 *
 *   main() {
 *     // create new HTTP client
 *     qhttpclient_t *httpclient = qhttpclient("http://www.qdecoder.org", 0);
 *     if(httpclient == NULL) return;
 *
 *     // open file
 *     int nFd = open(...);
 *     off_t nFileSize = ...;
 *     char *pFileMd5sum = ...;
 *     time_t nFileDate = ...;
 *
 *     // set additional custom headers
 *     qlisttbl_t *reqheaders = qlisttbl(QLISTTBL_UNIQUE | QLISTTBL_CASEINSENSITIVE);
 *     reqheaders->putstr(reqheaders, "X-FILE-MD5SUM", pFileMd5sum);
 *     reqheaders->putInt(reqheaders, "X-FILE-DATE", nFileDate);
 *
 *     // set userdata
 *     struct userdata mydata;
 *     ...(codes)...
 *
 *     // send file
 *     int nRescode = 0;
 *     qlisttbl_t *resheaders = qlisttbl(QLISTTBL_UNIQUE | QLISTTBL_CASEINSENSITIVE);
 *     bool bRet = httpclient->put(httpclient,
 *                                 "/img/qdecoder.png", nFd, nFileSize,
 *                                 &nRescode,
 *                                 reqheaders, resheaders,
 *                                 callback, (void*)&mydata);
 *     // to print out request, response headers
 *     reqheaders->debug(reqheaders, stdout);
 *     resheaders->debug(resheaders, stdout);
 *
 *     // check results
 *     if(bRet == false) {
 *       ...(error occured)...
 *     }
 *
 *     // free resources
 *     httpclient->free(httpclient);
 *     reqheaders->free(reqheaders);
 *     resheaders->free(resheaders);
 *     close(nFd);
 *   }
 * @endcode
 *
 * @note
 *  The call-back function will be called peridically whenever it send data as
 *  much as MAX_ATOMIC_DATA_SIZE. To stop uploading, return false in the
 *  call-back function, then PUT process will be stopped immediately.
 *  If a connection was not opened, it will open a connection automatically.
 *
 * @note
 *  The "rescode" will be set if it received any response code from a remote
 *  server even though it returns false.
 */
static bool put(qhttpclient_t *client, const char *uri, int fd, off_t length,
                int *rescode, qlisttbl_t *reqheaders, qlisttbl_t *resheaders,
                bool (*callback)(void *userdata, off_t sentbytes),
                void *userdata) {

    // reset rescode
    if (rescode != NULL)
        *rescode = 0;

    // generate request headers
    bool freeReqHeaders = false;
    if (reqheaders == NULL) {
        reqheaders = qlisttbl(
                QLISTTBL_UNIQUE | QLISTTBL_CASEINSENSITIVE);
        freeReqHeaders = true;
    }

    // add additional headers
    reqheaders->putstrf(reqheaders, "Content-Length", "%jd", length);
    reqheaders->putstr(reqheaders, "Expect", "100-continue");

    // send request
    bool sendret = sendrequest(client, "PUT", uri, reqheaders);
    if (freeReqHeaders == true) {
        reqheaders->free(reqheaders);
        reqheaders = NULL;
    }
    if (sendret == false) {
        _close(client);
        return false;
    }

    // wait 100-continue
    if (qio_wait_readable(client->socket, client->timeoutms) <= 0) {
        DEBUG("timed out %d", client->timeoutms);
        _close(client);
        return false;
    }

    // read response
    off_t clength = 0;
    int resno = readresponse(client, resheaders, &clength);
    if (resno != HTTP_CODE_CONTINUE) {
        if (rescode != NULL)
            *rescode = resno;

        if (clength > 0) {
            if (read_(client, NULL, clength) != clength) {
                _close(client);
            }
        }

        // close connection if required
        if (client->keepalive == false || client->connclose == true) {
            _close(client);
        }
        return false;
    }

    // send data
    off_t sent = 0;
    if (callback != NULL) {
        if (callback(userdata, sent) == false) {
            _close(client);
            return false;
        }
    }
    if (length > 0) {
        while (sent < length) {
            size_t sendsize;    // this time sending size
            if (length - sent < MAX_ATOMIC_DATA_SIZE)
                sendsize = length - sent;
            else
                sendsize = MAX_ATOMIC_DATA_SIZE;

            ssize_t ret = sendfile_(client, fd, sendsize);
            if (ret <= 0)
                break;  // Connection closed by peer
            sent += ret;

            if (callback != NULL) {
                if (callback(userdata, sent) == false) {
                    _close(client);
                    return false;
                }
            }
        }

        if (sent != length) {
            _close(client);
            return false;
        }

        if (callback != NULL) {
            if (callback(userdata, sent) == false) {
                _close(client);
                return false;
            }
        }
    }

    // read response
    clength = 0;
    resno = readresponse(client, resheaders, &clength);
    if (rescode != NULL)
        *rescode = resno;

    if (resno == HTTP_NO_RESPONSE) {
        _close(client);
        return false;
    }

    if (clength > 0) {
        if (read_(client, NULL, clength) != clength) {
            _close(client);
        }
    }

    // close connection
    if (client->keepalive == false || client->connclose == true) {
        _close(client);
    }

    if (resno == HTTP_CODE_CREATED)
        return true;
    return false;
}

/**
 * qhttpclient->cmd(): Sends a custom request(method) to the remote host
 * and reads it's response.
 *
 * @param client    qhttpclient object pointer.
 * @param method    method name.
 * @param uri       remote URL for uploading file.
 *                  ("/path" or "http://.../path")
 * @param data      data to send. (can be NULL)
 * @param size      data size.
 * @param rescode   if not NULL, remote response code will be stored.
 *                  (can be NULL)
 * @param contentslength if not NULL, the contents length will be stored.
 *                       (can be NULL)
 * @param reqheaders    qlisttbl_t pointer which contains additional user
 *                      request headers. (can be NULL)
 * @param resheaders    qlisttbl_t pointer for storing response headers.
 *                      (can be NULL)
 *
 * @return malloced content data if successful, otherwise returns NULL
 *
 * @code
 *   int nResCode;
 *   size_t nContentsLength;
 *   void *contents = httpclient->cmd(httpclient, "DELETE" "/img/qdecoder.png",
 *                                    NULL, 0
 *                                    &nRescode, &nContentsLength
 *                                    NULL, NULL);
 *   if(contents == NULL) {
 *     ...(error occured)...
 *   } else {
 *     printf("Response code : %d\n", nResCode);
 *     printf("Contents length : %zu\n", nContentsLength);
 *     printf("Contents : %s\n", (char*)contents);  // if contents is printable
 *     free(contents);  // de-allocate
 *   }
 * @endcode
 *
 * @note
 *  This store server's response into memory so if you expect server responses
 *  large amount of data, consider to use sendrequest() and readresponse()
 *  instead of using this. The returning malloced content will be allocated
 *  +1 byte than actual content size 'contentslength' and will be null
 *  terminated.
 */
static void *cmd(qhttpclient_t *client, const char *method, const char *uri,
                 void *data, size_t size, int *rescode, size_t *contentslength,
                 qlisttbl_t *reqheaders, qlisttbl_t *resheaders) {

    // reset rescode
    if (rescode != NULL)
        *rescode = 0;
    if (contentslength != NULL)
        *contentslength = 0;

    // send request
    bool freeReqHeaders = false;
    if (reqheaders == NULL && data != NULL && size > 0) {
        reqheaders = qlisttbl(
                QLISTTBL_UNIQUE | QLISTTBL_CASEINSENSITIVE);
        reqheaders->putstrf(reqheaders, "Content-Length", "%jd", size);
        freeReqHeaders = true;
    }

    bool sendret = sendrequest(client, method, uri, reqheaders);
    if (freeReqHeaders == true) {
        reqheaders->free(reqheaders);
        reqheaders = NULL;
    }
    if (sendret == false) {
        _close(client);
        return NULL;
    }

    // send data
    if (data != NULL && size > 0) {
        ssize_t written = write_(client, data, size);
        if (written != size) {
            _close(client);
            return NULL;
        }
    }

    // read response
    off_t clength = 0;
    int resno = readresponse(client, resheaders, &clength);
    if (rescode != NULL)
        *rescode = resno;
    if (contentslength != NULL)
        *contentslength = clength;

    // malloc data
    void *content = NULL;
    if (clength > 0) {
        content = malloc(clength + 1);
        if (content != NULL) {
            if (read_(client, content, clength) == clength) {
                *(char *) (content + clength) = '\0';
            } else {
                free(content);
                content = NULL;
                _close(client);
            }
        }
    } else {
        // succeed. to distinguish between ok and error
        content = strdup("");
    }

    // close connection
    if (client->keepalive == false || client->connclose == true) {
        _close(client);
    }

    return content;
}

/**
 * qhttpclient->sendrequest(): Sends a HTTP request to the remote host.
 *
 * @param client    qhttpclient object pointer
 * @param method    HTTP method name
 * @param uri       URI string for the method. ("/path" or "http://.../path")
 * @param reqheaders    qlisttbl_t pointer which contains additional user
 *                      request headers. (can be NULL)
 *
 * @return true if successful, otherwise returns false
 *
 * @note
 *  Default headers(Host, User-Agent, Connection) will be used if reqheaders
 *  does not have those headers in it.
 *
 * @code
 *   qlisttbl_t *reqheaders = qlisttbl(QLISTTBL_UNIQUE | QLISTTBL_CASEINSENSITIVE);
 *   reqheaders->putstr(reqheaders,  "Date", qTimeGetGmtStaticStr(0), true);
 *
 *   httpclient->sendrequest(client,
 *                           "DELETE", "/img/qdecoder.png", reqheaders);
 * @endcode
 */
static bool sendrequest(qhttpclient_t *client, const char *method,
                        const char *uri, qlisttbl_t *reqheaders) {
    if (open_(client) == false) {
        return false;
    }

    // generate request headers if necessary
    bool freeReqHeaders = false;
    if (reqheaders == NULL) {
        reqheaders = qlisttbl(
                QLISTTBL_UNIQUE | QLISTTBL_CASEINSENSITIVE);
        if (reqheaders == NULL)
            return false;
        freeReqHeaders = true;
    }

    // append default headers
    if (reqheaders->get(reqheaders, "Host", NULL, false) == NULL) {
        reqheaders->putstrf(reqheaders, "Host", "%s:%d", client->hostname,
                            client->port);
    }
    if (reqheaders->get(reqheaders, "User-Agent", NULL, false) == NULL) {
        reqheaders->putstr(reqheaders, "User-Agent", client->useragent);
    }
    if (reqheaders->get(reqheaders, "Connection", NULL, false) == NULL) {
        reqheaders->putstr(
                reqheaders, "Connection",
                (client->keepalive == true) ? "Keep-Alive" : "close");
    }

    // create stream buffer
    qvector_t *outBuf = qvector(0);
    if (outBuf == NULL)
        return false;

    // buffer out command
    outBuf->addstrf(outBuf, "%s %s %s\r\n", method, uri,
    HTTP_PROTOCOL_11);

    // buffer out headers
    qdlnobj_t obj;
    memset((void *) &obj, 0, sizeof(obj));  // must be cleared before call
    reqheaders->lock(reqheaders);
    while (reqheaders->getnext(reqheaders, &obj, NULL, false) == true) {
        outBuf->addstrf(outBuf, "%s: %s\r\n", obj.name, (char *) obj.data);
    }
    reqheaders->unlock(reqheaders);

    outBuf->addstrf(outBuf, "\r\n");

    // stream out
    size_t towrite = 0;
    char *final = outBuf->toarray(outBuf, &towrite);
    ssize_t written = 0;
    if (final != NULL) {
        written = write_(client, final, towrite);
        free(final);
    }

    // de-allocate
    outBuf->free(outBuf);
    if (freeReqHeaders == true)
        reqheaders->free(reqheaders);

    if (written > 0 && written == towrite)
        return true;
    return false;
}

/**
 * qhttpclient->readresponse(): Reads HTTP response header from the
 * remote host.
 *
 * @param client        qhttpclient object pointer
 * @param resheaders    qlisttbl_t pointer for storing response headers.
 *                      (can be NULL)
 * @param contentlength length of content body(or -1 for chunked transfer
 *                      encoding) will be stored. (can be NULL)
 *
 * @return numeric HTTP response code if successful, otherwise returns 0.
 *
 * @code
 *   // send request
 *   httpclient->sendrequest(client, "DELETE", "/img/qdecoder.png", NULL);
 *
 *   // read response
 *   qlisttbl_t *resheaders = qlisttbl(QLISTTBL_UNIQUE | QLISTTBL_CASEINSENSITIVE);
 *   off_t clength;
 *   int rescode = httpclient->readresponse(client, resheaders, &clength);
 *   if(clength > 0) {
 *     // read & throw out a content. don't need content
 *     httpclient->read(client, NULL, clength);
 *   }
 * @endcode
 *
 * @note
 *  Data of content body must be read by a application side, if you want to use
 *  Keep-Alive session. Please refer qhttpclient->read().
 */
static int readresponse(qhttpclient_t *client, qlisttbl_t *resheaders,
                        off_t *contentlength) {
    if (contentlength != NULL) {
        *contentlength = 0;
    }

    // read response
    char buf[1024];
    if (gets_(client, buf, sizeof(buf)) <= 0)
        return HTTP_NO_RESPONSE;

    // parse response code
    if (strncmp(buf, "HTTP/", CONST_STRLEN("HTTP/")))
        return HTTP_NO_RESPONSE;
    char *tmp = strstr(buf, " ");
    if (tmp == NULL)
        return HTTP_NO_RESPONSE;
    int rescode = atoi(tmp + 1);
    if (rescode == 0)
        return HTTP_NO_RESPONSE;

    // read headers
    while (gets_(client, buf, sizeof(buf)) > 0) {
        if (buf[0] == '\0')
            break;

        // parse header
        char *name = buf;
        char *value = strstr(buf, ":");
        if (value != NULL) {
            *value = '\0';
            value += 1;
            qstrtrim(value);
        } else {
            // missing colon
            value = "";
        }

        if (resheaders != NULL) {
            resheaders->putstr(resheaders, name, value);
        }

        // check Connection header
        if (!strcasecmp(name, "Connection")) {
            if (!strcasecmp(value, "close")) {
                client->connclose = true;
            }
        }
        // check Content-Length & Transfer-Encoding header
        else if (contentlength != NULL && *contentlength == 0) {
            if (!strcasecmp(name, "Content-Length")) {
                *contentlength = atoll(value);
            }
            // check transfer-encoding header
            else if (!strcasecmp(name, "Transfer-Encoding")
                    && !strcasecmp(value, "chunked")) {
                *contentlength = -1;
            }
        }
    }

    return rescode;
}

/**
 * qhttpclient->gets(): Reads a text line from a HTTP/HTTPS stream.
 *
 * @param   client      qhttpclient object pointer
 * @param   buf         data buffer pointer
 * @param   bufsize     buffer size
 *
 * @return the number of bytes read from file descriptor if successful,
 *         otherwise returns -1.
 *
 * @note
 *  Be sure the return value does not mean the length of actual stored data.
 *  It means how many bytes are read from the file descriptor, so the new-line
 *  characters will be counted, but not stored.
 */
static ssize_t gets_(qhttpclient_t *client, char *buf, size_t bufsize) {
#ifdef ENABLE_OPENSSL
    if (client->ssl == NULL) {
        return qio_gets(client->socket, buf, bufsize, client->timeoutms);
    } else {
        if (bufsize <= 1) return -1;

        struct SslConn *ssl = client->ssl;
        ssize_t readcnt = 0;
        char *ptr;

        for (ptr = buf; readcnt < (bufsize - 1); ptr++) {
            // wait readable
            //if (qio_wait_readable(client->socket, client->timeoutms) <= 0) {
            //    break;
            //}

            int rsize = SSL_read(ssl->ssl, ptr, 1);
            if (rsize != 1) {
                unsigned long sslerr = ERR_get_error();
                if (sslerr == SSL_ERROR_WANT_READ) {
                    continue;
                }

                DEBUG("OpenSSL: %s (%d)",
                        ERR_reason_error_string(sslerr), rsize);
                break;
            }

            readcnt++;
            if (*ptr == '\r') ptr--;
            else if (*ptr == '\n') break;
        }

        *ptr = '\0';
        DEBUG("SSL_read: %s (%zd)", buf, readcnt);

        if (readcnt > 0) return readcnt;
        return -1;
    }
#else
    return qio_gets(client->socket, buf, bufsize, client->timeoutms);
#endif
}

/**
 * qhttpclient->read(): Reads data from a HTTP/HTTPS stream.
 *
 * @param client    qhttpclient object pointer.
 * @param buf       a buffer pointer for storing content. (can be NULL, then
 *                  read & throw out content)
 * @param length    content size to read.
 *
 * @return number of bytes readed
 *
 * @code
 *   off_t clength = 0;
 *   int resno = client->readresponse(client, NULL, &clength);
 *   if(clength > 0) {
 *     void *buf = malloc(clength);
 *     client->read(client, buf, clength);
 *   }
 * @endcode
 */
static ssize_t read_(qhttpclient_t *client, void *buf, size_t nbytes) {
#ifdef ENABLE_OPENSSL
    if (client->ssl == NULL) {
        return qio_read(client->socket, buf, nbytes, client->timeoutms);
    } else {
        if (nbytes == 0) return 0;

        struct SslConn *ssl = client->ssl;
        ssize_t total = 0;
        while (total < nbytes) {
            //if (qio_wait_readable(client->socket, client->timeoutms) <= 0) {
            //    break;
            //}

            int rsize = 0;
            if (buf != NULL) {
                rsize = SSL_read(ssl->ssl, buf + total, nbytes - total);
            } else {
                char trash[1024];
                int toread = nbytes - total;
                if (toread > sizeof(trash)) toread = sizeof(trash);
                rsize = SSL_read(ssl->ssl, trash, toread);
            }
            if (rsize <= 0) {
                DEBUG("OpenSSL: %s (%d)",
                        ERR_reason_error_string(ERR_get_error()), rsize);
                unsigned long sslerr = ERR_get_error();
                if (sslerr == SSL_ERROR_WANT_READ) {
                    usleep(1);
                    continue;
                }
                break;
            }
            total += rsize;
        }

        DEBUG("SSL_read: %zd", total);
        if (total > 0) return total;
        return -1;
    }
#else
    return qio_read(client->socket, buf, nbytes, client->timeoutms);
#endif
}

/**
 * qhttpclient->write(): Writes data to a HTTP/HTTPS stream.
 *
 * @param client    qhttpclient object pointer.
 * @param buf       a data pointer.
 * @param length    content size to write.
 *
 * @return number of bytes written.
 */
static ssize_t write_(qhttpclient_t *client, const void *buf, size_t nbytes) {
#ifdef ENABLE_OPENSSL
    if (client->ssl == NULL) {
        return qio_write(client->socket, buf, nbytes, -1);
    } else {
        if (nbytes == 0) return 0;

        struct SslConn *ssl = client->ssl;
        ssize_t total = 0;
        while (total < nbytes) {
            errno = 0;
            int wsize = SSL_write(ssl->ssl, buf + total, nbytes - total);
            if (wsize <= 0) {
                DEBUG("OpenSSL: %s (%d)",
                        ERR_reason_error_string(ERR_get_error()), wsize);
                unsigned long sslerr = ERR_get_error();
                if (sslerr == SSL_ERROR_WANT_WRITE) {
                    usleep(1);
                    continue;
                }
                break;
            }
            total += wsize;
        }

        DEBUG("SSL_write: %zd/%zu", total, nbytes);
        if (total > 0) return total;
        return -1;
    }
#else
    return qio_write(client->socket, buf, nbytes, -1);
#endif
}

/**
 * qhttpclient->recvfile(): Reads data from a HTTP/HTTPS stream and save
 * into a file descriptor.
 *
 * @param   client      qhttpclient object pointer.
 * @param   fd          output file descriptor
 * @param   nbytes      the number of bytes to read and save.
 *
 * @return the number of bytes written if successful, otherwise returns -1.
 */
static off_t recvfile(qhttpclient_t *client, int fd, off_t nbytes) {
    if (nbytes == 0)
        return 0;

    unsigned char buf[MAX_ATOMIC_DATA_SIZE];

    off_t total = 0;  // total size sent
    while (total < nbytes) {
        size_t chunksize;  // this time sending size
        if (nbytes - total <= sizeof(buf))
            chunksize = nbytes - total;
        else
            chunksize = sizeof(buf);

        // read
        ssize_t rsize = read_(client, buf, chunksize);
        if (rsize <= 0)
            break;

        // write
        ssize_t wsize = qio_write(fd, buf, rsize, -1);
        DEBUG("FILE write: %zd", wsize);
        if (wsize <= 0)
            break;

        total += wsize;
        if (rsize != wsize) {
            DEBUG("size mismatch. read:%zd, write:%zd", rsize, wsize);
            break;
        }
    }

    if (total > 0)
        return total;
    return -1;
}

/**
 * qhttpclient->sendfile(): Send file data to a HTTP/HTTPS stream.
 *
 * @param   client      qhttpclient object pointer.
 * @param   fd          input file descriptor
 * @param   nbytes      the number of bytes to read and send.
 *
 * @return the number of bytes sent if successful, otherwise returns -1.
 */
static off_t sendfile_(qhttpclient_t *client, int fd, off_t nbytes) {
    if (nbytes == 0)
        return 0;

    unsigned char buf[MAX_ATOMIC_DATA_SIZE];

    off_t total = 0;  // total size sent
    while (total < nbytes) {
        size_t chunksize;  // this time sending size
        if (nbytes - total <= sizeof(buf))
            chunksize = nbytes - total;
        else
            chunksize = sizeof(buf);

        // read
        ssize_t rsize = qio_read(fd, buf, chunksize, -1);
        DEBUG("FILE read: %zd", rsize);
        if (rsize <= 0)
            break;

        // write
        ssize_t wsize = write_(client, buf, rsize);
        if (wsize <= 0)
            break;

        total += wsize;
        if (rsize != wsize) {
            DEBUG("size mismatch. read:%zd, write:%zd", rsize, wsize);
            break;
        }
    }

    if (total > 0)
        return total;
    return -1;
}

/**
 * qhttpclient->close(): Closes the connection.
 *
 * @param qhttpclient_t  HTTP object pointer
 *
 * @return true if successful, otherwise returns false
 *
 * @code
 *   httpclient->close(httpclient);
 * @endcode
 */
static bool _close(qhttpclient_t *client) {
    if (client->socket < 0)
        return false;

#ifdef ENABLE_OPENSSL
    // release ssl connection
    if (client->ssl != NULL) {
        struct SslConn *ssl = client->ssl;

        if (ssl->ssl != NULL) {
            SSL_shutdown(ssl->ssl);
            SSL_free(ssl->ssl);
            ssl->ssl = NULL;
        }

        if (ssl->ctx != NULL) {
            SSL_CTX_free(ssl->ctx);
            ssl->ctx = NULL;
        }
    }
#endif

    // shutdown connection
    if (client->ssl == NULL && MAX_SHUTDOWN_WAIT >= 0
            && shutdown(client->socket, SHUT_WR) == 0) {
        char buf[1024];
        while (qio_read(client->socket, buf, sizeof(buf), MAX_SHUTDOWN_WAIT) > 0);
    }

    // close connection
    close(client->socket);
    client->socket = -1;
    client->connclose = false;

    return true;
}

/**
 * qhttpclient->free(): Free object.
 *
 * @param qhttpclient_t  HTTP object pointer
 *
 * @note
 *  If the connection was not closed, it will close the connection first prior
 *  to de-allocate object.
 *
 * @code
 *   httpclient->free(httpclient);
 * @endcode
 */
static void _free(qhttpclient_t *client) {
    if (client->socket >= 0) {
        client->close(client);
    }

    if (client->ssl != NULL)
        free(client->ssl);
    if (client->hostname != NULL)
        free(client->hostname);
    if (client->useragent != NULL)
        free(client->useragent);

    free(client);
}

#ifndef _DOXYGEN_SKIP
static bool _set_socket_option(int socket) {
    bool ret = true;

    // linger option
    if (SET_TCP_LINGER_TIMEOUT > 0) {
        struct linger li;
        li.l_onoff = 1;
        li.l_linger = SET_TCP_LINGER_TIMEOUT;
        if (setsockopt(socket, SOL_SOCKET, SO_LINGER, &li,
                       sizeof(struct linger)) < 0) {
            ret = false;
        }
    }

    // nodelay option
    if (SET_TCP_NODELAY > 0) {
        int so_tcpnodelay = 1;
        if (setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &so_tcpnodelay,
                       sizeof(so_tcpnodelay)) < 0) {
            ret = false;
        }
    }

    return ret;
}

static bool _parse_uri(const char *uri, bool *protocol, char *hostname,
                       size_t namesize, int *port) {

    if (!strncasecmp(uri, "http://", CONST_STRLEN("http://"))) {
        *protocol = false;
        *port = 80;
    } else if (!strncasecmp(uri, "https://", CONST_STRLEN("https://"))) {
        *protocol = true;
        *port = 443;
    } else {
        return false;
    }

    char *t1 = strstr(uri, "://");
    t1 += 3;
    char *t2 = strstr(t1, "/");
    if (t2 == NULL)
        t2 = (char *) uri + strlen(uri);

    if (t2 - t1 + 1 > namesize)
        return false;
    qstrncpy(hostname, namesize, t1, t2 - t1);

    t1 = strstr(hostname, ":");
    if (t1 != NULL) {
        *t1 = '\0';
        *port = atoi(t1 + 1);
    }

    return true;
}
#endif /* _DOXYGEN_SKIP */

#endif /* DISABLE_QHTTPCLIENT */
