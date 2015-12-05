/*
Copyright (c) 2004-2011, John Hurst
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
/*! \file    MPEG2_Parser.cpp
    \version $Id: MPEG2_Parser.cpp,v 1.12 2014/01/02 23:29:22 jhurst Exp $
    \brief   AS-DCP library, MPEG2 raw essence reader implementation
*/

#include <KM_fileio.h>
#include <MPEG.h>

#include <KM_log.h>
using Kumu::DefaultLogSink;

using namespace ASDCP;
using namespace ASDCP::MPEG2;

// data will be read from a VES file in chunks of this size
const ui32_t VESReadSize = 4 * Kumu::Kilobyte;


//------------------------------------------------------------------------------------------

//
enum ParserState_t {
    ST_INIT,
    ST_SEQ,
    ST_PIC,
    ST_GOP,
    ST_EXT,
    ST_SLICE,
};

const char*
StringParserState(ParserState_t state)
{
  switch ( state )
    {
    case ST_INIT:  return "INIT";
    case ST_SEQ:   return "SEQ";
    case ST_PIC:   return "PIC";
    case ST_GOP:   return "GOP";
    case ST_EXT:   return "EXT";
    case ST_SLICE: return "SLICE";
    }

  return "*UNKNOWN*";
}



//
class h__ParserState
{
  ParserState_t m_State;
  ASDCP_NO_COPY_CONSTRUCT(h__ParserState);

 public:
  h__ParserState() : m_State(ST_INIT) {}
  ~h__ParserState() {}

  inline bool Test_SLICE() { return m_State == ST_SLICE; }
  inline void Reset() { m_State = ST_INIT; }

  //
  inline Result_t Goto_SEQ()
    {
      switch ( m_State )
	{
	case ST_INIT:
	case ST_EXT:
	  m_State = ST_SEQ;
	  return RESULT_OK;
	}
      
      DefaultLogSink().Error("SEQ follows %s\n", StringParserState(m_State));
      return RESULT_STATE;
    }


  //
  inline Result_t Goto_SLICE()
    {
      switch ( m_State )
	{
	case ST_PIC:
	case ST_EXT:
	  m_State = ST_SLICE;
	  return RESULT_OK;
	}
      
      DefaultLogSink().Error("Slice follows %s\n", StringParserState(m_State));
      return RESULT_STATE;
    }


  //
  inline Result_t Goto_PIC()
    {
      switch ( m_State )
	{
	case ST_INIT:
	case ST_SEQ:
	case ST_GOP:
	case ST_EXT:
	  m_State = ST_PIC;
	  return RESULT_OK;
	}
      
      DefaultLogSink().Error("PIC follows %s\n", StringParserState(m_State));
      return RESULT_STATE;
    }


  //
  inline Result_t Goto_GOP()
  {
    switch ( m_State )
      {
      case ST_EXT:
      case ST_SEQ:
	m_State = ST_GOP;
	return RESULT_OK;
      }
    
    DefaultLogSink().Error("GOP follows %s\n", StringParserState(m_State));
    return RESULT_STATE;
  }

  //
  inline Result_t Goto_EXT()
  {
    switch ( m_State )
      {
	case ST_PIC:
	case ST_EXT:
	case ST_SEQ:
	case ST_GOP:
	  m_State = ST_EXT;
	  return RESULT_OK;
      }

    DefaultLogSink().Error("EXT follows %s\n", StringParserState(m_State));
    return RESULT_STATE;
  }
};

//------------------------------------------------------------------------------------------

// This is a parser delagate that reads the stream params from a
// sequence header and sequence extension header. The parser is
// commanded to return after processing the sequence extension
// header.
class StreamParams : public VESParserDelegate
{
  h__ParserState m_State;

  ASDCP_NO_COPY_CONSTRUCT(StreamParams);

public:
  VideoDescriptor  m_VDesc;

  StreamParams()
  {
    m_VDesc.ContainerDuration = 0;
    m_VDesc.ComponentDepth = 8;
  }

  ~StreamParams() {}

  //
  Result_t Sequence(VESParser*, const byte_t* b, ui32_t s)
  {
    Result_t result = m_State.Goto_SEQ();

    if ( ASDCP_FAILURE(result) )
      return result;

    Accessor::Sequence SEQ(b);
    m_VDesc.AspectRatio = SEQ.AspectRatio();
    m_VDesc.FrameRate = SEQ.FrameRate();
    m_VDesc.StoredWidth = SEQ.HorizontalSize();
    m_VDesc.StoredHeight = SEQ.VerticalSize();
    m_VDesc.BitRate = SEQ.BitRate();
    m_VDesc.EditRate = SEQ.Pulldown() ? Rational(SEQ.FrameRate() * 1000, 1001) : Rational(SEQ.FrameRate(), 1);
    m_VDesc.SampleRate = m_VDesc.EditRate;
    return RESULT_OK;
  }

  //
  Result_t Extension(VESParser*, const byte_t* b, ui32_t s)
  {
    Result_t result = m_State.Goto_EXT();

    if ( ASDCP_FAILURE(result) )
      return result;

    Accessor::SequenceEx SEQX(b);
    m_VDesc.ProfileAndLevel = SEQX.ProfileAndLevel();
    m_VDesc.FrameLayout = SEQX.Progressive() ? 0 : 1;
    m_VDesc.CodedContentType = SEQX.Progressive() ? 1 : 2;
    m_VDesc.LowDelay = SEQX.LowDelay();
    m_VDesc.HorizontalSubsampling = SEQX.ChromaFormat() == 3 ? 1 : 2;
    m_VDesc.VerticalSubsampling = SEQX.ChromaFormat() >= 3 ? 1 : 2;

    if ( ( m_VDesc.HorizontalSubsampling == 2 ) && ( m_VDesc.VerticalSubsampling == 2 ) )
      m_VDesc.ColorSiting = 3;  // 4:2:0

    else if ( ( m_VDesc.HorizontalSubsampling == 2 ) && ( m_VDesc.VerticalSubsampling == 1 ) )
      m_VDesc.ColorSiting = 4;  // 4:2:2

    else if ( ( m_VDesc.HorizontalSubsampling == 1 ) && ( m_VDesc.VerticalSubsampling == 1 ) )
      m_VDesc.ColorSiting = 0;  // 4:4:4

    // TODO: get H&V size and bit rate extensions

    return RESULT_FALSE;
  }

  Result_t GOP(VESParser*, const byte_t*, ui32_t)     { return RESULT_FALSE; }
  Result_t Picture(VESParser*, const byte_t*, ui32_t) { return RESULT_FALSE; }
  Result_t Slice(VESParser*, byte_t)                  { return RESULT_FALSE; }
  Result_t Data(VESParser*, const byte_t*, i32_t)     { return RESULT_OK; }
};


//------------------------------------------------------------------------------------------

// This is a parser delagate that reads a VES stream and sets public
// instance variables to indicate state. It is used below to read an
// entire frame into a buffer. The delegate retains metadata about the
// frame for later access.
//
class FrameParser : public VESParserDelegate
{
  h__ParserState             m_State;
  ASDCP_NO_COPY_CONSTRUCT(FrameParser);

public:
  ui32_t       m_FrameSize;
  bool         m_CompletePicture;
  bool         m_HasGOP;
  bool         m_ClosedGOP;
  ui8_t        m_TemporalRef;
  ui32_t       m_PlaintextOffset;
  FrameType_t  m_FrameType;

  FrameParser()
  {
    Reset();
  }

  ~FrameParser() {}
  
  void Reset()
  {
    m_FrameSize = 0;
    m_HasGOP = m_ClosedGOP = false;
    m_CompletePicture = false;
    m_TemporalRef = 0;
    m_PlaintextOffset = 0;
    m_FrameType = FRAME_U;
    m_State.Reset();
 }

  Result_t Sequence(VESParser*, const byte_t* b, ui32_t s)
  {
    if ( m_State.Test_SLICE() )
      {
	m_CompletePicture = true;
	return RESULT_FALSE;
      }

    m_FrameSize += s;
    return m_State.Goto_SEQ();
  }

  Result_t Picture(VESParser*, const byte_t* b, ui32_t s)
  {
    if ( m_State.Test_SLICE() )
      {
	m_CompletePicture = true;
	return RESULT_FALSE;
      }

    Accessor::Picture pic(b);
    m_TemporalRef = pic.TemporalRef();
    m_FrameType = pic.FrameType();
    m_FrameSize += s;
    return m_State.Goto_PIC();
  }

  Result_t Slice(VESParser*, byte_t slice_id)
  {
    if ( slice_id == FIRST_SLICE )
      {
	m_PlaintextOffset = m_FrameSize;
	return m_State.Goto_SLICE();
      }

    return m_State.Test_SLICE() ? RESULT_OK : RESULT_FAIL;
  }

  Result_t Extension(VESParser*, const byte_t* b, ui32_t s)
  {
    m_FrameSize += s;
    return m_State.Goto_EXT();
  }

  Result_t GOP(VESParser*, const byte_t* b, ui32_t s)
  {
    Accessor::GOP GOP(b);
    m_ClosedGOP = GOP.Closed();
    m_HasGOP = true;
    m_FrameSize += s;
    return m_State.Goto_GOP();
  }

  Result_t Data(VESParser*, const byte_t* b, i32_t s)
  {
    m_FrameSize += s;
    return RESULT_OK;
  }
};

//------------------------------------------------------------------------------------------

// The following code assumes the following things:
// - each frame will begin with a picture header or a sequence header
// - any frame that begins with a sequence header is an I frame and is
//   assumed to contain a GOP header, a picture header and complete picture data
// - any frame that begins with a picture header is either an I, B or P frame
//   and is assumed to contain a complete picture header and picture data

class ASDCP::MPEG2::Parser::h__Parser
{
  StreamParams     m_ParamsDelegate;
  FrameParser      m_ParserDelegate;
  VESParser        m_Parser;
  Kumu::FileReader m_FileReader;
  ui32_t           m_FrameNumber;
  bool             m_EOF;
  ASDCP::MPEG2::FrameBuffer  m_TmpBuffer;

  ASDCP_NO_COPY_CONSTRUCT(h__Parser);

public:
  h__Parser() : m_TmpBuffer(VESReadSize*8) {}
  ~h__Parser() { Close(); }

  Result_t OpenRead(const std::string& filename);
  void     Close();
  Result_t Reset();
  Result_t ReadFrame(FrameBuffer&);
  Result_t FillVideoDescriptor(VideoDescriptor&);
};


//
Result_t
ASDCP::MPEG2::Parser::h__Parser::Reset()
{
  m_FrameNumber = 0;
  m_EOF = false;
  m_FileReader.Seek(0);
  m_ParserDelegate.Reset();
  return RESULT_OK;
}

//
void
ASDCP::MPEG2::Parser::h__Parser::Close()
{
  m_FileReader.Close();
}

//
ASDCP::Result_t
ASDCP::MPEG2::Parser::h__Parser::OpenRead(const std::string& filename)
{
  ui32_t read_count = 0;

  Result_t result = m_FileReader.OpenRead(filename);
  
  if ( ASDCP_SUCCESS(result) )
    result = m_FileReader.Read(m_TmpBuffer.Data(), m_TmpBuffer.Capacity(), &read_count);
  
  if ( ASDCP_SUCCESS(result) )
    {
      const byte_t* p = m_TmpBuffer.RoData();

      // the mxflib parser demanded the file start with a sequence header.
      // Since no one complained and that's the easiest thing to implement,
      // I have left it that way. Let me know if you want to be able to
      // locate the first GOP in the stream.
      ui32_t i = 0;
      while ( p[i] == 0 ) i++;

      if ( i < 2 || p[i] != 1 || ! ( p[i+1] == SEQ_START || p[i+1] == PIC_START ) )
	{
	  DefaultLogSink().Error("Frame buffer does not begin with a PIC or SEQ start code.\n");
	  return RESULT_RAW_FORMAT;
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  m_Parser.SetDelegate(&m_ParamsDelegate);
	  result = m_Parser.Parse(p, read_count);
	}
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui64_t tmp = m_FileReader.Size() / 65536; // a gross approximation
      m_ParamsDelegate.m_VDesc.ContainerDuration = (ui32_t) tmp;
      m_Parser.SetDelegate(&m_ParserDelegate);
      m_FileReader.Seek(0);
    }

  if ( ASDCP_FAILURE(result) )
    {
      DefaultLogSink().Error("Unable to identify a wrapping mode for the essence in file \"%s\"\n", filename.c_str());
      m_FileReader.Close();
    }

  return result;}


//
//
ASDCP::Result_t
ASDCP::MPEG2::Parser::h__Parser::ReadFrame(FrameBuffer& FB)
{
  Result_t result = RESULT_OK;
  ui32_t write_offset = 0;
  ui32_t read_count = 0;

  FB.Size(0);

  if ( m_EOF )
    return RESULT_ENDOFFILE;

  // Data is read in VESReadSize chunks. Each chunk is parsed, and the
  // process is stopped when a Sequence or Picture header is found or when
  // the input file is exhausted. The partial next frame is cached for the
  // next call.
  m_ParserDelegate.Reset();
  m_Parser.Reset();

  if ( m_TmpBuffer.Size() > 0 )
    {
      memcpy(FB.Data(), m_TmpBuffer.RoData(), m_TmpBuffer.Size());
      result = m_Parser.Parse(FB.RoData(), m_TmpBuffer.Size());
      write_offset = m_TmpBuffer.Size();
      m_TmpBuffer.Size(0);
    }

  while ( ! m_ParserDelegate.m_CompletePicture && result == RESULT_OK )
    {
      if ( FB.Capacity() < ( write_offset + VESReadSize ) )
	{
	  DefaultLogSink().Error("FrameBuf.Capacity: %u FrameLength: %u\n",
				 FB.Capacity(), ( write_offset + VESReadSize ));
	  return RESULT_SMALLBUF;
	}

      result = m_FileReader.Read(FB.Data() + write_offset, VESReadSize, &read_count);

      if ( result == RESULT_ENDOFFILE || read_count == 0 )
	{
	  m_EOF = true;

	  if ( write_offset > 0 )
	    result = RESULT_OK;
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  result = m_Parser.Parse(FB.RoData() + write_offset, read_count);
	  write_offset += read_count;
	}

      if ( m_EOF )
	break;
    }
  assert(m_ParserDelegate.m_FrameSize <= write_offset);

  if ( ASDCP_SUCCESS(result)
       && m_ParserDelegate.m_FrameSize < write_offset )
    {
      assert(m_TmpBuffer.Size() == 0);
      ui32_t diff = write_offset - m_ParserDelegate.m_FrameSize;
      assert(diff <= m_TmpBuffer.Capacity());

      memcpy(m_TmpBuffer.Data(), FB.RoData() + m_ParserDelegate.m_FrameSize, diff);
      m_TmpBuffer.Size(diff);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      const byte_t* p = FB.RoData();
      if ( p[0] != 0 || p[1] != 0 || p[2] != 1 || ! ( p[3] == SEQ_START || p[3] == PIC_START ) )
        {
          DefaultLogSink().Error("Frame buffer does not begin with a PIC or SEQ start code.\n");
          return RESULT_RAW_FORMAT;
        }
    }

  if ( ASDCP_SUCCESS(result) )
    {
      FB.Size(m_ParserDelegate.m_FrameSize);
      FB.TemporalOffset(m_ParserDelegate.m_TemporalRef);
      FB.FrameType(m_ParserDelegate.m_FrameType);
      FB.PlaintextOffset(m_ParserDelegate.m_PlaintextOffset);
      FB.FrameNumber(m_FrameNumber++);
      FB.GOPStart(m_ParserDelegate.m_HasGOP);
      FB.ClosedGOP(m_ParserDelegate.m_ClosedGOP);
    }

  return result;
}


// Fill a VideoDescriptor struct with the values from the file's header.
ASDCP::Result_t
ASDCP::MPEG2::Parser::h__Parser::FillVideoDescriptor(VideoDescriptor& VDesc)
{
  VDesc = m_ParamsDelegate.m_VDesc;
  return RESULT_OK;
}

//------------------------------------------------------------------------------------------

ASDCP::MPEG2::Parser::Parser()
{
}

ASDCP::MPEG2::Parser::~Parser()
{
}

// Opens the stream for reading, parses enough data to provide a complete
// set of stream metadata for the MXFWriter below.
ASDCP::Result_t
ASDCP::MPEG2::Parser::OpenRead(const std::string& filename) const
{
  const_cast<ASDCP::MPEG2::Parser*>(this)->m_Parser = new h__Parser;

  Result_t result = m_Parser->OpenRead(filename);

  if ( ASDCP_FAILURE(result) )
    const_cast<ASDCP::MPEG2::Parser*>(this)->m_Parser.release();

  return result;
}

// Rewinds the stream to the beginning.
ASDCP::Result_t
ASDCP::MPEG2::Parser::Reset() const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  return m_Parser->Reset();
}

// Places a frame of data in the frame buffer. Fails if the buffer is too small
// or the stream is empty.
ASDCP::Result_t
ASDCP::MPEG2::Parser::ReadFrame(FrameBuffer& FB) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  return m_Parser->ReadFrame(FB);
}

ASDCP::Result_t
ASDCP::MPEG2::Parser::FillVideoDescriptor(VideoDescriptor& VDesc) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  return m_Parser->FillVideoDescriptor(VDesc);
}

//
// end MPEG2_Parser.cpp
//
