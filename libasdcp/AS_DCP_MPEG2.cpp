/*
Copyright (c) 2004-2013, John Hurst
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
/*! \file    AS_DCP_MPEG2.cpp
    \version $Id: AS_DCP_MPEG2.cpp,v 1.44 2015/10/07 16:41:23 jhurst Exp $       
    \brief   AS-DCP library, MPEG2 essence reader and writer implementation
*/

#include "AS_DCP_internal.h"
#include <iostream>
#include <iomanip>


//------------------------------------------------------------------------------------------

static std::string MPEG_PACKAGE_LABEL = "File Package: SMPTE 381M frame wrapping of MPEG2 video elementary stream";
static std::string PICT_DEF_LABEL = "Picture Track";

//
ASDCP::Result_t
MD_to_MPEG2_VDesc(MXF::MPEG2VideoDescriptor* VDescObj, MPEG2::VideoDescriptor& VDesc)
{
  ASDCP_TEST_NULL(VDescObj);

  VDesc.SampleRate             = VDescObj->SampleRate;
  VDesc.EditRate               = VDescObj->SampleRate;
  VDesc.FrameRate              = VDescObj->SampleRate.Numerator;
  assert(VDescObj->ContainerDuration <= 0xFFFFFFFFL);
  VDesc.ContainerDuration      = (ui32_t) VDescObj->ContainerDuration;

  VDesc.FrameLayout            = VDescObj->FrameLayout;
  VDesc.StoredWidth            = VDescObj->StoredWidth;
  VDesc.StoredHeight           = VDescObj->StoredHeight;
  VDesc.AspectRatio            = VDescObj->AspectRatio;

  VDesc.ComponentDepth         = VDescObj->ComponentDepth;
  VDesc.HorizontalSubsampling  = VDescObj->HorizontalSubsampling;
  VDesc.VerticalSubsampling    = VDescObj->VerticalSubsampling;
  VDesc.ColorSiting            = VDescObj->ColorSiting;
  VDesc.CodedContentType       = VDescObj->CodedContentType;

  VDesc.LowDelay               = VDescObj->LowDelay.get() == 0 ? false : true;
  VDesc.BitRate                = VDescObj->BitRate;
  VDesc.ProfileAndLevel        = VDescObj->ProfileAndLevel;
  return RESULT_OK;
}


//
ASDCP::Result_t
MPEG2_VDesc_to_MD(MPEG2::VideoDescriptor& VDesc, MXF::MPEG2VideoDescriptor* VDescObj)
{
  ASDCP_TEST_NULL(VDescObj);

  VDescObj->SampleRate = VDesc.SampleRate;
  VDescObj->ContainerDuration = VDesc.ContainerDuration;

  VDescObj->FrameLayout = VDesc.FrameLayout;
  VDescObj->StoredWidth = VDesc.StoredWidth;
  VDescObj->StoredHeight = VDesc.StoredHeight;
  VDescObj->AspectRatio = VDesc.AspectRatio;

  VDescObj->ComponentDepth = VDesc.ComponentDepth;
  VDescObj->HorizontalSubsampling = VDesc.HorizontalSubsampling;
  VDescObj->VerticalSubsampling = VDesc.VerticalSubsampling;
  VDescObj->ColorSiting = VDesc.ColorSiting;
  VDescObj->CodedContentType = VDesc.CodedContentType;

  VDescObj->LowDelay = VDesc.LowDelay ? 1 : 0;
  VDescObj->BitRate = VDesc.BitRate;
  VDescObj->ProfileAndLevel = VDesc.ProfileAndLevel;
  return RESULT_OK;
}

//
std::ostream&
ASDCP::MPEG2::operator << (std::ostream& strm, const VideoDescriptor& VDesc)
{
  strm << "        SampleRate: " << VDesc.SampleRate.Numerator << "/" << VDesc.SampleRate.Denominator << std::endl;
  strm << "       FrameLayout: " << (unsigned) VDesc.FrameLayout << std::endl;
  strm << "       StoredWidth: " << (unsigned) VDesc.StoredWidth << std::endl;
  strm << "      StoredHeight: " << (unsigned) VDesc.StoredHeight << std::endl;
  strm << "       AspectRatio: " << VDesc.AspectRatio.Numerator << "/" << VDesc.AspectRatio.Denominator << std::endl;
  strm << "    ComponentDepth: " << (unsigned) VDesc.ComponentDepth << std::endl;
  strm << " HorizontalSubsmpl: " << (unsigned) VDesc.HorizontalSubsampling << std::endl;
  strm << "   VerticalSubsmpl: " << (unsigned) VDesc.VerticalSubsampling << std::endl;
  strm << "       ColorSiting: " << (unsigned) VDesc.ColorSiting << std::endl;
  strm << "  CodedContentType: " << (unsigned) VDesc.CodedContentType << std::endl;
  strm << "          LowDelay: " << (unsigned) VDesc.LowDelay << std::endl;
  strm << "           BitRate: " << (unsigned) VDesc.BitRate << std::endl;
  strm << "   ProfileAndLevel: " << (unsigned) VDesc.ProfileAndLevel << std::endl;
  strm << " ContainerDuration: " << (unsigned) VDesc.ContainerDuration << std::endl;

  return strm;
}

//
void
ASDCP::MPEG2::VideoDescriptorDump(const VideoDescriptor& VDesc, FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "\
        SampleRate: %d/%d\n\
       FrameLayout: %u\n\
       StoredWidth: %u\n\
      StoredHeight: %u\n\
       AspectRatio: %d/%d\n\
    ComponentDepth: %u\n\
 HorizontalSubsmpl: %u\n\
   VerticalSubsmpl: %u\n\
       ColorSiting: %u\n\
  CodedContentType: %u\n\
          LowDelay: %u\n\
           BitRate: %u\n\
   ProfileAndLevel: %u\n\
 ContainerDuration: %u\n",
	  VDesc.SampleRate.Numerator ,VDesc.SampleRate.Denominator,
	  VDesc.FrameLayout,
	  VDesc.StoredWidth,
	  VDesc.StoredHeight,
	  VDesc.AspectRatio.Numerator ,VDesc.AspectRatio.Denominator,
	  VDesc.ComponentDepth,
	  VDesc.HorizontalSubsampling,
	  VDesc.VerticalSubsampling,
	  VDesc.ColorSiting,
	  VDesc.CodedContentType,
	  VDesc.LowDelay,
	  VDesc.BitRate,
	  VDesc.ProfileAndLevel,
	  VDesc.ContainerDuration
	  );
}

//------------------------------------------------------------------------------------------
//
// hidden, internal implementation of MPEG2 reader

class ASDCP::MPEG2::MXFReader::h__Reader : public ASDCP::h__ASDCPReader
{
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);
  h__Reader();

public:
  VideoDescriptor m_VDesc;        // video parameter list

  h__Reader(const Dictionary& d) : ASDCP::h__ASDCPReader(d) {}
  virtual ~h__Reader() {}
  Result_t    OpenRead(const std::string&);
  Result_t    ReadFrame(ui32_t, FrameBuffer&, AESDecContext*, HMACContext*);
  Result_t    ReadFrameGOPStart(ui32_t, FrameBuffer&, AESDecContext*, HMACContext*);
  Result_t    FindFrameGOPStart(ui32_t, ui32_t&);
  Result_t    FrameType(ui32_t FrameNum, FrameType_t& type);
};


//
//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::h__Reader::OpenRead(const std::string& filename)
{
  Result_t result = OpenMXFRead(filename);

  if( ASDCP_SUCCESS(result) )
    {
      InterchangeObject* Object = 0;

      if ( ASDCP_SUCCESS(m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(MPEG2VideoDescriptor), &Object)) )
	{
	  if ( Object == 0 )
	    {
	      DefaultLogSink().Error("MPEG2VideoDescriptor object not found.\n");
	      return RESULT_FORMAT;
	    }

	  result = MD_to_MPEG2_VDesc((MXF::MPEG2VideoDescriptor*)Object, m_VDesc);
	}
    }

  return result;
}


//
//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::h__Reader::ReadFrameGOPStart(ui32_t FrameNum, FrameBuffer& FrameBuf,
						      AESDecContext* Ctx, HMACContext* HMAC)
{
  ui32_t KeyFrameNum;

  Result_t result = FindFrameGOPStart(FrameNum, KeyFrameNum);

  if ( ASDCP_SUCCESS(result) )
    result = ReadFrame(KeyFrameNum, FrameBuf, Ctx, HMAC);

  return result;
}


//
//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::h__Reader::FindFrameGOPStart(ui32_t FrameNum, ui32_t& KeyFrameNum)
{
  KeyFrameNum = 0;

  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  // look up frame index node
  IndexTableSegment::IndexEntry TmpEntry;

  if ( ASDCP_FAILURE(m_IndexAccess.Lookup(FrameNum, TmpEntry)) )
    {
      return RESULT_RANGE;
    }

  KeyFrameNum = FrameNum - TmpEntry.KeyFrameOffset;

  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::h__Reader::FrameType(ui32_t FrameNum, FrameType_t& type)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  // look up frame index node
  IndexTableSegment::IndexEntry TmpEntry;

  if ( ASDCP_FAILURE(m_IndexAccess.Lookup(FrameNum, TmpEntry)) )
    {
      return RESULT_RANGE;
    }

  type = ( (TmpEntry.Flags & 0x0f) == 3 ) ? FRAME_B : ( (TmpEntry.Flags & 0x0f) == 2 ) ? FRAME_P : FRAME_I;
  return RESULT_OK;
}


//
//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::h__Reader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
					      AESDecContext* Ctx, HMACContext* HMAC)
{
  assert(m_Dict);
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  Result_t result = ReadEKLVFrame(FrameNum, FrameBuf, m_Dict->ul(MDD_MPEG2Essence), Ctx, HMAC);

  if ( ASDCP_FAILURE(result) )
    return result;

  IndexTableSegment::IndexEntry TmpEntry;
  m_IndexAccess.Lookup(FrameNum, TmpEntry);

  switch ( ( TmpEntry.Flags >> 4 ) & 0x03 )
    {
    case 0:  FrameBuf.FrameType(FRAME_I); break;
    case 2:  FrameBuf.FrameType(FRAME_P); break;
    case 3:  FrameBuf.FrameType(FRAME_B); break;
    default: FrameBuf.FrameType(FRAME_U);
    }

  FrameBuf.TemporalOffset(TmpEntry.TemporalOffset);
  FrameBuf.GOPStart(TmpEntry.Flags & 0x40 ? true : false);
  FrameBuf.ClosedGOP(TmpEntry.Flags & 0x80 ? true : false);

  return RESULT_OK;
}

//------------------------------------------------------------------------------------------


//
void
ASDCP::MPEG2::FrameBuffer::Dump(FILE* stream, ui32_t dump_len) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "Frame: %06u, %c%-2hhu, %7u bytes",
	  m_FrameNumber, FrameTypeChar(m_FrameType), m_TemporalOffset, m_Size);

  if ( m_GOPStart )
    fprintf(stream, " (start %s GOP)", ( m_ClosedGOP ? "closed" : "open"));
  
  fputc('\n', stream);

  if ( dump_len > 0 )
    Kumu::hexdump(m_Data, dump_len, stream);
}


//------------------------------------------------------------------------------------------

ASDCP::MPEG2::MXFReader::MXFReader()
{
  m_Reader = new h__Reader(DefaultCompositeDict());
}


ASDCP::MPEG2::MXFReader::~MXFReader()
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    m_Reader->Close();
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::MPEG2::MXFReader::OP1aHeader()
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
ASDCP::MPEG2::MXFReader::OPAtomIndexFooter()
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
ASDCP::MPEG2::MXFReader::RIP()
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
ASDCP::MPEG2::MXFReader::OpenRead(const std::string& filename) const
{
  return m_Reader->OpenRead(filename);
}

//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
				   AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}


//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset, i8_t& temporalOffset, i8_t& keyFrameOffset) const
{
    return m_Reader->LocateFrame(FrameNum, streamOffset, temporalOffset, keyFrameOffset);
}

//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::ReadFrameGOPStart(ui32_t FrameNum, FrameBuffer& FrameBuf,
					   AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrameGOPStart(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}


//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::FindFrameGOPStart(ui32_t FrameNum, ui32_t& KeyFrameNum) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->FindFrameGOPStart(FrameNum, KeyFrameNum);

  return RESULT_INIT;
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::FillVideoDescriptor(VideoDescriptor& VDesc) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      VDesc = m_Reader->m_VDesc;
      return RESULT_OK;
    }

  return RESULT_INIT;
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::FillWriterInfo(WriterInfo& Info) const
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
ASDCP::MPEG2::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_HeaderPart.Dump(stream);
}


//
void
ASDCP::MPEG2::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_IndexAccess.Dump(stream);
}

//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::Close() const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      m_Reader->Close();
      return RESULT_OK;
    }

  return RESULT_INIT;
}

//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::FrameType(ui32_t FrameNum, FrameType_t& type) const
{
  if ( ! m_Reader )
    return RESULT_INIT;

  return m_Reader->FrameType(FrameNum, type);
}


//------------------------------------------------------------------------------------------

//
class ASDCP::MPEG2::MXFWriter::h__Writer : public ASDCP::h__ASDCPWriter
{
  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

public:
  VideoDescriptor m_VDesc;
  ui32_t          m_GOPOffset;
  byte_t          m_EssenceUL[SMPTE_UL_LENGTH];

  h__Writer(const Dictionary& d) : ASDCP::h__ASDCPWriter(d), m_GOPOffset(0) {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  virtual ~h__Writer(){}

  Result_t OpenWrite(const std::string&, ui32_t HeaderSize);
  Result_t SetSourceStream(const VideoDescriptor&);
  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);
  Result_t Finalize();
};


// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
ASDCP::MPEG2::MXFWriter::h__Writer::OpenWrite(const std::string& filename, ui32_t HeaderSize)
{
  if ( ! m_State.Test_BEGIN() )
    return RESULT_STATE;

  Result_t result = m_File.OpenWrite(filename);

  if ( ASDCP_SUCCESS(result) )
    {
      m_HeaderSize = HeaderSize;
      m_EssenceDescriptor = new MPEG2VideoDescriptor(m_Dict);
      result = m_State.Goto_INIT();
    }

  return result;
}

// Automatically sets the MXF file's metadata from the MPEG stream.
ASDCP::Result_t
ASDCP::MPEG2::MXFWriter::h__Writer::SetSourceStream(const VideoDescriptor& VDesc)
{
  assert(m_Dict);
  if ( ! m_State.Test_INIT() )
    return RESULT_STATE;

  m_VDesc = VDesc;
  Result_t result = MPEG2_VDesc_to_MD(m_VDesc, (MPEG2VideoDescriptor*)m_EssenceDescriptor);

  if ( ASDCP_SUCCESS(result) )
    {
      memcpy(m_EssenceUL, m_Dict->ul(MDD_MPEG2Essence), SMPTE_UL_LENGTH);
      m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container
      result = m_State.Goto_READY();
    }

  if ( ASDCP_SUCCESS(result) )
    {
      m_FooterPart.SetDeltaParams(IndexTableSegment::DeltaEntry(-1, 0, 0));

      result = WriteASDCPHeader(MPEG_PACKAGE_LABEL, UL(m_Dict->ul(MDD_MPEG2_VESWrappingFrame)), 
				PICT_DEF_LABEL, UL(m_EssenceUL), UL(m_Dict->ul(MDD_PictureDataDef)),
				m_VDesc.EditRate, derive_timecode_rate_from_edit_rate(m_VDesc.EditRate));
    }

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
//
ASDCP::Result_t
ASDCP::MPEG2::MXFWriter::h__Writer::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx,
					       HMACContext* HMAC)
{
  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() )
    result = m_State.Goto_RUNNING(); // first time through, get the body location

  IndexTableSegment::IndexEntry Entry;
  Entry.StreamOffset = m_StreamOffset;

  if ( ASDCP_SUCCESS(result) )
    result = WriteEKLVPacket(FrameBuf, m_EssenceUL, Ctx, HMAC);

  if ( ASDCP_FAILURE(result) )
    return result;

  // create mxflib flags
  int Flags = 0;

  switch ( FrameBuf.FrameType() )
    {
    case FRAME_I: Flags = 0x00; break;
    case FRAME_P: Flags = 0x22; break;
    case FRAME_B: Flags = 0x33; break;
    }

  if ( FrameBuf.GOPStart() )
    {
      m_GOPOffset = 0;
      Flags |= 0x40;

      if ( FrameBuf.ClosedGOP() )
	Flags |= 0x80;
    }

  // update the index manager
  Entry.TemporalOffset = - FrameBuf.TemporalOffset();
  Entry.KeyFrameOffset = 0 - m_GOPOffset;
  Entry.Flags = Flags;
  /*
  fprintf(stderr, "to: %4hd   ko: %4hd   c1: %4hd   c2: %4hd   fl: 0x%02x\n",
	  Entry.TemporalOffset, Entry.KeyFrameOffset,
	  m_GOPOffset + Entry.TemporalOffset,
	  Entry.KeyFrameOffset - Entry.TemporalOffset,
	  Entry.Flags);
  */
  m_FooterPart.PushIndexEntry(Entry);
  m_FramesWritten++;
  m_GOPOffset++;

  return RESULT_OK;
}


// Closes the MXF file, writing the index and other closing information.
//
ASDCP::Result_t
ASDCP::MPEG2::MXFWriter::h__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;

  m_State.Goto_FINAL();

  return WriteASDCPFooter();
}


//------------------------------------------------------------------------------------------



ASDCP::MPEG2::MXFWriter::MXFWriter()
{
}

ASDCP::MPEG2::MXFWriter::~MXFWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::MPEG2::MXFWriter::OP1aHeader()
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
ASDCP::MPEG2::MXFWriter::OPAtomIndexFooter()
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
ASDCP::MPEG2::MXFWriter::RIP()
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
ASDCP::MPEG2::MXFWriter::OpenWrite(const std::string& filename, const WriterInfo& Info,
				   const VideoDescriptor& VDesc, ui32_t HeaderSize)
{
  if ( Info.LabelSetType == LS_MXF_SMPTE )
    m_Writer = new h__Writer(DefaultSMPTEDict());
  else
    m_Writer = new h__Writer(DefaultInteropDict());

  m_Writer->m_Info = Info;
  
  Result_t result = m_Writer->OpenWrite(filename, HeaderSize);

  if ( ASDCP_SUCCESS(result) )
    result = m_Writer->SetSourceStream(VDesc);

  if ( ASDCP_FAILURE(result) )
    m_Writer.release();

  return result;
}


// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
ASDCP::Result_t
ASDCP::MPEG2::MXFWriter::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
ASDCP::Result_t
ASDCP::MPEG2::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}


//
// end AS_DCP_MPEG2.cpp
//
