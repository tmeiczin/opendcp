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
/*! \file    AtmosSyncChannel_Generator.cpp
    \version $Id: AtmosSyncChannel_Generator.cpp,v 1.1 2013/04/12 23:39:30 mikey Exp $
    \brief   Dolby Atmos sync channel generator implementation
*/

#include <AtmosSyncChannel_Generator.h>

#include <AS_DCP.h>

using namespace ASDCP;

//
ASDCP::PCM::AtmosSyncChannelGenerator::AtmosSyncChannelGenerator(ui16_t bitsPerSample, ui32_t sampleRate,
                                                                 const ASDCP::Rational& editRate, const byte_t* uuid)
    : m_syncEncoder(),
      m_audioTrackUUID(),
      m_ADesc(),
      m_syncSignalBuffer(NULL),
      m_numSamplesPerFrame(0),
      m_currentFrameNumber(0),
      m_numBytesPerFrame(0),
      m_isSyncEncoderInitialized(false)
{

    m_ADesc.EditRate = editRate;
    m_ADesc.ChannelCount = 1;
    m_ADesc.QuantizationBits = bitsPerSample;
    m_ADesc.AudioSamplingRate = Rational(sampleRate, 1);
    m_ADesc.BlockAlign = ((bitsPerSample + 7) / 8);
    m_ADesc.AvgBps = (sampleRate * m_ADesc.BlockAlign);

    memcpy(&m_audioTrackUUID.abyUUIDBytes[0], uuid, UUIDlen);
    m_numSamplesPerFrame = (editRate.Denominator * sampleRate) / editRate.Numerator;
    m_numBytesPerFrame = m_numSamplesPerFrame * m_ADesc.BlockAlign;

    if (bitsPerSample == 24)
    {
        ui32_t frameRate = editRate.Numerator/editRate.Denominator; // intentionally allowing for imprecise cast to int
        m_isSyncEncoderInitialized = (SyncEncoderInit(&m_syncEncoder, sampleRate, frameRate, &m_audioTrackUUID) == SYNC_ENCODER_ERROR_NONE);
        m_syncSignalBuffer = new float[m_numSamplesPerFrame];
    }
    else
    {
        m_isSyncEncoderInitialized = false;
    }
}

ASDCP::PCM::AtmosSyncChannelGenerator::~AtmosSyncChannelGenerator()
{
    delete [] m_syncSignalBuffer;
}

ASDCP::Result_t
ASDCP::PCM::AtmosSyncChannelGenerator::ReadFrame(FrameBuffer& OutFB)
{
    if (OutFB.Capacity() < m_numBytesPerFrame)
    {
        return RESULT_SMALLBUF;
    }

    /**
     * Update frame number and size.
     */
    OutFB.FrameNumber(m_currentFrameNumber);
    OutFB.Size(m_numBytesPerFrame);

    /**
     * Get pointer to frame essence.
     */
    byte_t* frameEssence = OutFB.Data();

    if (m_isSyncEncoderInitialized)
    {
        /**
         * Generate sync signal frame.
         */
        int ret = EncodeSync(&m_syncEncoder, m_numSamplesPerFrame, m_syncSignalBuffer, m_currentFrameNumber);
        if (ret == SYNC_ENCODER_ERROR_NONE)
        {
            for (unsigned int i = 0; i < m_numSamplesPerFrame; ++i)
            {
                /**
                 * Convert each encoded float sample to a signed 24 bit integer and
                 * copy into the essence buffer.
                 */
                i32_t sample = convertSampleFloatToInt24(m_syncSignalBuffer[i]);
                memcpy(frameEssence, ((byte_t*)(&sample))+1, NUM_BYTES_PER_INT24);
                frameEssence += NUM_BYTES_PER_INT24;
            }
        }
        else
        {
            /**
             * Encoding error, zero out the frame.
             */
            memset(frameEssence, 0, m_numBytesPerFrame);
        }
    }
    else
    {
        /**
         * Sync encoder not initialize, zero out the frame.
         */
        memset(frameEssence, 0, m_numBytesPerFrame);
    }
    ++m_currentFrameNumber;
    return RESULT_OK;
}

ASDCP::Result_t
ASDCP::PCM::AtmosSyncChannelGenerator::Reset()
{
  m_currentFrameNumber = 0;
  return RESULT_OK;
}

Result_t
ASDCP::PCM::AtmosSyncChannelGenerator::FillAudioDescriptor(AudioDescriptor& ADesc) const
{
  ADesc = m_ADesc;
  return RESULT_OK;
}

