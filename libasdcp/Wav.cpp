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
/*! \file    Wav.cpp
    \version $Id: Wav.cpp,v 1.12.2.1 2013/07/10 15:46:18 mikey Exp $
    \brief   Wave file common elements
*/

#include "Wav.h"
#include <assert.h>
#include <KM_log.h>
using Kumu::DefaultLogSink;


const ui32_t SimpleWavHeaderLength = 46;

//
ASDCP::Wav::SimpleWaveHeader::SimpleWaveHeader(ASDCP::PCM::AudioDescriptor& ADesc)
{
  format = 1;
  nchannels = ADesc.ChannelCount;
  bitspersample = ADesc.QuantizationBits;
  samplespersec = (ui32_t)ceil(ADesc.AudioSamplingRate.Quotient());
  blockalign = nchannels * ((bitspersample + 7) / 8);
  avgbps = samplespersec * blockalign;
  cbsize = 0;	  
  data_len = ASDCP::PCM::CalcFrameBufferSize(ADesc) * ADesc.ContainerDuration;
}

//
void
ASDCP::Wav::SimpleWaveHeader::FillADesc(ASDCP::PCM::AudioDescriptor& ADesc, ASDCP::Rational PictureRate) const
{
  ADesc.EditRate = PictureRate;

  ADesc.LinkedTrackID = 0;
  ADesc.Locked = 0;
  ADesc.ChannelCount = nchannels;
  ADesc.AudioSamplingRate = Rational(samplespersec, 1);
  ADesc.AvgBps = avgbps;
  ADesc.BlockAlign = blockalign;
  ADesc.QuantizationBits = bitspersample;
  ui32_t FrameBufferSize = ASDCP::PCM::CalcFrameBufferSize(ADesc);
  ADesc.ContainerDuration = data_len / FrameBufferSize;
  ADesc.ChannelFormat = PCM::CF_NONE;
}


//
ASDCP::Result_t
ASDCP::Wav::SimpleWaveHeader::WriteToFile(Kumu::FileWriter& OutFile) const
{
  ui32_t write_count;
  byte_t tmp_header[SimpleWavHeaderLength];
  byte_t* p = tmp_header;

  static ui32_t fmt_len =
    sizeof(format)
    + sizeof(nchannels)
    + sizeof(samplespersec)
    + sizeof(avgbps)
    + sizeof(blockalign)
    + sizeof(bitspersample)
    + sizeof(cbsize);

  ui32_t RIFF_len = data_len + SimpleWavHeaderLength - 8;

  memcpy(p, &FCC_RIFF, sizeof(fourcc)); p += 4;
  *((ui32_t*)p) = KM_i32_LE(RIFF_len); p += 4;
  memcpy(p, &FCC_WAVE, sizeof(fourcc)); p += 4;
  memcpy(p, &FCC_fmt_, sizeof(fourcc)); p += 4;
  *((ui32_t*)p) = KM_i32_LE(fmt_len); p += 4;
  *((ui16_t*)p) = KM_i16_LE(format); p += 2;
  *((ui16_t*)p) = KM_i16_LE(nchannels); p += 2;
  *((ui32_t*)p) = KM_i32_LE(samplespersec); p += 4;
  *((ui32_t*)p) = KM_i32_LE(avgbps); p += 4;
  *((ui16_t*)p) = KM_i16_LE(blockalign); p += 2;
  *((ui16_t*)p) = KM_i16_LE(bitspersample); p += 2;
  *((ui16_t*)p) = KM_i16_LE(cbsize); p += 2;
  memcpy(p, &FCC_data, sizeof(fourcc)); p += 4;
  *((ui32_t*)p) = KM_i32_LE(data_len); p += 4;

  return OutFile.Write(tmp_header, SimpleWavHeaderLength, &write_count);
}

//
ASDCP::Result_t
ASDCP::Wav::SimpleWaveHeader::ReadFromFile(const Kumu::FileReader& InFile, ui32_t* data_start)
{
  ui32_t read_count = 0;
  ui32_t local_data_start = 0;
  ASDCP::PCM::FrameBuffer TmpBuffer(MaxWavHeader);

  if ( data_start == 0 )
    data_start = &local_data_start;

  Result_t result = InFile.Read(TmpBuffer.Data(), TmpBuffer.Capacity(), &read_count);

  if ( ASDCP_SUCCESS(result) )
    result = ReadFromBuffer(TmpBuffer.RoData(), read_count, data_start);

    return result;
}

ASDCP::Result_t
ASDCP::Wav::SimpleWaveHeader::ReadFromBuffer(const byte_t* buf, ui32_t buf_len, ui32_t* data_start)
{
  if ( buf_len < SimpleWavHeaderLength )
    return RESULT_SMALLBUF;

  *data_start = 0;
  const byte_t* p = buf;
  const byte_t* end_p = p + buf_len;

  fourcc test_RIFF(p); p += 4;
  if ( test_RIFF != FCC_RIFF )
    {
      //      DefaultLogSink().Debug("File does not begin with RIFF header\n");      
      return RESULT_RAW_FORMAT;
    }

  ui32_t RIFF_len = KM_i32_LE(*(ui32_t*)p); p += 4;

  fourcc test_WAVE(p); p += 4;
  if ( test_WAVE != FCC_WAVE )
    {
      DefaultLogSink().Debug("File does not contain a WAVE header\n");
      return RESULT_RAW_FORMAT;
    }

  fourcc test_fcc;

  while ( p < end_p )
    {
      test_fcc = fourcc(p); p += 4;
      ui32_t chunk_size = KM_i32_LE(*(ui32_t*)p); p += 4;

      if ( test_fcc == FCC_data )
	{
	  if ( chunk_size > RIFF_len )
	    {
	      DefaultLogSink().Error("Chunk size %u larger than file: %u\n", chunk_size, RIFF_len);
	      return RESULT_RAW_FORMAT;
	    }

	  data_len = chunk_size;
	  *data_start = p - buf;
	  break;
	}

      if ( test_fcc == FCC_fmt_ )
	{
	  ui16_t format = KM_i16_LE(*(ui16_t*)p); p += 2;

	  if ( format != WAVE_FORMAT_PCM && format != WAVE_FORMAT_EXTENSIBLE )
	    {
	      DefaultLogSink().Error("Expecting uncompressed PCM data, got format type %hd\n", format);
	      return RESULT_RAW_FORMAT;
	    }

	  nchannels = KM_i16_LE(*(ui16_t*)p); p += 2;
	  samplespersec = KM_i32_LE(*(ui32_t*)p); p += 4;
	  avgbps = KM_i32_LE(*(ui32_t*)p); p += 4;
	  blockalign = KM_i16_LE(*(ui16_t*)p); p += 2;
	  bitspersample = KM_i16_LE(*(ui16_t*)p); p += 2;
	  p += chunk_size - 16; // 16 is the number of bytes read in this block
	}
      else
	{
	  p += chunk_size;
	}
    }

  if ( *data_start == 0 ) // can't have no data!
    {
      DefaultLogSink().Error("No data chunk found, file contains no essence\n");
      return RESULT_RAW_FORMAT;
    }

  return RESULT_OK;
}

//------------------------------------------------------------------------------------------
// conversion algorithms from http://www.borg.com/~jglatt/tech/aiff.htm

//
void
Rat_to_extended(ASDCP::Rational rate, byte_t* buf)
{
  memset(buf, 0, 10);
  ui32_t value = (ui32_t)ceil(rate.Quotient()); 
  ui32_t exp = value;
  exp >>= 1;
  ui8_t i = 0;

  for ( ; i < 32; i++ )
    {
      exp >>= 1;
      if ( ! exp )
	break;
    }

  *(buf+1) = i;

   for ( i = 32; i != 0 ; i-- )
     {
       if ( value & 0x80000000 )
	 break;
       value <<= 1;
     }

   *(ui32_t*)(buf+2) = KM_i32_BE(value);
}

//
ASDCP::Rational
extended_to_Rat(const byte_t* buf)
{
  ui32_t last = 0;
  ui32_t mantissa = KM_i32_BE(*(ui32_t*)(buf+2));

  byte_t exp = 30 - *(buf+1);

  while ( exp-- )
    {
      last = mantissa;
      mantissa >>= 1;
    }

  if ( last & 0x00000001 )
    mantissa++;

  return ASDCP::Rational(mantissa, 1);
}

//
void
ASDCP::AIFF::SimpleAIFFHeader::FillADesc(ASDCP::PCM::AudioDescriptor& ADesc, ASDCP::Rational PictureRate) const
{
  ADesc.EditRate = PictureRate;

  ADesc.ChannelCount = numChannels;
  ADesc.AudioSamplingRate = extended_to_Rat(sampleRate);
  ADesc.QuantizationBits = sampleSize;
  ADesc.BlockAlign = sampleSize / 8;
  ADesc.AvgBps = (ui32_t) (ADesc.BlockAlign * ADesc.AudioSamplingRate.Quotient());
  ui32_t FrameBufferSize = ASDCP::PCM::CalcFrameBufferSize(ADesc);
  ADesc.ContainerDuration = data_len / FrameBufferSize;
  ADesc.ChannelFormat = PCM::CF_NONE;
}

//
ASDCP::Result_t
ASDCP::AIFF::SimpleAIFFHeader::ReadFromFile(const Kumu::FileReader& InFile, ui32_t* data_start)
{
  ui32_t read_count = 0;
  ui32_t local_data_start = 0;
  ASDCP::PCM::FrameBuffer TmpBuffer(Wav::MaxWavHeader);

  if ( data_start == 0 )
    data_start = &local_data_start;

  Result_t result = InFile.Read(TmpBuffer.Data(), TmpBuffer.Capacity(), &read_count);

  if ( ASDCP_SUCCESS(result) )
    result = ReadFromBuffer(TmpBuffer.RoData(), read_count, data_start);

    return result;
}

//
ASDCP::Result_t
ASDCP::AIFF::SimpleAIFFHeader::ReadFromBuffer(const byte_t* buf, ui32_t buf_len, ui32_t* data_start)
{
  if ( buf_len < 32 )
    return RESULT_SMALLBUF;

  *data_start = 0;
  const byte_t* p = buf;
  const byte_t* end_p = p + buf_len;

  fourcc test_FORM(p); p += 4;
  if ( test_FORM != FCC_FORM )
    {
      //      DefaultLogSink().Debug("File does not begin with FORM header\n");
      return RESULT_RAW_FORMAT;
    }

  ui32_t RIFF_len = KM_i32_BE(*(ui32_t*)p); p += 4;

  fourcc test_AIFF(p); p += 4;
  if ( test_AIFF != FCC_AIFF )
    {
      DefaultLogSink().Debug("File does not contain an AIFF header\n");
      return RESULT_RAW_FORMAT;
    }

  fourcc test_fcc;

  while ( p < end_p )
    {
      test_fcc = fourcc(p); p += 4;
      ui32_t chunk_size = KM_i32_BE(*(ui32_t*)p); p += 4;

      if ( test_fcc == FCC_COMM )
	{
	  numChannels = KM_i16_BE(*(ui16_t*)p); p += 2;
	  numSampleFrames = KM_i32_BE(*(ui32_t*)p); p += 4;
	  sampleSize = KM_i16_BE(*(ui16_t*)p); p += 2;
	  memcpy(sampleRate, p, 10);
	  p += 10;
	}
      else if ( test_fcc == FCC_SSND )
	{
	  if ( chunk_size > RIFF_len )
            {
              DefaultLogSink().Error("Chunk size %u larger than file: %u\n", chunk_size, RIFF_len);
              return RESULT_RAW_FORMAT;
            }

	  ui32_t offset = KM_i32_BE(*(ui32_t*)p); p += 4;
	  p += 4; // blockSize;

	  data_len = chunk_size - 8;
	  *data_start = (p - buf) + offset;
	  break;
	}
      else
	{
	  p += chunk_size;
	}
    }

  if ( *data_start == 0 ) // can't have no data!
    {
      DefaultLogSink().Error("No data chunk found, file contains no essence\n");
      return RESULT_RAW_FORMAT;
    }

  return RESULT_OK;
}

ASDCP::RF64::SimpleRF64Header::SimpleRF64Header(ASDCP::PCM::AudioDescriptor& ADesc)
{
  format = 1;
  nchannels = ADesc.ChannelCount;
  bitspersample = ADesc.QuantizationBits;
  samplespersec = (ui32_t)ceil(ADesc.AudioSamplingRate.Quotient());
  blockalign = nchannels * ((bitspersample + 7) / 8);
  avgbps = samplespersec * blockalign;
  cbsize = 0;
  data_len = static_cast<ui64_t>(ASDCP::PCM::CalcFrameBufferSize(ADesc)) * ADesc.ContainerDuration;
}

//
void
ASDCP::RF64::SimpleRF64Header::FillADesc(ASDCP::PCM::AudioDescriptor& ADesc, ASDCP::Rational PictureRate) const
{
  ADesc.EditRate = PictureRate;

  ADesc.LinkedTrackID = 0;
  ADesc.Locked = 0;
  ADesc.ChannelCount = nchannels;
  ADesc.AudioSamplingRate = Rational(samplespersec, 1);
  ADesc.AvgBps = avgbps;
  ADesc.BlockAlign = blockalign;
  ADesc.QuantizationBits = bitspersample;
  ui32_t FrameBufferSize = ASDCP::PCM::CalcFrameBufferSize(ADesc);
  ADesc.ContainerDuration = data_len / FrameBufferSize;
  ADesc.ChannelFormat = PCM::CF_NONE;
}

//
ASDCP::Result_t
ASDCP::RF64::SimpleRF64Header::WriteToFile(Kumu::FileWriter& OutFile) const
{
  static ui32_t fmt_len =
    sizeof(format)
    + sizeof(nchannels)
    + sizeof(samplespersec)
    + sizeof(avgbps)
    + sizeof(blockalign)
    + sizeof(bitspersample)
    + sizeof(cbsize);

  ui32_t write_count = 0;
  ui64_t RIFF_len = data_len + SimpleWavHeaderLength - 8;
  DefaultLogSink().Debug("RIFF_len is %llu.\n", RIFF_len);
  byte_t* tmp_header = NULL;
  ui32_t header_len = 0;

  if (RIFF_len > MAX_RIFF_LEN)
  {
    DefaultLogSink().Debug("Will write out an RF64 wave file.\n");
    ui32_t data32_len = ((data_len < MAX_RIFF_LEN) ? data_len : MAX_RIFF_LEN);
    ui64_t data64_len = ((data_len < MAX_RIFF_LEN) ? 0 : data_len);
    static ui32_t ds64_len =
            sizeof(RIFF_len)
            + sizeof(data64_len)
            + sizeof(SAMPLE_COUNT)
            + sizeof(TABLE_LEN);

    header_len = SIMPLE_RF64_HEADER_LEN;
    tmp_header = new byte_t[header_len];
    byte_t* p = tmp_header;
    memcpy(p, &FCC_RF64, sizeof(fourcc)); p += 4;
    *((ui32_t*)p) = KM_i32_LE(MAX_RIFF_LEN); p += 4;
    memcpy(p, &Wav::FCC_WAVE, sizeof(fourcc)); p += 4;
    memcpy(p, &FCC_ds64, sizeof(fourcc)); p += 4;
    *((ui32_t*)p) = KM_i32_LE(ds64_len); p += 4;
    *((ui64_t*)p) = KM_i64_LE(RIFF_len); p += 8;
    *((ui64_t*)p) = KM_i64_LE(data64_len); p += 8;
    *((ui64_t*)p) = KM_i64_LE(SAMPLE_COUNT); p += 8;
    *((ui32_t*)p) = KM_i32_LE(TABLE_LEN); p += 4;
    memcpy(p, &Wav::FCC_fmt_, sizeof(fourcc)); p += 4;
    *((ui32_t*)p) = KM_i32_LE(fmt_len); p += 4;
    *((ui16_t*)p) = KM_i16_LE(format); p += 2;
    *((ui16_t*)p) = KM_i16_LE(nchannels); p += 2;
    *((ui32_t*)p) = KM_i32_LE(samplespersec); p += 4;
    *((ui32_t*)p) = KM_i32_LE(avgbps); p += 4;
    *((ui16_t*)p) = KM_i16_LE(blockalign); p += 2;
    *((ui16_t*)p) = KM_i16_LE(bitspersample); p += 2;
    *((ui16_t*)p) = KM_i16_LE(cbsize); p += 2;
    memcpy(p, &Wav::FCC_data, sizeof(fourcc)); p += 4;
    *((ui32_t*)p) = KM_i32_LE(data32_len); p += 4;
    write_count = (p - tmp_header);
  }
  else
  {
    DefaultLogSink().Debug("Will write out a regular wave file.\n");
    header_len = SimpleWavHeaderLength;
    tmp_header = new byte_t[header_len];
    byte_t* p = tmp_header;
    memcpy(p, &Wav::FCC_RIFF, sizeof(fourcc)); p += 4;
    *((ui32_t*)p) = KM_i32_LE(RIFF_len); p += 4;
    memcpy(p, &Wav::FCC_WAVE, sizeof(fourcc)); p += 4;
    memcpy(p, &Wav::FCC_fmt_, sizeof(fourcc)); p += 4;
    *((ui32_t*)p) = KM_i32_LE(fmt_len); p += 4;
    *((ui16_t*)p) = KM_i16_LE(format); p += 2;
    *((ui16_t*)p) = KM_i16_LE(nchannels); p += 2;
    *((ui32_t*)p) = KM_i32_LE(samplespersec); p += 4;
    *((ui32_t*)p) = KM_i32_LE(avgbps); p += 4;
    *((ui16_t*)p) = KM_i16_LE(blockalign); p += 2;
    *((ui16_t*)p) = KM_i16_LE(bitspersample); p += 2;
    *((ui16_t*)p) = KM_i16_LE(cbsize); p += 2;
    memcpy(p, &Wav::FCC_data, sizeof(fourcc)); p += 4;
    *((ui32_t*)p) = KM_i32_LE(data_len); p += 4;
    write_count = (p - tmp_header);
  }
  if (header_len != write_count)
  {
      DefaultLogSink().Warn("Expected to write %u bytes but wrote %u bytes for header.\n",
                            header_len, write_count);
  }
  write_count = 0;
  ASDCP::Result_t r = OutFile.Write(tmp_header, header_len, &write_count);
  delete [] tmp_header;
  return r;
}

//
ASDCP::Result_t
ASDCP::RF64::SimpleRF64Header::ReadFromFile(const Kumu::FileReader& InFile, ui32_t* data_start)
{
  ui32_t read_count = 0;
  ui32_t local_data_start = 0;
  ASDCP::PCM::FrameBuffer TmpBuffer(Wav::MaxWavHeader);

  if ( data_start == 0 )
    data_start = &local_data_start;

  Result_t result = InFile.Read(TmpBuffer.Data(), TmpBuffer.Capacity(), &read_count);

  if ( ASDCP_SUCCESS(result) )
    result = ReadFromBuffer(TmpBuffer.RoData(), read_count, data_start);
  else
    DefaultLogSink().Error("Failed to read %d bytes from file\n", Wav::MaxWavHeader);

    return result;
}

ASDCP::Result_t
ASDCP::RF64::SimpleRF64Header::ReadFromBuffer(const byte_t* buf, ui32_t buf_len, ui32_t* data_start)
{
    if ( buf_len < SIMPLE_RF64_HEADER_LEN )
        return RESULT_SMALLBUF;

    *data_start = 0;
    const byte_t* p = buf;
    const byte_t* end_p = p + buf_len;

    fourcc test_RF64(p); p += 4;
    if ( test_RF64 != FCC_RF64 )
    {
        DefaultLogSink().Debug("File does not begin with RF64 header\n");
        return RESULT_RAW_FORMAT;
    }

    ui32_t tmp_len = KM_i32_LE(*(ui32_t*)p); p += 4;

    fourcc test_WAVE(p); p += 4;
    if ( test_WAVE != Wav::FCC_WAVE )
    {
        DefaultLogSink().Debug("File does not contain a WAVE header\n");
        return RESULT_RAW_FORMAT;
    }

    fourcc test_ds64(p); p += 4;
    if ( test_ds64 != FCC_ds64 )
    {
        DefaultLogSink().Debug("File does not contain a ds64 chunk\n");
        return RESULT_RAW_FORMAT;
    }
    ui32_t ds64_len = KM_i32_LE(*(ui32_t*)p); p += 4;
    ui64_t RIFF_len = ((tmp_len == MAX_RIFF_LEN) ? KM_i64_LE(*(ui64_t*)p) : tmp_len); p += 8;
    data_len = KM_i64_LE(*(ui64_t*)p); p += 8;
    p += (ds64_len - 16); // skip rest of ds64 chunk

    fourcc test_fcc;

    while ( p < end_p )
    {
        test_fcc = fourcc(p); p += 4;
        ui32_t chunk_size = KM_i32_LE(*(ui32_t*)p); p += 4;

        if ( test_fcc == Wav::FCC_data )
        {
            if ( chunk_size > RIFF_len )
            {
                DefaultLogSink().Error("Chunk size %u larger than file: %u\n", chunk_size, RIFF_len);
                return RESULT_RAW_FORMAT;
            }

            if (chunk_size != MAX_RIFF_LEN)
                data_len = chunk_size;
            *data_start = p - buf;
            break;
        }

        if ( test_fcc == Wav::FCC_fmt_ )
        {
            ui16_t format = KM_i16_LE(*(ui16_t*)p); p += 2;

            if ( format != Wav::WAVE_FORMAT_PCM && format != Wav::WAVE_FORMAT_EXTENSIBLE )
            {
                DefaultLogSink().Error("Expecting uncompressed PCM data, got format type %hd\n", format);
                return RESULT_RAW_FORMAT;
            }

            nchannels = KM_i16_LE(*(ui16_t*)p); p += 2;
            samplespersec = KM_i32_LE(*(ui32_t*)p); p += 4;
            avgbps = KM_i32_LE(*(ui32_t*)p); p += 4;
            blockalign = KM_i16_LE(*(ui16_t*)p); p += 2;
            bitspersample = KM_i16_LE(*(ui16_t*)p); p += 2;
            p += chunk_size - 16; // 16 is the number of bytes read in this block
        }
        else
        {
            p += chunk_size;
        }
    }

    if ( *data_start == 0 ) // can't have no data!
    {
        DefaultLogSink().Error("No data chunk found, file contains no essence\n");
        return RESULT_RAW_FORMAT;
    }

  return RESULT_OK;
}

//
// end Wav.cpp
//
