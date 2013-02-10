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
/*! \file    Wav.h
    \version $Id: Wav.h,v 1.5 2009/04/09 19:24:14 msheby Exp $
    \brief   Wave file common elements
*/

#ifndef _WAV_H_
#define _WAV_H_

#include <KM_fileio.h>
#include <AS_DCP.h>

namespace ASDCP
{
  //
  class fourcc
    {
    private:
      byte_t data[4];

    public:
      inline fourcc() { memset( data, 0, 4 ); }
      inline fourcc( const char* v )   { memcpy( this->data, v, 4 ); }
      inline fourcc( const byte_t* v ) { memcpy( this->data, v, 4 ); }
      inline fourcc& operator=(const fourcc & s) { memcpy( this->data, s.data, 4); return *this; }
      inline bool operator==(const fourcc &rhs)  { return memcmp(data, rhs.data, 4) == 0 ? true : false; }
      inline bool operator!=(const fourcc &rhs)  { return memcmp(data, rhs.data, 4) != 0 ? true : false; }
    };

  namespace AIFF
    {
      const fourcc FCC_FORM("FORM");
      const fourcc FCC_AIFF("AIFF");
      const fourcc FCC_COMM("COMM");
      const fourcc FCC_SSND("SSND");

      class SimpleAIFFHeader
	{
	public:
	  ui16_t  numChannels;
	  ui32_t  numSampleFrames;
	  ui16_t  sampleSize;
	  byte_t  sampleRate[10]; // 80-bit IEEE 754 float
	  ui32_t  data_len;

	  SimpleAIFFHeader() :
	    numChannels(0), numSampleFrames(0), sampleSize(0), data_len(0) {
	    memset(sampleRate, 0, 10);
	  }
      	  
	  Result_t  ReadFromBuffer(const byte_t* buf, ui32_t buf_len, ui32_t* data_start);
	  Result_t  ReadFromFile(const Kumu::FileReader& InFile, ui32_t* data_start);
	  void      FillADesc(ASDCP::PCM::AudioDescriptor& ADesc, Rational PictureRate) const;
	};

    } // namespace AIFF

  namespace Wav
    {
      const ui32_t MaxWavHeader = 1024*32; // must find "data" within this space or no happy

      const fourcc FCC_RIFF("RIFF");
      const fourcc FCC_WAVE("WAVE");
      const fourcc FCC_fmt_("fmt ");
      const fourcc FCC_data("data");

      const ui16_t WAVE_FORMAT_PCM = 1;
      const ui16_t WAVE_FORMAT_EXTENSIBLE = 65534;

      //
      class SimpleWaveHeader
	{
	public:
	  ui16_t	format;
	  ui16_t	nchannels;
	  ui32_t	samplespersec;
	  ui32_t	avgbps;
	  ui16_t	blockalign;
	  ui16_t	bitspersample;
	  ui16_t	cbsize;
	  ui32_t	data_len;

	  SimpleWaveHeader() :
	    format(0), nchannels(0), samplespersec(0), avgbps(0),
	    blockalign(0), bitspersample(0), cbsize(0), data_len(0) {}
      
	  SimpleWaveHeader(ASDCP::PCM::AudioDescriptor& ADesc);
	  
	  Result_t  ReadFromBuffer(const byte_t* buf, ui32_t buf_len, ui32_t* data_start);
	  Result_t  ReadFromFile(const Kumu::FileReader& InFile, ui32_t* data_start);
	  Result_t  WriteToFile(Kumu::FileWriter& OutFile) const;
	  void      FillADesc(ASDCP::PCM::AudioDescriptor& ADesc, Rational PictureRate) const;
	};

    } // namespace Wav
} // namespace ASDCP

#endif // _WAV_H_

//
// end Wav.h
//
