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
 * @file qconfig.c INI-style configuration file parser.
 */

#ifndef DISABLE_QCONFIG

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "qinternal.h"
#include "utilities/qfile.h"
#include "utilities/qstring.h"
#include "utilities/qsystem.h"
#include "extensions/qconfig.h"

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
 * Load and parse a configuration file.
 *
 * @param tbl       qlisttbl_t pointer. If NULL, a new table is created.
 * @param filepath  path to the configuration file.
 * @param sepchar   separator used to split keys and values.
 *
 * @return qlisttbl_t pointer on success, or NULL if the file cannot be loaded.
 *
 * @code
 *   # This is the "config.conf" file.
 *   # A line that starts with # is a comment.
 *
 *   @INCLUDE config.def      => include the "config.def" file.
 *
 *   prefix=/tmp              => set a fixed value. "prefix" is the key.
 *   log=${prefix}/log        => use the value of the previously defined key "prefix".
 *   user=${%USER}            => use an environment variable.
 *   host=${!/bin/hostname -s} => run an external command and use its output.
 *   id=${user}@${host}
 *
 *   # Enter the "system" section.
 *   [system]                 => the key "system." with value "system" is inserted.
 *   ostype=${%OSTYPE}        => "system.ostype" is the key for this entry.
 *   machtype=${%MACHTYPE}    => "system.machtype" is the key for this entry.
 *
 *   # Enter the "daemon" section.
 *   [daemon]
 *   port=1234
 *   name=${user}_${host}_${system.ostype}_${system.machtype}
 *
 *   # Leave the section and go back to the root.
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
                               char sepchar) {
    char *str = qfile_load(filepath, NULL);
    if (str == NULL)
        return NULL;

    // Process @INCLUDE directives.
    char *strp = str;

    while ((strp = strstr(strp, _INCLUDE_DIRECTIVE)) != NULL) {
        if (strp == str || strp[-1] == '\n') {
            char buf[PATH_MAX];

            // Parse the file name.
            char *tmpp;
            for (tmpp = strp + CONST_STRLEN(_INCLUDE_DIRECTIVE);
                    *tmpp != '\n' && *tmpp != '\0'; tmpp++)
                ;
            int len = tmpp - (strp + CONST_STRLEN(_INCLUDE_DIRECTIVE));
            if (len >= sizeof(buf)) {
                DEBUG("Can't process %s directive.", _INCLUDE_DIRECTIVE);
                free(str);
                return NULL;
            }

            strncpy(buf, strp + CONST_STRLEN(_INCLUDE_DIRECTIVE), len);
            buf[len] = '\0';
            qstrtrim(buf);

            // Build the full file path.
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

            // Read the included file.
            char *incdata;
            if (strlen(buf) == 0 || (incdata = qfile_load(buf, NULL)) == NULL) {
                DEBUG("Can't process '%s%s' directive.", _INCLUDE_DIRECTIVE,
                        buf);
                free(str);
                return NULL;
            }

            // Replace the directive with the file contents.
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

    // Parse the final string.
    tbl = qconfig_parse_str(tbl, str, sepchar);
    free(str);

    return tbl;
}

/**
 * Parse a configuration string.
 *
 * @param tbl       qlisttbl_t pointer. If NULL, a new table is created.
 * @param str       string that contains key/value pairs.
 * @param sepchar   separator used to split keys and values.
 *
 * @return qlisttbl_t pointer on success, or NULL on failure.
 *
 * @see qconfig_parse_file
 *
 * @code
 *   qlisttbl_t *tbl;
 *   tbl = qconfig_parse_str(NULL, "key = value\nhello = world", '=');
 * @endcode
 */
qlisttbl_t *qconfig_parse_str(qlisttbl_t *tbl, const char *str, char sepchar) {
    if (str == NULL)
        return NULL;

    if (tbl == NULL) {
        tbl = qlisttbl(0);
        if (tbl == NULL)
            return NULL;
    }

    char *section = NULL;
    char *org, *buf, *offset;
    for (org = buf = offset = strdup(str); *offset != '\0';) {
        // Read one line into buf.
        for (buf = offset; *offset != '\n' && *offset != '\0'; offset++)
            ;
        if (*offset != '\0') {
            *offset = '\0';
            offset++;
        }
        qstrtrim(buf);

        // Skip blank lines and comments.
        if ((buf[0] == '#') || (buf[0] == '\0'))
            continue;

        // Parse a section header.
        if ((buf[0] == '[') && (buf[strlen(buf) - 1] == ']')) {
            // Extract the section name.
            if (section != NULL)
                free(section);
            section = strdup(buf + 1);
            section[strlen(section) - 1] = '\0';
            qstrtrim(section);

            // Clear the section if the name is empty, such as [].
            if (section[0] == '\0') {
                free(section);
                section = NULL;
                continue;
            }

            // Store the section name as "section.=section".
            sprintf(buf, "%c%s", sepchar, section);
        }

        // Parse and store the entry.
        char *value = strdup(buf);
        char *name = _q_makeword(value, sepchar);
        qstrtrim(value);
        qstrtrim(name);

        // Add the section name as a prefix.
        if (section != NULL) {
            char *newname = qstrdupf("%s.%s", section, name);
            free(name);
            name = newname;
        }

        // Resolve variables in the value.
        char *newvalue = _parsestr(tbl, value);
        if (newvalue != NULL) {
            tbl->putstr(tbl, name, newvalue);
            free(newvalue);
        }

        free(name);
        free(value);
    }
    free(org);
    if (section != NULL)
        free(section);

    return tbl;
}

#ifndef _DOXYGEN_SKIP

/**
 * (qlisttbl_t*)->parsestr(): Parse a string and replace variables with
 * values from this table.
 *
 * @param tbl   qlisttbl container pointer.
 * @param str   string value that may contain variables like ${...}
 *
 * @return allocated string on success, otherwise NULL.
 * @retval errno will be set on error.
 *  - EINVAL : Invalid argument.
 *
 * @code
 *   ${key_name}        - replace with the matching value from this table.
 *   ${!system_command} - run an external command and use its output.
 *   ${%PATH}           - get an environment variable.
 * @endcode
 *
 * @code
 *   --[tbl Table]------------------------
 *   NAME = qLibc
 *   -------------------------------------
 *
 *   char *str = _parsestr(tbl, "${NAME}, ${%HOME}, ${!date -u}");
 *   if (str != NULL) {
 *     printf("%s\n", str);
 *     free(str);
 *   }
 *
 *   [Output]
 *   qLibc, /home/qlibc, Wed Nov 24 00:30:58 UTC 2010
 * @endcode
 */
static char *_parsestr(qlisttbl_t *tbl, const char *str) {
    if (str == NULL) {
        errno = EINVAL;
        return NULL;
    }

    bool loop;
    char *value = strdup(str);
    do {
        loop = false;

        // Find the next ${ token.
        char *s, *e;
        int openedbrakets;
        for (s = value; *s != '\0'; s++) {
            if (!(*s == _VAR && *(s + 1) == _VAR_OPEN))
                continue;

            // Found ${. Now look for the matching }.
            openedbrakets = 1;  // Number of open brackets.
            for (e = s + 2; *e != '\0'; e++) {
                if (*e == _VAR && *(e + 1) == _VAR_OPEN) {  // Found a nested ${
                    // e is always greater than s, so this cannot underflow.
                    s = e - 1;
                    break;
                } else if (*e == _VAR_OPEN)
                    openedbrakets++;
                else if (*e == _VAR_CLOSE)
                    openedbrakets--;
                else
                    continue;

                if (openedbrakets == 0)
                    break;
            }
            if (*e == '\0')
                break;  // Brackets do not match.
            if (openedbrakets > 0)
                continue;  // A nested ${ was found.

            // Copy the text between ${ and }.
            int varlen = e - s - 2;  // Length between ${ and }.
            char *varstr = (char *) malloc(varlen + 3 + 1);
            if (varstr == NULL)
                continue;
            strncpy(varstr, s + 2, varlen);
            varstr[varlen] = '\0';

            // Resolve the replacement string.
            char *newstr = NULL;
            switch (varstr[0]) {
                case _VAR_CMD: {
                    if (varlen - 1 == 0) {
                        newstr = strdup("");
                        break;
                    }
                    if ((newstr = qstrtrim(qsyscmd(varstr + 1))) == NULL) {
                        newstr = strdup("");
                    }
                    break;
                }
                case _VAR_ENV: {
                    if (varlen - 1 == 0) {
                        newstr = strdup("");
                        break;
                    }
                    newstr = strdup(qgetenv(varstr + 1, ""));
                    break;
                }
                default: {
                    if (varlen == 0) {
                        newstr = strdup("");
                        break;
                    }
                    if ((newstr = tbl->getstr(tbl, varstr, true)) == NULL) {
                        s = e;  // No matching value was found.
                        continue;
                    }
                    break;
                }
            }

            // Replace the original token.
            strncpy(varstr, s, varlen + 3);  // Copy ${str}.
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

