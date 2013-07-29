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
/*! \file    PCMDataProviders.h
    \version $Id: PCMDataProviders.h,v 1.1 2013/04/12 23:39:31 mikey Exp $
    \brief   PCM sample data providers for WAV, AtmosSync and Silence.
*/

#ifndef _PCMDATAPROVIDERS_H_
#define _PCMDATAPROVIDERS_H_

#include <AS_DCP.h>
#include <AtmosSyncChannel_Generator.h>

namespace ASDCP
{

  // PCM Data Provider Interface
  class PCMDataProviderInterface
  {
      ASDCP_NO_COPY_CONSTRUCT(PCMDataProviderInterface);

  public:
      PCMDataProviderInterface() {};
      virtual ~PCMDataProviderInterface() = 0;
      virtual Result_t PutSample(const ui32_t numChannels, byte_t* buf, ui32_t* bytesWritten) = 0;
      virtual Result_t ReadFrame() = 0;
      virtual Result_t FillAudioDescriptor(PCM::AudioDescriptor& ADesc) const = 0;
      virtual Result_t Reset() = 0;
  };

  // WAV file implementation of the PCM Data Provider Interface
  class WAVDataProvider : public PCMDataProviderInterface
  {
      ASDCP_NO_COPY_CONSTRUCT(WAVDataProvider);
      PCM::WAVParser       m_Parser;
      PCM::FrameBuffer     m_FB;
      PCM::AudioDescriptor m_ADesc;
      const byte_t*        m_ptr;
      ui32_t               m_SampleSize;

  public:
      WAVDataProvider();
      virtual ~WAVDataProvider();
      virtual Result_t PutSample(const ui32_t numChannels, byte_t* buf, ui32_t* bytesWritten);
      virtual Result_t ReadFrame();
      virtual Result_t FillAudioDescriptor(PCM::AudioDescriptor& ADesc) const;
      virtual Result_t Reset();
      Result_t OpenRead(const char* filename, const Rational& PictureRate);

  };

  // Atmos Sync Channel implementation of the PCM Data Provider Interface
  class AtmosSyncDataProvider : public PCMDataProviderInterface
  {
      ASDCP_NO_COPY_CONSTRUCT(AtmosSyncDataProvider);
      PCM::AtmosSyncChannelGenerator m_Generator;
      PCM::FrameBuffer                m_FB;
      PCM::AudioDescriptor            m_ADesc;
      const byte_t*                   m_ptr;
      ui32_t                          m_SampleSize;

  public:
      AtmosSyncDataProvider(const ui16_t bitsPerSample, const ui32_t sampleRate,
                            const ASDCP::Rational& PictureRate, const byte_t* uuid);
      virtual ~AtmosSyncDataProvider();
      virtual Result_t PutSample(const ui32_t numChannels, byte_t* buf, ui32_t* bytesWritten);
      virtual Result_t ReadFrame();
      virtual Result_t FillAudioDescriptor(PCM::AudioDescriptor& ADesc) const;
      virtual Result_t Reset();
  };

  // Silence Channel(s) implementation of the PCM Data Provider Interface
  class SilenceDataProvider : public PCMDataProviderInterface
  {
      ASDCP_NO_COPY_CONSTRUCT(SilenceDataProvider);
      PCM::AudioDescriptor  m_ADesc;
      ui32_t                m_SampleSize;

  public:
      SilenceDataProvider(const ui16_t numChannels, const ui16_t bitsPerSample,
                          const ui32_t sampleRate, const ASDCP::Rational& editRate);
      virtual ~SilenceDataProvider();
      virtual Result_t PutSample(const ui32_t numChannels, byte_t* buf, ui32_t* bytesWritten);
      virtual Result_t ReadFrame();
      virtual Result_t FillAudioDescriptor(PCM::AudioDescriptor& ADesc) const;
      virtual Result_t Reset();
  };

} // namespace ASDCP

#endif // _PCMDATAPROVIDERS_H_

//
// end PCMDataProviders.h
//
