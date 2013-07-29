/*
Copyright (c) 2013-2013, John Hurst
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*! \file    SyncCommon.h
    \version $Id: SyncCommon.h,v 1.1 2013/04/12 23:39:31 mikey Exp $
    \brief   Common elements for ATMOS Sync Channel generation
*/

#ifndef _SYNC_COMMON_H_
#define _SYNC_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef USHORT
typedef unsigned short USHORT;
#endif

#ifndef INT
typedef int INT;
#endif

#ifndef FLOAT
typedef float FLOAT;
#endif

#define SYMBOL_RATE			(12000)

#define SYMBOL_LENGTH_48	(4)
#define SYMBOL_LENGTH_96	(8)

#define SYNC_HEADER			(0x4D56)
#define SYNC_HEADER1		(0x4D)
#define SYNC_HEADER2		(0x56)

#define SYNC_HEADER_BITS	(16)
#define FRAME_RATE_BITS		(4)
#define RESERVE_BITS		(2)
#define UUID_SUB_INDEX_BITS	(2)
#define UUID_SUB_BITS		(32)
#define FRAME_INDEX_BITS	(24)
#define CRC_BITS			(16)
#define MESSAGE_TOTAL_BITS	(96)
#define MESSAGE_TOTAL_BYTES (12)

#define MAX_PACKET			(32)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
