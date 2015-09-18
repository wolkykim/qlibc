################################################################################
## qLibc
##
## Copyright (c) 2010-2015 Seungyoung Kim.
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

CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
PROJECT(qlibc C)

SET(CMAKE_THREAD_PREFER_PTHREAD True)
FIND_PACKAGE(Threads REQUIRED)

IF (NOT CMAKE_USE_PTHREADS_INIT)
	MESSAGE(FATAL_ERROR "Couldn't find pthreads.")
ENDIF()

SET(SRC_SUBPATHS
		containers/*.c
		utilities/*.c
		ipc/*.c
		internal/*.c)

SET(SRC_SUBPATHS_EXT
		extensions/*.c)

SET(SRC_PATHS src)

IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	SET(SRC_PATHS ${SRC_PATHS} src/os/linux src/os/nix)
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	SET(SRC_PATHS ${SRC_PATHS} src/os/windows)
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	SET(SRC_PATHS ${SRC_PATHS} src/os/nix)
ENDIF()

FOREACH(TMP_path IN LISTS SRC_PATHS)
	FOREACH(TMP_subpath IN LISTS SRC_SUBPATHS)
		SET(TMP_search_path "${TMP_path}/${TMP_subpath}")
		SET(SRC_SEARCH_PATHS ${SRC_SEARCH_PATHS} ${TMP_search_path})
	ENDFOREACH(TMP_subpath)
ENDFOREACH(TMP_path)

FOREACH(TMP_path IN LISTS SRC_PATHS)
	FOREACH(TMP_subpath IN LISTS SRC_SUBPATHS_EXT)
		SET(TMP_search_path "${TMP_path}/${TMP_subpath}")
		SET(SRC_SEARCH_PATHS_EXT ${SRC_SEARCH_PATHS_EXT} ${TMP_search_path})
	ENDFOREACH(TMP_subpath)
ENDFOREACH(TMP_path)

FILE(GLOB_RECURSE SRC_LIB
		${SRC_SEARCH_PATHS})

FILE(GLOB_RECURSE SRC_LIB_EXT
		${SRC_SEARCH_PATHS_EXT})

SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${qlibc_SOURCE_DIR}/lib)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${qlibc_SOURCE_DIR}/lib)

ADD_LIBRARY(qlibc-static STATIC ${SRC_LIB})
ADD_LIBRARY(qlibc SHARED ${SRC_LIB})
ADD_LIBRARY(qlibcext-static STATIC ${SRC_LIB_EXT})
ADD_LIBRARY(qlibcext SHARED ${SRC_LIB_EXT})

INCLUDE_DIRECTORIES(${qlibc_SOURCE_DIR}/src/internal)
TARGET_INCLUDE_DIRECTORIES(qlibc-static PUBLIC ${qlibc_SOURCE_DIR}/include/qlibc)
TARGET_INCLUDE_DIRECTORIES(qlibc PUBLIC ${qlibc_SOURCE_DIR}/include/qlibc)
TARGET_INCLUDE_DIRECTORIES(qlibcext-static PUBLIC ${qlibc_SOURCE_DIR}/include/qlibc)

TARGET_LINK_LIBRARIES(qlibc-static PRIVATE ${CMAKE_THREAD_LIBS_INIT})
TARGET_LINK_LIBRARIES(qlibc PRIVATE ${CMAKE_THREAD_LIBS_INIT})
TARGET_LINK_LIBRARIES(qlibcext-static PRIVATE ${CMAKE_THREAD_LIBS_INIT})
TARGET_LINK_LIBRARIES(qlibcext PUBLIC qlibc)

