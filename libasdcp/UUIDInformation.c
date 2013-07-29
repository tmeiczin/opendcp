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
    \version $Id: UUIDInformation.c,v 1.1 2013/04/12 23:39:31 mikey Exp $
    \brief   Implementation of Atmos Sync UUID
*/

#include "UUIDInformation.h"
#include <stdlib.h>


void UUIDSynthesize(LPUUIDINFORMATION pUUID)
{
	INT n;

	for(n = 0; n < 16; n ++){
		pUUID->abyUUIDBytes[n] = (BYTE)(rand() & 0xFF);
	}

	pUUID->abyUUIDBytes[6] &= 0x0F;
	pUUID->abyUUIDBytes[6] |= 0x40;

	pUUID->abyUUIDBytes[8] &= 0x0F;
	pUUID->abyUUIDBytes[8] |= 0xA0;
}

void UUIDPrint(	FILE				*pFilePtr,
				LPUUIDINFORMATION	pUUID)
{
	if(pFilePtr != NULL){
		INT n;

		for(n = 0; n < 16; n ++){
			fprintf(pFilePtr,"%02x",pUUID->abyUUIDBytes[n]);
		}
	}
	else{
		INT n;

		for(n = 0; n < 16; n ++){
			fprintf(stdout,"%02x",pUUID->abyUUIDBytes[n]);
		}
	}
}

void UUIDPrintFormated(	FILE				*pFilePtr,
						LPUUIDINFORMATION	pUUID)
{
	if(pFilePtr != NULL){
		INT n;

		for(n = 0; n < 4; n ++){
			fprintf(pFilePtr,"%02x",pUUID->abyUUIDBytes[n]);
		}
		fprintf(pFilePtr,"-");
		for(n = 4; n < 6; n ++){
			fprintf(pFilePtr,"%02x",pUUID->abyUUIDBytes[n]);
		}
		fprintf(pFilePtr,"-");
		for(n = 6; n < 8; n ++){
			fprintf(pFilePtr,"%02x",pUUID->abyUUIDBytes[n]);
		}
		fprintf(pFilePtr,"-");
		for(n = 8; n < 10; n ++){
			fprintf(pFilePtr,"%02x",pUUID->abyUUIDBytes[n]);
		}
		fprintf(pFilePtr,"-");
		for(n = 10; n < 16; n ++){
			fprintf(pFilePtr,"%02x",pUUID->abyUUIDBytes[n]);
		}
	}
	else{
		INT n;

		for(n = 0; n < 4; n ++){
			fprintf(stdout,"%02x",pUUID->abyUUIDBytes[n]);
		}
		fprintf(stdout,"-");
		for(n = 4; n < 6; n ++){
			fprintf(stdout,"%02x",pUUID->abyUUIDBytes[n]);
		}
		fprintf(stdout,"-");
		for(n = 6; n < 8; n ++){
			fprintf(stdout,"%02x",pUUID->abyUUIDBytes[n]);
		}
		fprintf(stdout,"-");
		for(n = 8; n < 10; n ++){
			fprintf(stdout,"%02x",pUUID->abyUUIDBytes[n]);
		}
		fprintf(stdout,"-");
		for(n = 10; n < 16; n ++){
			fprintf(stdout,"%02x",pUUID->abyUUIDBytes[n]);
		}
	}
}
