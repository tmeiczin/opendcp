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
/*! \file    AtmosSyncChannel_Generator.h
    \version $Id: AtmosSyncChannel_Generator.h,v 1.1 2013/04/12 23:39:30 mikey Exp $
    \brief   Dolby Atmos sync channel generator
*/

#ifndef _ATMOSSYNCCHANNEL_GENERATOR_H_
#define _ATMOSSYNCCHANNEL_GENERATOR_H_

#include <AS_DCP.h>
#include "SyncEncoder.h"
#include "UUIDInformation.h"

#define INT24_MAX 8388607.0
#define INT24_MIN -8388608.0

namespace ASDCP
{
    namespace ATMOS
    {
        static const ui32_t SYNC_CHANNEL = 14;
    }

    namespace PCM
    {

        static const ui16_t NUM_BYTES_PER_INT24 = 3;

        class AtmosSyncChannelGenerator
        {
            SYNCENCODER  m_syncEncoder;
            UUIDINFORMATION m_audioTrackUUID;
            AudioDescriptor m_ADesc;
            float *m_syncSignalBuffer;
            ui32_t m_numSamplesPerFrame;
            ui32_t m_currentFrameNumber;
            ui32_t m_numBytesPerFrame;
            bool m_isSyncEncoderInitialized;

            ASDCP_NO_COPY_CONSTRUCT(AtmosSyncChannelGenerator);

        public:
            /**
             * Constructor
             *
             * @param bitsPerSample the number of bits in each sample of pcm data
             * @param sampleRate the sampling rate
             * @param editRate the edit rate of the associated picture track.
             * @param atmosUUID the UUID of the associated ATMOS track file.
             *
             */
            AtmosSyncChannelGenerator(ui16_t bitsPerSample, ui32_t sampleRate,
                               const ASDCP::Rational& editRate, const byte_t* uuid);
            ~AtmosSyncChannelGenerator();

            /**
             * Set the frame number when seeking
             * Use override the default starting frame number for a new track or
             * to set the frame number when doing random access.
             *
             * @param frameNumber
             *
             */
            void setFrameNumber(ui32_t frameNumber) { m_currentFrameNumber = frameNumber; };

            /**
             * Get the number of bytes per frame.
             *
             * @return Number of bytes per frame
             *
             */
            ui32_t getBytesPerFrame() { return m_numBytesPerFrame; }

            /**
             * Generates the next frame of sync data.
             * Generates the next frame of sync data and places it
             * the frame buffer. Fails if the buffer is too small.
             * **Automatically increments the frame number.**
             *
             * @param buf the buffer that the generated frame data will be written to.
             *
             * @return Kumu::RESULT_OK if the buffer is succesfully filled with sync
             *  data for the next frame.
             */
            Result_t ReadFrame(FrameBuffer& buf);

            /**
             * Reset the frame count.
             *
             * @return Kumu::RESULT_OK
             */
            Result_t Reset();

            /**
             * Fill the AudioDescriptor with the relevant information.
             *
             * @return Kumu::RESULT_OK
             */
            Result_t FillAudioDescriptor(PCM::AudioDescriptor& ADesc) const;

            /**
             * Converts a sample float into
             * 24-bit PCM data.
             *
             */
            static inline i32_t convertSampleFloatToInt24(float sample)
            {
                if (sample >= 0.0)
                {
                    return (static_cast<i32_t>(sample * INT24_MAX) << 8);
                }
                else
                {
                    return (static_cast<i32_t>(-sample * INT24_MIN) << 8);
                }
            }
        };

    } // namespace PCM
} // namespace ASDCP

#endif // _ATMOSSYNCCHANNEL_GENERATOR_H_

//
// end AtmosSyncChannel_Generator.h
//
