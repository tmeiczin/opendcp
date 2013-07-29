/*
Copyright (c) 20013-2013, John Hurst
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
/*! \file    PCMDataProviders.cpp
    \version $Id: PCMDataProviders.cpp,v 1.1 2013/04/12 23:39:31 mikey Exp $
    \brief   Implementation of PCM sample data providers for WAV, AtmosSync and Silence.
*/

#include <PCMDataProviders.h>

#include <KM_log.h>

using namespace ASDCP;
using namespace Kumu;

ASDCP::PCMDataProviderInterface::~PCMDataProviderInterface() {}

//
ASDCP::WAVDataProvider::WAVDataProvider()
    : m_Parser(), m_FB(), m_ADesc(), m_SampleSize(0), m_ptr(NULL)
{}

ASDCP::WAVDataProvider::~WAVDataProvider()
{}

Result_t
ASDCP::WAVDataProvider::PutSample(const ui32_t numChannels, byte_t* buf, ui32_t* bytesWritten)
{
  ASDCP_TEST_NULL(buf);
  ASDCP_TEST_NULL(m_ptr);
  if ( numChannels > m_ADesc.ChannelCount)
  {
    DefaultLogSink().Error("Requested %u channels from a wav file with %u channel.", numChannels,
                           m_ADesc.ChannelCount);
    return RESULT_FAIL;
  }
  *bytesWritten = m_SampleSize * numChannels;
  ::memcpy(buf, m_ptr, *bytesWritten);
  m_ptr += *bytesWritten;
  return RESULT_OK;
}

Result_t
ASDCP::WAVDataProvider::ReadFrame()
{
  Result_t result = m_Parser.ReadFrame(m_FB);
  m_ptr = ASDCP_SUCCESS(result) ? m_FB.RoData() : NULL;
  return result;
}

Result_t
ASDCP::WAVDataProvider::FillAudioDescriptor(PCM::AudioDescriptor& ADesc) const
{
  ADesc = m_ADesc;
  return RESULT_OK;
}

Result_t
ASDCP::WAVDataProvider::Reset()
{
    return m_Parser.Reset();
}

Result_t
ASDCP::WAVDataProvider::OpenRead(const char* filename, const Rational& PictureRate)
{
  ASDCP_TEST_NULL_STR(filename);

  Result_t result = m_Parser.OpenRead(filename, PictureRate);

  if ( ASDCP_SUCCESS(result) )
    result = m_Parser.FillAudioDescriptor(m_ADesc);

  if ( ASDCP_SUCCESS(result) )
  {
    m_ADesc.EditRate = PictureRate;
    m_SampleSize = ((m_ADesc.QuantizationBits + 7) / 8);
    result = m_FB.Capacity(PCM::CalcFrameBufferSize(m_ADesc));
  }

  return result;
}

//
ASDCP::AtmosSyncDataProvider::AtmosSyncDataProvider(const ui16_t bitsPerSample, const ui32_t sampleRate,
                                                    const ASDCP::Rational& editRate, const byte_t* uuid)
    : m_Generator(bitsPerSample, sampleRate, editRate, uuid), m_FB(), m_ADesc(), m_SampleSize()
{
    m_Generator.FillAudioDescriptor(m_ADesc);
    m_SampleSize = PCM::CalcSampleSize(m_ADesc);
    m_FB.Capacity(PCM::CalcFrameBufferSize(m_ADesc));
}

ASDCP::AtmosSyncDataProvider::~AtmosSyncDataProvider()
{}

Result_t
ASDCP::AtmosSyncDataProvider::PutSample(const ui32_t numChannels, byte_t* buf, ui32_t* bytesWritten)
{
  ASDCP_TEST_NULL(buf);
  ASDCP_TEST_NULL(m_ptr);
  if ( numChannels > m_ADesc.ChannelCount)
  {
    DefaultLogSink().Error("Requested %u channels from a wav file with %u channel.", numChannels,
                           m_ADesc.ChannelCount);
    return RESULT_FAIL;
  }

  (*bytesWritten) = m_SampleSize;
  ::memcpy(buf, m_ptr, m_SampleSize);
  m_ptr += m_SampleSize;
  return RESULT_OK;
}

Result_t
ASDCP::AtmosSyncDataProvider::ReadFrame()
{
  Result_t result = m_Generator.ReadFrame(m_FB);
  m_ptr = ASDCP_SUCCESS(result) ? m_FB.RoData() : NULL;
  return result;
}

Result_t
ASDCP::AtmosSyncDataProvider::FillAudioDescriptor(PCM::AudioDescriptor& ADesc) const
{
  ADesc = m_ADesc;
  return RESULT_OK;
}

Result_t
ASDCP::AtmosSyncDataProvider::Reset()
{
    return m_Generator.Reset();
}

//
ASDCP::SilenceDataProvider::SilenceDataProvider(const ui16_t numChannels, const ui16_t bitsPerSample,
                                                const ui32_t sampleRate, const ASDCP::Rational& editRate)
    : m_ADesc(), m_SampleSize(0)
{
    m_SampleSize = ((bitsPerSample + 7) / 8);
    m_ADesc.EditRate = editRate;
    m_ADesc.AudioSamplingRate = Rational(sampleRate, 1);
    m_ADesc.ChannelCount = numChannels;
    m_ADesc.QuantizationBits = bitsPerSample;
    m_ADesc.BlockAlign = numChannels * m_SampleSize;
    m_ADesc.AvgBps = sampleRate * m_ADesc.BlockAlign;
}

ASDCP::SilenceDataProvider::~SilenceDataProvider()
{}

Result_t
ASDCP::SilenceDataProvider::PutSample(const ui32_t numChannels, byte_t* buf, ui32_t* bytesWritten)
{
  ASDCP_TEST_NULL(buf);
  if ( numChannels > m_ADesc.ChannelCount)
  {
    DefaultLogSink().Error("Requested %u channels from a wav file with %u channel.", numChannels,
                           m_ADesc.ChannelCount);
    return RESULT_FAIL;
  }
  (*bytesWritten) = m_SampleSize * numChannels;
  ::memset(buf, 0, (*bytesWritten));
  return RESULT_OK;
}

Result_t
ASDCP::SilenceDataProvider::ReadFrame()
{
    // no op
    return RESULT_OK;
}

Result_t
ASDCP::SilenceDataProvider::FillAudioDescriptor(PCM::AudioDescriptor& ADesc) const
{
  ADesc = m_ADesc;
  return RESULT_OK;
}

Result_t
ASDCP::SilenceDataProvider::Reset()
{
    //no op
    return RESULT_OK;
}
