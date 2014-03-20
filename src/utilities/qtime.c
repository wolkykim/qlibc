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
 * $Id: qtime.c 63 2012-04-06 00:01:19Z seungyoung.kim $
 ******************************************************************************/

/**
 * @file qtime.c Time handling APIs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "qlibc.h"
#include "qinternal.h"

/**
 * Get custom formmatted local time string.
 *
 * @param buf       save buffer
 * @param size      buffer size
 * @param utctime   0 for current time, universal time for specific time
 * @param format    format for strftime()
 *
 * @return string pointer of buf
 *
 * @code
 *   char *timestr = qtime_localtime_strf(0, "%H:%M:%S"); // HH:MM:SS
 *   free(timestr);
 *   char *timestr = qtime_localtime_strf(0, "%Y%m%d%H%M%S"); // YYMMDDhhmmss
 *   free(timestr);
 * @endcode
 */
char *qtime_localtime_strf(char *buf, int size, time_t utctime,
                           const char *format)
{
    if (utctime == 0) utctime = time(NULL);
    struct tm *localtm = localtime(&utctime);

    if (strftime(buf, size, format, localtm) == 0) {
        snprintf(buf, size, "(buffer small)");
    }

    return buf;
}

/**
 * Get local time string formatted like '02-Nov-2007 16:37:39 +0900'.
 *
 * @param utctime   0 for current time, universal time for specific time
 *
 * @return mallocked string pointer of time string
 *
 * @code
 *   char *timestr;
 *   timestr = qtime_localtime_str(0);  // now
 *   free(timestr);
 *   timestr = qtime_localtime_str(time(NULL));  // same as above
 *   free(timestr);
 *   timestr = qtime_localtime_str(time(NULL) - 86400));  // 1 day before
 *   free(timestr);
 * @endcode
 */
char *qtime_localtime_str(time_t utctime)
{
    int size = sizeof(char) * (CONST_STRLEN("00-Jan-0000 00:00:00 +0000") + 1);
    char *timestr = (char *)malloc(size);
    qtime_localtime_strf(timestr, size, utctime, "%d-%b-%Y %H:%M:%S %z");
    return timestr;
}

/**
 * Get local time string formatted like '02-Nov-2007 16:37:39 +0900'.
 *
 * @param utctime   0 for current time, universal time for specific time
 *
 * @return internal static string pointer of time string
 *
 * @code
 *   printf("%s", qtime_localtime_staticstr(0));  // now
 *   printf("%s", qtime_localtime_staticstr(time(NULL) + 86400)); // 1 day later
 * @endcode
 */
const char *qtime_localtime_staticstr(time_t utctime)
{
    static char timestr[sizeof(char)
                        * (CONST_STRLEN("00-Jan-0000 00:00:00 +0000") + 1)];
    qtime_localtime_strf(timestr,
                         sizeof(timestr),
                         utctime,
                         "%d-%b-%Y %H:%M:%S %z");
    return timestr;
}

/**
 * Get custom formmatted GMT time string.
 *
 * @param buf       save buffer
 * @param size      buffer size
 * @param utctime   0 for current time, universal time for specific time
 * @param format    format for strftime()
 *
 * @return string pointer of buf
 *
 * @code
 *   char timestr[8+1];
 *   qtime_gmt_strf(buf, sizeof(buf), 0, "%H:%M:%S"); // HH:MM:SS
 * @endcode
 */
char *qtime_gmt_strf(char *buf, int size, time_t utctime, const char *format)
{
    if (utctime == 0) utctime = time(NULL);
    struct tm *gmtm = gmtime(&utctime);

    strftime(buf, size, format, gmtm);
    return buf;
}

/**
 * Get GMT time string formatted like 'Wed, 11-Nov-2007 23:19:25 GMT'.
 *
 * @param utctime   0 for current time, universal time for specific time
 *
 * @return malloced string pointer which points GMT time string.
 *
 * @code
 *   char *timestr;
 *   timestr = qtime_gmt_str(0);           // now
 *   free(timestr);
 *   timestr = qtime_gmt_str(time(NULL));      // same as above
 *   free(timestr);
 *   timestr = qtime_gmt_str(time(NULL) - 86400)); // 1 day before
 *   free(timestr);
 * @endcode
 */
char *qtime_gmt_str(time_t utctime)
{
    int size = sizeof(char)
               * (CONST_STRLEN("Mon, 00 Jan 0000 00:00:00 GMT") + 1);
    char *timestr = (char *)malloc(size);
    qtime_gmt_strf(timestr, size, utctime, "%a, %d %b %Y %H:%M:%S GMT");
    return timestr;
}

/**
 * Get GMT time string formatted like 'Wed, 11-Nov-2007 23:19:25 GMT'.
 *
 * @param utctime   0 for current time, universal time for specific time
 *
 * @return internal static string pointer which points GMT time string.
 *
 * @code
 *   printf("%s", qtime_gmt_staticstr(0));         // now
 *   printf("%s", qtime_gmt_staticstr(time(NULL) + 86400));    // 1 day later
 * @endcode
 */
const char *qtime_gmt_staticstr(time_t utctime)
{
    static char timestr[sizeof(char)
                        * (CONST_STRLEN("Mon, 00-Jan-0000 00:00:00 GMT") + 1)];
    qtime_gmt_strf(timestr,
                   sizeof(timestr),
                   utctime,
                   "%a, %d %b %Y %H:%M:%S GMT");
    return timestr;
}

/**
 * This parses GMT/Timezone(+/-) formatted time sting like
 * 'Sun, 04 May 2008 18:50:39 GMT', 'Mon, 05 May 2008 03:50:39 +0900'
 * and returns as universal time.
 *
 * @param gmtstr    GMT/Timezone(+/-) formatted time string
 *
 * @return universal time(UTC). in case of conversion error, returns -1.
 *
 * @code
 *   time_t t = time(NULL);
 *   char *s =  qtime_parse_gmtstr(t);
 *   printf("%d\n", t);
 *   printf("%s\n", s);
 *   printf("%d\n", qtime_parse_gmtstr(s)); // this must be same as t
 *   free(s);
 * @endcode
 */
time_t qtime_parse_gmtstr(const char *gmtstr)
{
    struct tm gmtm;
    if (strptime(gmtstr, "%a, %d %b %Y %H:%M:%S", &gmtm) == NULL) return 0;
    time_t utc = timegm(&gmtm);
    if (utc < 0) return -1;

    // parse timezone
    char *p;
    if ((p = strstr(gmtstr, "+")) != NULL) {
        utc -= ((atoi(p + 1) / 100) * 60 * 60);
        if (utc < 0) return -1;
    } else if ((p = strstr(gmtstr, "-")) != NULL) {
        utc += ((atoi(p + 1) / 100) * 60 * 60);
    }

    return utc;
}
