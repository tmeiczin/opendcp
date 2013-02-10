/*
Copyright (c) 2004-2012, John Hurst
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
/*! \file    PCMParserList.h
    \version $Id: PCMParserList.h,v 1.4 2012/02/03 19:49:56 jhurst Exp $
    \brief   Read WAV file(s), multiplex multiple PCM frame buffers into one
*/

#ifndef _PCMPARSERLIST_H_
#define _PCMPARSERLIST_H_

#include <KM_fileio.h>
#include <AS_DCP.h>
#include <vector>

namespace ASDCP
{
  //
  class ParserInstance
    {
      const byte_t* m_p;
      ui32_t        m_SampleSize;

      ASDCP_NO_COPY_CONSTRUCT(ParserInstance);

    public:
      PCM::WAVParser       Parser;
      PCM::FrameBuffer     FB;
      PCM::AudioDescriptor ADesc;

      ParserInstance();
      virtual ~ParserInstance();

      Result_t OpenRead(const char* filename, const Rational& PictureRate);
      Result_t PutSample(byte_t* p);
      Result_t ReadFrame();
      inline ui32_t SampleSize()  { return m_SampleSize; }
    };

  //
  class PCMParserList : public std::vector<ParserInstance*>
    {
      ASDCP_NO_COPY_CONSTRUCT(PCMParserList);

    protected:
      PCM::AudioDescriptor m_ADesc;
      ui32_t m_ChannelCount;

    public:
      PCMParserList();
      virtual ~PCMParserList();

      Result_t OpenRead(ui32_t argc, const char** argv, const Rational& PictureRate);
      Result_t OpenRead(const Kumu::PathList_t& argv, const Rational& PictureRate);
      Result_t FillAudioDescriptor(PCM::AudioDescriptor& ADesc) const;
      Result_t Reset();
      Result_t ReadFrame(PCM::FrameBuffer& OutFB);
    };
}


#endif // _PCMPARSERLIST_H_

//
// end PCMParserList.h
//
