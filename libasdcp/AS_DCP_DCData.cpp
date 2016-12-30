/*
Copyright (c) 2004-2016, John Hurst
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
/*! \file    AS_DCP_DCData.cpp
    \version $Id: AS_DCP_DCData.cpp,v 1.9 2016/11/22 17:58:18 jhurst Exp $
    \brief   AS-DCP library, Dcinema generic data essence reader and writer implementation
*/

#include <iostream>

#include "AS_DCP.h"
#include "AS_DCP_internal.h"

namespace ASDCP
{
  namespace DCData
  {
    static std::string DC_DATA_PACKAGE_LABEL = "File Package: SMPTE-GC frame wrapping of D-Cinema Generic data";
    static std::string DC_DATA_DEF_LABEL = "D-Cinema Generic Data Track";
  } // namespace DCData
} // namespace ASDCP

//
std::ostream&
ASDCP::DCData::operator << (std::ostream& strm, const DCDataDescriptor& DDesc)
{
  char str_buf[40];
  strm << "          EditRate: " << DDesc.EditRate.Numerator << "/" << DDesc.EditRate.Denominator << std::endl;
  strm << " ContainerDuration: " << (unsigned) DDesc.ContainerDuration << std::endl;
  strm << " DataEssenceCoding: " << UL(DDesc.DataEssenceCoding).EncodeString(str_buf, 40) << std::endl;
  return strm;
}

//
void
ASDCP::DCData::DCDataDescriptorDump(const DCDataDescriptor& DDesc, FILE* stream)
{
  char str_buf[40];
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "\
            EditRate: %d/%d\n\
   ContainerDuration: %u\n\
   DataEssenceCoding: %s\n",
          DDesc.EditRate.Numerator, DDesc.EditRate.Denominator,
          DDesc.ContainerDuration,
          UL(DDesc.DataEssenceCoding).EncodeString(str_buf, 40));
}


//------------------------------------------------------------------------------------------

typedef std::list<MXF::InterchangeObject*> SubDescriptorList_t;

class ASDCP::DCData::MXFReader::h__Reader : public ASDCP::h__ASDCPReader
{
  bool m_PrivateLabelCompatibilityMode;
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);
  h__Reader();

 public:
  DCDataDescriptor m_DDesc;

  h__Reader(const Dictionary& d) : ASDCP::h__ASDCPReader(d), m_PrivateLabelCompatibilityMode(false), m_DDesc() {}
  ~h__Reader() {}
  Result_t    OpenRead(const std::string&);
  Result_t    ReadFrame(ui32_t, FrameBuffer&, AESDecContext*, HMACContext*);
  Result_t    MD_to_DCData_DDesc(const MXF::DCDataDescriptor& descriptor_object, DCData::DCDataDescriptor& DDesc);
  Result_t    MD_to_DCData_DDesc(const MXF::PrivateDCDataDescriptor& descriptor_object, DCData::DCDataDescriptor& DDesc);
};

//
ASDCP::Result_t
ASDCP::DCData::MXFReader::h__Reader::MD_to_DCData_DDesc(const MXF::DCDataDescriptor& descriptor_object,
							DCData::DCDataDescriptor& DDesc)
{
  DDesc.EditRate = descriptor_object.SampleRate;
  assert(descriptor_object.ContainerDuration.const_get() <= 0xFFFFFFFFL);
  DDesc.ContainerDuration = static_cast<ui32_t>(descriptor_object.ContainerDuration.const_get());
  memcpy(DDesc.DataEssenceCoding, descriptor_object.DataEssenceCoding.Value(), SMPTE_UL_LENGTH);
  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::DCData::MXFReader::h__Reader::MD_to_DCData_DDesc(const MXF::PrivateDCDataDescriptor& descriptor_object,
							DCData::DCDataDescriptor& DDesc)
{
  DDesc.EditRate = descriptor_object.SampleRate;
  assert(descriptor_object.ContainerDuration.const_get() <= 0xFFFFFFFFL);
  DDesc.ContainerDuration = static_cast<ui32_t>(descriptor_object.ContainerDuration.const_get());
  memcpy(DDesc.DataEssenceCoding, descriptor_object.DataEssenceCoding.Value(), SMPTE_UL_LENGTH);
  return RESULT_OK;
}

//
//
ASDCP::Result_t
ASDCP::DCData::MXFReader::h__Reader::OpenRead(const std::string& filename)
{
  Result_t result = OpenMXFRead(filename);

  if( KM_SUCCESS(result) )
    {
      InterchangeObject* iObj = 0;
      result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(DCDataDescriptor), &iObj);

      if ( KM_SUCCESS(result) )
	{
	  const MXF::DCDataDescriptor* p = dynamic_cast<const MXF::DCDataDescriptor*>(iObj);
	  assert(p);
	  result = MD_to_DCData_DDesc(*p, m_DDesc);
	}
      else
	{
	  result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(PrivateDCDataDescriptor), &iObj);
	  
	  if ( KM_SUCCESS(result) )
	    {
	      m_PrivateLabelCompatibilityMode = true;
	      const MXF::PrivateDCDataDescriptor* p = dynamic_cast<const MXF::PrivateDCDataDescriptor*>(iObj);
	      assert(p);
	      result = MD_to_DCData_DDesc(*p, m_DDesc);
	    }
	}

      if ( KM_FAILURE(result) )
	{
	  DefaultLogSink().Error("DCDataDescriptor object not found in ST 429-14 file.\n");
	  result = RESULT_FORMAT;
	}
    }

  // check for sample/frame rate sanity
  if ( ASDCP_SUCCESS(result)
       && m_DDesc.EditRate != EditRate_24
       && m_DDesc.EditRate != EditRate_25
       && m_DDesc.EditRate != EditRate_30
       && m_DDesc.EditRate != EditRate_48
       && m_DDesc.EditRate != EditRate_50
       && m_DDesc.EditRate != EditRate_60
       && m_DDesc.EditRate != EditRate_96
       && m_DDesc.EditRate != EditRate_100
       && m_DDesc.EditRate != EditRate_120
       && m_DDesc.EditRate != EditRate_192
       && m_DDesc.EditRate != EditRate_200
       && m_DDesc.EditRate != EditRate_240 )
  {
    DefaultLogSink().Error("DC Data file EditRate is not a supported value: %d/%d\n", // lu
                           m_DDesc.EditRate.Numerator, m_DDesc.EditRate.Denominator);

    return RESULT_FORMAT;
  }

  return result;
}

//
//
ASDCP::Result_t
ASDCP::DCData::MXFReader::h__Reader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
		      AESDecContext* Ctx, HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  assert(m_Dict);
  if ( m_PrivateLabelCompatibilityMode )
    {
      return ReadEKLVFrame(FrameNum, FrameBuf, m_Dict->ul(MDD_PrivateDCDataEssence), Ctx, HMAC);
    }

  return ReadEKLVFrame(FrameNum, FrameBuf, m_Dict->ul(MDD_DCDataEssence), Ctx, HMAC);
}



//------------------------------------------------------------------------------------------


//
void
ASDCP::DCData::FrameBuffer::Dump(FILE* stream, ui32_t dump_len) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "Frame: %06u, %7u bytes\n", m_FrameNumber, m_Size);

  if ( dump_len > 0 )
    Kumu::hexdump(m_Data, dump_len, stream);
}


//------------------------------------------------------------------------------------------

ASDCP::DCData::MXFReader::MXFReader()
{
  m_Reader = new h__Reader(DefaultSMPTEDict());
}


ASDCP::DCData::MXFReader::~MXFReader()
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    m_Reader->Close();
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::DCData::MXFReader::OP1aHeader()
{
  if ( m_Reader.empty() )
    {
      assert(g_OP1aHeader);
      return *g_OP1aHeader;
    }

  return m_Reader->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OPAtomIndexFooter&
ASDCP::DCData::MXFReader::OPAtomIndexFooter()
{
  if ( m_Reader.empty() )
    {
      assert(g_OPAtomIndexFooter);
      return *g_OPAtomIndexFooter;
    }

  return m_Reader->m_IndexAccess;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::RIP&
ASDCP::DCData::MXFReader::RIP()
{
  if ( m_Reader.empty() )
    {
      assert(g_RIP);
      return *g_RIP;
    }

  return m_Reader->m_RIP;
}

// Open the file for reading. The file must exist. Returns error if the
// operation cannot be completed.
ASDCP::Result_t
ASDCP::DCData::MXFReader::OpenRead(const std::string& filename) const
{
  return m_Reader->OpenRead(filename);
}

//
ASDCP::Result_t
ASDCP::DCData::MXFReader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
                                    AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}

ASDCP::Result_t
ASDCP::DCData::MXFReader::LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset, i8_t& temporalOffset, i8_t& keyFrameOffset) const
{
    return m_Reader->LocateFrame(FrameNum, streamOffset, temporalOffset, keyFrameOffset);
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::DCData::MXFReader::FillDCDataDescriptor(DCDataDescriptor& DDesc) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      DDesc = m_Reader->m_DDesc;
      return RESULT_OK;
    }

  return RESULT_INIT;
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::DCData::MXFReader::FillWriterInfo(WriterInfo& Info) const
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
ASDCP::DCData::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_HeaderPart.Dump(stream);
}


//
void
ASDCP::DCData::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_IndexAccess.Dump(stream);
}

//
ASDCP::Result_t
ASDCP::DCData::MXFReader::Close() const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      m_Reader->Close();
      return RESULT_OK;
    }

  return RESULT_INIT;
}


//------------------------------------------------------------------------------------------


class ASDCP::DCData::MXFWriter::h__Writer : public ASDCP::h__ASDCPWriter
{
  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

public:
  DCDataDescriptor m_DDesc;
  byte_t           m_EssenceUL[SMPTE_UL_LENGTH];

  h__Writer(const Dictionary& d) : ASDCP::h__ASDCPWriter(d) {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  ~h__Writer(){}

  Result_t OpenWrite(const std::string&, ui32_t HeaderSize, const SubDescriptorList_t& subDescriptors);
  Result_t SetSourceStream(const DCDataDescriptor&, const byte_t*, const std::string&, const std::string&);
  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);
  Result_t Finalize();
  Result_t DCData_DDesc_to_MD(DCData::DCDataDescriptor& DDesc);
};


//
ASDCP::Result_t
ASDCP::DCData::MXFWriter::h__Writer::DCData_DDesc_to_MD(DCData::DCDataDescriptor& DDesc)
{
  ASDCP_TEST_NULL(m_EssenceDescriptor);
  MXF::DCDataDescriptor* DDescObj = static_cast<MXF::DCDataDescriptor *>(m_EssenceDescriptor);

  DDescObj->SampleRate = DDesc.EditRate;
  DDescObj->ContainerDuration = DDesc.ContainerDuration;
  DDescObj->DataEssenceCoding.Set(DDesc.DataEssenceCoding);

  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::DCData::MXFWriter::h__Writer::OpenWrite(const std::string& filename, ui32_t HeaderSize,
                                    const SubDescriptorList_t& subDescriptors)
{
  if ( ! m_State.Test_BEGIN() )
    return RESULT_STATE;

  Result_t result = m_File.OpenWrite(filename);

  if ( ASDCP_SUCCESS(result) )
    {
      m_HeaderSize = HeaderSize;
      m_EssenceDescriptor = new MXF::DCDataDescriptor(m_Dict);
      SubDescriptorList_t::const_iterator sDObj;
      SubDescriptorList_t::const_iterator lastDescriptor = subDescriptors.end();
      for (sDObj = subDescriptors.begin(); sDObj != lastDescriptor; ++sDObj)
      {
          m_EssenceSubDescriptorList.push_back(*sDObj);
          GenRandomValue((*sDObj)->InstanceUID);
          m_EssenceDescriptor->SubDescriptors.push_back((*sDObj)->InstanceUID);
      }
      result = m_State.Goto_INIT();
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::DCData::MXFWriter::h__Writer::SetSourceStream(DCDataDescriptor const& DDesc,
                                          const byte_t * essenceCoding,
                                          const std::string& packageLabel,
                                          const std::string& defLabel)
{
  if ( ! m_State.Test_INIT() )
    return RESULT_STATE;

  if ( DDesc.EditRate != EditRate_24
       && DDesc.EditRate != EditRate_25
       && DDesc.EditRate != EditRate_30
       && DDesc.EditRate != EditRate_48
       && DDesc.EditRate != EditRate_50
       && DDesc.EditRate != EditRate_60
       && DDesc.EditRate != EditRate_96
       && DDesc.EditRate != EditRate_100
       && DDesc.EditRate != EditRate_120
       && DDesc.EditRate != EditRate_192
       && DDesc.EditRate != EditRate_200
       && DDesc.EditRate != EditRate_240 )
  {
    DefaultLogSink().Error("DCDataDescriptor.EditRate is not a supported value: %d/%d\n",
                           DDesc.EditRate.Numerator, DDesc.EditRate.Denominator);
    return RESULT_RAW_FORMAT;
  }

  assert(m_Dict);
  m_DDesc = DDesc;
  if (NULL != essenceCoding)
      memcpy(m_DDesc.DataEssenceCoding, essenceCoding, SMPTE_UL_LENGTH);
  Result_t result = DCData_DDesc_to_MD(m_DDesc);

  if ( ASDCP_SUCCESS(result) )
  {
    memcpy(m_EssenceUL, m_Dict->ul(MDD_DCDataEssence), SMPTE_UL_LENGTH);
    m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container
    result = m_State.Goto_READY();
  }

  if ( ASDCP_SUCCESS(result) )
  {
    ui32_t TCFrameRate = m_DDesc.EditRate.Numerator;

    result = WriteASDCPHeader(packageLabel, UL(m_Dict->ul(MDD_DCDataWrappingFrame)),
			      defLabel, UL(m_EssenceUL), UL(m_Dict->ul(MDD_DataDataDef)),
			      m_DDesc.EditRate, TCFrameRate);
  }

  return result;
}

//
ASDCP::Result_t
ASDCP::DCData::MXFWriter::h__Writer::WriteFrame(const FrameBuffer& FrameBuf,
                                                ASDCP::AESEncContext* Ctx, ASDCP::HMACContext* HMAC)
{
  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() )
    result = m_State.Goto_RUNNING(); // first time through

  ui64_t StreamOffset = m_StreamOffset;

  if ( ASDCP_SUCCESS(result) )
    result = WriteEKLVPacket(FrameBuf, m_EssenceUL, Ctx, HMAC);

  if ( ASDCP_SUCCESS(result) )
  {
    IndexTableSegment::IndexEntry Entry;
    Entry.StreamOffset = StreamOffset;
    m_FooterPart.PushIndexEntry(Entry);
    m_FramesWritten++;
  }
  return result;
}

// Closes the MXF file, writing the index and other closing information.
//
ASDCP::Result_t
ASDCP::DCData::MXFWriter::h__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;

  m_State.Goto_FINAL();

  return WriteASDCPFooter();
}



//------------------------------------------------------------------------------------------

ASDCP::DCData::MXFWriter::MXFWriter()
{
}

ASDCP::DCData::MXFWriter::~MXFWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::DCData::MXFWriter::OP1aHeader()
{
  if ( m_Writer.empty() )
    {
      assert(g_OP1aHeader);
      return *g_OP1aHeader;
    }

  return m_Writer->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OPAtomIndexFooter&
ASDCP::DCData::MXFWriter::OPAtomIndexFooter()
{
  if ( m_Writer.empty() )
    {
      assert(g_OPAtomIndexFooter);
      return *g_OPAtomIndexFooter;
    }

  return m_Writer->m_FooterPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::RIP&
ASDCP::DCData::MXFWriter::RIP()
{
  if ( m_Writer.empty() )
    {
      assert(g_RIP);
      return *g_RIP;
    }

  return m_Writer->m_RIP;
}

// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
ASDCP::DCData::MXFWriter::OpenWrite(const std::string& filename, const WriterInfo& Info,
				       const DCDataDescriptor& DDesc, ui32_t HeaderSize)
{
  if ( Info.LabelSetType != LS_MXF_SMPTE )
  {
    DefaultLogSink().Error("DC Data support requires LS_MXF_SMPTE\n");
    return RESULT_FORMAT;
  }

  m_Writer = new h__Writer(DefaultSMPTEDict());
  m_Writer->m_Info = Info;

  Result_t result = m_Writer->OpenWrite(filename, HeaderSize, SubDescriptorList_t());

  if ( ASDCP_SUCCESS(result) )
      result = m_Writer->SetSourceStream(DDesc, NULL, DC_DATA_PACKAGE_LABEL, DC_DATA_DEF_LABEL);

  if ( ASDCP_FAILURE(result) )
    m_Writer.release();

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
ASDCP::Result_t
ASDCP::DCData::MXFWriter::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
ASDCP::Result_t
ASDCP::DCData::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}


//
// end AS_DCP_DCData.cpp
//
