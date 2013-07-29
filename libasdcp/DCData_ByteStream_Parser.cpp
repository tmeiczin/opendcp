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
    \version $Id: DCData_ByteStream_Parser.cpp,v 1.1 2013/04/12 23:39:30 mikey Exp $
    \brief   AS-DCP library, Digital Cinema Data bytestream essence reader
*/

#include "AS_DCP.h"
#include "KM_fileio.h"
#include "KM_log.h"
using Kumu::DefaultLogSink;

//------------------------------------------------------------------------------------------

class ASDCP::DCData::BytestreamParser::h__BytestreamParser
{
  ASDCP_NO_COPY_CONSTRUCT(h__BytestreamParser);

public:
  DCDataDescriptor  m_DDesc;
  Kumu::FileReader   m_File;

  h__BytestreamParser()
  {
    memset(&m_DDesc, 0, sizeof(m_DDesc));
    m_DDesc.EditRate = Rational(24,1);
  }

  ~h__BytestreamParser() {}

  Result_t OpenReadFrame(const char* filename, FrameBuffer& FB)
  {
    ASDCP_TEST_NULL_STR(filename);
    m_File.Close();
    Result_t result = m_File.OpenRead(filename);

    if ( ASDCP_SUCCESS(result) )
    {
        Kumu::fsize_t file_size = m_File.Size();

        if ( FB.Capacity() < file_size )
        {
            DefaultLogSink().Error("FrameBuf.Capacity: %u frame length: %u\n", FB.Capacity(), (ui32_t)file_size);
            return RESULT_SMALLBUF;
        }
    }

    ui32_t read_count;

    if ( ASDCP_SUCCESS(result) )
        result = m_File.Read(FB.Data(), FB.Capacity(), &read_count);

    if ( ASDCP_SUCCESS(result) )
        FB.Size(read_count);

    return result;
  }
};


//------------------------------------------------------------------------------------------

ASDCP::DCData::BytestreamParser::BytestreamParser()
{
}

ASDCP::DCData::BytestreamParser::~BytestreamParser()
{
}

// Opens the stream for reading, parses enough data to provide a complete
// set of stream metadata for the MXFWriter below.
ASDCP::Result_t
ASDCP::DCData::BytestreamParser::OpenReadFrame(const char* filename, FrameBuffer& FB) const
{
  const_cast<ASDCP::DCData::BytestreamParser*>(this)->m_Parser = new h__BytestreamParser;
  return m_Parser->OpenReadFrame(filename, FB);
}

//
ASDCP::Result_t
ASDCP::DCData::BytestreamParser::FillDCDataDescriptor(DCDataDescriptor& DDesc) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  DDesc = m_Parser->m_DDesc;
  return RESULT_OK;
}


//
// end DCData_Bytestream_Parser.cpp
//



