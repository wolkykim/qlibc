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
 * $Id: qsystem.c 102 2012-05-05 00:22:27Z seungyoung.kim $
 ******************************************************************************/

/**
 * @file qsystem.c System APIs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "qlibc.h"
#include "qinternal.h"

/**
 * Get system environment variable
 *
 * @param envname   environment name
 * @param nullstr   if not found, return this string
 *
 * @return a pointer of environment variable
 */
const char *qgetenv(const char *envname, const char *nullstr)
{
    const char *envstr = getenv(envname);
    if (envstr != NULL) return envstr;
    return nullstr;
}

/**
 * Get the result string of external command execution
 *
 * @param cmd       external command
 *
 * @return malloced string pointer which contains result if successful,
 *         otherwise returns NULL
 *
 * @note
 *  If the command does not report result but it is executed successfully,
 *  this will returns empty string(not null)
 */
char *qsyscmd(const char *cmd)
{
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) return NULL;
    char *str = qfile_read(fp, NULL);
    pclose(fp);

    if (str == NULL) str = strdup("");
    return str;
}

/**
 * Get system IP address string.
 *
 * @return malloced string pointer which contains IP address string if
 *         successful, otherwise returns NULL
 */
bool qsysgetip(char *buf, size_t bufsize)
{
    char szHostname[63+1];
    if (gethostname(szHostname, sizeof(szHostname)) != 0) return false;

    struct hostent *pHostEntry = gethostbyname(szHostname);
    if (pHostEntry == NULL) return false;

    char *pszLocalIp = inet_ntoa(*(struct in_addr *)*pHostEntry->h_addr_list);
    if (pszLocalIp == NULL) return false;

    qstrcpy(buf, bufsize, pszLocalIp);
    return true;
}
