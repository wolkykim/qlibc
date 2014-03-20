################################################################################
## qLibc - http://www.qdecoder.org
##
## Copyright (c) 2010-2012 Seungyoung Kim.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## 1. Redistributions of source code must retain the above copyright notice,
##    this list of conditions and the following disclaimer.
## 2. Redistributions in binary form must reproduce the above copyright notice,
##    this list of conditions and the following disclaimer in the documentation
##    and/or other materials provided with the distribution.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
################################################################################
## $Id: configure.ac 125 2012-06-11 22:14:29Z seungyoung.kim $
################################################################################

AC_COPYRIGHT([
==============================================================================
 qLibc - http://www.qdecoder.org

 Copyright (c) 2010-2012 Seungyoung Kim.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
==============================================================================
])

## Internal functions
AC_DEFUN([Q_ARG_ENABLE], [
	AC_ARG_ENABLE([$1],[AS_HELP_STRING([--enable-$1], [$2])],,[enableval=no])
	if test "$enableval" = yes; then
		AC_MSG_NOTICE(['$1' feature is enabled])
		CPPFLAGS="$CPPFLAGS $3"
	fi
])

AC_DEFUN([Q_ARG_DISABLE], [
	AC_ARG_ENABLE([$1],[AS_HELP_STRING([--disable-$1], [$2])],,[enableval=yes])
	if test "$enableval" = no; then
		AC_MSG_NOTICE(['$1' feature is disabled])
		CPPFLAGS="$CPPFLAGS $3"
	fi
])

## Initialize
AC_INIT([qLibc], [2 RELEASE], [http://www.qdecoder.org/])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile src/Makefile examples/Makefile])
AC_SUBST(BUILD_TARGETS, ["qlibc qlibcext"])
AC_SUBST(INSTALL_TARGETS, ["install-qlibc install-qlibcext"])
AC_SUBST(EXAMPLES_TARGETS, ["TARGETS1"])
AC_SUBST(DEPLIBS, [""])

## Set path
PATH="$PATH:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin"
CPPFLAGS="$CPPFLAGS -I/usr/include -I/usr/local/include"
CPPFLAGS="$CPPFLAGS -D_GNU_SOURCE"
CPPFLAGS="$CPPFLAGS -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64"
LDFLAGS="$LDFLAGS -L/usr/lib -L/usr/local/lib"

## Set autoconf setting
#AC_CANONICAL_TARGET
AC_PREFIX_DEFAULT([/usr/local])
#AC_PRESERVE_HELP_ORDER

## Checks for programs.
AC_PROG_CC
if test $ac_compiler_gnu = yes; then
	CFLAGS="-Wall -fPIC"
#else
#	AC_MSG_FAILURE([GCC is required.])
fi
AC_PROG_CC_C99
if test $ac_cv_prog_cc_c99 = no; then
	AC_MSG_FAILURE([Compiler does not support C99 mode.])
fi

AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

AC_PATH_PROG(AR, ar)
AC_PATH_PROG(CHMOD, chmod)
AC_PATH_PROG(LD, ld)
AC_PATH_PROG(RM, rm)

## Checks for header files.
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_HEADER_DIRENT
AC_CHECK_HEADER([inttypes.h])

## Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_OFF_T

## Checks for libraries.

## Checks for library functions.
#AC_CHECK_FUNCS([socket sendfile])

##
## --enable section
##

Q_ARG_ENABLE([debug], [Enable debugging output. This will print out internal debugging messages to stdout.], [-DBUILD_DEBUG])

Q_ARG_ENABLE([threadsafe], [Enable thread-safe feature. When it's enabled, user applications need to link pthread library. (ex: -lpthread)], [-DENABLE_THREADSAFE])
if test "$enableval" = yes; then
	AC_CHECK_LIB([pthread], [main], [], AC_MSG_ERROR([Cannot find pthread library.]))
	AC_SUBST(DEPLIBS, ["-lpthread"])
fi

Q_ARG_DISABLE([ipc], [Disable IPC APIs(src/ipc/) in qlibc library.], [-DDISABLE_IPC])
#Q_ARG_ENABLE([lfs], [Enable large file supports.], [-DENABLE_LFS -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64])
#Q_ARG_DISABLE([ext], [Disable all extension APIs in src/extensions directory.], [-DDISABLE_QCONFIG -DDISABLE_QLOG -DDISABLE_QHTTPCLIENT -DDISABLE_QDATABASE])
Q_ARG_DISABLE([ext], [Disable building qlibext extension library.], [])
if test "$enableval" = no; then
	AC_SUBST(BUILD_TARGETS, ["qlibc"])
	AC_SUBST(INSTALL_TARGETS, ["install-qlibc"])
	AC_SUBST(EXAMPLES_TARGETS, ["TARGETS2"])
fi
#Q_ARG_DISABLE([ext-all], [Disable all extension APIs in src/extensions directory.], [-DDISABLE_QCONFIG -DDISABLE_QLOG -DDISABLE_QHTTPCLIENT -DDISABLE_QDATABASE])
Q_ARG_DISABLE([ext-qconfig], [Disable qconfig extension in qlibext library.], [-DDISABLE_QCONFIG])
Q_ARG_DISABLE([ext-qaconf], [Disable qaconf extension in qlibext library.], [-DDISABLE_QACONF])
Q_ARG_DISABLE([ext-qlog], [Disable qlog extension in qlibext library.], [-DDISABLE_QLOG])
Q_ARG_DISABLE([ext-qhttpclient], [Disable qhttpclient extension in qlibext library.], [-DDISABLE_QHTTPCLIENT])
Q_ARG_DISABLE([ext-qdatabase], [Disable qdatabase extension in qlibext library.], [-DDISABLE_QDATABASE])

if test "$enableval" = yes; then
	CFLAGS="$CFLAGS -g -O2"
else
	CFLAGS="$CFLAGS -g -O2"
fi

##
## --with section
##

AC_ARG_WITH([openssl],[AS_HELP_STRING([--with-openssl], [This will enable HTTPS support in qhttpclient extension API. When it's enabled, user applications are needed to be linked with the openssl library. (ex: -lssl)])],[withval=yes],[withval=no])
if test "$withval" = yes; then
	if test "$with_openssl" = yes; then
		with_openssl="/usr/include"
	fi

	AC_CHECK_FILE([$with_openssl/openssl/ssl.h],[withval=yes],[withval=no])
	if test "$withval" = yes; then
		AC_MSG_NOTICE([HTTPS support in qhttpclient API is enabled])
		CPPFLAGS="$CPPFLAGS -DENABLE_OPENSSL -I$with_openssl"
	else
		AC_MSG_FAILURE([Cannot find '/openssl/ssl.h' header. Use --with-openssl=/PATH/ to specify the directory where 'openssl' directory is located.])
	fi
fi

AC_ARG_WITH([mysql],[AS_HELP_STRING([--with-mysql], [This will enable MySQL database support in qdb extension API. When it's enabled, user applications need to link mysql client library. (ex: -lmysqlclient)])],[withval=yes],[withval=no])
if test "$withval" = yes; then
	if test "$with_mysql" = yes; then
		with_mysql="/usr/include/mysql"
	fi

	AC_CHECK_FILE([$with_mysql/mysql.h],[withval=yes],[withval=no])
	if test "$withval" = yes; then
		AC_MSG_NOTICE([MySQL support in qdb API is enabled])
		CPPFLAGS="$CPPFLAGS -DENABLE_MYSQL -I$with_mysql"
	else
		AC_MSG_FAILURE([Cannot find '$with_mysql/mysql.h' header. Use --with-mysql=/PATH/ to specify the directory where 'mysql.h' is located.])
	fi
fi

AC_MSG_NOTICE([CFLAGS $CFLAGS])
AC_MSG_NOTICE([CPPFLAGS $CPPFLAGS])
#AC_MSG_NOTICE([LIBS $LIBS])

## Create Makefile
AC_OUTPUT
