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
 ******************************************************************************/

/**
 * Apache-style Configuration File Parser.
 *
 * This is a qLibc extension implementing Apache-style configuration file
 * parser.
 *
 * @file qaconf.h
 */

#ifndef _QACONF_H
#define _QACONF_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* types */
typedef struct qaconf_s qaconf_t;
typedef struct qaconf_option_s qaconf_option_t;
typedef struct qaconf_cbdata_s qaconf_cbdata_t;
typedef char *(qaconf_cb_t) (qaconf_cbdata_t *data, void *userdata);

/* public functions */
extern qaconf_t *qaconf(void);

/* user's callback function prototype */
#define QAC_CB(func) char *func(qaconf_cbdata_t *data, void *userdata)
#define QAC_TAKEn(n) (n)

/* parser option flags */
enum {
    QAC_CASEINSENSITIVE     = (1),
    QAC_IGNOREUNKNOWN       = (2),  // Ignore unknown option directives.
};

/**
 * Argument type check
 *
 * uint32_t type 32bit variable is used for passing argument types.
 *  notused  bool   float   int     #arg
 *  ---- ---====== ---- --== ==== ---- ----
 *  rrrr rrBb bbbb Ffff ffIi iiii aaaa aaaa  (32bit mask)
 *   (6bit) (6bit)  (6bit) (6bit)   (8bit)
 *
 *  r : Not Used
 *  B : Consider all arguments as BOOL type unless individually specified.
 *  b : Flaged argument(1~5) must be bool type.
 *  F : Consider all arguments as FLOAT type unless individually specified.
 *  f : Flaged argument(1~5) must be float type.
 *  I : Consider all arguments as INTEGER type unless individually specified.
 *  i : Flaged argument(1~5) must be integer type.
 *  a : Number of arguments (0~254).
 *      Value 255 means take any number of arguments.
 *
 *  An option takes 1 argument.
 *    QAC_TAKE_STR    <= String(any) type
 *    QAC_TAKE_INT    <= Integer type
 *    QAC_TAKE_FLOAT  <= Float type
 *    QAC_TAKE_BOOL   <= Bool type
 *
 *    QAC_TAKE1               <= equivalent to QAC_TAKE_STR
 *    QAC_TAKE1 | QAC_A1_BOOL <= equivalent to QAC_TAKE_BOOL
 *
 *  An option takes 2 arguments, bool and float.
 *    QAC_TAKE2 | QAC_A1_BOOL | QAC_A2_FLOAT
 *
 *  An option takes any number of arguments in any type.
 *    QAC_TAKEALL
 *
 *  An option takes any number of arguments but 1st one must be bool and
 *  2nd one must be integer and rest of them must be float.
 *    QAC_TAKEALL | QAC_A1_BOOL | QAC_A2_INT | QAC_AA_FLOAT
 */
enum qaconf_take {
    // Define string(any) type argument.
    QAC_A1_STR          = 0,
    QAC_A2_STR          = 0,
    QAC_A3_STR          = 0,
    QAC_A4_STR          = 0,
    QAC_A5_STR          = 0,
    QAC_AA_STR          = 0, // All string unless individually specified.

    // Define integer type argument.
    QAC_A1_INT          = (1 << 8),
    QAC_A2_INT          = (QAC_A1_INT << 1),
    QAC_A3_INT          = (QAC_A1_INT << 2),
    QAC_A4_INT          = (QAC_A1_INT << 3),
    QAC_A5_INT          = (QAC_A1_INT << 4),
    QAC_AA_INT          = (QAC_A1_INT << 5), // All integer unless specified.

    // Define floating point type argument.
    QAC_A1_FLOAT        = (1 << 16),
    QAC_A2_FLOAT        = (QAC_A1_FLOAT << 1),
    QAC_A3_FLOAT        = (QAC_A1_FLOAT << 2),
    QAC_A4_FLOAT        = (QAC_A1_FLOAT << 3),
    QAC_A5_FLOAT        = (QAC_A1_FLOAT << 4),
    QAC_AA_FLOAT        = (QAC_A1_FLOAT << 5), // All float unless specified.

    // Define bool(true/false, yes/no, on/off, 1/0)  type argument.
    QAC_A1_BOOL         = (1 << 24),
    QAC_A2_BOOL         = (QAC_A1_BOOL << 1),
    QAC_A3_BOOL         = (QAC_A1_BOOL << 2),
    QAC_A4_BOOL         = (QAC_A1_BOOL << 3),
    QAC_A5_BOOL         = (QAC_A1_BOOL << 4),
    QAC_AA_BOOL         = (QAC_A1_BOOL << 5), // All bool unless specified.

    // Number of arguments to take
    QAC_TAKENONE        = QAC_TAKEn(0),
    QAC_TAKE0           = QAC_TAKENONE,
    QAC_TAKE1           = QAC_TAKEn(1),
    QAC_TAKE2           = QAC_TAKEn(2),
    QAC_TAKE3           = QAC_TAKEn(3),
    QAC_TAKE4           = QAC_TAKEn(4),
    QAC_TAKE5           = QAC_TAKEn(5),
    // use QAC_TAKEn(N) macro for 6~254 arguments.
    QAC_TAKEALL         = 0xFF, // Take any number of elements. (0 ~ INT_MAX)

    // Convenient synonyms
    QAC_TAKE_STR        = (QAC_TAKE1 | QAC_A1_STR),
    QAC_TAKE_INT        = (QAC_TAKE1 | QAC_A1_INT),
    QAC_TAKE_FLOAT      = (QAC_TAKE1 | QAC_A1_FLOAT),
    QAC_TAKE_BOOL       = (QAC_TAKE1 | QAC_A1_BOOL),
};
#define QAC_TAKEn(n)    (n)

/* pre-defined sections */
enum qaconf_section {
    QAC_SECTION_ALL  = (0),        /* pre-defined */
    QAC_SECTION_ROOT = (1)         /* pre-defined */
};

/* option type */
enum qaconf_otype {
    QAC_OTYPE_OPTION     = 0,    /* general option */
    QAC_OTYPE_SECTIONOPEN  = 1,  /* section open <XXX> */
    QAC_OTYPE_SECTIONCLOSE = 2   /* section close </XXX> */
};

/**
 * qaconf_t object.
 */
struct qaconf_s {
    /* capsulated member functions */
    int (*addoptions) (qaconf_t *qaconf, const qaconf_option_t *options);
    void (*setdefhandler) (qaconf_t *qaconf, const qaconf_cb_t *callback);
    void (*setuserdata) (qaconf_t *qaconf, const void *userdata);
    int (*parse) (qaconf_t *qaconf, const char *filepath, uint8_t flags);
    const char *(*errmsg) (qaconf_t *qaconf);
    void (*reseterror) (qaconf_t *qaconf);
    void (*free) (qaconf_t *qaconf);

    /* private variables - do not access directly */
    int numoptions;             /*!< a number of user defined options */
    qaconf_option_t *options;   /*!< option data */

    qaconf_cb_t *defcb;         /*!< default callback for unregistered option */
    void *userdata;             /*!< userdata */

    char *filepath;             /*!< current processing file's path */
    int lineno;                 /*!< current processing line number */

    char *errstr;               /*!< last error string */
};

/**
 * structure for user option definition.
 */
struct qaconf_option_s {
    char *name;             /*!< name of option. */
    uint32_t take;          /*!< number of arguments and type checking */
    qaconf_cb_t *cb;        /*!< callback function */
    uint64_t sectionid;     /*!< sectionid if this is a section */
    uint64_t sections;      /*!< ORed sectionid(s) where this option is allowed */
};
#define QAC_OPTION_END  {NULL, 0, NULL, 0, 0}

/**
 * callback data structure.
 */
struct qaconf_cbdata_s {
    enum qaconf_otype otype;  /*!< option type */
    uint64_t section;         /*!< current section where this option is located */
    uint64_t sections;        /*!< ORed all parent's sectionid(s) including current sections */
    uint8_t level;            /*!< number of parents(level), root level is 0 */
    qaconf_cbdata_t *parent;  /*!< upper parent link */

    int argc;       /*!< number arguments. always equal or greater than 1. */
    char **argv;    /*!< argument pointers. argv[0] is option name. */

    void *data;     /*!< where actual data is stored */
};

#ifdef __cplusplus
}
#endif

#endif /*_QACONF_H */
