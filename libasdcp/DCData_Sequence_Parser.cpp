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
    \version $Id: DCData_Sequence_Parser.cpp,v 1.2 2014/01/02 23:29:22 jhurst Exp $
    \brief   AS-DCP library, DCinema data seqence reader implementation
*/

#include "AS_DCP.h"

#include <algorithm>
#include <list>
#include <string>

#include "KM_fileio.h"
#include "KM_log.h"

using ASDCP::Result_t;

//------------------------------------------------------------------------------------------
namespace ASDCP
{
namespace DCData
{
class FileList;
} // namespace DCData
} // namespace ASDCP


class ASDCP::DCData::FileList : public std::list<std::string>
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
    Result_t InitFromDirectory(const std::string& path)
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

class ASDCP::DCData::SequenceParser::h__SequenceParser
{
  ui32_t             m_FramesRead;
  Rational           m_PictureRate;
  FileList           m_FileList;
  FileList::iterator m_CurrentFile;
  BytestreamParser   m_Parser;

  Result_t OpenRead();

  ASDCP_NO_COPY_CONSTRUCT(h__SequenceParser);

 public:
  DCDataDescriptor  m_DDesc;

  h__SequenceParser() : m_FramesRead(0)
  {
    memset(&m_DDesc, 0, sizeof(m_DDesc));
    m_DDesc.EditRate = Rational(24,1);
  }

  ~h__SequenceParser()
  {
    Close();
  }

  Result_t OpenRead(const std::string& filename);
  Result_t OpenRead(const std::list<std::string>& file_list);
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
ASDCP::DCData::SequenceParser::h__SequenceParser::OpenRead()
{
  if ( m_FileList.empty() )
    return RESULT_ENDOFFILE;

  m_CurrentFile = m_FileList.begin();
  BytestreamParser Parser;
  FrameBuffer TmpBuffer;

  Kumu::fsize_t file_size = Kumu::FileSize((*m_CurrentFile).c_str());

  if ( file_size == 0 )
    return RESULT_NOT_FOUND;

  assert(file_size <= 0xFFFFFFFFL);
  Result_t result = TmpBuffer.Capacity((ui32_t) file_size);

  if ( ASDCP_SUCCESS(result) )
    result = Parser.OpenReadFrame((*m_CurrentFile).c_str(), TmpBuffer);

  if ( ASDCP_SUCCESS(result) )
    result = Parser.FillDCDataDescriptor(m_DDesc);

  // how big is it?
  if ( ASDCP_SUCCESS(result) )
    m_DDesc.ContainerDuration = m_FileList.size();

  return result;
}

//
ASDCP::Result_t
ASDCP::DCData::SequenceParser::h__SequenceParser::OpenRead(const std::string& filename)
{
  Result_t result = m_FileList.InitFromDirectory(filename);

  if ( ASDCP_SUCCESS(result) )
    result = OpenRead();

  return result;
}


//
ASDCP::Result_t
ASDCP::DCData::SequenceParser::h__SequenceParser::OpenRead(const std::list<std::string>& file_list)
{
  m_FileList = file_list;
  return OpenRead();
}

//
ASDCP::Result_t
ASDCP::DCData::SequenceParser::h__SequenceParser::ReadFrame(FrameBuffer& FB)
{
  if ( m_CurrentFile == m_FileList.end() )
    return RESULT_ENDOFFILE;

  // open the file
  Result_t result = m_Parser.OpenReadFrame((*m_CurrentFile).c_str(), FB);

  if ( ASDCP_SUCCESS(result) )
  {
      FB.FrameNumber(m_FramesRead++);
      m_CurrentFile++;
  }

  return result;
}


//------------------------------------------------------------------------------------------

ASDCP::DCData::SequenceParser::SequenceParser()
{
}

ASDCP::DCData::SequenceParser::~SequenceParser()
{
}

// Opens the stream for reading, parses enough data to provide a complete
// set of stream metadata for the MXFWriter below.
ASDCP::Result_t
ASDCP::DCData::SequenceParser::OpenRead(const std::string& filename) const
{
  const_cast<ASDCP::DCData::SequenceParser*>(this)->m_Parser = new h__SequenceParser;

  Result_t result = m_Parser->OpenRead(filename);

  if ( ASDCP_FAILURE(result) )
    const_cast<ASDCP::DCData::SequenceParser*>(this)->m_Parser.release();

  return result;
}

//
Result_t
ASDCP::DCData::SequenceParser::OpenRead(const std::list<std::string>& file_list) const
{
  const_cast<ASDCP::DCData::SequenceParser*>(this)->m_Parser = new h__SequenceParser;

  Result_t result = m_Parser->OpenRead(file_list);

  if ( ASDCP_FAILURE(result) )
    const_cast<ASDCP::DCData::SequenceParser*>(this)->m_Parser.release();

  return result;
}


// Rewinds the stream to the beginning.
ASDCP::Result_t
ASDCP::DCData::SequenceParser::Reset() const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  return m_Parser->Reset();
}

// Places a frame of data in the frame buffer. Fails if the buffer is too small
// or the stream is empty.
ASDCP::Result_t
ASDCP::DCData::SequenceParser::ReadFrame(FrameBuffer& FB) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  return m_Parser->ReadFrame(FB);
}

//
ASDCP::Result_t
ASDCP::DCData::SequenceParser::FillDCDataDescriptor(DCDataDescriptor& DDesc) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  DDesc = m_Parser->m_DDesc;
  return RESULT_OK;
}


//
// end DCData_Sequence_Parser.cpp
//

