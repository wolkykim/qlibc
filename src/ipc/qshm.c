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
 * $Id: qshm.c 101 2012-05-05 00:13:45Z seungyoung.kim $
 ******************************************************************************/

/**
 * @file qshm.c Shared-memory APIs.
 *
 * @note
 * @code
 *   [your header file]
 *   struct SharedData {
 *     (... structrue definitions ...)
 *   }
 *
 *   [shared memory creater]
 *   // create shared memory
 *   int shmid = qshm_init("/some/file/for/generating/unique/key", 's', sizeof(struct SharedData), true);
 *   if(shmid < 0) {
 *     printf("ERROR: Can't initialize shared memory.\n");
 *     return -1;
 *   }
 *
 *   // get shared memory pointer
 *   struct SharedData *sdata = (SharedData *)qshm_get(shmid);
 *   if(sdata == NULL) {
 *     printf("ERROR: Can't get shared memory.\n");
 *     return -1;
 *   }
 *
 *   [shared memory user]
 *   // get shared memory id
 *   int shmid = qshm_getid("/some/file/for/generating/unique/key", 's');
 *   if(shmid < 0) {
 *     printf("ERROR: Can't get shared memory id.\n");
 *     return -1;
 *   }
 *
 *   // get shared memory pointer
 *   struct SharedData *sdata = (SharedData *)qshm_get(shmid);
 *   if(sdata == NULL) {
 *     printf("ERROR: Can't get shared memory.\n");
 *     return -1;
 *   }
 * @endcode
 */

#ifndef DISABLE_IPC

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/shm.h>
#include "qlibc.h"
#include "qinternal.h"

/**
 * Initialize shared-memory
 *
 * @param keyfile   seed for generating unique IPC key
 * @param keyid     seed for generating unique IPC key
 * @param size      size of shared memory
 * @param recreate  set to true to re-create shared-memory if already exists
 *
 * @return non-negative shared memory identifier if successful, otherwise returns -1
 */
int qshm_init(const char *keyfile, int keyid, size_t size, bool recreate)
{
    key_t semkey;
    int shmid;

    /* generate unique key using ftok() */
    if (keyfile != NULL) {
        semkey = ftok(keyfile, keyid);
        if (semkey == -1) return -1;
    } else {
        semkey = IPC_PRIVATE;
    }

    /* create shared memory */
    if ((shmid = shmget(semkey, size, IPC_CREAT | IPC_EXCL | 0666)) == -1) {
        if (recreate == false) return -1;

        /* destroy & re-create */
        if ((shmid = qshm_getid(keyfile, keyid)) >= 0) qshm_free(shmid);
        if ((shmid = shmget(semkey, size, IPC_CREAT | IPC_EXCL | 0666)) == -1) return -1;
    }

    return shmid;
}

/**
 * Get shared memory identifier by keyfile and keyid for existing shared memory
 *
 * @param keyfile   seed for generating unique IPC key
 * @param keyid     seed for generating unique IPC key
 *
 * @return non-negative shared memory identifier if successful, otherwise returns -1
 */
int qshm_getid(const char *keyfile, int keyid)
{
    int shmid;

    /* generate unique key using ftok() */
    key_t semkey = ftok(keyfile, keyid);
    if (semkey == -1) return -1;

    /* get current shared memory id */
    if ((shmid = shmget(semkey, 0, 0)) == -1) return -1;

    return shmid;
}

/**
 * Get a pointer of shared memory
 *
 * @param shmid     shared memory identifier
 *
 * @return a pointer of shared memory
 */
void *qshm_get(int shmid)
{
    void *pShm;

    if (shmid < 0) return NULL;
    pShm = shmat(shmid, 0, 0);
    if (pShm == (void *)-1) return NULL;
    return pShm;
}

/**
 * De-allocate shared memory
 *
 * @param shmid     shared memory identifier
 *
 * @return true if successful, otherwise returns false
 */
bool qshm_free(int shmid)
{
    if (shmid < 0) return false;
    if (shmctl(shmid, IPC_RMID, 0) != 0) return false;
    return true;
}

#endif /* DISABLE_IPC */
