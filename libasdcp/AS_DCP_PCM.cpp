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
/*! \file    AS_DCP_PCM.cpp
    \version $Id: AS_DCP_PCM.cpp,v 1.39.2.1 2013/12/21 00:13:17 jhurst Exp $       
    \brief   AS-DCP library, PCM essence reader and writer implementation
*/

#include "AS_DCP_internal.h"
#include <map>
#include <iostream>
#include <iomanip>

//------------------------------------------------------------------------------------------

static std::string PCM_PACKAGE_LABEL = "File Package: SMPTE 382M frame wrapping of wave audio";
static std::string SOUND_DEF_LABEL = "Sound Track";

//
Result_t
ASDCP::PCM_ADesc_to_MD(PCM::AudioDescriptor& ADesc, MXF::WaveAudioDescriptor* ADescObj)
{
  ASDCP_TEST_NULL(ADescObj);
  ADescObj->SampleRate = ADesc.EditRate;
  ADescObj->AudioSamplingRate = ADesc.AudioSamplingRate;
  ADescObj->Locked = ADesc.Locked;
  ADescObj->ChannelCount = ADesc.ChannelCount;
  ADescObj->QuantizationBits = ADesc.QuantizationBits;
  ADescObj->BlockAlign = ADesc.BlockAlign;
  ADescObj->AvgBps = ADesc.AvgBps;
  ADescObj->LinkedTrackID = ADesc.LinkedTrackID;
  ADescObj->ContainerDuration = ADesc.ContainerDuration;

  ADescObj->ChannelAssignment.Reset();

  switch ( ADesc.ChannelFormat )
    {
      case PCM::CF_CFG_1:
	ADescObj->ChannelAssignment = DefaultSMPTEDict().Type(MDD_DCAudioChannelCfg_1_5p1).ul;
	break;

      case PCM::CF_CFG_2:
	ADescObj->ChannelAssignment = DefaultSMPTEDict().Type(MDD_DCAudioChannelCfg_2_6p1).ul;
	break;

      case PCM::CF_CFG_3:
	ADescObj->ChannelAssignment = DefaultSMPTEDict().Type(MDD_DCAudioChannelCfg_3_7p1).ul;
	break;

      case PCM::CF_CFG_4:
	ADescObj->ChannelAssignment = DefaultSMPTEDict().Type(MDD_DCAudioChannelCfg_4_WTF).ul;
	break;

      case PCM::CF_CFG_5:
	ADescObj->ChannelAssignment = DefaultSMPTEDict().Type(MDD_DCAudioChannelCfg_5_7p1_DS).ul;
	break;

      case PCM::CF_CFG_6:
	ADescObj->ChannelAssignment = DefaultSMPTEDict().Type(MDD_DCAudioChannelCfg_MCA).ul;
	break;
    }

  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::MD_to_PCM_ADesc(MXF::WaveAudioDescriptor* ADescObj, PCM::AudioDescriptor& ADesc)
{
  ASDCP_TEST_NULL(ADescObj);
  ADesc.EditRate = ADescObj->SampleRate;
  ADesc.AudioSamplingRate = ADescObj->AudioSamplingRate;
  ADesc.Locked = ADescObj->Locked;
  ADesc.ChannelCount = ADescObj->ChannelCount;
  ADesc.QuantizationBits = ADescObj->QuantizationBits;
  ADesc.BlockAlign = ADescObj->BlockAlign;
  ADesc.AvgBps = ADescObj->AvgBps;
  ADesc.LinkedTrackID = ADescObj->LinkedTrackID;
  assert(ADescObj->ContainerDuration <= 0xFFFFFFFFL);
  ADesc.ContainerDuration = (ui32_t) ADescObj->ContainerDuration;

  ADesc.ChannelFormat = PCM::CF_NONE;

  if ( ADescObj->ChannelAssignment.HasValue() )
    {
      if ( ADescObj->ChannelAssignment == DefaultSMPTEDict().Type(MDD_DCAudioChannelCfg_1_5p1).ul )
	ADesc.ChannelFormat = PCM::CF_CFG_1;

      else if ( ADescObj->ChannelAssignment == DefaultSMPTEDict().Type(MDD_DCAudioChannelCfg_2_6p1).ul )
	ADesc.ChannelFormat = PCM::CF_CFG_2;

      else if ( ADescObj->ChannelAssignment == DefaultSMPTEDict().Type(MDD_DCAudioChannelCfg_3_7p1).ul )
	ADesc.ChannelFormat = PCM::CF_CFG_3;

      else if ( ADescObj->ChannelAssignment == DefaultSMPTEDict().Type(MDD_DCAudioChannelCfg_4_WTF).ul )
	ADesc.ChannelFormat = PCM::CF_CFG_4;

      else if ( ADescObj->ChannelAssignment == DefaultSMPTEDict().Type(MDD_DCAudioChannelCfg_5_7p1_DS).ul )
	ADesc.ChannelFormat = PCM::CF_CFG_5;

      else if ( ADescObj->ChannelAssignment == DefaultSMPTEDict().Type(MDD_DCAudioChannelCfg_MCA).ul )
	ADesc.ChannelFormat = PCM::CF_CFG_6;
    }

  return RESULT_OK;
}

//
std::ostream&
ASDCP::PCM::operator << (std::ostream& strm, const AudioDescriptor& ADesc)
{
  strm << "        SampleRate: " << ADesc.EditRate.Numerator << "/" << ADesc.EditRate.Denominator << std::endl;
  strm << " AudioSamplingRate: " << ADesc.AudioSamplingRate.Numerator << "/" << ADesc.AudioSamplingRate.Denominator << std::endl;
  strm << "            Locked: " << (unsigned) ADesc.Locked << std::endl;
  strm << "      ChannelCount: " << (unsigned) ADesc.ChannelCount << std::endl;
  strm << "  QuantizationBits: " << (unsigned) ADesc.QuantizationBits << std::endl;
  strm << "        BlockAlign: " << (unsigned) ADesc.BlockAlign << std::endl;
  strm << "            AvgBps: " << (unsigned) ADesc.AvgBps << std::endl;
  strm << "     LinkedTrackID: " << (unsigned) ADesc.LinkedTrackID << std::endl;
  strm << " ContainerDuration: " << (unsigned) ADesc.ContainerDuration << std::endl;
  strm << "     ChannelFormat: ";
  switch (ADesc.ChannelFormat)
  {
    case CF_NONE:
    default:
      strm << "No Channel Format";
      break;

    case CF_CFG_1:
      strm << "Config 1 (5.1 with optional HI/VI)";
      break;

    case CF_CFG_2:
      strm << "Config 2 (5.1 + center surround with optional HI/VI)";
      break;

    case CF_CFG_3:
      strm << "Config 3 (7.1 with optional HI/VI)";
      break;

    case CF_CFG_4:
      strm << "Config 4";
      break;

    case CF_CFG_5:
      strm << "Config 5 (7.1 DS with optional HI/VI)";
      break;

    case CF_CFG_6:
      strm << "Config 6 (ST 377-1 MCA)";
      break;
  }
  strm << std::endl;

  return strm;
}

//
void
ASDCP::PCM::AudioDescriptorDump(const AudioDescriptor& ADesc, FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "\
        EditRate: %d/%d\n\
 AudioSamplingRate: %d/%d\n\
            Locked: %u\n\
      ChannelCount: %u\n\
  QuantizationBits: %u\n\
        BlockAlign: %u\n\
            AvgBps: %u\n\
     LinkedTrackID: %u\n\
 ContainerDuration: %u\n\
     ChannelFormat: %u\n",
	  ADesc.EditRate.Numerator, ADesc.EditRate.Denominator,
	  ADesc.AudioSamplingRate.Numerator, ADesc.AudioSamplingRate.Denominator,
	  ADesc.Locked,
	  ADesc.ChannelCount,
	  ADesc.QuantizationBits,
	  ADesc.BlockAlign,
	  ADesc.AvgBps,
	  ADesc.LinkedTrackID,
	  ADesc.ContainerDuration,
          ADesc.ChannelFormat
	  );
}


//
//
static ui32_t
calc_CBR_frame_size(ASDCP::WriterInfo& Info, const ASDCP::PCM::AudioDescriptor& ADesc)
{
  ui32_t CBR_frame_size = 0;

  if ( Info.EncryptedEssence )
    {
      CBR_frame_size =
	SMPTE_UL_LENGTH
	+ MXF_BER_LENGTH
	+ klv_cryptinfo_size
	+ calc_esv_length(ASDCP::PCM::CalcFrameBufferSize(ADesc), 0)
	+ ( Info.UsesHMAC ? klv_intpack_size : (MXF_BER_LENGTH * 3) );
    }
  else
    {
      CBR_frame_size = ASDCP::PCM::CalcFrameBufferSize(ADesc) + SMPTE_UL_LENGTH + MXF_BER_LENGTH;
    }

  return CBR_frame_size;
}


//------------------------------------------------------------------------------------------


class ASDCP::PCM::MXFReader::h__Reader : public ASDCP::h__ASDCPReader
{
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);
  h__Reader();

public:
  AudioDescriptor m_ADesc;

  h__Reader(const Dictionary& d) : ASDCP::h__ASDCPReader(d) {}
  ~h__Reader() {}
  Result_t    OpenRead(const char*);
  Result_t    ReadFrame(ui32_t, FrameBuffer&, AESDecContext*, HMACContext*);
};


//
//
ASDCP::Result_t
ASDCP::PCM::MXFReader::h__Reader::OpenRead(const char* filename)
{
  Result_t result = OpenMXFRead(filename);

  if( ASDCP_SUCCESS(result) )
    {
      InterchangeObject* Object;
      if ( ASDCP_SUCCESS(m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(WaveAudioDescriptor), &Object)) )
	{
	  assert(Object);
	  result = MD_to_PCM_ADesc((MXF::WaveAudioDescriptor*)Object, m_ADesc);
	}
    }

  // check for sample/frame rate sanity
  if ( ASDCP_SUCCESS(result)
       && m_ADesc.EditRate != EditRate_24
       && m_ADesc.EditRate != EditRate_25
       && m_ADesc.EditRate != EditRate_30
       && m_ADesc.EditRate != EditRate_48
       && m_ADesc.EditRate != EditRate_50
       && m_ADesc.EditRate != EditRate_60
       && m_ADesc.EditRate != EditRate_96
       && m_ADesc.EditRate != EditRate_100
       && m_ADesc.EditRate != EditRate_120
       && m_ADesc.EditRate != EditRate_16
       && m_ADesc.EditRate != EditRate_18
       && m_ADesc.EditRate != EditRate_20
       && m_ADesc.EditRate != EditRate_22
       && m_ADesc.EditRate != EditRate_23_98 )
    {
      DefaultLogSink().Error("PCM file EditRate is not a supported value: %d/%d\n", // lu
			     m_ADesc.EditRate.Numerator, m_ADesc.EditRate.Denominator);

      // oh, they gave us the audio sampling rate instead, assume 24/1
      if ( m_ADesc.EditRate == SampleRate_48k )
	{
	  DefaultLogSink().Warn("adjusting EditRate to 24/1\n"); 
	  m_ADesc.EditRate = EditRate_24;
	}
      else
	{
      DefaultLogSink().Error("PCM EditRate not in expected value range.\n");
	  // or we just drop the hammer
	  return RESULT_FORMAT;
	}
    }

  if( ASDCP_SUCCESS(result) )
    result = InitMXFIndex();

  if( ASDCP_SUCCESS(result) )
    result = InitInfo();

  // TODO: test file for sane CBR index BytesPerEditUnit

  return result;
}


//
//
ASDCP::Result_t
ASDCP::PCM::MXFReader::h__Reader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
					    AESDecContext* Ctx, HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  assert(m_Dict);
  return ReadEKLVFrame(FrameNum, FrameBuf, m_Dict->ul(MDD_WAVEssence), Ctx, HMAC);
}

//------------------------------------------------------------------------------------------


//
void
ASDCP::PCM::FrameBuffer::Dump(FILE* stream, ui32_t dump_len) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "Frame: %06u, %7u bytes\n",
	  m_FrameNumber, m_Size);

  if ( dump_len )
    Kumu::hexdump(m_Data, dump_len, stream);
}

//------------------------------------------------------------------------------------------

ASDCP::PCM::MXFReader::MXFReader()
{
  m_Reader = new h__Reader(DefaultCompositeDict());
}


ASDCP::PCM::MXFReader::~MXFReader()
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    m_Reader->Close();
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OPAtomHeader&
ASDCP::PCM::MXFReader::OPAtomHeader()
{
  if ( m_Reader.empty() )
    {
      assert(g_OPAtomHeader);
      return *g_OPAtomHeader;
    }

  return m_Reader->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OPAtomIndexFooter&
ASDCP::PCM::MXFReader::OPAtomIndexFooter()
{
  if ( m_Reader.empty() )
    {
      assert(g_OPAtomIndexFooter);
      return *g_OPAtomIndexFooter;
    }

  return m_Reader->m_FooterPart;
}

// Open the file for reading. The file must exist. Returns error if the
// operation cannot be completed.
ASDCP::Result_t
ASDCP::PCM::MXFReader::OpenRead(const char* filename) const
{
  return m_Reader->OpenRead(filename);
}

// Reads a frame of essence from the MXF file. If the optional AESEncContext
// argument is present, the essence is decrypted after reading. If the MXF
// file is encrypted and the AESDecContext argument is NULL, the frame buffer
// will contain the ciphertext frame data.
ASDCP::Result_t
ASDCP::PCM::MXFReader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
				 AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}


ASDCP::Result_t
ASDCP::PCM::MXFReader::LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset, i8_t& temporalOffset, i8_t& keyFrameOffset) const
{
    return m_Reader->LocateFrame(FrameNum, streamOffset, temporalOffset, keyFrameOffset);
}

// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::PCM::MXFReader::FillAudioDescriptor(AudioDescriptor& ADesc) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      ADesc = m_Reader->m_ADesc;
      return RESULT_OK;
    }

  return RESULT_INIT;
}

// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::PCM::MXFReader::FillWriterInfo(WriterInfo& Info) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      Info = m_Reader->m_Info;
      return RESULT_OK;
    }

  return RESULT_INIT;
}

//
void
ASDCP::PCM::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    m_Reader->m_HeaderPart.Dump(stream);
}


//
void
ASDCP::PCM::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_FooterPart.Dump(stream);
}

//
ASDCP::Result_t
ASDCP::PCM::MXFReader::Close() const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      m_Reader->Close();
      return RESULT_OK;
    }

  return RESULT_INIT;
}


//------------------------------------------------------------------------------------------

//
class ASDCP::PCM::MXFWriter::h__Writer : public ASDCP::h__Writer
{
  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

public:
  AudioDescriptor m_ADesc;
  byte_t          m_EssenceUL[SMPTE_UL_LENGTH];
  
  h__Writer(const Dictionary& d) : ASDCP::h__Writer(d) {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  ~h__Writer(){}

  Result_t OpenWrite(const char*, ui32_t HeaderSize);
  Result_t SetSourceStream(const AudioDescriptor&);
  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);
  Result_t Finalize();
};



// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
ASDCP::PCM::MXFWriter::h__Writer::OpenWrite(const char* filename, ui32_t HeaderSize)
{
  if ( ! m_State.Test_BEGIN() )
    return RESULT_STATE;

  Result_t result = m_File.OpenWrite(filename);

  if ( ASDCP_SUCCESS(result) )
    {
      m_HeaderSize = HeaderSize;
      m_EssenceDescriptor = new WaveAudioDescriptor(m_Dict);
      result = m_State.Goto_INIT();
    }

  return result;
}


// Automatically sets the MXF file's metadata from the WAV parser info.
ASDCP::Result_t
ASDCP::PCM::MXFWriter::h__Writer::SetSourceStream(const AudioDescriptor& ADesc)
{
  if ( ! m_State.Test_INIT() )
    return RESULT_STATE;

  if ( ADesc.EditRate != EditRate_24
       && ADesc.EditRate != EditRate_25
       && ADesc.EditRate != EditRate_30
       && ADesc.EditRate != EditRate_48
       && ADesc.EditRate != EditRate_50
       && ADesc.EditRate != EditRate_60
       && ADesc.EditRate != EditRate_96
       && ADesc.EditRate != EditRate_100
       && ADesc.EditRate != EditRate_120
       && ADesc.EditRate != EditRate_16
       && ADesc.EditRate != EditRate_18
       && ADesc.EditRate != EditRate_20
       && ADesc.EditRate != EditRate_22
       && ADesc.EditRate != EditRate_23_98 )
    {
      DefaultLogSink().Error("AudioDescriptor.EditRate is not a supported value: %d/%d\n",
			     ADesc.EditRate.Numerator, ADesc.EditRate.Denominator);
      return RESULT_RAW_FORMAT;
    }

  if ( ADesc.AudioSamplingRate != SampleRate_48k && ADesc.AudioSamplingRate != SampleRate_96k )
    {
      DefaultLogSink().Error("AudioDescriptor.AudioSamplingRate is not 48000/1 or 96000/1: %d/%d\n",
			     ADesc.AudioSamplingRate.Numerator, ADesc.AudioSamplingRate.Denominator);
      return RESULT_RAW_FORMAT;
    }

  assert(m_Dict);
  m_ADesc = ADesc;

  Result_t result = PCM_ADesc_to_MD(m_ADesc, (WaveAudioDescriptor*)m_EssenceDescriptor);
  
  if ( ASDCP_SUCCESS(result) )
    {
      memcpy(m_EssenceUL, m_Dict->ul(MDD_WAVEssence), SMPTE_UL_LENGTH);
      m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container
      result = m_State.Goto_READY();
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t TCFrameRate = m_ADesc.EditRate.Numerator;

      if ( m_ADesc.EditRate == EditRate_23_98  )
	TCFrameRate = 24;
      else if ( m_ADesc.EditRate == EditRate_18  )
	TCFrameRate = 18;
      else if ( m_ADesc.EditRate == EditRate_22  )
	TCFrameRate = 22;
      
      result = WriteMXFHeader(PCM_PACKAGE_LABEL, UL(m_Dict->ul(MDD_WAVWrapping)),
			      SOUND_DEF_LABEL, UL(m_EssenceUL), UL(m_Dict->ul(MDD_SoundDataDef)),
			      m_ADesc.EditRate, TCFrameRate, calc_CBR_frame_size(m_Info, m_ADesc));
    }

  return result;
}


//
//
ASDCP::Result_t
ASDCP::PCM::MXFWriter::h__Writer::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx,
					     HMACContext* HMAC)
{
  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() )
    result = m_State.Goto_RUNNING(); // first time through

  if ( ASDCP_SUCCESS(result) )
    result = WriteEKLVPacket(FrameBuf, m_EssenceUL, Ctx, HMAC);

  if ( ASDCP_SUCCESS(result) )
    m_FramesWritten++;

  return result;
}

// Closes the MXF file, writing the index and other closing information.
//
ASDCP::Result_t
ASDCP::PCM::MXFWriter::h__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;

  m_State.Goto_FINAL();

  return WriteMXFFooter();
}


//------------------------------------------------------------------------------------------
//



ASDCP::PCM::MXFWriter::MXFWriter()
{
}

ASDCP::PCM::MXFWriter::~MXFWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OPAtomHeader&
ASDCP::PCM::MXFWriter::OPAtomHeader()
{
  if ( m_Writer.empty() )
    {
      assert(g_OPAtomHeader);
      return *g_OPAtomHeader;
    }

  return m_Writer->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OPAtomIndexFooter&
ASDCP::PCM::MXFWriter::OPAtomIndexFooter()
{
  if ( m_Writer.empty() )
    {
      assert(g_OPAtomIndexFooter);
      return *g_OPAtomIndexFooter;
    }

  return m_Writer->m_FooterPart;
}

// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
ASDCP::PCM::MXFWriter::OpenWrite(const char* filename, const WriterInfo& Info,
				 const AudioDescriptor& ADesc, ui32_t HeaderSize)
{
  if ( Info.LabelSetType == LS_MXF_SMPTE )
    m_Writer = new h__Writer(DefaultSMPTEDict());
  else
    m_Writer = new h__Writer(DefaultInteropDict());

  m_Writer->m_Info = Info;
  
  Result_t result = m_Writer->OpenWrite(filename, HeaderSize);

  if ( ASDCP_SUCCESS(result) )
    result = m_Writer->SetSourceStream(ADesc);

  if ( ASDCP_FAILURE(result) )
    m_Writer.release();

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
ASDCP::Result_t
ASDCP::PCM::MXFWriter::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
ASDCP::Result_t
ASDCP::PCM::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}

//
// end AS_DCP_PCM.cpp
//

