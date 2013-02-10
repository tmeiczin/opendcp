/*
Copyright (c) 2005-2009, John Hurst
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
/*! \file    WavFileWriter.h
    \version $Id: WavFileWriter.h,v 1.6 2009/04/09 19:16:49 msheby Exp $
    \brief   demux and write PCM data to WAV file(s)
*/

#include <KM_fileio.h>
#include <KM_log.h>
#include <Wav.h>
#include <list>

#ifndef _WAVFILEWRITER_H_
#define _WAVFILEWRITER_H_


//
class WavFileElement : public Kumu::FileWriter
{
  ASDCP::PCM::FrameBuffer m_Buf;
  byte_t* m_p;

  WavFileElement();
  KM_NO_COPY_CONSTRUCT(WavFileElement);

public:
  WavFileElement(ui32_t s) : m_Buf(s), m_p(0)
  {
    m_p = m_Buf.Data();
  }

  ~WavFileElement() {}

  void WriteSample(const byte_t* sample, ui32_t sample_size)
  {
    memcpy(m_p, sample, sample_size);
    m_p += sample_size;
  }

  ASDCP::Result_t Flush()
  {
    ui32_t write_count = 0;

    if ( m_p == m_Buf.Data() )
      return ASDCP::RESULT_EMPTY_FB;

    ui32_t write_size = m_p - m_Buf.Data();
    m_p = m_Buf.Data();
    return Write(m_Buf.RoData(), write_size, &write_count);
  }
};


//
class WavFileWriter
{
  ASDCP::PCM::AudioDescriptor m_ADesc;
  std::list<WavFileElement*>  m_OutFile;
  ui32_t                      m_ChannelCount;
  ASDCP_NO_COPY_CONSTRUCT(WavFileWriter);

 public:
  WavFileWriter() : m_ChannelCount(0) {}
  ~WavFileWriter()
    {
      while ( ! m_OutFile.empty() )
	{
	  delete m_OutFile.back();
	  m_OutFile.pop_back();
	}
    }

  //
  enum SplitType_t {
    ST_NONE,   // write all channels to a single WAV file
    ST_MONO,   // write each channel a separate WAV file
    ST_STEREO  // write channel pairs to separate WAV files
  };

  ASDCP::Result_t
    OpenWrite(ASDCP::PCM::AudioDescriptor &ADesc, const char* file_root, SplitType_t split = ST_NONE)
    {
      ASDCP_TEST_NULL_STR(file_root);
      char filename[Kumu::MaxFilePath];
      ui32_t file_count = 0;
      ASDCP::Result_t result = ASDCP::RESULT_OK;
      m_ADesc = ADesc;

      switch ( split )
	{
	case ST_NONE:
	  file_count = 1;
	  m_ChannelCount = m_ADesc.ChannelCount;
	  break;

	case ST_MONO:
	  file_count = m_ADesc.ChannelCount;
	  m_ChannelCount = 1;
	  break;

	case ST_STEREO:
	  if ( m_ADesc.ChannelCount % 2 != 0 )
	    {
	      Kumu::DefaultLogSink().Error("Unable to create 2-channel splits with odd number of input channels.\n");
	      return ASDCP::RESULT_PARAM;
	    }

	  file_count = m_ADesc.ChannelCount / 2;
	  m_ChannelCount = 2;
	  break;
	}
      assert(file_count && m_ChannelCount);

      ui32_t element_size = ASDCP::PCM::CalcFrameBufferSize(m_ADesc) / file_count;

      for ( ui32_t i = 0; i < file_count && ASDCP_SUCCESS(result); i++ )
	{
	  snprintf(filename, Kumu::MaxFilePath, "%s_%u.wav", file_root, (i + 1));
	  m_OutFile.push_back(new WavFileElement(element_size));
	  result = m_OutFile.back()->OpenWrite(filename);

	  if ( ASDCP_SUCCESS(result) )
	    {
	      ASDCP::PCM::AudioDescriptor tmpDesc = m_ADesc;
	      tmpDesc.ChannelCount = m_ChannelCount;
	      ASDCP::Wav::SimpleWaveHeader Wav(tmpDesc);
	      result = Wav.WriteToFile(*(m_OutFile.back()));
	    }
	}
      
      return result;
    }

  ASDCP::Result_t
    WriteFrame(ASDCP::PCM::FrameBuffer& FB)
    {
      if ( m_OutFile.empty() )
	return ASDCP::RESULT_STATE;

      if ( m_OutFile.size() == 1 ) // no de-interleave needed, just write out the frame
	return m_OutFile.back()->Write(FB.RoData(), FB.Size(), 0);
 
      std::list<WavFileElement*>::iterator fi;
      ui32_t sample_size = m_ADesc.QuantizationBits / 8;
      const byte_t* p = FB.RoData();
      const byte_t* end_p = p + FB.Size();

      while ( p < end_p )
	{
	  for ( fi = m_OutFile.begin(); fi != m_OutFile.end(); fi++ )
	    {
	      for ( ui32_t c = 0; c < m_ChannelCount; c++ )
		{
		  (*fi)->WriteSample(p, sample_size);
		  p += sample_size;
		}
	    }
	}

      ASDCP::Result_t result = ASDCP::RESULT_OK;

      for ( fi = m_OutFile.begin(); fi != m_OutFile.end() && ASDCP_SUCCESS(result); fi++ )
	result = (*fi)->Flush();

      return result;
    }
};


#endif // _WAVFILEWRITER_H_

//
// end WavFileWriter.h
//
