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
 * qlibc header file.
 *
 * @file qlibc.h
 */

#ifndef _QLIBC_H
#define _QLIBC_H

/* containers */
#include "containers/qtype.h"
#include "containers/qlist.h"
#include "containers/qlisttbl.h"
#include "containers/qhashtbl.h"
#include "containers/qhasharr.h"
#include "containers/qqueue.h"
#include "containers/qstack.h"
#include "containers/qvector.h"

/* utilities */
#include "utilities/qcount.h"
#include "utilities/qencode.h"
#include "utilities/qfile.h"
#include "utilities/qhash.h"
#include "utilities/qio.h"
#include "utilities/qsocket.h"
#include "utilities/qstring.h"
#include "utilities/qsystem.h"
#include "utilities/qtime.h"

/* ipc */
#include "ipc/qsem.h"
#include "ipc/qshm.h"

#endif /*_QLIBC_H */

