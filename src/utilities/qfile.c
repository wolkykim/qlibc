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
 * $Id: qfile.c 129 2012-07-16 23:14:34Z seungyoung.kim $
 ******************************************************************************/

/**
 * @file qfile.c File handling APIs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "qlibc.h"
#include "qinternal.h"

/**
 * Lock file
 *
 * @param fd        file descriptor
 *
 * @return true if successful, otherwise returns false.
 *
 * @code
 *   // for file descriptor
 *   int fd = open(...);
 *   if(qfile_lock(fd) == true) {
 *     (...atomic file access...)
 *     qfile_unlock(fd);
 *   }
 *
 *   // for FILE stream object
 *   FILE *fp = fopen(...);
 *   int fd = fileno(fp);
 *   if(qfile_lock(fd) == true) {
 *     (...atomic file access...)
 *     qfile_unlock(fd);
 *   }
 * @endcode
 */
bool qfile_lock(int fd)
{
#ifdef _WIN32
    return false;
#else
    struct flock lock;
    memset((void *)&lock, 0, sizeof(flock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    int ret = fcntl(fd, F_SETLK, &lock);
    if (ret == 0) return true;
    return false;
#endif
}

/**
 * Unlock file which is locked by qfile_lock()
 *
 * @param fd        file descriptor
 *
 * @return true if successful, otherwise returns false.
 */
bool qfile_unlock(int fd)
{
#ifdef _WIN32
    return false;
#else
    struct flock lock;
    memset((void *)&lock, 0, sizeof(flock));
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    int ret = fcntl(fd, F_SETLK, &lock);
    if (ret == 0) return true;
    return false;
#endif
}

/**
 * Check file existence.
 *
 * @param filepath  file or directory path
 *
 * @return true if exists, otherwise returns false.
 */
bool qfile_exist(const char *filepath)
{
    if (access(filepath, F_OK) == 0) return true;
    return false;
}

/**
 * Load file into memory.
 *
 * @param filepath  file path
 * @param nbytes    has two purpost, one is to set how many bytes are readed.
 *                  the other is actual the number loaded bytes will be stored.
 *                  nbytes must be point 0 or NULL to read entire file.
 *
 * @return allocated memory pointer if successful, otherwise returns NULL.
 *
 * @code
 *   // loading text file
 *   char *text = (char *)qfile_load("/tmp/text.txt", NULL);
 *
 *   // loading binary file
 *   int binlen = 0;
 *   char *bin = (char *)qfile_load("/tmp/binary.bin", &binlen);
 *
 *   // loading partial
 *   int binlen = 10;
 *   char *bin = (char *)qfile_load("/tmp/binary.bin", &binlen);
 * @endcode
 *
 * @note
 *  This method actually allocates memory more than 1 bytes than filesize then
 *  append NULL character at the end. For example, when the file size is 10
 *  bytes long, 10+1 bytes will allocated and the last byte is always NULL
 *  character. So you can load text file and use without appending NULL
 *  character. By the way, the actual file size 10 will be returned at nbytes
 *  variable.
 */
void *qfile_load(const char *filepath, size_t *nbytes)
{
    int fd;
    if ((fd = open(filepath, O_RDONLY, 0)) < 0) return NULL;

    struct stat fs;
    if (fstat(fd, &fs) < 0) {
        close(fd);
        return NULL;
    }

    size_t size = fs.st_size;
    if (nbytes != NULL && *nbytes > 0 && *nbytes < fs.st_size) size = *nbytes;

    void *buf = malloc(size + 1);
    if (buf == NULL) {
        close(fd);
        return NULL;
    }

    ssize_t count = read(fd, buf, size);
    close(fd);

    if (count != size) {
        free(buf);
        return NULL;
    }

    ((char *)buf)[count] = '\0';

    if (nbytes != NULL) *nbytes = count;
    return buf;
}

/**
 * Read data from a file stream.
 *
 * @param fp        FILE pointer
 * @param nbytes    has two purpose, one is to set bytes to read.
 *                  the other is to return actual number of bytes loaded.
 *                  0 or NULL can be set to read file until the end.
 *
 * @return allocated memory pointer if successful, otherwise returns NULL.
 *
 * @code
 *   int binlen = 0;
 *   char *bin = (char *)qfile_read(fp, &binlen);
 * @endcode
 *
 * @note
 *  This method append NULL character at the end of stream. but nbytes only
 *  counts actual readed bytes.
 */
void *qfile_read(FILE *fp, size_t *nbytes)
{
    size_t memsize = 1024;
    size_t size = 0;

    if (nbytes != NULL && *nbytes > 0) {
        memsize = *nbytes;
        size = *nbytes;
    }

    int c;
    size_t c_count;
    char *data = NULL;
    for (c_count = 0; (c = fgetc(fp)) != EOF;) {
        if (size > 0 && c_count == size) break;

        if (c_count == 0) {
            data = (char *)malloc(sizeof(char) * memsize);
            if (data == NULL) {
                DEBUG("Memory allocation failed.");
                return NULL;
            }
        } else if (c_count == memsize - 1) {
            memsize *= 2;

            /* Here, we don't use realloc(). Sometimes it is unstable. */
            char *datatmp = (char *)malloc(sizeof(char) * (memsize + 1));
            if (datatmp == NULL) {
                DEBUG("Memory allocation failed.");
                free(data);
                return NULL;
            }
            memcpy(datatmp, data, c_count);
            free(data);
            data = datatmp;
        }
        data[c_count++] = (char)c;
    }

    if (c_count == 0 && c == EOF) return NULL;
    data[c_count] = '\0';

    if (nbytes != NULL) *nbytes = c_count;

    return (void *)data;
}

/**
 * Save data into file.
 *
 * @param filepath  file path
 * @param buf       data
 * @param size      the number of bytes to save
 * @param append    false for new(if exists truncate), true for appending
 *
 * @return the number of bytes written if successful, otherwise returns -1.
 *
 * @code
 *   // save text
 *   char *text = "hello";
 *   qfile_save("/tmp/text.txt", (void*)text, strlen(text), false);
 *
 *   // save binary
 *   int integer1 = 75;
 *   qfile_save("/tmp/integer.bin, (void*)&integer, sizeof(int));
 * @endcode
 */
ssize_t qfile_save(const char *filepath, const void *buf, size_t size,
                   bool append)
{
    int fd;

    if (append == false) {
        fd = open(filepath, O_CREAT|O_WRONLY|O_TRUNC, DEF_FILE_MODE);
    } else {
        fd = open(filepath, O_CREAT|O_WRONLY|O_APPEND, DEF_FILE_MODE);
    }
    if (fd < 0) return -1;

    ssize_t count = write(fd, buf, size);
    close(fd);

    return count;
}

/**
 * Attempts to create a directory recursively.
 *
 * @param dirpath   directory path
 * @param mode      permissions to use
 * @param recursive whether or not to create parent directories automatically
 *
 * @return true if successful, otherwise returns false.
 */
bool qfile_mkdir(const char *dirpath, mode_t mode, bool recursive)
{
    DEBUG("try to create directory %s", dirpath);
    if (mkdir(dirpath, mode) == 0) return true;

    bool ret = false;
    if (recursive == true && errno == ENOENT) {
        char *parentpath = qfile_get_dir(dirpath);
        if (qfile_mkdir(parentpath, mode, true) == true
            && mkdir(dirpath, mode) == 0) {
            ret = true;
        }
        free(parentpath);
    }

    return ret;
}

/**
 * Check path string contains invalid characters.
 *
 * @param path      path string
 *
 * @return true if ok, otherwise returns false.
 */
bool qfile_check_path(const char *path)
{
    if (path == NULL) return false;

    int nLen = strlen(path);
    if (nLen == 0 || nLen >= PATH_MAX) return false;
    else if (strpbrk(path, "\\:*?\"<>|") != NULL) return false;
    return true;
}


/**
 * Get filename from filepath
 *
 * @param filepath  file or directory path
 *
 * @return malloced filename string
 */
char *qfile_get_name(const char *filepath)
{
    char *path = strdup(filepath);
    char *bname = basename(path);
    char *filename = strdup(bname);
    free(path);
    return filename;
}

/**
 * Get directory suffix from filepath
 *
 * @param filepath  file or directory path
 *
 * @return malloced filepath string
 */
char *qfile_get_dir(const char *filepath)
{
    char *path = strdup(filepath);
    char *dname = dirname(path);
    char *dir = strdup(dname);
    free(path);
    return dir;
}

/**
 * Get extension from filepath.
 *
 * @param filepath  file or directory path
 *
 * @return malloced extension string which is converted to lower case.
 */
char *qfile_get_ext(const char *filepath)
{
#define MAX_EXTENSION_LENGTH        (8)
    char *filename = qfile_get_name(filepath);
    char *p = strrchr(filename, '.');
    char *ext = NULL;
    if (p != NULL
        && strlen(p+1) <= MAX_EXTENSION_LENGTH
        && qstrtest(isalnum, p+1) == true) {
        ext = strdup(p+1);
        qstrlower(ext);
    } else {
        ext = strdup("");
    }

    free(filename);
    return ext;
}

/**
 * Get file size.
 *
 * @param filepath  file or directory path
 *
 * @return the file size if exists, otherwise returns -1.
 */
off_t qfile_get_size(const char *filepath)
{
    struct stat finfo;
    if (stat(filepath, &finfo) < 0) return -1;
    return finfo.st_size;
}

/**
 * Correct path string.
 *
 * @param path  path string
 *
 * @return path string pointer
 *
 * @note
 * This modify path argument itself.
 *
 * @code
 *   "/hello//my/../world" => "/hello/world"
 *   "././././hello/./world" => "./hello/world"
 *   "/../hello//world" => "/hello/world"
 *   "/../hello//world/" => "/hello/world"
 * @endcode
 */
char *qfile_correct_path(char *path)
{
    if (path == NULL) return NULL;

    // take off heading & tailing white spaces
    qstrtrim(path);

    while (true) {
        // take off double slashes
        if (strstr(path, "//") != NULL) {
            qstrreplace("sr", path, "//", "/");
            continue;
        }

        if (strstr(path, "/./") != NULL) {
            qstrreplace("sr", path, "/./", "/");
            continue;
        }

        if (strstr(path, "/../") != NULL) {
            char *pszTmp = strstr(path, "/../");
            if (pszTmp == path) {
                // /../hello => /hello
                strcpy(path, pszTmp + 3);
            } else {
                // /hello/../world => /world
                *pszTmp = '\0';
                char *pszNewPrefix = qfile_get_dir(path);
                strcpy(path, pszNewPrefix);
                strcat(path, pszTmp + 3);
                free(pszNewPrefix);
            }
            continue;
        }

        // take off tailing slash
        size_t nLen = strlen(path);
        if (nLen > 1) {
            if (path[nLen - 1] == '/') {
                path[nLen - 1] = '\0';
                continue;
            }
        }

        // take off tailing /.
        if (nLen > 2) {
            if (!strcmp(path + (nLen - 2), "/.")) {
                path[nLen - 2] = '\0';
                continue;
            }
        }

        // take off tailing /.
        if (nLen > 2) {
            if (!strcmp(path + (nLen - 2), "/.")) {
                path[nLen - 2] = '\0';
                continue;
            }
        }

        // take off tailing /.
        if (nLen > 3) {
            if (!strcmp(path + (nLen - 3), "/..")) {
                path[nLen - 3] = '\0';
                char *pszNewPath = qfile_get_dir(path);
                strcpy(path, pszNewPath);
                free(pszNewPath);
                continue;
            }
        }

        break;
    }

    return path;
}

/**
 * Make full absolute system path string.
 *
 * @param buf       buffer string pointer
 * @param bufsize   buffer size
 * @param path      path string
 *
 * @return buffer pointer if successful, otherwise returns NULL.
 *
 * @code
 *   char buf[PATH_MAX];
 *   chdir("/usr/local");
 *   qfile_abspath(buf, sizeof(buf), ".");    // returns "/usr/local"
 *   qfile_abspath(buf, sizeof(buf), "..");   // returns "/usr"
 *   qfile_abspath(buf, sizeof(buf), "etc");  // returns "/usr/local/etc"
 *   qfile_abspath(buf, sizeof(buf), "/etc"); // returns "/etc"
 *   qfile_abspath(buf, sizeof(buf), "/etc/"); // returns "/etc/"
 * @endcode
 */
char *qfile_abspath(char *buf, size_t bufsize, const char *path)
{
    if (bufsize == 0) return NULL;

    if (path[0] == '/') {
        qstrcpy(buf, bufsize, path);
    } else {
        if (getcwd(buf, bufsize) == NULL) return NULL;
        strcat(buf, "/");
        strcat(buf, path);
    }
    qfile_correct_path(buf);

    return buf;
}
