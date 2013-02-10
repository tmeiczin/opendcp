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
/*! \file    PCM_Parser.cpp
    \version $Id: PCM_Parser.cpp,v 1.7 2011/05/13 01:50:19 jhurst Exp $
    \brief   AS-DCP library, PCM raw essence reader implementation
*/

#include <Wav.h>
#include <assert.h>
#include <KM_log.h>
using Kumu::DefaultLogSink;

using namespace ASDCP;
using namespace ASDCP::PCM;
using namespace ASDCP::Wav;


//------------------------------------------------------------------------------------------

//
class ASDCP::PCM::WAVParser::h__WAVParser
{
  Kumu::FileReader m_FileReader;
  bool             m_EOF;
  ui32_t           m_DataStart;
  ui32_t           m_DataLength;
  ui32_t           m_ReadCount;
  ui32_t           m_FrameBufferSize;
  ui32_t           m_FramesRead;
  Rational         m_PictureRate;

  ASDCP_NO_COPY_CONSTRUCT(h__WAVParser);

public:
  AudioDescriptor  m_ADesc;


  h__WAVParser() :
    m_EOF(false), m_DataStart(0), m_DataLength(0), m_ReadCount(0),
    m_FrameBufferSize(0), m_FramesRead(0) {}

  ~h__WAVParser()
  {
    Close();
   }

  Result_t OpenRead(const char* filename, const Rational& PictureRate);
  void     Close();
  void     Reset();
  Result_t ReadFrame(FrameBuffer&);
};


//
void
ASDCP::PCM::WAVParser::h__WAVParser::Close()
{
  m_FileReader.Close();
}

//
void
ASDCP::PCM::WAVParser::h__WAVParser::Reset()
{
  m_FileReader.Seek(m_DataStart);
  m_FramesRead = 0;
  m_ReadCount = 0;
}

//
ASDCP::Result_t
ASDCP::PCM::WAVParser::h__WAVParser::OpenRead(const char* filename, const Rational& PictureRate)
{
  ASDCP_TEST_NULL_STR(filename);

  Result_t result = m_FileReader.OpenRead(filename);

  if ( ASDCP_SUCCESS(result) )
    {
      SimpleWaveHeader WavHeader;
      result = WavHeader.ReadFromFile(m_FileReader, &m_DataStart);
  
      if ( ASDCP_SUCCESS(result) )
	{
	  WavHeader.FillADesc(m_ADesc, PictureRate);
	  m_FrameBufferSize = ASDCP::PCM::CalcFrameBufferSize(m_ADesc);
	  m_DataLength = WavHeader.data_len;
	  m_ADesc.ContainerDuration = m_DataLength / m_FrameBufferSize;
	  m_ADesc.ChannelFormat = PCM::CF_NONE;
	  Reset();
	}
      else
	{
	  ASDCP::AIFF::SimpleAIFFHeader AIFFHeader;
	  m_FileReader.Seek(0);

	  result = AIFFHeader.ReadFromFile(m_FileReader, &m_DataStart);

	  if ( ASDCP_SUCCESS(result) )
	    {
	      AIFFHeader.FillADesc(m_ADesc, PictureRate);
	      m_FrameBufferSize = ASDCP::PCM::CalcFrameBufferSize(m_ADesc);
	      m_DataLength = AIFFHeader.data_len;
	      m_ADesc.ContainerDuration = m_DataLength / m_FrameBufferSize;
	      m_ADesc.ChannelFormat = PCM::CF_NONE;
	      Reset();
	    }
	}
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::PCM::WAVParser::h__WAVParser::ReadFrame(FrameBuffer& FB)
{
  FB.Size(0);

  if ( m_EOF || m_ReadCount >= m_DataLength )
    return RESULT_ENDOFFILE;

  if ( FB.Capacity() < m_FrameBufferSize )
    {
      DefaultLogSink().Error("FrameBuf.Capacity: %u FrameLength: %u\n",
			     FB.Capacity(), m_FrameBufferSize);
      return RESULT_SMALLBUF;
    }

  ui32_t read_count = 0;
  Result_t result = m_FileReader.Read(FB.Data(), m_FrameBufferSize, &read_count);

  if ( result == RESULT_ENDOFFILE )
    {
      m_EOF = true;

      if ( read_count > 0 )
	result = RESULT_OK;
    }

  if ( ASDCP_SUCCESS(result) )
    {
      m_ReadCount += read_count;
      FB.Size(read_count);
      FB.FrameNumber(m_FramesRead++);
    }

  return result;
}


//------------------------------------------------------------------------------------------

ASDCP::PCM::WAVParser::WAVParser()
{
}

ASDCP::PCM::WAVParser::~WAVParser()
{
}

// Opens the stream for reading, parses enough data to provide a complete
// set of stream metadata for the MXFWriter below.
ASDCP::Result_t
ASDCP::PCM::WAVParser::OpenRead(const char* filename, const Rational& PictureRate) const
{
  const_cast<ASDCP::PCM::WAVParser*>(this)->m_Parser = new h__WAVParser;

  Result_t result = m_Parser->OpenRead(filename, PictureRate);

  if ( ASDCP_FAILURE(result) )
    const_cast<ASDCP::PCM::WAVParser*>(this)->m_Parser.release();

  return result;
}

// Rewinds the stream to the beginning.
ASDCP::Result_t
ASDCP::PCM::WAVParser::Reset() const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  m_Parser->Reset();
  return RESULT_OK;
}

// Places a frame of data in the frame buffer. Fails if the buffer is too small
// or the stream is empty.
ASDCP::Result_t
ASDCP::PCM::WAVParser::ReadFrame(FrameBuffer& FB) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  return m_Parser->ReadFrame(FB);
}

ASDCP::Result_t
ASDCP::PCM::WAVParser::FillAudioDescriptor(AudioDescriptor& ADesc) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  ADesc = m_Parser->m_ADesc;
  return RESULT_OK;
}


//
// end PCM_Parser.cpp
//
