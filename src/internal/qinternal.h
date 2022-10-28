/******************************************************************************
 * qLibc
 *
 * Copyright (c) 2010-2015 Seungyoung Kim.
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

#ifndef QINTERNAL_H
#define QINTERNAL_H

/*
 * Macro Functions
 */
#define ASSERT(c)           assert(c)
#define CONST_STRLEN(s)     (sizeof(s) - 1)
#define IS_EMPTY_STR(s)     ((*s == '\0') ? true : false)
#define ENDING_CHAR(s)      (*(s + strlen(s) - 1))

#define DYNAMIC_VSPRINTF(s, f) do {                                     \
        size_t _strsize;                                                \
        for(_strsize = 1024; ; _strsize *= 2) {                         \
            s = (char*)malloc(_strsize);                                \
            if(s == NULL) {                                             \
                DEBUG("DYNAMIC_VSPRINTF(): can't allocate memory.");    \
                break;                                                  \
            }                                                           \
            va_list _arglist;                                           \
            va_start(_arglist, f);                                      \
            int _n = vsnprintf(s, _strsize, f, _arglist);               \
            va_end(_arglist);                                           \
            if(_n >= 0 && _n < _strsize) break;                         \
            free(s);                                                    \
        }                                                               \
    } while(0)

/*
 * Q_MUTEX Macros
 */
#ifndef _MULTI_THREADED
#define _MULTI_THREADED
#endif

#include <unistd.h>
#include <pthread.h>

typedef struct qmutex_s qmutex_t;    /*!< qlibc pthread mutex type*/

struct qmutex_s {
    pthread_mutex_t mutex;  /*!< pthread mutex */
    pthread_t owner;        /*!< mutex owner thread id */
    int count;              /*!< recursive lock counter */
};

#define Q_MUTEX_NEW(m,r) do {                                           \
        qmutex_t *x = (qmutex_t *)calloc(1, sizeof(qmutex_t));          \
        pthread_mutexattr_t _mutexattr;                                 \
        pthread_mutexattr_init(&_mutexattr);                            \
        if(r == true) {                                                 \
            pthread_mutexattr_settype(&_mutexattr, PTHREAD_MUTEX_RECURSIVE); \
        }                                                               \
        int _ret = pthread_mutex_init(&(x->mutex), &_mutexattr);        \
        pthread_mutexattr_destroy(&_mutexattr);                         \
        if(_ret == 0) {                                                 \
            m = x;                                                      \
        } else {                                                        \
            DEBUG("Q_MUTEX: can't initialize mutex. [%d]", _ret);       \
            free(x);                                                    \
            m = NULL;                                                   \
        }                                                               \
    } while(0)

#define Q_MUTEX_LEAVE(m) do {                                           \
        if(m == NULL) break;                                            \
        if(!pthread_equal(((qmutex_t *)m)->owner, pthread_self())) {    \
            DEBUG("Q_MUTEX: unlock - owner mismatch.");                 \
        }                                                               \
        if((((qmutex_t *)m)->count--) < 0) ((qmutex_t *)m)->count = 0;  \
        pthread_mutex_unlock(&(((qmutex_t *)m)->mutex));                \
    } while(0)

#define MAX_MUTEX_LOCK_WAIT (5000)
#define Q_MUTEX_ENTER(m) do {                                           \
        if(m == NULL) break;                                            \
        while(true) {                                                   \
            int _ret, i;                                                \
            for(i = 0; (_ret = pthread_mutex_trylock(&(((qmutex_t *)m)->mutex))) != 0 \
                        && i < MAX_MUTEX_LOCK_WAIT; i++) {              \
                if(i == 0) {                                            \
                    DEBUG("Q_MUTEX: mutex is already locked - retrying"); \
                }                                                       \
                usleep(1);                                              \
            }                                                           \
            if(_ret == 0) break;                                        \
            DEBUG("Q_MUTEX: can't get lock - force to unlock. [%d]",    \
                  _ret);                                                \
            Q_MUTEX_LEAVE(m);                                           \
        }                                                               \
        ((qmutex_t *)m)->count++;                                       \
        ((qmutex_t *)m)->owner = pthread_self();                        \
    } while(0)

#define Q_MUTEX_DESTROY(m) do {                                         \
        if(m == NULL) break;                                            \
        if(((qmutex_t *)m)->count != 0) DEBUG("Q_MUTEX: mutex counter is not 0."); \
        int _ret;                                                       \
        while((_ret = pthread_mutex_destroy(&(((qmutex_t *)m)->mutex))) != 0) { \
            DEBUG("Q_MUTEX: force to unlock mutex. [%d]", _ret);        \
            Q_MUTEX_LEAVE(m);                                           \
        }                                                               \
        free(m);                                                        \
    } while(0)

/*
 * Debug Macros
 */
#ifdef BUILD_DEBUG
#define DEBUG(fmt, args...) fprintf(stderr, "[DEBUG] " fmt " (%s:%d)\n", \
                                    ##args, __FILE__, __LINE__);
#else
#ifdef __cplusplus
#define DEBUG(fmt, args...) static_cast<void>(0)
#else
#define DEBUG(fmt, args...) (void)(0)
#endif
#endif  /* BUILD_DEBUG */

/*
 * Other internal use
 */
#define MAX_HUMANOUT        (60)

/*
 * qInternal.c
 */

extern char _q_x2c(char hex_up, char hex_low);
extern char *_q_makeword(char *str, char stop);
extern void _q_textout(FILE *fp, void *data, size_t size, size_t max);

#endif /* QINTERNAL_H */
