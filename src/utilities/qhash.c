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
 * $Id: qhash.c 133 2013-11-02 05:01:23Z seungyoung.kim $
 ******************************************************************************/

/**
 * @file qhash.c Hash APIs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "md5/md5.h"
#include "qlibc.h"
#include "qinternal.h"

/**
 * Calculate 128-bit(16-bytes) MD5 hash.
 *
 * @param data      source object
 * @param nbytes    size of data
 * @param retbuf    user buffer. It must be at leat 16-bytes long.
 *
 * @return true if successful, otherwise false.
 *
 * @code
 *   // get MD5
 *   unsigned char md5hash[16];
 *   qhashmd5((void*)"hello", 5, md5hash);
 *
 *   // hex encode
 *   char *md5ascii = qhex_encode(md5hash, 16);
 *   printf("Hex encoded MD5: %s\n", md5ascii);
 *   free(md5ascii);
 * @endcode
 */
bool qhashmd5(const void *data, size_t nbytes, void *retbuf)
{
    if (data == NULL || retbuf == NULL) {
        errno = EINVAL;
        return false;
    }

    MD5_CTX context;
    MD5Init(&context);
    MD5Update(&context, (unsigned char *)data, (unsigned int)nbytes);
    MD5Final(retbuf, &context);

    return true;
}

/**
 * Get 128-bit MD5 hash of a file contents.
 *
 * @param filepath  file path
 * @param offset    start offset. Set to 0 to digest from beginning of file.
 * @param nbytes    number of bytes to digest. Set to 0 to digest until end
 *                  of file.
 * @param retbuf    user buffer. It must be at leat 16-bytes long.
 *
 * @return true if successful, otherwise false.
 *
 * @code
 *   unsigned char md5hash[16];
 *   qhashmd5_file("/tmp/test.dat", 0, 0, md5hash);
 * @endcode
 */
bool qhashmd5_file(const char *filepath, off_t offset, ssize_t nbytes,
                   void *retbuf)
{
    if (filepath == NULL || offset < 0 || nbytes < 0 || retbuf == NULL) {
        errno = EINVAL;
        return false;
    }

    int fd = open(filepath, O_RDONLY, 0);
    if (fd < 0) return false;

    struct stat st;
    if (fstat(fd, &st) < 0) return false;
    size_t size = st.st_size;

    // check filesize
    if (size < offset + nbytes) {
        errno = EINVAL;
        close(fd);
        return false;
    }

    // seek
    if (offset > 0) {
        if (lseek(fd, offset, SEEK_SET) != offset) {
            close(fd);
            return false;
        }
    }

    MD5_CTX context;
    MD5Init(&context);
    ssize_t toread, nread;
    unsigned char buf[32*1024];
    for (toread = nbytes; toread > 0; toread -= nread) {
        if (toread > sizeof(buf)) nread = read(fd, buf, sizeof(buf));
        else nread = read(fd, buf, toread);
        if (nread < 0) break;
        MD5Update(&context, buf, nread);
    }
    close(fd);
    if (toread != 0) return false;
    MD5Final(retbuf, &context);

    return true;
}

/**
 * Get 32-bit FNV1 hash.
 *
 * @param data      source data
 * @param nbytes    size of data
 *
 * @return 32-bit unsigned hash value.
 *
 * @code
 *  uint32_t hashval = qhashfnv1_32((void*)"hello", 5);
 * @endcode
 *
 * @code
 *  Fowler/Noll/Vo hash
 *
 *  The basis of this hash algorithm was taken from an idea sent as reviewer
 *  comments to the IEEE POSIX P1003.2 committee by:
 *
 *      Phong Vo (http://www.research.att.com/info/kpv/)
 *      Glenn Fowler (http://www.research.att.com/~gsf/)
 *
 *  In a subsequent ballot round:
 *
 *      Landon Curt Noll (http://www.isthe.com/chongo/)
 *
 *  improved on their algorithm.  Some people tried this hash and found that
 *  it worked rather well. In an EMail message to Landon, they named it the
 *  ``Fowler/Noll/Vo'' or FNV hash.
 *
 *  FNV hashes are designed to be fast while maintaining a low collision rate.
 *  The FNV speed allows one to quickly hash lots of data while maintaining
 *  a reasonable collision rate.  See:
 *
 *      http://www.isthe.com/chongo/tech/comp/fnv/index.html
 *
 *  for more details as well as other forms of the FNV hash.
 * @endcode
 */
uint32_t qhashfnv1_32(const void *data, size_t nbytes)
{
    if (data == NULL || nbytes == 0) return 0;

    unsigned char *dp;
    uint32_t h = 0x811C9DC5;

    for (dp = (unsigned char *)data; *dp && nbytes > 0; dp++, nbytes--) {
#ifdef __GNUC__
        h += (h<<1) + (h<<4) + (h<<7) + (h<<8) + (h<<24);
#else
        h *= 0x01000193;
#endif
        h ^= *dp;
    }

    return h;
}

/**
 * Get 64-bit FNV1 hash integer.
 *
 * @param data      source data
 * @param nbytes    size of data
 *
 * @return 64-bit unsigned hash value.
 *
 * @code
 *   uint64_t fnv64 = qhashfnv1_64((void*)"hello", 5);
 * @endcode
 */
uint64_t qhashfnv1_64(const void *data, size_t nbytes)
{
    if (data == NULL || nbytes == 0) return 0;

    unsigned char *dp;
    uint64_t h = 0xCBF29CE484222325ULL;

    for (dp = (unsigned char *)data; *dp && nbytes > 0; dp++, nbytes--) {
#ifdef __GNUC__
        h += (h << 1) + (h << 4) + (h << 5) +
             (h << 7) + (h << 8) + (h << 40);
#else
        h *= 0x100000001B3ULL;
#endif
        h ^= *dp;
    }

    return h;
}

/**
 * Get 32-bit Murmur3 hash.
 *
 * @param data      source data
 * @param nbytes    size of data
 *
 * @return 32-bit unsigned hash value.
 *
 * @code
 *  uint32_t hashval = qhashmurmur3_32((void*)"hello", 5);
 * @endcode
 *
 * @code
 *  MurmurHash3 was created by Austin Appleby  in 2008. The cannonical
 *  implementations are in C++ and placed in the public.
 *
 *    https://sites.google.com/site/murmurhash/
 *
 *  Seungyoung Kim has ported it's cannonical implementation to C language
 *  in 2012 and published it as a part of qLibc component.
 * @endcode
 */
uint32_t qhashmurmur3_32(const void *data, size_t nbytes)
{
    if (data == NULL || nbytes == 0) return 0;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const int nblocks = nbytes / 4;
    const uint32_t *blocks = (const uint32_t *)(data);
    const uint8_t *tail = (const uint8_t *)(data + (nblocks * 4));

    uint32_t h = 0;

    int i;
    uint32_t k;
    for (i = 0; i < nblocks; i++) {
        k = blocks[i];

        k *= c1;
        k = (k << 15) | (k >> (32 - 15));
        k *= c2;

        h ^= k;
        h = (h << 13) | (h >> (32 - 13));
        h = (h * 5) + 0xe6546b64;
    }

    k = 0;
    switch (nbytes & 3) {
        case 3:
            k ^= tail[2] << 16;
        case 2:
            k ^= tail[1] << 8;
        case 1:
            k ^= tail[0];
            k *= c1;
            k = (k << 15) | (k >> (32 - 15));
            k *= c2;
            h ^= k;
    };

    h ^= nbytes;

    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

/**
 * Get 128-bit Murmur3 hash.
 *
 * @param data      source data
 * @param nbytes    size of data
 * @param retbuf    user buffer. It must be at leat 16-bytes long.
 *
 * @return true if successful, otherwise false.
 *
 * @code
 *   // get 128-bit Murmur3 hash.
 *   unsigned char hash[16];
 *   qhashmurmur3_128((void*)"hello", 5, hash);
 *
 *   // hex encode
 *   char *ascii = qhex_encode(hash, 16);
 *   printf("Hex encoded Murmur3: %s\n", ascii);
 *   free(ascii);
 * @endcode
 */
bool qhashmurmur3_128(const void *data, size_t nbytes, void *retbuf)
{
    if (data == NULL || nbytes == 0) return false;

    const uint64_t c1 = 0x87c37b91114253d5ULL;
    const uint64_t c2 = 0x4cf5ad432745937fULL;

    const int nblocks = nbytes / 16;
    const uint64_t *blocks = (const uint64_t *)(data);
    const uint8_t *tail = (const uint8_t *)(data + (nblocks * 16));

    uint64_t h1 = 0;
    uint64_t h2 = 0;

    int i;
    uint64_t k1, k2;
    for (i = 0; i < nblocks; i++) {
        k1 = blocks[i*2+0];
        k2 = blocks[i*2+1];

        k1 *= c1;
        k1  = (k1 << 31) | (k1 >> (64 - 31));
        k1 *= c2;
        h1 ^= k1;

        h1 = (h1 << 27) | (h1 >> (64 - 27));
        h1 += h2;
        h1 = h1*5+0x52dce729;

        k2 *= c2;
        k2  = (k2 << 33) | (k2 >> (64 - 33));
        k2 *= c1;
        h2 ^= k2;

        h2 = (h2 << 31) | (h2 >> (64 - 31));
        h2 += h1;
        h2 = h2*5+0x38495ab5;
    }

    k1 = k2 = 0;
    switch (nbytes & 15) {
        case 15:
            k2 ^= (uint64_t)(tail[14]) << 48;
        case 14:
            k2 ^= (uint64_t)(tail[13]) << 40;
        case 13:
            k2 ^= (uint64_t)(tail[12]) << 32;
        case 12:
            k2 ^= (uint64_t)(tail[11]) << 24;
        case 11:
            k2 ^= (uint64_t)(tail[10]) << 16;
        case 10:
            k2 ^= (uint64_t)(tail[ 9]) << 8;
        case  9:
            k2 ^= (uint64_t)(tail[ 8]) << 0;
            k2 *= c2;
            k2  = (k2 << 33) | (k2 >> (64 - 33));
            k2 *= c1;
            h2 ^= k2;

        case  8:
            k1 ^= (uint64_t)(tail[ 7]) << 56;
        case  7:
            k1 ^= (uint64_t)(tail[ 6]) << 48;
        case  6:
            k1 ^= (uint64_t)(tail[ 5]) << 40;
        case  5:
            k1 ^= (uint64_t)(tail[ 4]) << 32;
        case  4:
            k1 ^= (uint64_t)(tail[ 3]) << 24;
        case  3:
            k1 ^= (uint64_t)(tail[ 2]) << 16;
        case  2:
            k1 ^= (uint64_t)(tail[ 1]) << 8;
        case  1:
            k1 ^= (uint64_t)(tail[ 0]) << 0;
            k1 *= c1;
            k1  = (k1 << 31) | (k1 >> (64 - 31));
            k1 *= c2;
            h1 ^= k1;
    };

    //----------
    // finalization

    h1 ^= nbytes;
    h2 ^= nbytes;

    h1 += h2;
    h2 += h1;

    h1 ^= h1 >> 33;
    h1 *= 0xff51afd7ed558ccdULL;
    h1 ^= h1 >> 33;
    h1 *= 0xc4ceb9fe1a85ec53ULL;
    h1 ^= h1 >> 33;

    h2 ^= h2 >> 33;
    h2 *= 0xff51afd7ed558ccdULL;
    h2 ^= h2 >> 33;
    h2 *= 0xc4ceb9fe1a85ec53ULL;
    h2 ^= h2 >> 33;

    h1 += h2;
    h2 += h1;

    ((uint64_t *)retbuf)[0] = h1;
    ((uint64_t *)retbuf)[1] = h2;

    return true;
}
