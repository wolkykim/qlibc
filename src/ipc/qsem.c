/******************************************************************************
 * qLibc
 *
 * Copyright (c) 2010-2026 Seungyoung Kim.
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
 * @file qsem.c Semaphore APIs.
 *
 * @note
 * @code
 *   [daemon main]
 *   #define MAX_SEMAPHORES (2)
 *
 *   // create semaphores
 *   int semid = qsem_init("/some/file/for/generating/unique/key", 'q', MAX_SEMAPHORES, true);
 *   if(semid < 0) {
 *     printf("ERROR: Can't initialize semaphores.\n");
 *     return -1;
 *   }
 *
 *   // fork children
 *   (... child forking codes ...)
 *
 *   // at the end of daemon, free semaphores
 *   if(semid >= 0) qsem_free(semid);
 *
 *   [forked child]
 *   // critical section for resource 0
 *   qsem_enter(semid, 0);
 *   (... guaranteed as atomic procedure ...)
 *   qsem_leave(semid, 0);
 *
 *   (... some codes ...)
 *
 *   // critical section for resource 1
 *   qsem_enter(semid, 1);
 *   (... guaranteed as atomic procedure ...)
 *   qsem_leave(semid, 1);
 *
 *   [other program which uses resource 1]
 *   int semid = qsem_getid("/some/file/for/generating/unique/key", 'q');
 *   if(semid < 0) {
 *     printf("ERROR: Can't get semaphore id.\n");
 *     return -1;
 *   }
 *
 *   // critical section for resource 1
 *   qsem_enter(semid, 1);
 *   (... guaranteed as atomic procedure ...)
 *   qsem_leave(semid, 1);
 *
 * @endcode
 */

#ifndef DISABLE_IPC

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/sem.h>
#include "qinternal.h"
#include "ipc/qsem.h"

/**
 * Initialize semaphores.
 *
 * @param keyfile   seed used to generate a unique IPC key
 * @param keyid     seed used to generate a unique IPC key
 * @param nsems     number of semaphores to initialize
 * @param recreate  set to true to recreate the semaphore set if it already exists
 *
 * @return non-negative semaphore identifier on success, or -1 on failure.
 *
 * @code
 *   int semid = qsem_init("/tmp/mydaemon.pid", 'q', 10, true);
 * @endcode
 */
int qsem_init(const char *keyfile, int keyid, int nsems, bool recreate) {
    key_t semkey;
    int semid;

    // generate unique key using ftok();
    if (keyfile != NULL) {
        semkey = ftok(keyfile, keyid);
        if (semkey == -1)
            return -1;
    } else {
        semkey = IPC_PRIVATE;
    }

    // create semaphores
    if ((semid = semget(semkey, nsems, IPC_CREAT | IPC_EXCL | 0666)) == -1) {
        if (recreate == false)
            return -1;

        // destroy & re-create
        if ((semid = qsem_getid(keyfile, keyid)) >= 0)
            qsem_free(semid);
        if ((semid = semget(semkey, nsems, IPC_CREAT | IPC_EXCL | 0666)) == -1)
            return -1;
    }

    // initializing
    int i;
    for (i = 0; i < nsems; i++) {
        struct sembuf sbuf;

        /* set sbuf */
        sbuf.sem_num = i;
        sbuf.sem_op = 1;
        sbuf.sem_flg = 0;

        /* initialize */
        if (semop(semid, &sbuf, 1) != 0) {
            qsem_free(semid);
            return -1;
        }
    }

    return semid;
}

/**
 * Get the identifier of an existing semaphore set.
 *
 * @param keyfile   seed used to generate a unique IPC key
 * @param keyid     seed used to generate a unique IPC key
 *
 * @return non-negative semaphore identifier on success, or -1 on failure.
 */
int qsem_getid(const char *keyfile, int keyid) {
    int semid;

    /* generate unique key using ftok() */
    key_t semkey = ftok(keyfile, keyid);
    if (semkey == -1)
        return -1;

    /* get current semaphore id */
    if ((semid = semget(semkey, 0, 0)) == -1)
        return -1;

    return semid;
}

/**
 * Lock a semaphore and enter a critical section.
 *
 * @param semid     semaphore identifier
 * @param semno     semaphore number
 *
 * @return true on success, otherwise false.
 *
 * @note If the semaphore is already locked, this function waits until it is released.
 */
bool qsem_enter(int semid, int semno) {
    struct sembuf sbuf;

    /* set sbuf */
    sbuf.sem_num = semno;
    sbuf.sem_op = -1;
    sbuf.sem_flg = SEM_UNDO;

    /* lock */
    if (semop(semid, &sbuf, 1) != 0)
        return false;
    return true;
}

/**
 * Try to lock a semaphore without waiting.
 *
 * @param semid     semaphore identifier
 * @param semno     semaphore number
 *
 * @return true on success, or false if it is already locked or an error occurs.
 */
bool qsem_enter_nowait(int semid, int semno) {
    struct sembuf sbuf;

    /* set sbuf */
    sbuf.sem_num = semno;
    sbuf.sem_op = -1;
    sbuf.sem_flg = SEM_UNDO | IPC_NOWAIT;

    /* lock */
    if (semop(semid, &sbuf, 1) != 0)
        return false;
    return true;
}

/**
 * Wait for a semaphore, then force it open if needed.
 *
 * @param semid     semaphore identifier
 * @param semno     semaphore number
 * @param maxwaitms maximum time to wait, in milliseconds
 * @param forceflag status output. This can be NULL if you do not need it.
 *
 * @return true on success, otherwise false.
 *
 * @note
 * This function waits up to `maxwaitms` for the semaphore to be released.
 * If it is released normally in time, `forceflag` is set to false.
 * If the wait time is exceeded and the semaphore is forcibly released,
 * `forceflag` is set to true.
 */
bool qsem_enter_force(int semid, int semno, int maxwaitms, bool *forceflag) {
    int wait;
    for (wait = 0; wait < maxwaitms; wait += 10) {
        if (qsem_enter_nowait(semid, semno) == true) {
            if (forceflag != NULL)
                *forceflag = false;
            return true;
        }
        usleep(10 * 1000);  // sleep 10ms
    }

    DEBUG("force to unlock semaphore %d-%d", semid, semno);
    while (true) {
        qsem_leave(semid, semno);
        if (qsem_enter_nowait(semid, semno) == true)
            break;
    }

    if (forceflag != NULL)
        *forceflag = true;
    return true;
}

/**
 * Unlock a semaphore and leave the critical section.
 *
 * @param semid     semaphore identifier
 * @param semno     semaphore number
 *
 * @return true on success, otherwise false.
 */
bool qsem_leave(int semid, int semno) {
    struct sembuf sbuf;

    /* set sbuf */
    sbuf.sem_num = semno;
    sbuf.sem_op = 1;
    sbuf.sem_flg = SEM_UNDO;

    /* unlock */
    if (semop(semid, &sbuf, 1) != 0)
        return false;
    return true;
}

/**
 * Get the status of a semaphore.
 *
 * @param semid     semaphore identifier
 * @param semno     semaphore number
 *
 * @return true if the semaphore is locked, or false if it is unlocked.
 */
bool qsem_check(int semid, int semno) {
    if (semctl(semid, semno, GETVAL, 0) == 0)
        return true;  // locked
    return false;  // unlocked
}

/**
 * Release a semaphore set from the system.
 *
 * @param semid     semaphore identifier
 *
 * @return true on success, otherwise false.
 */
bool qsem_free(int semid) {
    if (semid < 0)
        return false;
    if (semctl(semid, 0, IPC_RMID, 0) != 0)
        return false;
    return true;
}

#endif /* DISABLE_IPC */
