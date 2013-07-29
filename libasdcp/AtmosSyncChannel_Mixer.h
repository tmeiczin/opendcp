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
/*! \file    AtmosSyncChannel_Mixer.h
    \version $Id: AtmosSyncChannel_Mixer.h,v 1.1 2013/04/12 23:39:30 mikey Exp $
    \brief   Read WAV files(s), multiplex multiple PCM frame buffers including Atmos Sync into one
*/

#ifndef _ATMOSSYNCCHANNEL_MIXER_H_
#define _ATMOSSYNCCHANNEL_MIXER_H_

#include <AS_DCP.h>
#include <KM_error.h>
#include <PCMDataProviders.h>
#include <vector>

namespace ASDCP
{

  //
  class AtmosSyncChannelMixer
  {
    typedef std::pair<ui32_t, PCMDataProviderInterface*> InputBus;
    typedef std::vector<InputBus> OutputList;
    typedef std::vector<PCMDataProviderInterface*> SourceList;

    SourceList m_inputs;
    OutputList m_outputs;
    byte_t m_trackUUID[ASDCP::UUIDlen];

    Result_t OpenRead(const std::string& file, const Rational& PictureRate);
    Result_t MixInSilenceChannels();
    Result_t MixInAtmosSyncChannel();
    void clear();

    // functor for deleting
    struct delete_input
    {
        void operator()(PCMDataProviderInterface* i)
        {
            delete i;
        }
    };

    ASDCP_NO_COPY_CONSTRUCT(AtmosSyncChannelMixer);

    protected:
      PCM::AudioDescriptor m_ADesc;
      ui32_t m_ChannelCount;
      ui32_t m_FramesRead;

    public:
      AtmosSyncChannelMixer(const byte_t * trackUUID);
      virtual ~AtmosSyncChannelMixer();

      Result_t OpenRead(ui32_t argc, const char** argv, const Rational& PictureRate);
      Result_t OpenRead(const Kumu::PathList_t& argv, const Rational& PictureRate);
      Result_t FillAudioDescriptor(PCM::AudioDescriptor& ADesc) const;
      Result_t Reset();
      Result_t ReadFrame(PCM::FrameBuffer& OutFB);
    };
} // namespace ASDCP

#endif // _ATMOSSYNCCHANNEL_MIXER_H_

//
// end AtmosSyncChannel_Mixer.h
//
