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
/*! \file    SyncEncoder.c
    \version $Id: SyncEncoder.c,v 1.1 2013/04/12 23:39:31 mikey Exp $
    \brief   Implementation of Atmos Sync Frame Encoder
*/

#include "SyncEncoder.h"
#include "CRC16.h"

#include <memory.h>

void ConstructFrame(LPSYNCENCODER	pSyncEncoder,
					INT				iFrameIndex);

FLOAT SEWriteBits(	INT			iSampleRate,		/* In:	Sample rate of signal */
					FLOAT		*pfAudioBuffer,		/* Out: Audio buffer containing signal */
					INT			iBits,				/* In:	Number of bits to write */
					BYTE		*pbyData,			/* In:	Data to write */
					FLOAT		fSymbolPhase);		/* In:	Symbol phase */



INT SyncEncoderInit(LPSYNCENCODER		pSyncEncoder,	/* Out: SYNCENCODER structure to be initialized */
					INT					iSampleRate,	/* In:	Signal sample rate */
					INT					iFrameRate,		/* In:	frame rate */
					LPUUIDINFORMATION	pUUID)			/* In:	UUID */
{
	pSyncEncoder->iError = SYNC_ENCODER_ERROR_NONE;

	/* Check and set sample rate */
	pSyncEncoder->iSymbolLength = 1;
	switch(iSampleRate){
		case 48000:
			pSyncEncoder->iSampleRate = iSampleRate;
			pSyncEncoder->iSymbolLength = SYMBOL_LENGTH_48;
		break;
		case 96000:
			pSyncEncoder->iSampleRate = iSampleRate;
			pSyncEncoder->iSymbolLength = SYMBOL_LENGTH_96;
		break;
		default:
			pSyncEncoder->iError = SYNC_ENCODER_ERROR_INVALID_SR;
	};

	if(pSyncEncoder->iError != SYNC_ENCODER_ERROR_NONE){
		return pSyncEncoder->iError;
	}

	/* check and set frame rate */
	switch(iFrameRate){
		case 24:
			pSyncEncoder->iFrameRate = iFrameRate;
			pSyncEncoder->iFrameRateCode = 0;
			pSyncEncoder->iPacketsPerFrame = 4;
		break;
		case 25:
			pSyncEncoder->iFrameRate = iFrameRate;
			pSyncEncoder->iFrameRateCode = 1;
			pSyncEncoder->iPacketsPerFrame = 4;
		break;
		case 30:
			pSyncEncoder->iFrameRate = iFrameRate;
			pSyncEncoder->iFrameRateCode = 2;
			pSyncEncoder->iPacketsPerFrame = 4;
		break;
		case 48:
			pSyncEncoder->iFrameRate = iFrameRate;
			pSyncEncoder->iFrameRateCode = 3;
			pSyncEncoder->iPacketsPerFrame = 2;
		break;
		case 50:
			pSyncEncoder->iFrameRate = iFrameRate;
			pSyncEncoder->iFrameRateCode = 4;
			pSyncEncoder->iPacketsPerFrame = 2;
		break;
		case 60:
			pSyncEncoder->iFrameRate = iFrameRate;
			pSyncEncoder->iFrameRateCode = 5;
			pSyncEncoder->iPacketsPerFrame = 2;
		break;
		case 96:
			pSyncEncoder->iFrameRate = iFrameRate;
			pSyncEncoder->iFrameRateCode = 6;
			pSyncEncoder->iPacketsPerFrame = 1;
		break;
		case 100:
			pSyncEncoder->iFrameRate = iFrameRate;
			pSyncEncoder->iFrameRateCode = 7;
			pSyncEncoder->iPacketsPerFrame = 1;
		break;
		case 120:
			pSyncEncoder->iFrameRate = iFrameRate;
			pSyncEncoder->iFrameRateCode = 8;
			pSyncEncoder->iPacketsPerFrame = 1;
		break;
		default:
			pSyncEncoder->iError = SYNC_ENCODER_ERROR_INVALID_FR;
	};

	if(pSyncEncoder->iError != SYNC_ENCODER_ERROR_NONE){
		return pSyncEncoder->iError;
	}

	/* calculate required buffer length */
	pSyncEncoder->iAudioBufferLength = pSyncEncoder->iSampleRate / pSyncEncoder->iFrameRate;

	/* Calculate total packet bits including wash bits */
	pSyncEncoder->iPacketBits = pSyncEncoder->iAudioBufferLength / (pSyncEncoder->iSymbolLength * pSyncEncoder->iPacketsPerFrame);

	/* Initialize symbol phase */
	pSyncEncoder->fSymbolPhase = 1.0f;

	/* Initialize UUD information */
	pSyncEncoder->iUUIDSubIndex = 0;
	memcpy(&pSyncEncoder->UUID,pUUID,sizeof(UUIDINFORMATION));

	return pSyncEncoder->iError;
}

INT GetSyncEncoderAudioBufferLength(LPSYNCENCODER pSyncEncoder)	/* In: Sync encoder structure */
{
	if(pSyncEncoder->iError != SYNC_ENCODER_ERROR_NONE){
		return pSyncEncoder->iError;
	}

	return pSyncEncoder->iAudioBufferLength;
}



INT EncodeSync(	LPSYNCENCODER	pSyncEncoder,	/* In:	Sync encoder structure */
				INT				iBufferLength,	/* In:	Length of audio buffer */
				FLOAT			*pfAudioBuffer,	/* Out: Audio buffer with signal */
				INT				iFrameIndex)	/* In:	Frame Index */
{
	INT		n;
	INT		iBufferIndex;


	if(pSyncEncoder->iError != SYNC_ENCODER_ERROR_NONE){
		return pSyncEncoder->iError;
	}
	if(iBufferLength != pSyncEncoder->iAudioBufferLength){
		return SYNC_ENCODER_ERROR_INVALID_BL;
	}

	iBufferIndex = 0;
	for(n = 0; n < pSyncEncoder->iPacketsPerFrame; n ++){
		/* Construct message */
		ConstructFrame(pSyncEncoder,iFrameIndex);

		/* Write Message */
		pSyncEncoder->fSymbolPhase = SEWriteBits(pSyncEncoder->iSampleRate,
												&pfAudioBuffer[iBufferIndex],
												pSyncEncoder->iPacketBits,
												pSyncEncoder->abyPacket,
												pSyncEncoder->fSymbolPhase);

		iBufferIndex += (pSyncEncoder->iPacketBits * pSyncEncoder->iSymbolLength);

	}

	return pSyncEncoder->iError;
}

void ConstructFrame(LPSYNCENCODER	pSyncEncoder,
					INT				iFrameIndex)
{
	USHORT	ushCRC;
	BYTE	byByte;
	INT		iUUIDIndex;

	/* Flush the packet buffer */
	memset(pSyncEncoder->abyPacket,0,MAX_PACKET);

	/* Sync Header */
	pSyncEncoder->abyPacket[0] = SYNC_HEADER1;
	pSyncEncoder->abyPacket[1] = SYNC_HEADER2;

	/* Frame Rate code */
	byByte = 0;
	byByte = (unsigned char)(pSyncEncoder->iFrameRateCode << 4);

	/* UUID sub index */
	byByte |= (unsigned char)(pSyncEncoder->iUUIDSubIndex & 0x3);

	pSyncEncoder->abyPacket[2] = byByte;

	/* UUID Sub */
	iUUIDIndex = pSyncEncoder->iUUIDSubIndex << 2;
	pSyncEncoder->abyPacket[3] = pSyncEncoder->UUID.abyUUIDBytes[iUUIDIndex];
	pSyncEncoder->abyPacket[4] = pSyncEncoder->UUID.abyUUIDBytes[iUUIDIndex + 1];
	pSyncEncoder->abyPacket[5] = pSyncEncoder->UUID.abyUUIDBytes[iUUIDIndex + 2];
	pSyncEncoder->abyPacket[6] = pSyncEncoder->UUID.abyUUIDBytes[iUUIDIndex + 3];

	/* Update UUID sub index */
	pSyncEncoder->iUUIDSubIndex ++;
	pSyncEncoder->iUUIDSubIndex &= 0x3;

	/* Frame Index */
	byByte = (unsigned char)((iFrameIndex >> 16) & 0XFF);
	pSyncEncoder->abyPacket[7] = byByte;
	byByte = (unsigned char)((iFrameIndex >> 8) & 0XFF);
	pSyncEncoder->abyPacket[8] = byByte;
	byByte = (unsigned char)(iFrameIndex & 0XFF);
	pSyncEncoder->abyPacket[9] = byByte;

	/* calculate CRC */
	ushCRC = CRC16(&pSyncEncoder->abyPacket[2],MESSAGE_TOTAL_BYTES - 4);

	/* Insert CRC */
	byByte = (unsigned char)((ushCRC >> 8) & 0XFF);
	pSyncEncoder->abyPacket[10] = byByte;
	byByte = (unsigned char)(ushCRC & 0XFF);
	pSyncEncoder->abyPacket[11] = byByte;

}

static FLOAT g_afSymbol0_48[SYMBOL_LENGTH_48] = {
	0.3827f,
    0.9239f,
    0.9239f,
    0.3827f,
};

static FLOAT g_afSymbol1_48[SYMBOL_LENGTH_48] = {
	0.7071f,
    0.7071f,
   -0.7071f,
   -0.7071f,
};

static FLOAT g_afSymbol0_96[SYMBOL_LENGTH_96] = {
	0.1951f,
    0.5556f,
    0.8315f,
    0.9808f,
    0.9808f,
    0.8315f,
    0.5556f,
    0.1951f,
};

static FLOAT g_afSymbol1_96[SYMBOL_LENGTH_96] = {
	0.3827f,
    0.9239f,
    0.9239f,
    0.3827f,
   -0.3827f,
   -0.9239f,
   -0.9239f,
   -0.3827f,
};

/* Symbol gain */
static FLOAT g_fGain = 0.1f;

FLOAT SEWriteBits(	INT			iSampleRate,		/* In:	Sample rate of signal */
					FLOAT		*pfAudioBuffer,		/* Out: Audio buffer containing signal */
					INT			iBits,				/* In:	Number of bits to write */
					BYTE		*pbyData,			/* In:	Data to write */
					FLOAT		fSymbolPhase)		/* In:	Symbol phase */
{
	INT		n;
	INT		i;
	INT		iSymbolLength;
	FLOAT	*pfSymbol0;
	FLOAT	*pfSymbol1;
	BYTE	byByte;

	/* Select the correct symbol length and symbol signal based on sample rate */
	switch (iSampleRate){
		case 96000:
			iSymbolLength = SYMBOL_LENGTH_96;
			pfSymbol0 = g_afSymbol0_96;
			pfSymbol1 = g_afSymbol1_96;
		break;
		case 48000:
			iSymbolLength = SYMBOL_LENGTH_48;
			pfSymbol0 = g_afSymbol0_48;
			pfSymbol1 = g_afSymbol1_48;
		break;
		default:
			iSymbolLength = 0;
			pfSymbol0 = g_afSymbol0_96;
			pfSymbol1 = g_afSymbol1_96;
	};

	/* Write bits */
	n = 0;
	i = 0;
	while(n < iBits){
		INT		k;
		FLOAT	*pfSymbol;

		/* Grab next byte of data */
		if(i == 0){
			byByte = *pbyData;
			pbyData ++;
		}

		pfSymbol = (byByte & 0x80) ? pfSymbol1 : pfSymbol0;

		for(k = 0; k < iSymbolLength; k ++){
			*pfAudioBuffer =  *pfSymbol * fSymbolPhase * g_fGain;
			pfAudioBuffer ++;
			pfSymbol ++;
		}

		fSymbolPhase *= (byByte & 0x80) ? 1.0f : -1.0f;

		byByte <<= 1;

		n ++;

		i ++;
		i &= 0x7;
	}

	return fSymbolPhase;
}
