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
/*! \file    JP2K_Sequence_Parser.cpp
    \version $Id: JP2K_Sequence_Parser.cpp,v 1.8 2011/05/13 01:50:19 jhurst Exp $
    \brief   AS-DCP library, JPEG 2000 codestream essence reader implementation
*/

#include <AS_DCP.h>
#include <KM_fileio.h>
#include <KM_log.h>
#include <list>
#include <string>
#include <algorithm>
#include <string.h>
#include <assert.h>

using namespace ASDCP;


//------------------------------------------------------------------------------------------
  
class FileList : public std::list<std::string>
{
  std::string m_DirName;

public:
  FileList() {}
  ~FileList() {}

  const FileList& operator=(const std::list<std::string>& pathlist) {
    std::list<std::string>::const_iterator i;
    for ( i = pathlist.begin(); i != pathlist.end(); i++ )
      push_back(*i);
    return *this;
  }

  //
  Result_t InitFromDirectory(const char* path)
  {
    char next_file[Kumu::MaxFilePath];
    Kumu::DirScanner Scanner;

    Result_t result = Scanner.Open(path);

    if ( ASDCP_SUCCESS(result) )
      {
	m_DirName = path;

	while ( ASDCP_SUCCESS(Scanner.GetNext(next_file)) )
	  {
	    if ( next_file[0] == '.' ) // no hidden files or internal links
	      continue;

	    std::string Str(m_DirName);
	    Str += "/";
	    Str += next_file;

	    if ( ! Kumu::PathIsDirectory(Str) )
	      push_back(Str);
	  }

	sort();
      }

    return result;
  }
};

//------------------------------------------------------------------------------------------

class ASDCP::JP2K::SequenceParser::h__SequenceParser
{
  ui32_t             m_FramesRead;
  Rational           m_PictureRate;
  FileList           m_FileList;
  FileList::iterator m_CurrentFile;
  CodestreamParser   m_Parser;
  bool               m_Pedantic;

  Result_t OpenRead();

  ASDCP_NO_COPY_CONSTRUCT(h__SequenceParser);

public:
  PictureDescriptor  m_PDesc;

  h__SequenceParser() : m_FramesRead(0), m_Pedantic(false)
  {
    memset(&m_PDesc, 0, sizeof(m_PDesc));
    m_PDesc.EditRate = Rational(24,1); 
  }

  ~h__SequenceParser()
  {
    Close();
  }

  Result_t OpenRead(const char* filename, bool pedantic);
  Result_t OpenRead(const std::list<std::string>& file_list, bool pedantic);
  void     Close() {}

  Result_t Reset()
  {
    m_FramesRead = 0;
    m_CurrentFile = m_FileList.begin();
    return RESULT_OK;
  }

  Result_t ReadFrame(FrameBuffer&);
};


//
ASDCP::Result_t
ASDCP::JP2K::SequenceParser::h__SequenceParser::OpenRead()
{
  if ( m_FileList.empty() )
    return RESULT_ENDOFFILE;

  m_CurrentFile = m_FileList.begin();
  CodestreamParser Parser;
  FrameBuffer TmpBuffer;

  Kumu::fsize_t file_size = Kumu::FileSize((*m_CurrentFile).c_str());

  if ( file_size == 0 )
    return RESULT_NOT_FOUND;

  assert(file_size <= 0xFFFFFFFFL);
  Result_t result = TmpBuffer.Capacity((ui32_t) file_size);

  if ( ASDCP_SUCCESS(result) )
    result = Parser.OpenReadFrame((*m_CurrentFile).c_str(), TmpBuffer);
      
  if ( ASDCP_SUCCESS(result) )
    result = Parser.FillPictureDescriptor(m_PDesc);

  // how big is it?
  if ( ASDCP_SUCCESS(result) )
    m_PDesc.ContainerDuration = m_FileList.size();

  return result;
}

//
ASDCP::Result_t
ASDCP::JP2K::SequenceParser::h__SequenceParser::OpenRead(const char* filename, bool pedantic)
{
  ASDCP_TEST_NULL_STR(filename);
  m_Pedantic = pedantic;

  Result_t result = m_FileList.InitFromDirectory(filename);

  if ( ASDCP_SUCCESS(result) )
    result = OpenRead();

  return result;
}


//
ASDCP::Result_t
ASDCP::JP2K::SequenceParser::h__SequenceParser::OpenRead(const std::list<std::string>& file_list, bool pedantic)
{
  m_Pedantic = pedantic;
  m_FileList = file_list;
  return OpenRead();
}


//
bool
operator==(const ASDCP::JP2K::ImageComponent_t& lhs, const ASDCP::JP2K::ImageComponent_t& rhs)
{
  if ( lhs.Ssize != rhs.Ssize ) return false;
  if ( lhs.XRsize != rhs.XRsize ) return false;
  if ( lhs.YRsize != rhs.YRsize ) return false;
  return true;
}

//
bool
operator==(const ASDCP::JP2K::QuantizationDefault_t& lhs, const ASDCP::JP2K::QuantizationDefault_t& rhs)
{
  if ( lhs.Sqcd != rhs.Sqcd ) return false;
  if ( lhs.SPqcdLength != rhs.SPqcdLength ) return false;
  
  for ( ui32_t i = 0; i < JP2K::MaxDefaults; i++ )
    {
      if ( lhs.SPqcd[i] != rhs.SPqcd[i]  )
	return false;
    }

  return true;
}

//
bool
operator==(const ASDCP::JP2K::CodingStyleDefault_t& lhs, const ASDCP::JP2K::CodingStyleDefault_t& rhs)
{
  if ( lhs.Scod != rhs.Scod ) return false;

  // SGcod
  if ( lhs.SGcod.ProgressionOrder != rhs.SGcod.ProgressionOrder ) return false;
  if ( lhs.SGcod.MultiCompTransform != rhs.SGcod.MultiCompTransform ) return false;

  for ( ui32_t i = 0; i < sizeof(ui16_t); i++ )
    {
      if ( lhs.SGcod.NumberOfLayers[i] != lhs.SGcod.NumberOfLayers[i]  )
	return false;
    }

  // SPcod
  if ( lhs.SPcod.DecompositionLevels != rhs.SPcod.DecompositionLevels ) return false;
  if ( lhs.SPcod.CodeblockWidth != rhs.SPcod.CodeblockWidth ) return false;
  if ( lhs.SPcod.CodeblockHeight != rhs.SPcod.CodeblockHeight ) return false;
  if ( lhs.SPcod.CodeblockStyle != rhs.SPcod.CodeblockStyle ) return false;
  if ( lhs.SPcod.Transformation != rhs.SPcod.Transformation ) return false;
  
  for ( ui32_t i = 0; i < JP2K::MaxPrecincts; i++ )
    {
      if ( lhs.SPcod.PrecinctSize[i] != rhs.SPcod.PrecinctSize[i]  )
	return false;
    }

  return true;
}

//
bool
operator==(const ASDCP::JP2K::PictureDescriptor& lhs, const ASDCP::JP2K::PictureDescriptor& rhs)
{
  if ( lhs.EditRate != rhs.EditRate ) return false;
  //  if ( lhs.ContainerDuration != rhs.ContainerDuration ) return false;
  if ( lhs.SampleRate != rhs.SampleRate ) return false;
  if ( lhs.StoredWidth != rhs.StoredWidth ) return false;
  if ( lhs.StoredHeight != rhs.StoredHeight ) return false;
  if ( lhs.AspectRatio != rhs.AspectRatio ) return false;
  if ( lhs.Rsize != rhs.Rsize ) return false;
  if ( lhs.Xsize != rhs.Xsize ) return false;
  if ( lhs.Ysize != rhs.Ysize ) return false;
  if ( lhs.XOsize != rhs.XOsize ) return false;
  if ( lhs.YOsize != rhs.YOsize ) return false;
  if ( lhs.XTsize != rhs.XTsize ) return false;
  if ( lhs.YTsize != rhs.YTsize ) return false;
  if ( lhs.XTOsize != rhs.XTOsize ) return false;
  if ( lhs.YTOsize != rhs.YTOsize ) return false;
  if ( lhs.Csize != rhs.Csize ) return false;
  if ( ! ( lhs.CodingStyleDefault == rhs.CodingStyleDefault ) ) return false;
  if ( ! ( lhs.QuantizationDefault == rhs.QuantizationDefault ) ) return false;
  
  for ( ui32_t i = 0; i < JP2K::MaxComponents; i++ )
    {
      if ( ! ( lhs.ImageComponents[i] == rhs.ImageComponents[i] ) )
	return false;
    }

  return true;
}

//
ASDCP::Result_t
ASDCP::JP2K::SequenceParser::h__SequenceParser::ReadFrame(FrameBuffer& FB)
{
  if ( m_CurrentFile == m_FileList.end() )
    return RESULT_ENDOFFILE;

  // open the file
  Result_t result = m_Parser.OpenReadFrame((*m_CurrentFile).c_str(), FB);

  if ( ASDCP_SUCCESS(result) && m_Pedantic )
    {
      PictureDescriptor PDesc;
      result = m_Parser.FillPictureDescriptor(PDesc);

      if ( ASDCP_SUCCESS(result) && ! ( m_PDesc == PDesc ) )
	{
	  Kumu::DefaultLogSink().Error("JPEG-2000 codestream parameters do not match at frame %d\n", m_FramesRead + 1);
	  result = RESULT_RAW_FORMAT;
	}
    }

  if ( ASDCP_SUCCESS(result) )
    {
      FB.FrameNumber(m_FramesRead++);
      m_CurrentFile++;
    }

  return result;
}


//------------------------------------------------------------------------------------------

ASDCP::JP2K::SequenceParser::SequenceParser()
{
}

ASDCP::JP2K::SequenceParser::~SequenceParser()
{
}

// Opens the stream for reading, parses enough data to provide a complete
// set of stream metadata for the MXFWriter below.
ASDCP::Result_t
ASDCP::JP2K::SequenceParser::OpenRead(const char* filename, bool pedantic) const
{
  const_cast<ASDCP::JP2K::SequenceParser*>(this)->m_Parser = new h__SequenceParser;

  Result_t result = m_Parser->OpenRead(filename, pedantic);

  if ( ASDCP_FAILURE(result) )
    const_cast<ASDCP::JP2K::SequenceParser*>(this)->m_Parser.release();

  return result;
}

//
Result_t
ASDCP::JP2K::SequenceParser::OpenRead(const std::list<std::string>& file_list, bool pedantic) const
{
  const_cast<ASDCP::JP2K::SequenceParser*>(this)->m_Parser = new h__SequenceParser;

  Result_t result = m_Parser->OpenRead(file_list, pedantic);

  if ( ASDCP_FAILURE(result) )
    const_cast<ASDCP::JP2K::SequenceParser*>(this)->m_Parser.release();

  return result;
}


// Rewinds the stream to the beginning.
ASDCP::Result_t
ASDCP::JP2K::SequenceParser::Reset() const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  return m_Parser->Reset();
}

// Places a frame of data in the frame buffer. Fails if the buffer is too small
// or the stream is empty.
ASDCP::Result_t
ASDCP::JP2K::SequenceParser::ReadFrame(FrameBuffer& FB) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  return m_Parser->ReadFrame(FB);
}

//
ASDCP::Result_t
ASDCP::JP2K::SequenceParser::FillPictureDescriptor(PictureDescriptor& PDesc) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  PDesc = m_Parser->m_PDesc;
  return RESULT_OK;
}


//
// end JP2K_Sequence_Parser.cpp
//
