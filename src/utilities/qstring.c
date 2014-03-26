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
 * @file qstring.c String APIs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include "qinternal.h"
#include "utilities/qencode.h"
#include "utilities/qhash.h"
#include "utilities/qstring.h"

/**
 * Remove white spaces(including CR, LF) from head and tail of the string.
 *
 * @param str       source string
 *
 * @return a pointer of source string if rsuccessful, otherewise returns NULL
 *
 * @note This modify source string directly.
 */
char *qstrtrim(char *str) {
    if (str == NULL)
        return NULL;

    char *ss, *se;
    for (ss = str; *ss == ' ' || *ss == '\t' || *ss == '\r' || *ss == '\n';
            ss++)
        ;
    for (se = ss; *se != '\0'; se++)
        ;
    for (se--;
            se >= ss
                    && (*se == ' ' || *se == '\t' || *se == '\r' || *se == '\n');
            se--)
        ;
    se++;
    *se = '\0';

    if (ss > str) {
        size_t len = (se - ss) + 1;
        memmove(str, ss, len);
    }

    return str;
}

/**
 * Remove heading white spaces of the string.
 *
 * @param str       source string
 *
 * @return a pointer of source string if rsuccessful, otherewise returns NULL
 *
 * @note This modify source string directly.
 */
char *qstrtrim_head(char *str) {
    if (str == NULL)
        return NULL;

    char *ss;
    for (ss = str; *ss == ' ' || *ss == '\t' || *ss == '\r' || *ss == '\n';
            ss++)
        ;

    if (ss > str) {
        size_t len = strlen(ss) + 1;
        memmove(str, ss, len);
    }

    return str;
}

/**
 * Remove tailing white spaces(including CR, LF) of the string.
 *
 * @param str       source string
 *
 * @return a pointer of source string if rsuccessful, otherewise returns NULL
 *
 * @note This modify source string directly.
 */
char *qstrtrim_tail(char *str) {
    if (str == NULL)
        return NULL;

    char *se;
    for (se = str + strlen(str) - 1;
            se >= str
                    && (*se == ' ' || *se == '\t' || *se == '\r' || *se == '\n');
            se--)
        ;
    se++;
    *se = '\0';

    return str;
}

/**
 * Remove character from head and tail of the string.
 *
 * @param str       source string
 * @param head      heading character
 * @param tail      tailing character
 *
 * @return a pointer of source string if successful, otherewise returns NULL
 *
 * @note This modify source string directly.
 *
 * @code
 *   char *str = strdup("   \"hello world\"   ");
 *   qstrtrim(str); // to remove white spaces
 *   qstrunchar(str, '"', '"'); // to unquote
 * @endcode
 */
char *qstrunchar(char *str, char head, char tail) {
    if (str == NULL)
        return NULL;

    int len = strlen(str);
    if (len >= 2 && str[0] == head && str[len - 1] == tail) {
        memmove(str, str + 1, len - 2);
        str[len - 2] = '\0';
    } else {
        return NULL;
    }

    return str;
}

/**
 * Replace string or tokens as word from source string with given mode.
 *
 * @param mode      replacing mode
 * @param srcstr    source string
 * @param tokstr    token or string
 * @param word      target word to be replaced
 *
 * @return a pointer of malloced or source string depending on the mode if
 *         successful, otherewise returns NULL
 *
 * @note
 * The mode argument has two separated character. First character
 * is used to decide replacing method and can be 't' or 's'.
 * The character 't' and 's' stand on [t]oken and [s]tring.
 *
 * When 't' is given each character of the token string(third argument)
 * will be compared with source string individually. If matched one
 * is found. the character will be replaced with given work.
 *
 * If 's' is given instead of 't'. Token string will be analyzed
 * only one chunk word. So the replacement will be occured when
 * the case of whole word matched.
 *
 * Second character is used to decide returning memory type and
 * can be 'n' or 'r' which are stand on [n]ew and [r]eplace.
 *
 * When 'n' is given the result will be placed into new array so
 * you should free the return string after using. Instead of this,
 * you can also use 'r' character to modify source string directly.
 * In this case, given source string should have enough space. Be
 * sure that untouchable value can not be used for source string.
 *
 * So there are four associatable modes such like below.
 *
 * Mode "tn" : [t]oken replacing & putting the result into [n]ew array.
 * Mode "tr" : [t]oken replacing & [r]eplace source string directly.
 * Mode "sn" : [s]tring replacing & putting the result into [n]ew array.
 * Mode "sr" : [s]tring replacing & [r]eplace source string directly.
 *
 * @code
 *   char srcstr[256], *retstr;
 *   char mode[4][2+1] = {"tn", "tr", "sn", "sr"};
 *
 *   for(i = 0; i < 4; i++) {
 *     strcpy(srcstr, "Welcome to The qDecoder Project.");
 *
 *     printf("before %s : srcstr = %s\n", mode[i], srcstr);
 *     retstr = qstrreplace(mode[i], srcstr, "The", "_");
 *     printf("after  %s : srcstr = %s\n", mode[i], srcstr);
 *     printf("            retstr = %s\n\n", retstr);
 *     if(mode[i][1] == 'n') free(retstr);
 *   }
 *
 *   --[Result]--
 *   before tn : srcstr = Welcome to The qDecoder Project.
 *   after  tn : srcstr = Welcome to The qDecoder Project.
 *               retstr = W_lcom_ _o ___ qD_cod_r Proj_c_.
 *
 *   before tr : srcstr = Welcome to The qDecoder Project.
 *   after  tr : srcstr = W_lcom_ _o ___ qD_cod_r Proj_c_.
 *               retstr = W_lcom_ _o ___ qD_cod_r Proj_c_.
 *
 *   before sn : srcstr = Welcome to The qDecoder Project.
 *   after  sn : srcstr = Welcome to The qDecoder Project.
 *               retstr = Welcome to _ qDecoder Project.
 *
 *   before sr : srcstr = Welcome to The qDecoder Project.
 *   after  sr : srcstr = Welcome to _ qDecoder Project.
 *               retstr = Welcome to _ qDecoder Project.
 * @endcode
 */
char *qstrreplace(const char *mode, char *srcstr, const char *tokstr,
                  const char *word) {
    if (mode == NULL || strlen(mode) != 2 || srcstr == NULL || tokstr == NULL
            || word == NULL) {
        DEBUG("Unknown mode \"%s\".", mode);
        return NULL;
    }

    char *newstr, *newp, *srcp, *tokenp, *retp;
    newstr = newp = srcp = tokenp = retp = NULL;

    char method = mode[0], memuse = mode[1];
    int maxstrlen, tokstrlen;

    /* Put replaced string into malloced 'newstr' */
    if (method == 't') { /* Token replace */
        maxstrlen = strlen(srcstr) * ((strlen(word) > 0) ? strlen(word) : 1);
        newstr = (char *) malloc(maxstrlen + 1);

        for (srcp = (char *) srcstr, newp = newstr; *srcp; srcp++) {
            for (tokenp = (char *) tokstr; *tokenp; tokenp++) {
                if (*srcp == *tokenp) {
                    char *wordp;
                    for (wordp = (char *) word; *wordp; wordp++) {
                        *newp++ = *wordp;
                    }
                    break;
                }
            }
            if (!*tokenp)
                *newp++ = *srcp;
        }
        *newp = '\0';
    } else if (method == 's') { /* String replace */
        if (strlen(word) > strlen(tokstr)) {
            maxstrlen = ((strlen(srcstr) / strlen(tokstr)) * strlen(word))
                    + (strlen(srcstr) % strlen(tokstr));
        } else {
            maxstrlen = strlen(srcstr);
        }
        newstr = (char *) malloc(maxstrlen + 1);
        tokstrlen = strlen(tokstr);

        for (srcp = srcstr, newp = newstr; *srcp; srcp++) {
            if (!strncmp(srcp, tokstr, tokstrlen)) {
                char *wordp;
                for (wordp = (char *) word; *wordp; wordp++)
                    *newp++ = *wordp;
                srcp += tokstrlen - 1;
            } else
                *newp++ = *srcp;
        }
        *newp = '\0';
    } else {
        DEBUG("Unknown mode \"%s\".", mode);
        return NULL;
    }

    /* decide whether newing the memory or replacing into exist one */
    if (memuse == 'n')
        retp = newstr;
    else if (memuse == 'r') {
        strcpy(srcstr, newstr);
        free(newstr);
        retp = srcstr;
    } else {
        DEBUG("Unknown mode \"%s\".", mode);
        free(newstr);
        return NULL;
    }

    return retp;
}

/**
 * Copy src string to dst. The dst string array will be always terminated by
 * NULL character. Also allows overlap between src and dst.
 *
 * @param dst       a pointer of the string to be copied
 * @param size      the size of dst character arrary
 * @param src       a pointer of source string
 *
 * @return always returns a pointer of dst
 */
char *qstrcpy(char *dst, size_t size, const char *src) {
    if (dst == NULL || size == 0 || src == NULL)
        return dst;

    size_t nbytes = strlen(src);
    return qstrncpy(dst, size, src, nbytes);
}

/**
 * Copy src string to dst no more than n bytes. The dst string array will be
 * always terminated by NULL character. Also allows overlap between src and dst.
 *
 * @param dst       a pointer of the string to be copied
 * @param size      the size of dst character arrary
 * @param src       a pointer of source string
 * @param nbytes    number of bytes to copy
 *
 * @return always returns a pointer of dst
 */
char *qstrncpy(char *dst, size_t size, const char *src, size_t nbytes) {
    if (dst == NULL || size == 0 || src == NULL)
        return dst;

    if (nbytes >= size)
        nbytes = size - 1;
    memmove((void *) dst, (void *) src, nbytes);
    dst[nbytes] = '\0';

    return dst;
}

/**
 * Duplicate a formatted string.
 *
 * @param format    string format
 *
 * @return a pointer of malloced string if successful, otherwise returns NULL
 */
char *qstrdupf(const char *format, ...) {
    char *str;
    DYNAMIC_VSPRINTF(str, format);
    if (str == NULL)
        return NULL;

    char *dup = strdup(str);
    free(str);

    return dup;
}

/**
 * Duplicate a substing set
 *
 * @param str       a pointer of original string
 * @param start     substring which is started with this
 * @param end       substring which is ended with this
 *
 * @return a pointer of malloced string if successful, otherwise returns NULL
 */
char *qstrdup_between(const char *str, const char *start, const char *end) {
    char *s;
    if ((s = strstr(str, start)) == NULL)
        return NULL;
    s += strlen(start);

    char *e;
    if ((e = strstr(s, end)) == NULL)
        return NULL;

    int len = e - s;

    char *buf = (char *) malloc(sizeof(char) * (len + 1));
    strncpy(buf, s, len);
    buf[len] = '\0';

    return buf;
}

/**
 * Append formatted string to the end of the source str
 *
 * @param str       a pointer of original string
 * @param format    string format to append
 *
 * @return a pointer of str if successful, otherwise returns NULL
 */
char *qstrcatf(char *str, const char *format, ...) {
    char *buf;
    DYNAMIC_VSPRINTF(buf, format);
    if (buf == NULL)
        return NULL;

    char *ret = strcat(str, buf);
    free(buf);
    return ret;
}

/**
 * Get one line from the string offset.
 *
 * @param buf       buffer pointer
 * @param size      buffer size
 * @param offset    a offset pointer which point source string
 *
 * @return a pointer of buffer if successful, otherwise(EOF) returns NULL
 *
 * @note
 *   CR and LF will not be stored.
 *
 * @code
 *   char *text="Hello\nWorld";
 *
 *   char *offset = text;
 *   char buf[1024];
 *   while(qstrgets(buf, sizeof(buf), &offset) == NULL) {
 *     printf("%s\n", buf);
 *   }
 * @endcode
 */
char *qstrgets(char *buf, size_t size, char **offset) {
    if (offset == NULL || *offset == NULL || **offset == '\0')
        return NULL;

    size_t i;
    char *from = *offset;
    char *to = buf;
    for (i = 0; *from != '\0' && i < (size - 1); i++, from++) {
        if (*from == '\r')
            continue;
        if (*from == '\n') {
            from++;
            break;
        }
        *to = *from;
        to++;
    }
    *to = '\0';
    *offset = from;

    return buf;
}

/**
 * Reverse the order of characters in the string
 *
 * @param str       a pointer of source string
 *
 * @return always returns a pointer of str
 *
 * @note This modify str directly.
 */
char *qstrrev(char *str) {
    if (str == NULL)
        return str;

    char *p1, *p2;
    for (p1 = str, p2 = str + (strlen(str) - 1); p2 > p1; p1++, p2--) {
        char t = *p1;
        *p1 = *p2;
        *p2 = t;
    }

    return str;
}

/**
 * Convert character to bigger character.
 *
 * @param str       a pointer of source string
 *
 * @return always returns a pointer of str
 *
 * @note This modify str directly.
 */
char *qstrupper(char *str) {
    char *cp;

    if (!str)
        return NULL;
    for (cp = str; *cp; cp++)
        if (*cp >= 'a' && *cp <= 'z')
            *cp -= 32;
    return str;
}

/**
 * Convert character to lower character.
 *
 * @param str       a pointer of source string
 *
 * @return always returns a pointer of str
 *
 * @note This modify str directly.
 */
char *qstrlower(char *str) {
    char *cp;

    if (!str)
        return NULL;
    for (cp = str; *cp; cp++)
        if (*cp >= 'A' && *cp <= 'Z')
            *cp += 32;
    return str;
}

/**
 * Split string into tokens
 *
 * @param str           source string
 * @param delimiters    string that specifies a set of delimiters that may
 *                      surround the token being extracted
 * @param retstop       stop delimiter character will be stored. it can be NULL
 *                      if you don't want to know.
 * @param offset        integer pointer used for store last position.
 *                      (must be reset to 0)
 *
 * @return a pointer to the first byte of a token if successful, otherwise
 *         returns NULL.
 *
 * @code
 *   char *str = strdup("Hello,world|Thank,you");
 *   char *token;
 *   int offset = 0;
 *   while((token = qstrtok(str, "|,", NULL, &offset)) != NULL) {
 *     printf("%s\n", token);
 *  }
 * @endcode
 *
 * @note
 *  This may modify str argument.
 *  The major difference between qstrtok() and standard strtok() is that
 *  qstrtok() can returns empty string tokens. If the str is "a:b::d", qstrtok()
 *  returns "a", "b", "", "d". But strtok() returns "a","b","d".
 */
char *qstrtok(char *str, const char *delimiters, char *retstop, int *offset) {
    char *tokensp, *tokenep;

    tokensp = tokenep = (char *) (str + *offset);
    int numdel = strlen(delimiters);
    for (; *tokenep; tokenep++) {
        int j;
        for (j = 0; j < numdel; j++) {
            if (*tokenep == delimiters[j]) {
                if (retstop != NULL)
                    *retstop = delimiters[j];
                *tokenep = '\0';
                tokenep++;
                *offset = tokenep - str;
                return tokensp;
            }
        }
    }

    if (retstop != NULL)
        *retstop = '\0';
    if (tokensp != tokenep) {
        *offset = tokenep - str;
        return tokensp;
    }
    return NULL;
}

/**
 * String Tokenizer
 *
 * @param str           source string
 * @param delimiters    string that specifies a set of delimiters that may
 *                      surround the token being extracted
 *
 * @return qlist container pointer otherwise returns NULL.
 *
 * @code
 *   qlist_t *tokens = qstr_tokenizer("a:b:c", ":");
 *   char *str;
 *   while((str = tokens->popfirst(tokens, NULL)) != NULL) {
 *     printf("%s\n", str);
 *   }
 *   tokens->free(tokens);
 * @endcode
 */
qlist_t *qstrtokenizer(const char *str, const char *delimiters) {
    qlist_t *list = qlist(0);
    if (list == NULL)
        return NULL;

    int i;
    char *dupstr = strdup(str);
    char *token;
    int offset = 0;
    for (i = 1, token = qstrtok(dupstr, delimiters, NULL, &offset);
            token != NULL;
            token = qstrtok(dupstr, delimiters, NULL, &offset), i++) {
        list->addlast(list, token, strlen(token) + 1);
    }
    free(dupstr);

    return list;
}

/**
 * Generate unique id
 *
 * @param seed      additional seed string. this can be NULL
 *
 * @return a pointer of malloced string
 *
 * @note
 *  The length of returned string is 32+1 bytes long including terminating NULL
 *  character. It's a good idea to call srand() once before calling this because
 *  it uses rand().
 */
char *qstrunique(const char *seed) {
    long int usec;
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    usec = ft.dwLowDateTime % 1000000;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    usec = tv.tv_usec;
#endif

    char uniquestr[128];
    snprintf(uniquestr, sizeof(uniquestr), "%u%d%lu%ld%s", getpid(), rand(),
             (unsigned long int) time(NULL), usec, (seed != NULL) ? seed : "");

    unsigned char md5hash[16];
    qhashmd5(uniquestr, strlen(uniquestr), md5hash);
    char *md5ascii = qhex_encode(md5hash, 16);

    return md5ascii;
}

/**
 * Convert integer to comma string.
 *
 * @param number    integer
 *
 * @return a pointer of malloced string which contains comma separated number
 *         if successful, otherwise returns NULL
 */
char *qstr_comma_number(int number) {
    char *str, *strp;

    str = strp = (char *) malloc(sizeof(char) * (14 + 1));
    if (str == NULL)
        return NULL;

    char buf[10 + 1], *bufp;
    snprintf(buf, sizeof(buf), "%d", abs(number));

    if (number < 0)
        *strp++ = '-';
    for (bufp = buf; *bufp != '\0'; strp++, bufp++) {
        *strp = *bufp;
        if ((strlen(bufp)) % 3 == 1 && *(bufp + 1) != '\0')
            *(++strp) = ',';
    }
    *strp = '\0';

    return str;
}

/**
 * Test for an alpha-numeric string
 *
 * @param testfunc  test function for individual character
 * @param str       a pointer of string
 *
 * @return true for ok, otherwise returns false
 *
 * @code
 *   if(qstrtest(isalnum, "hello1234") == true) {
 *     printf("It is alpha-numeric string.");
 *   }
 *
 *   if(qstrtest(isdigit, "0123456789") == true) {
 *     printf("It is alpha-numeric string.");
 *   }
 * @endcode
 *
 * @note
 *  Basically you can use below test functios without creating your own version.
 *  Make sure <ctype.h> header should be included before using any of these
 *  functions.
 *    isalnum - checks for an alphanumeric character.
 *    isalpha - checks for an alphabetic character.
 *    isascii - checks  whether  c is a 7-bit unsigned char value that fits into
 *              the ASCII character set.
 *    isblank - checks for a blank character; that is, a space or a tab.
 *    iscntrl - checks for a control character.
 *    isdigit - checks for a digit (0 through 9).
 *    isgraph - checks for any printable character except space.
 *    islower - checks for a lower-case character.
 *    isprint - checks for any printable character including space.
 *    ispunct - checks for any printable character which is not a  space or an
 *              alphanumeric character.
 *    isspace - checks  for  white-space  characters.
 *    isupper - checks for an uppercase letter.
 *    isxdigit -  checks for a hexadecimal digits.
 *  Please refer "man isalnum" for more details about these functions.
 */
bool qstrtest(int (*testfunc)(int), const char *str) {
    for (; *str; str++) {
        if (testfunc(*str) == 0)
            return false;
    }
    return true;
}

/**
 * Test for an email-address formatted string
 *
 * @param email     email-address formatted string
 *
 * @return true if successful, otherwise returns false
 */
bool qstr_is_email(const char *email) {
    int i, alpa, dot, gol;

    if (email == NULL)
        return false;

    for (i = alpa = dot = gol = 0; email[i] != '\0'; i++) {
        switch (email[i]) {
            case '@': {
                if (alpa == 0)
                    return false;
                if (gol > 0)
                    return false;
                gol++;
                break;
            }
            case '.': {
                if ((i > 0) && (email[i - 1] == '@'))
                    return false;
                if ((gol > 0) && (email[i - 1] == '.'))
                    return false;
                dot++;
                break;
            }
            default: {
                alpa++;
                if ((email[i] >= '0') && (email[i] <= '9'))
                    break;
                else if ((email[i] >= 'A') && (email[i] <= 'Z'))
                    break;
                else if ((email[i] >= 'a') && (email[i] <= 'z'))
                    break;
                else if ((email[i] == '-') || (email[i] == '_'))
                    break;
                else
                    return false;
            }
        }
    }

    if ((alpa <= 3) || (gol == 0) || (dot == 0))
        return false;
    return true;
}

/**
 * Test for an IPv4 address string
 *
 * @param url       IPv4 address string
 *
 * @return true if successful, otherwise returns false
 *
 * @code
 *   if(qstr_is_ip4addr("1.2.3.4") == true) {
 *     printf("It is IPv4 address string.");
 *   }
 * @endcode
 */
bool qstr_is_ip4addr(const char *str) {
    char *dupstr = strdup(str);

    char *s1, *s2;
    int periodcnt;
    for (s1 = dupstr, periodcnt = 0; (s2 = strchr(s1, '.')) != NULL;
            s1 = s2 + 1, periodcnt++) {
        *s2 = '\0';

        int n;
        if (qstrtest(isdigit, s1) == false || (n = atoi(s1)) <= 0 || n >= 256) {
            free(dupstr);
            return false;
        }
    }

    free(dupstr);
    if (periodcnt != 3)
        return false;
    return true;
}

#ifdef __linux__
#include <iconv.h>
#endif

/**
 * Convert character encoding
 *
 * @param str       additional seed string. this can be NULL
 * @param fromcode  encoding type of str
 * @param tocode    encoding to convert
 * @param mag       magnification between fromcode and tocode
 *
 * @return a pointer of malloced converted string if successful,
 *         otherwise returns NULL
 *
 * @code
 *   qCharEncode("KOREAN_EUCKR_STRING", "EUC-KR", "UTF-8", 1.5);
 * @endcode
 */
char *qstr_conv_encoding(const char *str, const char *fromcode,
                         const char *tocode, float mag) {
#ifdef __linux__
    if (str == NULL) return NULL;

    char *fromstr = (char *)str;
    size_t fromsize = strlen(fromstr) + 1;

    size_t tosize = sizeof(char) * ((mag * (fromsize - 1)) + 1);
    char *tostr = (char *)malloc(tosize);
    if (tostr == NULL) return NULL;
    char *tostr1 = tostr;

    iconv_t it = iconv_open(tocode, fromcode);
    if (it < 0) {
        DEBUG("iconv_open() failed.");
        return NULL;
    }

    int ret = iconv(it, &fromstr, &fromsize, &tostr, &tosize);
    iconv_close(it);

    if (ret < 0) {
        DEBUG("iconv() failed.");
        free(tostr1);
        return NULL;
    }

    return tostr1;
#else
    return NULL;
#endif
}
