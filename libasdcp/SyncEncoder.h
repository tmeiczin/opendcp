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
/*! \file    SyncEncoder.h
    \version $Id: SyncEncoder.h,v 1.1 2013/04/12 23:39:31 mikey Exp $
    \brief   Declaration of Atmos Sync Frame Encoder
*/

#ifndef _SYNC_ENCODER_H_
#define _SYNC_ENCODER_H_

#include "SyncCommon.h"
#include "UUIDInformation.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SyncEncoder{
	INT				iSampleRate;			/* Signal sample rate */
	INT				iSymbolLength;			/* Symbol Length */
	INT				iFrameRate;				/* Frame rate */
	INT				iFrameRateCode;			/* Frame rate code */
	INT				iAudioBufferLength;		/* Length of audio buffer */
	INT				iPacketBits;			/* Bits in each packet includes wash bits */
	INT				iPacketsPerFrame;		/* Number of packets per frame */
	FLOAT			fSymbolPhase;			/* Symbol phase */

	INT				iUUIDSubIndex;			/* UUID transmission sub index */
	UUIDINFORMATION	UUID;					/* UUID */

	BYTE			abyPacket[MAX_PACKET];

	INT				iError;					/* Error state */
}SYNCENCODER,*LPSYNCENCODER;

enum{
	SYNC_ENCODER_ERROR_NONE = 0,			/* No error */
	SYNC_ENCODER_ERROR_INVALID_SR = -1,		/* Invalid sample rate */
	SYNC_ENCODER_ERROR_INVALID_FR = -2,		/* Invalid frame rate */
	SYNC_ENCODER_ERROR_INVALID_BL = -10,	/* Buffer length is incorrect */
	SYNC_ENCODER_ERROR_UNKNOWN = -100,		/* Unknown */
};


INT SyncEncoderInit(LPSYNCENCODER		pSyncEncoder,	/* Out: SYNCENCODER structure to be initialized */
					INT					iSampleRate,	/* In:	Signal sample rate */
					INT					iFrameRate,		/* In:	frame rate */
					LPUUIDINFORMATION	pUUID);			/* In:	UUID */

INT GetSyncEncoderAudioBufferLength(LPSYNCENCODER pSyncEncoder);

INT EncodeSync(	LPSYNCENCODER	pSyncEncoder,	/* In:	Sync encoder structure */
				INT				iBufferLength,	/* In:	Length of audio buffer */
				FLOAT			*pfAudioBuffer,	/* Out: Audio buffer with signal */
				INT				iFrameIndex);	/* In:	Frame Index */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

