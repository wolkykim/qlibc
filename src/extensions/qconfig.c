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
 * $Id: qconfig.c 109 2012-05-25 17:38:18Z seungyoung.kim $
 ******************************************************************************/

/**
 * @file qconfig.c INI-style configuration file parser.
 */

#ifndef DISABLE_QCONFIG

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "qlibc.h"
#include "qlibcext.h"
#include "qinternal.h"

#define _INCLUDE_DIRECTIVE  "@INCLUDE "

#ifndef _DOXYGEN_SKIP
#define _VAR        '$'
#define _VAR_OPEN   '{'
#define _VAR_CLOSE  '}'
#define _VAR_CMD    '!'
#define _VAR_ENV    '%'

/* internal functions */
static char *_parsestr(qlisttbl_t *tbl, const char *str);
#endif

/**
 * Load & parse configuration file
 *
 * @param tbl       a pointer of qlisttbl_t. NULL can be used for empty.
 * @param filepath  configuration file path
 * @param sepchar   separater used in configuration file to divice key and value
 * @param uniquekey false to allow key duplication, true for overwriting lastest
 *                  key if there is a same key set already.
 *
 * @return a pointer of qlisttbl_t in case of successful,
 *  otherwise(file not found) returns NULL
 *
 * @code
 *   # This is "config.conf" file.
 *   # A line which starts with # character is comment
 *
 *   @INCLUDE config.def  => include 'config.def' file.
 *
 *   prefix=/tmp        => set static value. 'prefix' is the key for this entry.
 *   log=${prefix}/log  => get the value from previously defined key 'prefix'.
 *   user=${%USER}      => get environment variable.
 *   host=${!/bin/hostname -s}  => run external command and put it's output.
 *   id=${user}@${host}
 *
 *   # now entering into 'system' section.
 *   [system]           => a key 'system.' with value 'system' will be inserted.
 *   ostype=${%OSTYPE}  => 'system.ostype' is the key for this entry.
 *   machtype=${%MACHTYPE}  => 'system.machtype' is the key for this entry.
 *
 *   # entering into 'daemon' section.
 *   [daemon]
 *   port=1234
 *   name=${user}_${host}_${system.ostype}_${system.machtype}
 *
 *   # escape section. (go back to root)
 *   []
 *   rev=822
 * @endcode
 *
 * @code
 *   # This is "config.def" file.
 *   prefix = /usr/local
 *   bin = ${prefix}/bin
 *   log = ${prefix}/log
 *   user = unknown
 *   host = unknown
 * @endcode
 *
 * @code
 *   qlisttbl_t *tbl = qconfig_parse_file(NULL, "config.conf", '=', true);
 *   tbl->debug(tbl, stdout);
 *
 *   [Output]
 *   bin=/usr/local/bin? (15)
 *   prefix=/tmp? (5)
 *   log=/tmp/log? (9)
 *   user=seungyoung.kim? (9)
 *   host=eng22? (6)
 *   id=seungyoung.kim@eng22? (15)
 *   system.=system? (7)
 *   system.ostype=linux? (6)
 *   system.machtype=x86_64? (7)
 *   daemon.=daemon? (7)
 *   daemon.port=1234? (5)
 *   daemon.name=seungyoung.kim_eng22_linux_x86_64? (28)
 *   rev=822? (4)
 * @endcode
 */
qlisttbl_t *qconfig_parse_file(qlisttbl_t *tbl, const char *filepath,
                               char sepchar, bool uniquekey)
{
    char *str = qfile_load(filepath, NULL);
    if (str == NULL) return NULL;

    // process include directive
    char *strp = str;;
    while ((strp = strstr(strp, _INCLUDE_DIRECTIVE)) != NULL) {
        if (strp == str || strp[-1] == '\n') {
            char buf[PATH_MAX];

            // parse filename
            char *tmpp;
            for (tmpp = strp + CONST_STRLEN(_INCLUDE_DIRECTIVE); *tmpp != '\n'
                 && *tmpp != '\0'; tmpp++);
            int len = tmpp - (strp + CONST_STRLEN(_INCLUDE_DIRECTIVE));
            if (len >= sizeof(buf)) {
                DEBUG("Can't process %s directive.", _INCLUDE_DIRECTIVE);
                free(str);
                return NULL;
            }

            strncpy(buf, strp + CONST_STRLEN(_INCLUDE_DIRECTIVE), len);
            buf[len] = '\0';
            qstrtrim(buf);

            // get full file path
            if (!(buf[0] == '/' || buf[0] == '\\')) {
                char tmp[PATH_MAX];
                char *dir = qfile_get_dir(filepath);
                if (strlen(dir) + 1 + strlen(buf) >= sizeof(buf)) {
                    DEBUG("Can't process %s directive.", _INCLUDE_DIRECTIVE);
                    free(dir);
                    free(str);
                    return NULL;
                }
                snprintf(tmp, sizeof(tmp), "%s/%s", dir, buf);
                free(dir);

                strcpy(buf, tmp);
            }

            // read file
            char *incdata;
            if (strlen(buf) == 0 || (incdata = qfile_load(buf, NULL)) == NULL) {
                DEBUG("Can't process '%s%s' directive.", _INCLUDE_DIRECTIVE,
                      buf);
                free(str);
                return NULL;
            }

            // replace
            strncpy(buf, strp, CONST_STRLEN(_INCLUDE_DIRECTIVE) + len);
            buf[CONST_STRLEN(_INCLUDE_DIRECTIVE) + len] = '\0';
            strp = qstrreplace("sn", str, buf, incdata);
            free(incdata);
            free(str);
            str = strp;
        } else {
            strp += CONST_STRLEN(_INCLUDE_DIRECTIVE);
        }
    }

    // parse
    tbl = qconfig_parse_str(tbl, str, sepchar, uniquekey);
    free(str);

    return tbl;
}

/**
 * Parse string
 *
 * @param tbl       a pointer of qlisttbl_t. NULL can be used for empty.
 * @param str       key, value pair strings
 * @param sepchar   separater used in configuration file to divice key and value
 * @param uniquekey false to allow key duplication, true for overwriting lastest
 *                  key if there is a same key set already.
 *
 * @return a pointer of qlisttbl_t in case of successful,
 *         otherwise(file not found) returns NULL
 *
 * @see qconfig_parse_file
 *
 * @code
 *  qlisttbl_t *tbl;
 *  tbl = qconfig_parse_str(NULL, "key = value\nhello = world", '=');
 * @endcode
 */
qlisttbl_t *qconfig_parse_str(qlisttbl_t *tbl, const char *str, char sepchar,
                              bool uniquekey)
{
    if (str == NULL) return NULL;

    if (tbl == NULL) {
        tbl = qlisttbl();
        if (tbl == NULL) return NULL;
    }

    char *section = NULL;
    char *org, *buf, *offset;
    for (org = buf = offset = strdup(str); *offset != '\0'; ) {
        // get one line into buf
        for (buf = offset; *offset != '\n' && *offset != '\0'; offset++);
        if (*offset != '\0') {
            *offset = '\0';
            offset++;
        }
        qstrtrim(buf);

        // skip blank or comment line
        if ((buf[0] == '#') || (buf[0] == '\0')) continue;

        // section header
        if ((buf[0] == '[') && (buf[strlen(buf) - 1] == ']')) {
            // extract section name
            if (section != NULL) free(section);
            section = strdup(buf + 1);
            section[strlen(section) - 1] = '\0';
            qstrtrim(section);

            // remove section if section name is empty. ex) []
            if (section[0] == '\0') {
                free(section);
                section = NULL;
                continue;
            }

            // in order to put 'section.=section'
            sprintf(buf, "%c%s", sepchar, section);
        }

        // parse & store
        char *value = strdup(buf);
        char *name  = _q_makeword(value, sepchar);
        qstrtrim(value);
        qstrtrim(name);

        // put section name as a prefix
        if (section != NULL) {
            char *newname = qstrdupf("%s.%s", section, name);
            free(name);
            name = newname;
        }

        // get parsed string
        char *newvalue = _parsestr(tbl, value);
        if (newvalue != NULL) {
            tbl->putstr(tbl, name, newvalue, uniquekey);
            free(newvalue);
        }

        free(name);
        free(value);
    }
    free(org);
    if (section != NULL) free(section);

    return tbl;
}

#ifndef _DOXYGEN_SKIP

/**
 * (qlisttbl_t*)->parsestr(): Parse a string and replace variables in the
 * string to the data in this list.
 *
 * @param tbl   qlisttbl container pointer.
 * @param str   string value which may contain variables like ${...}
 *
 * @return malloced string if successful, otherwise returns NULL.
 * @retval errno will be set in error condition.
 *  - EINVAL : Invalid argument.
 *
 * @code
 *  ${key_name}          - replace this with a matched value data in this list.
 *  ${!system_command}   - run external command and put it's output here.
 *  ${%PATH}             - get environment variable.
 * @endcode
 *
 * @code
 *  --[tbl Table]------------------------
 *  NAME = qLibc
 *  -------------------------------------
 *
 *  char *str = _parsestr(tbl, "${NAME}, ${%HOME}, ${!date -u}");
 *  if(str != NULL) {
 *    printf("%s\n", str);
 *    free(str);
 *  }
 *
 *  [Output]
 *  qLibc, /home/qlibc, Wed Nov 24 00:30:58 UTC 2010
 * @endcode
 */
static char *_parsestr(qlisttbl_t *tbl, const char *str)
{
    if (str == NULL) {
        errno = EINVAL;
        return NULL;
    }

    bool loop;
    char *value = strdup(str);
    do {
        loop = false;

        // find ${
        char *s, *e;
        int openedbrakets;
        for (s = value; *s != '\0'; s++) {
            if (!(*s == _VAR && *(s+1) == _VAR_OPEN)) continue;

            // found ${, try to find }. s points $
            openedbrakets = 1; // braket open counter
            for (e = s + 2; *e != '\0'; e++) {
                if (*e == _VAR && *(e+1) == _VAR_OPEN) { // found internal ${
                    // e is always bigger than s, negative overflow never occure
                    s = e - 1;
                    break;
                } else if (*e == _VAR_OPEN) openedbrakets++;
                else if (*e == _VAR_CLOSE) openedbrakets--;
                else continue;

                if (openedbrakets == 0) break;
            }
            if (*e == '\0') break; // braket mismatch
            if (openedbrakets > 0) continue; // found internal ${

            // pick string between ${, }
            int varlen = e - s - 2; // length between ${ , }
            char *varstr = (char *)malloc(varlen + 3 + 1);
            if (varstr == NULL) continue;
            strncpy(varstr, s + 2, varlen);
            varstr[varlen] = '\0';

            // get the new string to replace
            char *newstr = NULL;
            switch (varstr[0]) {
                case _VAR_CMD : {
                    if ((newstr = qstrtrim(qsyscmd(varstr + 1))) == NULL) {
                        newstr = strdup("");
                    }
                    break;
                }
                case _VAR_ENV : {
                    newstr = strdup(qgetenv(varstr + 1, ""));
                    break;
                }
                default : {
                    if ((newstr = tbl->getstr(tbl, varstr, true)) == NULL) {
                        s = e; // not found
                        continue;
                    }
                    break;
                }
            }

            // replace
            strncpy(varstr, s, varlen + 3); // ${str}
            varstr[varlen + 3] = '\0';

            s = qstrreplace("sn", value, varstr, newstr);
            free(newstr);
            free(varstr);
            free(value);
            value = s;

            loop = true;
            break;
        }
    } while (loop == true);

    return value;
}

#endif /* _DOXYGEN_SKIP */

#endif /* DISABLE_QCONFIG */

