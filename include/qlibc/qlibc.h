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
 * qlibc header file
 *
 * @file qlibc.h
 */

#ifndef _QLIBC_H
#define _QLIBC_H

#define _Q_PRGNAME "qlibc"  /*!< qlibc human readable name */
#define _Q_VERSION "2.1.5"  /*!< qlibc version number string */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <sys/types.h>
#include <netinet/in.h>

/******************************************************************************
 * COMMON DATA STRUCTURES
 ******************************************************************************/

typedef struct qmutex_s qmutex_t;    /*!< qlibc pthread mutex type*/
typedef struct qobj_s qobj_t;        /*!< object type*/
typedef struct qnobj_s qnobj_t;      /*!< named-object type*/
typedef struct qdlobj_s qdlobj_t;    /*!< doubly-linked-object type*/
typedef struct qdlnobj_s qdlnobj_t;  /*!< doubly-linked-named-object type*/
typedef struct qhnobj_s qhnobj_t;    /*!< hashed-named-object type*/

/**
 * qlibc pthread mutex data structure.
 */
struct qmutex_s {
    pthread_mutex_t mutex;  /*!< pthread mutex */
    pthread_t owner;        /*!< mutex owner thread id */
    int count;              /*!< recursive lock counter */
};

/**
 * object data structure.
 */
struct qobj_s {
    void *data;         /*!< data */
    size_t size;        /*!< data size */
    uint8_t type;       /*!< data type */
};

/**
 * named-object data structure.
 */
struct qnobj_s {
    char *name;         /*!< object name */
    void *data;         /*!< data */
    size_t size;        /*!< data size */
};

/**
 * doubly-linked-object data structure.
 */
struct qdlobj_s {
    void *data;         /*!< data */
    size_t size;        /*!< data size */

    qdlobj_t *prev;     /*!< previous link */
    qdlobj_t *next;     /*!< next link */
};

/**
 * doubly-linked-named-object data structure.
 */
struct qdlnobj_s {
    uint32_t hash;      /*!< 32bit-hash value of object name */
    char *name;         /*!< object name */
    void *data;         /*!< data */
    size_t size;        /*!< data size */

    qdlnobj_t *prev;    /*!< previous link */
    qdlnobj_t *next;    /*!< next link */
};

/**
 * hashed-named-object data structure.
 */
struct qhnobj_s {
    uint32_t hash;      /*!< 32bit-hash value of object name */
    char *name;         /*!< object name */
    void *data;         /*!< data */
    size_t size;        /*!< data size */

    qhnobj_t *next;     /*!< for chaining next collision object */
};

/******************************************************************************
 * Doubly Linked-list Container
 * qlist.c
 ******************************************************************************/

/* constants */
#define QLIST_OPT_THREADSAFE (0x01)

/* types */
typedef struct qlist_s qlist_t;

/* public functions */
extern qlist_t *qlist(int options);  /*!< qlist constructor */

/**
 * qlist container
 */
struct qlist_s {
    /* capsulated member functions */
    size_t (*setsize) (qlist_t *list, size_t max);

    bool (*addfirst) (qlist_t *list, const void *data, size_t size);
    bool (*addlast) (qlist_t *list, const void *data, size_t size);
    bool (*addat) (qlist_t *list, int index, const void *data, size_t size);

    void *(*getfirst) (qlist_t *list, size_t *size, bool newmem);
    void *(*getlast) (qlist_t *list, size_t *size, bool newmem);
    void *(*getat) (qlist_t *list, int index, size_t *size, bool newmem);
    bool (*getnext) (qlist_t *list, qdlobj_t *obj, bool newmem);

    void *(*popfirst) (qlist_t *list, size_t *size);
    void *(*poplast) (qlist_t *list, size_t *size);
    void *(*popat) (qlist_t *list, int index, size_t *size);

    bool (*removefirst) (qlist_t *list);
    bool (*removelast) (qlist_t *list);
    bool (*removeat) (qlist_t *list, int index);

    void (*reverse) (qlist_t *list);
    void (*clear) (qlist_t *list);

    size_t (*size) (qlist_t *list);
    size_t (*datasize) (qlist_t *list);

    void *(*toarray) (qlist_t *list, size_t *size);
    char *(*tostring) (qlist_t *list);
    bool (*debug) (qlist_t *list, FILE *out);

    void (*lock) (qlist_t *list);
    void (*unlock) (qlist_t *list);

    void (*free) (qlist_t *list);

    /* private variables - do not access directly */
    qmutex_t *qmutex; /*!< initialized when QLIST_OPT_THREADSAFE is given */
    size_t num;       /*!< number of elements */
    size_t max;       /*!< maximum number of elements. 0 means no limit */
    size_t datasum;   /*!< total sum of data size, does not include name size */

    qdlobj_t *first;  /*!< first object pointer */
    qdlobj_t *last;   /*!< last object pointer */
};

/******************************************************************************
 * Key-Value Table in Linked-list Container
 * qlisttbl.c
 ******************************************************************************/

/* constants */
#define QLISTTBL_OPT_THREADSAFE (0x01)

/* types */
typedef struct qlisttbl_s qlisttbl_t;

/* public functions */
extern qlisttbl_t *qlisttbl(int options);  /*!< qlisttbl constructor */

/**
 * qlisttbl container
 */
struct qlisttbl_s {
    /* capsulated member functions */
    bool (*setcase) (qlisttbl_t *tbl, bool insensitive);
    int (*setsort) (qlisttbl_t *tbl, bool sort, bool descending);
    bool (*setputdir) (qlisttbl_t *tbl, bool before);
    bool (*setgetdir) (qlisttbl_t *tbl, bool forward);
    bool (*setnextdir) (qlisttbl_t *tbl, bool backward);

    bool (*put) (qlisttbl_t *tbl, const char *name, const void *data,
                 size_t size, bool replace);
    bool (*putstr) (qlisttbl_t *tbl, const char *name, const char *str,
                    bool replace);
    bool (*putstrf) (qlisttbl_t *tbl, bool replace, const char *name,
                     const char *format, ...);
    bool (*putint) (qlisttbl_t *tbl, const char *name, int64_t num,
                    bool replace);

    void *(*get) (qlisttbl_t *tbl, const char *name, size_t *size, bool newmem);
    char *(*getstr) (qlisttbl_t *tbl, const char *name, bool newmem);
    int64_t (*getint) (qlisttbl_t *tbl, const char *name);

    qobj_t *(*getmulti) (qlisttbl_t *tbl, const char *name, bool newmem,
                         size_t *numobjs);
    void (*freemulti) (qobj_t *objs);

    bool (*getnext) (qlisttbl_t *tbl, qdlnobj_t *obj, const char *name,
                     bool newmem);

    size_t (*remove) (qlisttbl_t *tbl, const char *name);
    bool (*removeobj) (qlisttbl_t *tbl, const qdlnobj_t *obj);

    size_t (*size) (qlisttbl_t *tbl);
    void (*sort) (qlisttbl_t *tbl, bool descending);
    void (*reverse) (qlisttbl_t *tbl);
    void (*clear) (qlisttbl_t *tbl);

    bool (*save) (qlisttbl_t *tbl, const char *filepath, char sepchar,
                  bool encode);
    ssize_t (*load) (qlisttbl_t *tbl, const char *filepath, char sepchar,
                     bool decode);
    bool (*debug) (qlisttbl_t *tbl, FILE *out);

    void (*lock) (qlisttbl_t *tbl);
    void (*unlock) (qlisttbl_t *tbl);

    void (*free) (qlisttbl_t *tbl);

    /* private methods */
    bool (*namematch) (qdlnobj_t *obj, const char *name, uint32_t hash);
    int (*namecmp) (const char *s1, const char *s2);

    /* private variables - do not access directly */
    bool lookupcase; /*!< key lookup case-sensivity. false: case-sensitive */
    int  sortflag; /*!< sort flag. 0:disabled, 1:ascending, 2:descending */
    bool putdir;  /*!< object appending direction for put(). false: to bottom */
    bool getdir;  /*!< object lookup direction for get(). false:from bottom */
    bool nextdir; /*!< object traversal direction for next(). false:from top */

    qmutex_t *qmutex;   /*!< initialized when QLISTTBL_OPT_THREADSAFE is given */
    size_t num;         /*!< number of elements */
    qdlnobj_t *first;   /*!< first object pointer */
    qdlnobj_t *last;    /*!< last object pointer */
};

/******************************************************************************
 * Dynamic Hash Table Container
 * qhashtbl.c
 ******************************************************************************/

/* constants */
#define QHASHTBL_OPT_THREADSAFE (0x01)

/* types */
typedef struct qhashtbl_s qhashtbl_t;

/* public functions */
extern qhashtbl_t *qhashtbl(size_t range, int options);  /*!< qhashtbl constructor */

/**
 * qhashtbl container
 */
struct qhashtbl_s {
    /* encapsulated member functions */
    bool (*put) (qhashtbl_t *tbl, const char *name, const void *data, size_t size);
    bool (*putstr) (qhashtbl_t *tbl, const char *name, const char *str);
    bool (*putstrf) (qhashtbl_t *tbl, const char *name, const char *format, ...);
    bool (*putint) (qhashtbl_t *tbl, const char *name, const int64_t num);

    void *(*get) (qhashtbl_t *tbl, const char *name, size_t *size, bool newmem);
    char *(*getstr) (qhashtbl_t *tbl, const char *name, bool newmem);
    int64_t (*getint) (qhashtbl_t *tbl, const char *name);

    bool (*getnext) (qhashtbl_t *tbl, qhnobj_t *obj, bool newmem);

    bool (*remove) (qhashtbl_t *tbl, const char *name);

    size_t (*size) (qhashtbl_t *tbl);
    void (*clear) (qhashtbl_t *tbl);
    bool (*debug) (qhashtbl_t *tbl, FILE *out);

    void (*lock) (qhashtbl_t *tbl);
    void (*unlock) (qhashtbl_t *tbl);

    void (*free) (qhashtbl_t *tbl);

    /* private variables - do not access directly */
    qmutex_t *qmutex;   /*!< initialized when QHASHTBL_OPT_THREADSAFE is given */
    size_t num;         /*!< number of objects in this table */
    size_t range;       /*!< hash range, vertical number of slots */
    qhnobj_t **slots;   /*!< slot pointer container */
};

/******************************************************************************
 * Static Hash Table Container - works in fixed size memory
 * qhasharr.c
 ******************************************************************************/

/* tunable knobs */
#define _Q_HASHARR_KEYSIZE (16)    /*!< knob for maximum key size. */
#define _Q_HASHARR_VALUESIZE (32)  /*!< knob for maximum data size in a slot. */

/* types */
typedef struct qhasharr_s qhasharr_t;
typedef struct qhasharr_slot_s qhasharr_slot_t;
typedef struct qhasharr_data_s qhasharr_data_t;

/* public functions */
extern qhasharr_t *qhasharr(void *memory, size_t memsize);
extern size_t qhasharr_calculate_memsize(int max);

/**
 * qhasharr internal data slot structure
 */
struct qhasharr_slot_s {
    short  count;   /*!< hash collision counter. 0 indicates empty slot,
                     -1 is used for collision resolution, -2 is used for
                     indicating linked block */
    uint32_t  hash; /*!< key hash. we use FNV32 */

    uint8_t size;   /*!< value size in this slot*/
    int link;       /*!< next link */

    union {
        /*!< key/value data */
        struct _Q_HASHARR_SLOT_KEYVAL {
            unsigned char value[_Q_HASHARR_VALUESIZE];  /*!< value */

            char key[_Q_HASHARR_KEYSIZE];  /*!< key string, can be cut */
            uint16_t  keylen;              /*!< original key length */
            unsigned char keymd5[16];      /*!< md5 hash of the key */
        } pair;

        /*!< extended data block, used only when the count value is -2 */
        struct _Q_HASHARR_SLOT_EXT {
            unsigned char value[sizeof(struct _Q_HASHARR_SLOT_KEYVAL)];
        } ext;
    } data;
};

/**
 * qhasharr memory structure
 */
struct qhasharr_data_s {
    int maxslots;       /*!< number of maximum slots */
    int usedslots;      /*!< number of used slots */
    int num;            /*!< number of stored keys */
    qhasharr_slot_t *slots;  /*!< data area pointer */
};

/**
 * qhasharr container
 */
struct qhasharr_s {
    /* capsulated member functions */
    bool (*put) (qhasharr_t *tbl, const char *key, const void *value,
                 size_t size);
    bool (*putstr) (qhasharr_t *tbl, const char *key, const char *str);
    bool (*putstrf) (qhasharr_t *tbl, const char *key, const char *format, ...);
    bool (*putint) (qhasharr_t *tbl, const char *key, int64_t num);

    void *(*get) (qhasharr_t *tbl, const char *key, size_t *size);
    char *(*getstr) (qhasharr_t *tbl, const char *key);
    int64_t (*getint) (qhasharr_t *tbl, const char *key);
    bool (*getnext) (qhasharr_t *tbl, qnobj_t *obj, int *idx);

    bool (*remove) (qhasharr_t *tbl, const char *key);

    int  (*size) (qhasharr_t *tbl, int *maxslots, int *usedslots);
    void (*clear) (qhasharr_t *tbl);
    bool (*debug) (qhasharr_t *tbl, FILE *out);

    void (*free) (qhasharr_t *tbl);

    /* private variables */
    qhasharr_data_t *data;
};

/******************************************************************************
 * Vector Container
 * qvector.c
 ******************************************************************************/

/* constants */
#define QVECTOR_OPT_THREADSAFE (QLIST_OPT_THREADSAFE)

/* types */
typedef struct qvector_s qvector_t;

/* public functions */
extern qvector_t *qvector(int options);

/**
 * qvector container.
 */
struct qvector_s {
    /* capsulated member functions */
    bool (*add) (qvector_t *vector, const void *data, size_t size);
    bool (*addstr) (qvector_t *vector, const char *str);
    bool (*addstrf) (qvector_t *vector, const char *format, ...);

    void *(*toarray) (qvector_t *vector, size_t *size);
    char *(*tostring) (qvector_t *vector);

    size_t (*size) (qvector_t *vector);
    size_t (*datasize) (qvector_t *vector);
    void (*clear) (qvector_t *vector);
    bool (*debug) (qvector_t *vector, FILE *out);

    void (*free) (qvector_t *vector);

    /* private variables - do not access directly */
    qlist_t *list;  /*!< data container */
};

/******************************************************************************
 * Queue Container
 * qqueue.c
 ******************************************************************************/

/* constants */
#define QQUEUE_OPT_THREADSAFE (QLIST_OPT_THREADSAFE)

/* types */
typedef struct qqueue_s qqueue_t;

/* public functions */
extern qqueue_t *qqueue(int options);

/**
 * qqueue container
 */
struct qqueue_s {
    /* capsulated member functions */
    size_t (*setsize) (qqueue_t *stack, size_t max);

    bool (*push) (qqueue_t *stack, const void *data, size_t size);
    bool (*pushstr) (qqueue_t *stack, const char *str);
    bool (*pushint) (qqueue_t *stack, int64_t num);

    void *(*pop) (qqueue_t *stack, size_t *size);
    char *(*popstr) (qqueue_t *stack);
    int64_t (*popint) (qqueue_t *stack);
    void *(*popat) (qqueue_t *stack, int index, size_t *size);

    void *(*get) (qqueue_t *stack, size_t *size, bool newmem);
    char *(*getstr) (qqueue_t *stack);
    int64_t (*getint) (qqueue_t *stack);
    void *(*getat) (qqueue_t *stack, int index, size_t *size, bool newmem);

    size_t (*size) (qqueue_t *stack);
    void (*clear) (qqueue_t *stack);
    bool (*debug) (qqueue_t *stack, FILE *out);
    void (*free) (qqueue_t *stack);

    /* private variables - do not access directly */
    qlist_t  *list;  /*!< data container */
};

/******************************************************************************
 * LIFO Stack Container
 * qstack.c
 ******************************************************************************/

/* constants */
#define QSTACK_OPT_THREADSAFE (QLIST_OPT_THREADSAFE)

/* types */
typedef struct qstack_s qstack_t;

/* public functions */
extern qstack_t *qstack(int options);

/**
 * qstack container
 */
struct qstack_s {
    /* capsulated member functions */
    size_t (*setsize) (qstack_t *stack, size_t max);

    bool (*push) (qstack_t *stack, const void *data, size_t size);
    bool (*pushstr) (qstack_t *stack, const char *str);
    bool (*pushint) (qstack_t *stack, int64_t num);

    void *(*pop) (qstack_t *stack, size_t *size);
    char *(*popstr) (qstack_t *stack);
    int64_t (*popint) (qstack_t *stack);
    void *(*popat) (qstack_t *stack, int index, size_t *size);

    void *(*get) (qstack_t *stack, size_t *size, bool newmem);
    char *(*getstr) (qstack_t *stack);
    int64_t (*getint) (qstack_t *stack);
    void *(*getat) (qstack_t *stack, int index, size_t *size, bool newmem);

    size_t (*size) (qstack_t *stack);
    void (*clear) (qstack_t *stack);
    bool (*debug) (qstack_t *stack, FILE *out);
    void (*free) (qstack_t *stack);

    /* private variables - do not access directly */
    qlist_t  *list;  /*!< data container */
};

/******************************************************************************
 * UTILITIES SECTION
 ******************************************************************************/

/* qcount.c */
extern int64_t qcount_read(const char *filepath);
extern bool qcount_save(const char *filepath, int64_t number);
extern int64_t qcount_update(const char *filepath, int64_t number);

/* qencode.c */
extern qlisttbl_t *qparse_queries(qlisttbl_t *tbl, const char *query,
                                  char equalchar, char sepchar, int *count);
extern char *qurl_encode(const void *bin, size_t size);
extern size_t qurl_decode(char *str);
extern char *qbase64_encode(const void *bin, size_t size);
extern size_t qbase64_decode(char *str);
extern char *qhex_encode(const void *bin, size_t size);
extern size_t qhex_decode(char *str);

/* qfile.c */
extern bool qfile_lock(int fd);
extern bool qfile_unlock(int fd);
extern bool qfile_exist(const char *filepath);
extern void *qfile_load(const char *filepath, size_t *nbytes);
extern void *qfile_read(FILE *fp, size_t *nbytes);
extern ssize_t qfile_save(const char *filepath, const void *buf, size_t size,
                          bool append);
extern bool qfile_mkdir(const char *dirpath, mode_t mode, bool recursive);

extern char *qfile_get_name(const char *filepath);
extern char *qfile_get_dir(const char *filepath);
extern char *qfile_get_ext(const char *filepath);
extern off_t qfile_get_size(const char *filepath);

extern bool qfile_check_path(const char *path);
extern char *qfile_correct_path(char *path);
extern char *qfile_abspath(char *buf, size_t bufsize, const char *path);

/* qhash.c */
extern bool qhashmd5(const void *data, size_t nbytes, void *retbuf);
extern bool qhashmd5_file(const char *filepath, off_t offset, ssize_t nbytes,
                          void *retbuf);
extern uint32_t qhashfnv1_32(const void *data, size_t nbytes);
extern uint64_t qhashfnv1_64(const void *data, size_t nbytes);
extern uint32_t qhashmurmur3_32(const void *data, size_t nbytes);
extern bool qhashmurmur3_128(const void *data, size_t nbytes, void *retbuf);

/* qio.c */
extern int qio_wait_readable(int fd, int timeoutms);
extern int qio_wait_writable(int fd, int timeoutms);
extern ssize_t qio_read(int fd, void *buf, size_t nbytes, int timeoutms);
extern ssize_t qio_write(int fd, const void *data, size_t nbytes,
                         int timeoutms);
extern off_t qio_send(int outfd, int infd, off_t nbytes, int timeoutms);
extern ssize_t qio_gets(int fd, char *buf, size_t bufsize, int timeoutms);
extern ssize_t qio_puts(int fd, const char *str, int timeoutms);
extern ssize_t qio_printf(int fd, int timeoutms, const char *format, ...);

/* qlibc.c */
extern const char *qlibc_version(void);

/* qsocket.c */
extern int qsocket_open(const char *hostname, int port, int timeoutms);
extern bool qsocket_close(int sockfd, int timeoutms);
extern bool qsocket_get_addr(struct sockaddr_in *addr, const char *hostname,
                             int port);

/* qstring.c */
extern char *qstrtrim(char *str);
extern char *qstrtrim_head(char *str);
extern char *qstrtrim_tail(char *str);
extern char *qstrunchar(char *str, char head, char tail);
extern char *qstrreplace(const char *mode, char *srcstr, const char *tokstr,
                         const char *word);
extern char *qstrcpy(char *dst, size_t size, const char *src);
extern char *qstrncpy(char *dst, size_t size, const char *src, size_t nbytes);
extern char *qstrdupf(const char *format, ...);
extern char *qstrdup_between(const char *str, const char *start,
                             const char *end);
extern char *qstrcatf(char *str, const char *format, ...);
extern char *qstrgets(char *buf, size_t size, char **offset);
extern char *qstrrev(char *str);
extern char *qstrupper(char *str);
extern char *qstrlower(char *str);
extern char *qstrtok(char *str, const char *delimiters, char *retstop,
                     int *offset);
extern qlist_t *qstrtokenizer(const char *str, const char *delimiters);
extern char *qstrunique(const char *seed);
extern char *qstr_comma_number(int number);
extern bool qstrtest(int (*testfunc) (int), const char *str);
extern bool qstr_is_email(const char *email);
extern bool qstr_is_ip4addr(const char *str);
extern char *qstr_conv_encoding(const char *fromstr, const char *fromcode,
                                const char *tocode, float mag);

/* qsystem.c */
extern const char *qgetenv(const char *envname, const char *nullstr);
extern char *qsyscmd(const char *cmd);
extern bool qsysgetip(char *buf, size_t bufsize);

/* qtime.c */
extern char *qtime_localtime_strf(char *buf, int size, time_t utctime,
                                  const char *format);
extern char *qtime_localtime_str(time_t utctime);
extern const char *qtime_localtime_staticstr(time_t utctime);
extern char *qtime_gmt_strf(char *buf, int size, time_t utctime,
                            const char *format);
extern char *qtime_gmt_str(time_t utctime);
extern const char *qtime_gmt_staticstr(time_t utctime);
extern time_t qtime_parse_gmtstr(const char *gmtstr);

/******************************************************************************
 * IPC SECTION
 ******************************************************************************/

/* qsem.c */
extern int qsem_init(const char *keyfile, int keyid, int nsems, bool recreate);
extern int qsem_getid(const char *keyfile, int keyid);
extern bool qsem_enter(int semid, int semno);
extern bool qsem_enter_nowait(int semid, int semno);
extern bool qsem_enter_force(int semid, int semno, int maxwaitms,
                             bool *forceflag);
extern bool qsem_leave(int semid, int semno);
extern bool qsem_check(int semid, int semno);
extern bool qsem_free(int semid);

/* qshm.c */
extern int qshm_init(const char *keyfile, int keyid, size_t size,
                     bool recreate);
extern int qshm_getid(const char *keyfile, int keyid);
extern void *qshm_get(int shmid);
extern bool qshm_free(int shmid);

#ifdef __cplusplus
}
#endif

#endif /*_QLIBC_H */

