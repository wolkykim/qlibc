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
 * @file qencode.c Encoding/decoding APIs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "qinternal.h"
#include "utilities/qstring.h"
#include "utilities/qencode.h"

/**
 * Parse URL encoded query string
 *
 * @param tbl       a pointer of qlisttbl_t container. NULL can be used to
 *                  create new table.
 * @param query     URL encoded string.
 * @param equalchar separater of key, value pair.
 * @param sepchar   separater of line.
 * @param count     if count is not NULL, a number of parsed entries are stored.
 *
 * @return qlisttbl container pointer, otherwise returns NULL.
 *
 * @code
 *  cont char query = "category=love&str=%C5%A5%B5%F0%C4%DA%B4%F5&sort=asc";
 *  qlisttbl_t *tbl = qparse_queries(NULL, req->pszQueryString, '=', '&', NULL);
 *  printf("category = %s\n", tbl->get_str(tbl, "category", false));
 *  printf("str = %s\n", tbl->get_str(tbl, "str", false));
 *  printf("sort = %s\n", tbl->get_str(tbl, "sort", false));
 *  tbl->free(tbl);
 * @endcode
 */
qlisttbl_t *qparse_queries(qlisttbl_t *tbl, const char *query, char equalchar,
                           char sepchar, int *count)
{
    if (tbl == NULL) {
        tbl = qlisttbl(0);
        if (tbl == NULL) return NULL;
    }

    char *newquery = NULL;
    int cnt = 0;

    if (query != NULL) newquery = strdup(query);
    while (newquery && *newquery) {
        char *value = _q_makeword(newquery, sepchar);
        char *name = qstrtrim(_q_makeword(value, equalchar));
        qurl_decode(name);
        qurl_decode(value);

        if (tbl->putstr(tbl, name, value) == true) cnt++;
        free(name);
        free(value);
    }
    if (newquery != NULL) free(newquery);
    if (count != NULL) *count = cnt;

    return tbl;
}

/**
 * Encode data using URL encoding(Percent encoding) algorithm.
 *
 * @param bin   a pointer of input data.
 * @param size  the length of input data.
 *
 * @return a malloced string pointer of URL encoded string in case of
 *         successful, otherwise returns NULL
 *
 * @code
 *   const char *text = "hello 'qLibc' world";
 *
 *   char *encstr = qurl_encode(text, strlen(text));
 *   if(encstr == NULL) return -1;
 *
 *   printf("Original: %s\n", text);
 *   printf("Encoded : %s\n", encstr);
 *
 *   size_t decsize = qurl_decode(encstr);
 *
 *   printf("Decoded : %s (%zu bytes)\n", encstr, decsize);
 *   free(encstr);
 *
 *   --[output]--
 *   Original: hello 'qLibc' world
 *   Encoded:  hello%20%27qLibc%27%20world
 *   Decoded:  hello 'qLibc' world (19 bytes)
 * @endcode
 */
char *qurl_encode(const void *bin, size_t size)
{
    const char URLCHARTBL[16*16] = {
        00 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,// 00-0F
        00 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,// 10-1F
        00 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,'-','.','/',// 20-2F
        '0','1','2','3','4','5','6','7','8','9',':', 0 , 0 , 0 , 0 , 0 ,// 30-3F
        '@','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',// 40-4F
        'P','Q','R','S','T','U','V','W','X','Y','Z', 0 ,'\\',0 , 0 ,'_',// 50-5F
        00 ,'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',// 60-6f
        'p','q','r','s','t','u','v','w','x','y','z', 0 , 0 , 0 , 0 , 0 ,// 70-7F
        00 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,// 80-8F
        00 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,// 90-9F
        00 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,// A0-AF
        00 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,// B0-BF
        00 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,// C0-CF
        00 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,// D0-DF
        00 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,// E0-EF
        00 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0  // F0-FF
    }; // 0 means must be encoded.

    if (bin == NULL) return NULL;
    if (size == 0) return strdup("");

    // malloc buffer
    char *pszEncStr = (char *)malloc((size * 3) + 1);
    if (pszEncStr == NULL) return NULL;

    char *pszEncPt = pszEncStr;
    char *pBinPt = (char *)bin;
    const char *pBinEnd = (bin + size - 1);
    for (; pBinPt <= pBinEnd; pBinPt++) {
        unsigned char c = *pBinPt;
        if (URLCHARTBL[c] != 0) {
            *pszEncPt++ = *pBinPt;
        } else {
            unsigned char cUpper4 = (c >> 4);
            unsigned char cLower4 = (c & 0x0F);

            *pszEncPt++ = '%';
            *pszEncPt++ = (cUpper4 < 0x0A) ? (cUpper4 + '0')
                          : ((cUpper4 - 0x0A) + 'a');
            *pszEncPt++ = (cLower4 < 0x0A) ? (cLower4 + '0')
                          : ((cLower4 - 0x0A) + 'a');
        }
    }
    *pszEncPt = '\0';

    return pszEncStr;
}

/**
 * Decode URL encoded string.
 *
 * @param str   a pointer of URL encoded string.
 *
 * @return the length of bytes stored in the str memory in case of successful,
 *         otherwise returns NULL
 *
 * @note
 *  This modify str directly. And the 'str' is always terminated by NULL
 *  character.
 */
size_t qurl_decode(char *str)
{
    if (str == NULL) {
        return 0;
    }

    char *pEncPt, *pBinPt = str;
    for (pEncPt = str; *pEncPt != '\0'; pEncPt++) {
        switch (*pEncPt) {
            case '+': {
                *pBinPt++ = ' ';
                break;
            }
            case '%': {
                *pBinPt++ = _q_x2c(*(pEncPt + 1), *(pEncPt + 2));
                pEncPt += 2;
                break;
            }
            default: {
                *pBinPt++ = *pEncPt;
                break;
            }
        }
    }
    *pBinPt = '\0';

    return (pBinPt - str);
}

/**
 * Encode data using BASE64 algorithm.
 *
 * @param bin   a pointer of input data.
 * @param size  the length of input data.
 *
 * @return a malloced string pointer of BASE64 encoded string in case of
 *         successful, otherwise returns NULL
 *
 * @code
 *   const char *text = "hello world";
 *
 *   char *encstr = qbase64_encode(text, strlen(text));
 *   if(encstr == NULL) return -1;
 *
 *   printf("Original: %s\n", text);
 *   printf("Encoded : %s\n", encstr);
 *
 *   size_t decsize = qbase64_decode(encstr);
 *
 *   printf("Decoded : %s (%zu bytes)\n", encstr, decsize);
 *   free(encstr);
 *
 *   --[output]--
 *   Original: hello world
 *   Encoded:  aGVsbG8gd29ybGQ=
 *   Decoded:  hello world (11 bytes)
 * @endcode
 */
char *qbase64_encode(const void *bin, size_t size)
{
    const char B64CHARTBL[64] = {
        'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',// 00-0F
        'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',// 10-1F
        'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',// 20-2F
        'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/' // 30-3F
    };

    if (size == 0) {
        return strdup("");
    }

    // malloc for encoded string
    char *pszB64 = (char *)malloc(4 * ((size / 3) + ((size % 3 == 0) ? 0 : 1))
                                  + 1);
    if (pszB64 == NULL) {
        return NULL;
    }

    char *pszB64Pt = pszB64;
    unsigned char *pBinPt, *pBinEnd = (unsigned char *)(bin + size - 1);
    unsigned char szIn[3] = {0,0,0};
    int nOffset;
    for (pBinPt = (unsigned char *)bin, nOffset = 0;
         pBinPt <= pBinEnd;
         pBinPt++, nOffset++) {
        int nIdxOfThree = nOffset % 3;
        szIn[nIdxOfThree] = *pBinPt;
        if (nIdxOfThree < 2 && pBinPt < pBinEnd) continue;

        *pszB64Pt++ = B64CHARTBL[((szIn[0] & 0xFC) >> 2)];
        *pszB64Pt++ = B64CHARTBL[(((szIn[0] & 0x03) << 4)
                                  | ((szIn[1] & 0xF0) >> 4))];
        *pszB64Pt++ = (nIdxOfThree >= 1) ? B64CHARTBL[(((szIn[1] & 0x0F) << 2)
                      | ((szIn[2] & 0xC0) >> 6))]
                      : '=';
        *pszB64Pt++ = (nIdxOfThree >= 2) ? B64CHARTBL[(szIn[2] & 0x3F)] : '=';

        memset((void *)szIn, 0, sizeof(szIn));
    }
    *pszB64Pt = '\0';

    return pszB64;
}

/**
 * Decode BASE64 encoded string.
 *
 * @param str   a pointer of Base64 encoded string.
 *
 * @return the length of bytes stored in the str memory in case of successful,
 *         otherwise returns NULL
 *
 * @note
 *  This modify str directly. And the 'str' is always terminated by NULL
 *  character.
 */
size_t qbase64_decode(char *str)
{
    const char B64MAPTBL[16*16] = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 00-0F
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 10-1F
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63, // 20-2F
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64, // 30-3F
        64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, // 40-4F
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64, // 50-5F
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, // 60-6F
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64, // 70-7F
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 80-8F
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // 90-9F
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // A0-AF
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // B0-BF
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // C0-CF
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // D0-DF
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, // E0-EF
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64  // F0-FF
    };

    char *pEncPt, *pBinPt = str;
    int nIdxOfFour = 0;
    char cLastByte = 0;
    for (pEncPt = str; *pEncPt != '\0'; pEncPt++) {
        char cByte = B64MAPTBL[(unsigned char)(*pEncPt)];
        if (cByte == 64) continue;

        if (nIdxOfFour == 0) {
            nIdxOfFour++;
        } else if (nIdxOfFour == 1) {
            // 00876543 0021????
            //*pBinPt++ = ( ((cLastByte << 2) & 0xFC) | ((cByte >> 4) & 0x03) );
            *pBinPt++ = ( (cLastByte << 2) | (cByte >> 4) );
            nIdxOfFour++;
        } else if (nIdxOfFour == 2) {
            // 00??8765 004321??
            //*pBinPt++ = ( ((cLastByte << 4) & 0xF0) | ((cByte >> 2) & 0x0F) );
            *pBinPt++ = ( (cLastByte << 4) | (cByte >> 2) );
            nIdxOfFour++;
        } else {
            // 00????87 00654321
            //*pBinPt++ = ( ((cLastByte << 6) & 0xC0) | (cByte & 0x3F) );
            *pBinPt++ = ( (cLastByte << 6) | cByte );
            nIdxOfFour = 0;
        }

        cLastByte = cByte;
    }
    *pBinPt = '\0';

    return (pBinPt - str);
}

/**
 * Encode data to Hexadecimal digit format.
 *
 * @param bin   a pointer of input data.
 * @param size  the length of input data.
 *
 * @return a malloced string pointer of Hexadecimal encoded string in case of
 *         successful, otherwise returns NULL
 *
 * @code
 *   const char *text = "hello world";
 *
 *   char *encstr = qhex_encode(text, strlen(text));
 *   if(encstr == NULL) return -1;
 *
 *   printf("Original: %s\n", text);
 *   printf("Encoded : %s\n", encstr);
 *
 *   size_t decsize = qhex_decode(encstr);
 *
 *   printf("Decoded : %s (%zu bytes)\n", encstr, decsize);
 *   free(encstr);
 *
 *   return 0;
 *
 *   --[output]--
 *   Original: hello world
 *   Encoded : 68656c6c6f20776f726c64
 *   Decoded : hello world (11 bytes)
 * @endcode
 */
char *qhex_encode(const void *bin, size_t size)
{
    const char HEXCHARTBL[16] = {
        '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
    };

    char *pHexStr = (char *)malloc(sizeof(char) * ((size * 2) + 1));
    if (pHexStr == NULL) return NULL;

    unsigned char *pSrc = (unsigned char *)bin;
    char *pHexPt = pHexStr;
    int i;
    for (i = 0; i < size; i++) {
        *pHexPt++ = HEXCHARTBL[(pSrc[i] >> 4)];
        *pHexPt++ = HEXCHARTBL[(pSrc[i] & 0x0F)];
    }
    *pHexPt = '\0';

    return pHexStr;
}

/**
 * Decode Hexadecimal encoded data.
 *
 * @param str   a pointer of Hexadecimal encoded string.
 *
 * @return the length of bytes stored in the str memory in case of successful,
 *         otherwise returns NULL
 *
 * @note
 *  This modify str directly. And the 'str' is always terminated by NULL
 *  character.
 */
size_t qhex_decode(char *str)
{
    const char HEXMAPTBL[16*16] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 00-0F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 10-1F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 20-2F
        0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  0,  0,  0,  0,  0, // 30-3F
        0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 40-4F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 50-5F
        0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 60-6f
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 70-7F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 80-8F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 90-9F
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // A0-AF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // B0-BF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // C0-CF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // D0-DF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // E0-EF
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  // F0-FF
    };

    char *pEncPt, *pBinPt = str;
    for (pEncPt = str; *pEncPt != '\0'; pEncPt += 2) {
        *pBinPt++ = (HEXMAPTBL[(unsigned char)(*pEncPt)] << 4)
                    + HEXMAPTBL[(unsigned char)(*(pEncPt+1))];
    }
    *pBinPt = '\0';

    return (pBinPt - str);
}
