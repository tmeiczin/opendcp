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
/*! \file    AtmosSyncChannel_Mixer.cpp
    \version $Id: AtmosSyncChannel_Mixer.cpp,v 1.1 2013/04/12 23:39:30 mikey Exp $
    \brief   Read WAV files(s), multiplex multiple PCM frame buffers including Atmos Sync into one
*/

#include <AtmosSyncChannel_Mixer.h>

#include <algorithm>

#include <AS_DCP.h>
#include <KM_log.h>
#include <PCMDataProviders.h>

using namespace ASDCP;
using namespace Kumu;

//
ASDCP::AtmosSyncChannelMixer::AtmosSyncChannelMixer(const byte_t * trackUUID)
    : m_inputs(), m_outputs(), m_trackUUID(), m_ADesc(), m_ChannelCount(0), m_FramesRead(0)
{
  ::memcpy(m_trackUUID, trackUUID, UUIDlen);
}

ASDCP::AtmosSyncChannelMixer::~AtmosSyncChannelMixer()
{
  clear();
}

void
ASDCP::AtmosSyncChannelMixer::clear()
{
  m_outputs.clear();
  std::for_each(m_inputs.begin(), m_inputs.end(), delete_input());
  m_inputs.clear();
}

//
Result_t
ASDCP::AtmosSyncChannelMixer::OpenRead(ui32_t argc, const char** argv, const Rational& PictureRate)
{
  ASDCP_TEST_NULL_STR(argv);
  PathList_t TmpFileList;

  for ( ui32_t i = 0; i < argc; ++i )
    TmpFileList.push_back(argv[i]);

  return OpenRead(TmpFileList, PictureRate);
}

//
Result_t
ASDCP::AtmosSyncChannelMixer::OpenRead(const Kumu::PathList_t& argv, const Rational& PictureRate)
{
  Result_t result = RESULT_OK;
  PathList_t::iterator fi;
  Kumu::PathList_t file_list;
  PCM::AudioDescriptor tmpDesc;

  if ( argv.size() == 1 && PathIsDirectory(argv.front()) )
  {
    DirScanner Dir;
    char name_buf[MaxFilePath];
    result = Dir.Open(argv.front().c_str());

    if ( KM_SUCCESS(result) )
      result = Dir.GetNext(name_buf);

    while ( KM_SUCCESS(result) )
	{
	  if ( name_buf[0] != '.' ) // no hidden files
      {
        std::string tmp_path = argv.front() + "/" + name_buf;
        file_list.push_back(tmp_path);
      }

	  result = Dir.GetNext(name_buf);
	}

    if ( result == RESULT_ENDOFFILE )
	{
	  result = RESULT_OK;
	  file_list.sort();
	}
  }
  else
  {
    file_list = argv;
  }

  for ( fi = file_list.begin(); KM_SUCCESS(result) && fi != file_list.end(); ++fi )
  {
    result = OpenRead(*fi, PictureRate);
  }

  if ( ASDCP_SUCCESS(result) && (m_ChannelCount < ATMOS::SYNC_CHANNEL))
  {
    // atmos sync channel has not been added
    result = MixInSilenceChannels();
    if ( ASDCP_SUCCESS(result) )
      result = MixInAtmosSyncChannel();
  }

  if ( ASDCP_SUCCESS(result) )
  {
    m_ADesc.ChannelCount = m_ChannelCount;
    m_ADesc.AvgBps = (ui32_t)(ceil(m_ADesc.AudioSamplingRate.Quotient()) * m_ADesc.BlockAlign);
  }
  else
  {
    clear();
  }

  return result;
}

//
Result_t
ASDCP::AtmosSyncChannelMixer::OpenRead(const std::string& file, const Rational& PictureRate)
{
  Result_t result = RESULT_OK;
  PCM::AudioDescriptor tmpDesc;
  ui32_t numChannels = 0;
  mem_ptr<WAVDataProvider> I = new WAVDataProvider;
  result = I->OpenRead(file.c_str(), PictureRate);

  if ( ASDCP_SUCCESS(result))
  {
    result = I->FillAudioDescriptor(tmpDesc);
  }

  if ( ASDCP_SUCCESS(result) )
  {

    if ( m_ChannelCount == 0 )
    {
      m_ADesc = tmpDesc;
    }
    else
    {

      if ( tmpDesc.AudioSamplingRate != m_ADesc.AudioSamplingRate )
      {
        DefaultLogSink().Error("AudioSamplingRate mismatch in PCM parser list.");
        return RESULT_FORMAT;
      }

      if ( tmpDesc.QuantizationBits  != m_ADesc.QuantizationBits )
      {
        DefaultLogSink().Error("QuantizationBits mismatch in PCM parser list.");
        return RESULT_FORMAT;
      }

      if ( tmpDesc.ContainerDuration < m_ADesc.ContainerDuration )
        m_ADesc.ContainerDuration = tmpDesc.ContainerDuration;

      m_ADesc.BlockAlign += tmpDesc.BlockAlign;
    }
  }


  if ( ASDCP_SUCCESS(result) )
  {
    numChannels = tmpDesc.ChannelCount; // default to all channels
    if ((m_ChannelCount < ATMOS::SYNC_CHANNEL) && (m_ChannelCount + numChannels) > (ATMOS::SYNC_CHANNEL - 1))
    {
      // need to insert an atmos channel between the channels of this file.
      numChannels = ATMOS::SYNC_CHANNEL - m_ChannelCount - 1;
      m_outputs.push_back(std::make_pair(numChannels, I.get()));
      m_ChannelCount += numChannels;
      MixInAtmosSyncChannel();
      numChannels = tmpDesc.ChannelCount - numChannels;
    }
    m_outputs.push_back(std::make_pair(numChannels, I.get()));
    m_inputs.push_back(I);
    I.release();
    m_ChannelCount += numChannels;
  }
  return result;
}

Result_t
ASDCP::AtmosSyncChannelMixer::MixInSilenceChannels()
{
  Result_t result = RESULT_OK;
  PCM::AudioDescriptor tmpDesc;
  ui32_t numSilenceChannels = ATMOS::SYNC_CHANNEL - m_ChannelCount - 1;
  if (numSilenceChannels > 0)
  {
    mem_ptr<SilenceDataProvider> I = new SilenceDataProvider(numSilenceChannels,
                                                             m_ADesc.QuantizationBits,
                                                             m_ADesc.AudioSamplingRate.Numerator,
                                                             m_ADesc.EditRate);
    result = I->FillAudioDescriptor(tmpDesc);
    if ( ASDCP_SUCCESS(result) )
    {
      m_ADesc.BlockAlign += tmpDesc.BlockAlign;
      m_ChannelCount += tmpDesc.ChannelCount;
      m_outputs.push_back(std::make_pair(numSilenceChannels, I.get()));
      m_inputs.push_back(I);
      I.release();
      assert(m_ChannelCount == (ATMOS::SYNC_CHANNEL - 1));
    }
  }
  return result;
}

//
Result_t
ASDCP::AtmosSyncChannelMixer::MixInAtmosSyncChannel()
{
  Result_t result = RESULT_OK;
  PCM::AudioDescriptor tmpDesc;
  mem_ptr<AtmosSyncDataProvider> I = new AtmosSyncDataProvider(m_ADesc.QuantizationBits,
                                                               m_ADesc.AudioSamplingRate.Numerator,
                                                               m_ADesc.EditRate, m_trackUUID);
  result = I->FillAudioDescriptor(tmpDesc);
  if ( ASDCP_SUCCESS(result) )
  {
    m_ADesc.BlockAlign += tmpDesc.BlockAlign;
    m_ChannelCount += tmpDesc.ChannelCount;
    m_outputs.push_back(std::make_pair(tmpDesc.ChannelCount, I.get()));
    m_inputs.push_back(I);
    I.release();
    assert(m_ChannelCount == ATMOS::SYNC_CHANNEL);
  }
  return result;
}

//
Result_t
ASDCP::AtmosSyncChannelMixer::FillAudioDescriptor(PCM::AudioDescriptor& ADesc) const
{
  ADesc = m_ADesc;
  return RESULT_OK;
}

//
Result_t
ASDCP::AtmosSyncChannelMixer::Reset()
{
  Result_t result = RESULT_OK;
  SourceList::iterator it;
  SourceList::iterator lastInput = m_inputs.end();

  for ( it = m_inputs.begin(); it != lastInput && ASDCP_SUCCESS(result) ; ++it )
    result = (*it)->Reset();

  return result;
}


//2
Result_t
ASDCP::AtmosSyncChannelMixer::ReadFrame(PCM::FrameBuffer& OutFB)
{


  Result_t result = RESULT_OK;
  SourceList::iterator iter;
  SourceList::iterator lastInput = m_inputs.end();
  ui32_t bufSize = PCM::CalcFrameBufferSize(m_ADesc);
  assert( bufSize <= OutFB.Capacity());

  for ( iter = m_inputs.begin(); iter != lastInput && ASDCP_SUCCESS(result) ; ++iter )
    result = (*iter)->ReadFrame();

  if ( ASDCP_SUCCESS(result) )
  {
    OutFB.Size(bufSize);
    byte_t* Out_p = OutFB.Data();
    byte_t* End_p = Out_p + OutFB.Size();
    ui32_t bytesWritten = 0;
    OutputList::iterator iter;
    OutputList::iterator lastOutput = m_outputs.end();

    while ( Out_p < End_p && ASDCP_SUCCESS(result) )
	{
        iter = m_outputs.begin();
        while ( iter != lastOutput && ASDCP_SUCCESS(result) )
        {
            result = ((*iter).second)->PutSample((*iter).first, Out_p, &bytesWritten);
            Out_p += bytesWritten;
            ++iter;
        }
    }

    if ( ASDCP_SUCCESS(result) )
    {
      assert(Out_p == End_p);
      OutFB.FrameNumber(m_FramesRead++);
    }
  }

  return result;
}


//
// end AtmosSyncChannel_Mixer.cpp
//
