/*
Copyright (c) 2004-2014, John Hurst
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
/*! \file    AS_DCP_JP2k.cpp
    \version $Id: AS_DCP_JP2K.cpp,v 1.69 2015/10/07 16:41:23 jhurst Exp $
    \brief   AS-DCP library, JPEG 2000 essence reader and writer implementation
*/

#include "AS_DCP_internal.h"
#include <iostream>
#include <iomanip>

using namespace ASDCP::JP2K;
using Kumu::GenRandomValue;

//------------------------------------------------------------------------------------------

static std::string JP2K_PACKAGE_LABEL = "File Package: SMPTE 429-4 frame wrapping of JPEG 2000 codestreams";
static std::string JP2K_S_PACKAGE_LABEL = "File Package: SMPTE 429-10 frame wrapping of stereoscopic JPEG 2000 codestreams";
static std::string PICT_DEF_LABEL = "Picture Track";

int s_exp_lookup[16] = { 0, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024,2048, 4096, 8192, 16384, 32768 };

//
std::ostream&
ASDCP::JP2K::operator << (std::ostream& strm, const PictureDescriptor& PDesc)
{
  strm << "       AspectRatio: " << PDesc.AspectRatio.Numerator << "/" << PDesc.AspectRatio.Denominator << std::endl;
  strm << "          EditRate: " << PDesc.EditRate.Numerator << "/" << PDesc.EditRate.Denominator << std::endl;
  strm << "        SampleRate: " << PDesc.SampleRate.Numerator << "/" << PDesc.SampleRate.Denominator << std::endl;
  strm << "       StoredWidth: " << (unsigned) PDesc.StoredWidth << std::endl;
  strm << "      StoredHeight: " << (unsigned) PDesc.StoredHeight << std::endl;
  strm << "             Rsize: " << (unsigned) PDesc.Rsize << std::endl;
  strm << "             Xsize: " << (unsigned) PDesc.Xsize << std::endl;
  strm << "             Ysize: " << (unsigned) PDesc.Ysize << std::endl;
  strm << "            XOsize: " << (unsigned) PDesc.XOsize << std::endl;
  strm << "            YOsize: " << (unsigned) PDesc.YOsize << std::endl;
  strm << "            XTsize: " << (unsigned) PDesc.XTsize << std::endl;
  strm << "            YTsize: " << (unsigned) PDesc.YTsize << std::endl;
  strm << "           XTOsize: " << (unsigned) PDesc.XTOsize << std::endl;
  strm << "           YTOsize: " << (unsigned) PDesc.YTOsize << std::endl;
  strm << " ContainerDuration: " << (unsigned) PDesc.ContainerDuration << std::endl;

  strm << "-- JPEG 2000 Metadata --" << std::endl;
  strm << "    ImageComponents:" << std::endl;
  strm << "  bits  h-sep v-sep" << std::endl;

  ui32_t i;
  for ( i = 0; i < PDesc.Csize && i < MaxComponents; ++i )
    {
      strm << "  " << std::setw(4) << PDesc.ImageComponents[i].Ssize + 1 /* See ISO 15444-1, Table A11, for the origin of '+1' */
	   << "  " << std::setw(5) << PDesc.ImageComponents[i].XRsize
	   << " " << std::setw(5) << PDesc.ImageComponents[i].YRsize
	   << std::endl;
    }

  strm << "               Scod: " << (short) PDesc.CodingStyleDefault.Scod << std::endl;
  strm << "   ProgressionOrder: " << (short) PDesc.CodingStyleDefault.SGcod.ProgressionOrder << std::endl;
  strm << "     NumberOfLayers: " << (short) KM_i16_BE(Kumu::cp2i<ui16_t>(PDesc.CodingStyleDefault.SGcod.NumberOfLayers)) << std::endl;
  strm << " MultiCompTransform: " << (short) PDesc.CodingStyleDefault.SGcod.MultiCompTransform << std::endl;
  strm << "DecompositionLevels: " << (short) PDesc.CodingStyleDefault.SPcod.DecompositionLevels << std::endl;
  strm << "     CodeblockWidth: " << (short) PDesc.CodingStyleDefault.SPcod.CodeblockWidth << std::endl;
  strm << "    CodeblockHeight: " << (short) PDesc.CodingStyleDefault.SPcod.CodeblockHeight << std::endl;
  strm << "     CodeblockStyle: " << (short) PDesc.CodingStyleDefault.SPcod.CodeblockStyle << std::endl;
  strm << "     Transformation: " << (short) PDesc.CodingStyleDefault.SPcod.Transformation << std::endl;


  ui32_t precinct_set_size = 0;

  for ( i = 0; PDesc.CodingStyleDefault.SPcod.PrecinctSize[i] != 0 && i < MaxPrecincts; ++i )
    precinct_set_size++;

  strm << "          Precincts: " << (short) precinct_set_size << std::endl;
  strm << "precinct dimensions:" << std::endl;

  for ( i = 0; i < precinct_set_size && i < MaxPrecincts; ++i )
    strm << "    " << i + 1 << ": " << s_exp_lookup[PDesc.CodingStyleDefault.SPcod.PrecinctSize[i]&0x0f] << " x "
	 << s_exp_lookup[(PDesc.CodingStyleDefault.SPcod.PrecinctSize[i]>>4)&0x0f] << std::endl;

  strm << "               Sqcd: " << (short) PDesc.QuantizationDefault.Sqcd << std::endl;

  char tmp_buf[MaxDefaults*2];
  strm << "              SPqcd: " << Kumu::bin2hex(PDesc.QuantizationDefault.SPqcd, PDesc.QuantizationDefault.SPqcdLength, tmp_buf, MaxDefaults*2)
       << std::endl;

  return strm;
}

//
void
ASDCP::JP2K::PictureDescriptorDump(const PictureDescriptor& PDesc, FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "\
       AspectRatio: %d/%d\n\
          EditRate: %d/%d\n\
        SampleRate: %d/%d\n\
       StoredWidth: %u\n\
      StoredHeight: %u\n\
             Rsize: %u\n\
             Xsize: %u\n\
             Ysize: %u\n\
            XOsize: %u\n\
            YOsize: %u\n\
            XTsize: %u\n\
            YTsize: %u\n\
           XTOsize: %u\n\
           YTOsize: %u\n\
 ContainerDuration: %u\n",
	  PDesc.AspectRatio.Numerator, PDesc.AspectRatio.Denominator,
	  PDesc.EditRate.Numerator, PDesc.EditRate.Denominator,
	  PDesc.SampleRate.Numerator, PDesc.SampleRate.Denominator,
	  PDesc.StoredWidth,
	  PDesc.StoredHeight,
	  PDesc.Rsize,
	  PDesc.Xsize,
	  PDesc.Ysize,
	  PDesc.XOsize,
	  PDesc.YOsize,
	  PDesc.XTsize,
	  PDesc.YTsize,
	  PDesc.XTOsize,
	  PDesc.YTOsize,
	  PDesc.ContainerDuration
	  );

  fprintf(stream, "-- JPEG 2000 Metadata --\n");
  fprintf(stream, "    ImageComponents:\n");
  fprintf(stream, "  bits  h-sep v-sep\n");

  ui32_t i;
  for ( i = 0; i < PDesc.Csize && i < MaxComponents; i++ )
    {
      fprintf(stream, "  %4d  %5d %5d\n",
	      PDesc.ImageComponents[i].Ssize + 1, // See ISO 15444-1, Table A11, for the origin of '+1'
	      PDesc.ImageComponents[i].XRsize,
	      PDesc.ImageComponents[i].YRsize
	      );
    }
  
  fprintf(stream, "               Scod: %hhu\n", PDesc.CodingStyleDefault.Scod);
  fprintf(stream, "   ProgressionOrder: %hhu\n", PDesc.CodingStyleDefault.SGcod.ProgressionOrder);
  fprintf(stream, "     NumberOfLayers: %hd\n",
	  KM_i16_BE(Kumu::cp2i<ui16_t>(PDesc.CodingStyleDefault.SGcod.NumberOfLayers)));

  fprintf(stream, " MultiCompTransform: %hhu\n", PDesc.CodingStyleDefault.SGcod.MultiCompTransform);
  fprintf(stream, "DecompositionLevels: %hhu\n", PDesc.CodingStyleDefault.SPcod.DecompositionLevels);
  fprintf(stream, "     CodeblockWidth: %hhu\n", PDesc.CodingStyleDefault.SPcod.CodeblockWidth);
  fprintf(stream, "    CodeblockHeight: %hhu\n", PDesc.CodingStyleDefault.SPcod.CodeblockHeight);
  fprintf(stream, "     CodeblockStyle: %hhu\n", PDesc.CodingStyleDefault.SPcod.CodeblockStyle);
  fprintf(stream, "     Transformation: %hhu\n", PDesc.CodingStyleDefault.SPcod.Transformation);


  ui32_t precinct_set_size = 0;

  for ( i = 0; PDesc.CodingStyleDefault.SPcod.PrecinctSize[i] != 0 && i < MaxPrecincts; ++i )
    precinct_set_size++;

  fprintf(stream, "          Precincts: %u\n", precinct_set_size);
  fprintf(stream, "precinct dimensions:\n");

  for ( i = 0; i < precinct_set_size && i < MaxPrecincts; i++ )
    fprintf(stream, "    %d: %d x %d\n", i + 1,
	    s_exp_lookup[PDesc.CodingStyleDefault.SPcod.PrecinctSize[i]&0x0f],
	    s_exp_lookup[(PDesc.CodingStyleDefault.SPcod.PrecinctSize[i]>>4)&0x0f]
	    );

  fprintf(stream, "               Sqcd: %hhu\n", PDesc.QuantizationDefault.Sqcd);

  char tmp_buf[MaxDefaults*2];
  fprintf(stream, "              SPqcd: %s\n",
	  Kumu::bin2hex(PDesc.QuantizationDefault.SPqcd, PDesc.QuantizationDefault.SPqcdLength,
			tmp_buf, MaxDefaults*2)
	  );
}


const int VideoLineMapSize = 16; // See SMPTE 377M D.2.1
const int PixelLayoutSize = 8*2; // See SMPTE 377M D.2.3
static const byte_t s_PixelLayoutXYZ[PixelLayoutSize] = { 0xd8, 0x0c, 0xd9, 0x0c, 0xda, 0x0c, 0x00 };

//
ASDCP::Result_t
ASDCP::JP2K_PDesc_to_MD(const JP2K::PictureDescriptor& PDesc,
			const ASDCP::Dictionary& dict,
			ASDCP::MXF::GenericPictureEssenceDescriptor& EssenceDescriptor,
			ASDCP::MXF::JPEG2000PictureSubDescriptor& EssenceSubDescriptor)
{
  EssenceDescriptor.ContainerDuration = PDesc.ContainerDuration;
  EssenceDescriptor.SampleRate = PDesc.EditRate;
  EssenceDescriptor.FrameLayout = 0;
  EssenceDescriptor.StoredWidth = PDesc.StoredWidth;
  EssenceDescriptor.StoredHeight = PDesc.StoredHeight;
  EssenceDescriptor.AspectRatio = PDesc.AspectRatio;

  EssenceSubDescriptor.Rsize = PDesc.Rsize;
  EssenceSubDescriptor.Xsize = PDesc.Xsize;
  EssenceSubDescriptor.Ysize = PDesc.Ysize;
  EssenceSubDescriptor.XOsize = PDesc.XOsize;
  EssenceSubDescriptor.YOsize = PDesc.YOsize;
  EssenceSubDescriptor.XTsize = PDesc.XTsize;
  EssenceSubDescriptor.YTsize = PDesc.YTsize;
  EssenceSubDescriptor.XTOsize = PDesc.XTOsize;
  EssenceSubDescriptor.YTOsize = PDesc.YTOsize;
  EssenceSubDescriptor.Csize = PDesc.Csize;

  const ui32_t tmp_buffer_len = 1024;
  byte_t tmp_buffer[tmp_buffer_len];

  *(ui32_t*)tmp_buffer = KM_i32_BE(MaxComponents); // three components
  *(ui32_t*)(tmp_buffer+4) = KM_i32_BE(sizeof(ASDCP::JP2K::ImageComponent_t));
  memcpy(tmp_buffer + 8, &PDesc.ImageComponents, sizeof(ASDCP::JP2K::ImageComponent_t) * MaxComponents);

  const ui32_t pcomp_size = (sizeof(ui32_t) * 2) + (sizeof(ASDCP::JP2K::ImageComponent_t) * MaxComponents);
  memcpy(EssenceSubDescriptor.PictureComponentSizing.get().Data(), tmp_buffer, pcomp_size);
  EssenceSubDescriptor.PictureComponentSizing.get().Length(pcomp_size);
  EssenceSubDescriptor.PictureComponentSizing.set_has_value();

  ui32_t precinct_set_size = 0;
  for ( ui32_t i = 0; PDesc.CodingStyleDefault.SPcod.PrecinctSize[i] != 0 && i < MaxPrecincts; ++i )
    precinct_set_size++;

  ui32_t csd_size = sizeof(CodingStyleDefault_t) - MaxPrecincts + precinct_set_size;
  memcpy(EssenceSubDescriptor.CodingStyleDefault.get().Data(), &PDesc.CodingStyleDefault, csd_size);
  EssenceSubDescriptor.CodingStyleDefault.get().Length(csd_size);
  EssenceSubDescriptor.CodingStyleDefault.set_has_value();

  ui32_t qdflt_size = PDesc.QuantizationDefault.SPqcdLength + 1;
  memcpy(EssenceSubDescriptor.QuantizationDefault.get().Data(), &PDesc.QuantizationDefault, qdflt_size);
  EssenceSubDescriptor.QuantizationDefault.get().Length(qdflt_size);
  EssenceSubDescriptor.QuantizationDefault.set_has_value();

  return RESULT_OK;
}


//
ASDCP::Result_t
ASDCP::MD_to_JP2K_PDesc(const ASDCP::MXF::GenericPictureEssenceDescriptor&  EssenceDescriptor,
			const ASDCP::MXF::JPEG2000PictureSubDescriptor& EssenceSubDescriptor,
			const ASDCP::Rational& EditRate, const ASDCP::Rational& SampleRate,
			ASDCP::JP2K::PictureDescriptor& PDesc)
{
  memset(&PDesc, 0, sizeof(PDesc));

  PDesc.EditRate           = EditRate;
  PDesc.SampleRate         = SampleRate;
  assert(EssenceDescriptor.ContainerDuration.const_get() <= 0xFFFFFFFFL);
  PDesc.ContainerDuration  = static_cast<ui32_t>(EssenceDescriptor.ContainerDuration.const_get());
  PDesc.StoredWidth        = EssenceDescriptor.StoredWidth;
  PDesc.StoredHeight       = EssenceDescriptor.StoredHeight;
  PDesc.AspectRatio        = EssenceDescriptor.AspectRatio;

  PDesc.Rsize   = EssenceSubDescriptor.Rsize;
  PDesc.Xsize   = EssenceSubDescriptor.Xsize;
  PDesc.Ysize   = EssenceSubDescriptor.Ysize;
  PDesc.XOsize  = EssenceSubDescriptor.XOsize;
  PDesc.YOsize  = EssenceSubDescriptor.YOsize;
  PDesc.XTsize  = EssenceSubDescriptor.XTsize;
  PDesc.YTsize  = EssenceSubDescriptor.YTsize;
  PDesc.XTOsize = EssenceSubDescriptor.XTOsize;
  PDesc.YTOsize = EssenceSubDescriptor.YTOsize;
  PDesc.Csize   = EssenceSubDescriptor.Csize;

  // PictureComponentSizing
  ui32_t tmp_size = EssenceSubDescriptor.PictureComponentSizing.const_get().Length();

  if ( tmp_size == 17 ) // ( 2 * sizeof(ui32_t) ) + 3 components * 3 byte each
    {
      memcpy(&PDesc.ImageComponents, EssenceSubDescriptor.PictureComponentSizing.const_get().RoData() + 8, tmp_size - 8);
    }
  else
    {
      DefaultLogSink().Warn("Unexpected PictureComponentSizing size: %u, should be 17\n", tmp_size);
    }

  // CodingStyleDefault
  memset(&PDesc.CodingStyleDefault, 0, sizeof(CodingStyleDefault_t));
  memcpy(&PDesc.CodingStyleDefault,
	 EssenceSubDescriptor.CodingStyleDefault.const_get().RoData(),
	 EssenceSubDescriptor.CodingStyleDefault.const_get().Length());

  // QuantizationDefault
  memset(&PDesc.QuantizationDefault, 0, sizeof(QuantizationDefault_t));
  memcpy(&PDesc.QuantizationDefault,
	 EssenceSubDescriptor.QuantizationDefault.const_get().RoData(),
	 EssenceSubDescriptor.QuantizationDefault.const_get().Length());
  
  PDesc.QuantizationDefault.SPqcdLength = EssenceSubDescriptor.QuantizationDefault.const_get().Length() - 1;
  return RESULT_OK;
}


//------------------------------------------------------------------------------------------
//
// hidden, internal implementation of JPEG 2000 reader


class lh__Reader : public ASDCP::h__ASDCPReader
{
  RGBAEssenceDescriptor*        m_EssenceDescriptor;
  JPEG2000PictureSubDescriptor* m_EssenceSubDescriptor;
  ASDCP::Rational               m_EditRate;
  ASDCP::Rational               m_SampleRate;
  EssenceType_t                 m_Format;

  ASDCP_NO_COPY_CONSTRUCT(lh__Reader);

public:
  PictureDescriptor m_PDesc;        // codestream parameter list

  lh__Reader(const Dictionary& d) :
    ASDCP::h__ASDCPReader(d), m_EssenceDescriptor(0), m_EssenceSubDescriptor(0), m_Format(ESS_UNKNOWN) {}

  virtual ~lh__Reader() {}

  Result_t    OpenRead(const std::string&, EssenceType_t);
  Result_t    ReadFrame(ui32_t, JP2K::FrameBuffer&, AESDecContext*, HMACContext*);
};


//
//
ASDCP::Result_t
lh__Reader::OpenRead(const std::string& filename, EssenceType_t type)
{
  Result_t result = OpenMXFRead(filename);

  if( ASDCP_SUCCESS(result) )
    {
      InterchangeObject* tmp_iobj = 0;
      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(RGBAEssenceDescriptor), &tmp_iobj);
      m_EssenceDescriptor = static_cast<RGBAEssenceDescriptor*>(tmp_iobj);

      if ( m_EssenceDescriptor == 0 )
	{
	  DefaultLogSink().Error("RGBAEssenceDescriptor object not found.\n");
	  return RESULT_FORMAT;
	}

      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(JPEG2000PictureSubDescriptor), &tmp_iobj);
      m_EssenceSubDescriptor = static_cast<JPEG2000PictureSubDescriptor*>(tmp_iobj);

      if ( m_EssenceSubDescriptor == 0 )
	{
	  m_EssenceDescriptor = 0;
	  DefaultLogSink().Error("JPEG2000PictureSubDescriptor object not found.\n");
	  return RESULT_FORMAT;
	}

      std::list<InterchangeObject*> ObjectList;
      m_HeaderPart.GetMDObjectsByType(OBJ_TYPE_ARGS(Track), ObjectList);

      if ( ObjectList.empty() )
	{
	  DefaultLogSink().Error("MXF Metadata contains no Track Sets.\n");
	  return RESULT_FORMAT;
	}

      m_EditRate = ((Track*)ObjectList.front())->EditRate;
      m_SampleRate = m_EssenceDescriptor->SampleRate;

      if ( type == ASDCP::ESS_JPEG_2000 )
	{
	  if ( m_EditRate != m_SampleRate )
	    {
	      DefaultLogSink().Warn("EditRate and SampleRate do not match (%.03f, %.03f).\n",
				    m_EditRate.Quotient(), m_SampleRate.Quotient());
	      
	      if ( ( m_EditRate == EditRate_24 && m_SampleRate == EditRate_48 )
		   || ( m_EditRate == EditRate_25 && m_SampleRate == EditRate_50 )
		   || ( m_EditRate == EditRate_30 && m_SampleRate == EditRate_60 )
		   || ( m_EditRate == EditRate_48 && m_SampleRate == EditRate_96 )
		   || ( m_EditRate == EditRate_50 && m_SampleRate == EditRate_100 )
		   || ( m_EditRate == EditRate_60 && m_SampleRate == EditRate_120 ) )
		{
		  DefaultLogSink().Debug("File may contain JPEG Interop stereoscopic images.\n");
		  return RESULT_SFORMAT;
		}

	      return RESULT_FORMAT;
	    }
	}
      else if ( type == ASDCP::ESS_JPEG_2000_S )
	{
	  if ( m_EditRate == EditRate_24 )
	    {
	      if ( m_SampleRate != EditRate_48 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 24/48 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_25 )
	    {
	      if ( m_SampleRate != EditRate_50 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 25/50 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_30 )
	    {
	      if ( m_SampleRate != EditRate_60 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 30/60 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_48 )
	    {
	      if ( m_SampleRate != EditRate_96 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 48/96 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_50 )
	    {
	      if ( m_SampleRate != EditRate_100 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 50/100 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_60 )
	    {
	      if ( m_SampleRate != EditRate_120 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 60/120 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else
	    {
	      DefaultLogSink().Error("EditRate not correct for stereoscopic essence: %d/%d.\n",
				     m_EditRate.Numerator, m_EditRate.Denominator);
	      return RESULT_FORMAT;
	    }
	}
      else
	{
	  DefaultLogSink().Error("'type' argument unexpected: %x\n", type);
	  return RESULT_STATE;
	}

      result = MD_to_JP2K_PDesc(*m_EssenceDescriptor, *m_EssenceSubDescriptor, m_EditRate, m_SampleRate, m_PDesc);
    }

  return result;
}

//
//
ASDCP::Result_t
lh__Reader::ReadFrame(ui32_t FrameNum, JP2K::FrameBuffer& FrameBuf,
		      AESDecContext* Ctx, HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  assert(m_Dict);
  return ReadEKLVFrame(FrameNum, FrameBuf, m_Dict->ul(MDD_JPEG2000Essence), Ctx, HMAC);
}


//
class ASDCP::JP2K::MXFReader::h__Reader : public lh__Reader
{
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);
  h__Reader();

public:
  h__Reader(const Dictionary& d) : lh__Reader(d) {}
};



//------------------------------------------------------------------------------------------


//
void
ASDCP::JP2K::FrameBuffer::Dump(FILE* stream, ui32_t dump_len) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "Frame: %06u, %7u bytes", m_FrameNumber, m_Size);
  
  fputc('\n', stream);

  if ( dump_len > 0 )
    Kumu::hexdump(m_Data, dump_len, stream);
}


//------------------------------------------------------------------------------------------

ASDCP::JP2K::MXFReader::MXFReader()
{
  m_Reader = new h__Reader(DefaultCompositeDict());
}


ASDCP::JP2K::MXFReader::~MXFReader()
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    m_Reader->Close();
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::JP2K::MXFReader::OP1aHeader()
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
ASDCP::JP2K::MXFReader::OPAtomIndexFooter()
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
ASDCP::JP2K::MXFReader::RIP()
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
ASDCP::JP2K::MXFReader::OpenRead(const std::string& filename) const
{
  return m_Reader->OpenRead(filename, ASDCP::ESS_JPEG_2000);
}

//
ASDCP::Result_t
ASDCP::JP2K::MXFReader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
				   AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}

ASDCP::Result_t
ASDCP::JP2K::MXFReader::LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset, i8_t& temporalOffset, i8_t& keyFrameOffset) const
{
    return m_Reader->LocateFrame(FrameNum, streamOffset, temporalOffset, keyFrameOffset);
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::JP2K::MXFReader::FillPictureDescriptor(PictureDescriptor& PDesc) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      PDesc = m_Reader->m_PDesc;
      return RESULT_OK;
    }

  return RESULT_INIT;
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::JP2K::MXFReader::FillWriterInfo(WriterInfo& Info) const
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
ASDCP::JP2K::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_HeaderPart.Dump(stream);
}


//
void
ASDCP::JP2K::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_IndexAccess.Dump(stream);
}

//
ASDCP::Result_t
ASDCP::JP2K::MXFReader::Close() const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      m_Reader->Close();
      return RESULT_OK;
    }

  return RESULT_INIT;
}


//------------------------------------------------------------------------------------------


class ASDCP::JP2K::MXFSReader::h__SReader : public lh__Reader
{
  ui32_t m_StereoFrameReady;

public:
  h__SReader(const Dictionary& d) : lh__Reader(d), m_StereoFrameReady(0xffffffff) {}

  //
  Result_t ReadFrame(ui32_t FrameNum, StereoscopicPhase_t phase, FrameBuffer& FrameBuf,
		     AESDecContext* Ctx, HMACContext* HMAC)
  {
    // look up frame index node
    IndexTableSegment::IndexEntry TmpEntry;

    if ( ASDCP_FAILURE(m_IndexAccess.Lookup(FrameNum, TmpEntry)) )
      {
	return RESULT_RANGE;
      }

    // get frame position
    Kumu::fpos_t FilePosition = m_HeaderPart.BodyOffset + TmpEntry.StreamOffset;
    Result_t result = RESULT_OK;

    if ( phase == SP_LEFT )
      {    
	if ( FilePosition != m_LastPosition )
	  {
	    m_LastPosition = FilePosition;
	    result = m_File.Seek(FilePosition);
	  }

	// the call to ReadEKLVPacket() will leave the file on an R frame
	m_StereoFrameReady = FrameNum;
      }
    else if ( phase == SP_RIGHT )
      {
	if ( m_StereoFrameReady != FrameNum )
	  {
	    // the file is not already positioned, we must do some work
	    // seek to the companion SP_LEFT frame and read the frame's key and length
	    if ( FilePosition != m_LastPosition )
	      {
		m_LastPosition = FilePosition;
		result = m_File.Seek(FilePosition);
	      }

	    KLReader Reader;
	    result = Reader.ReadKLFromFile(m_File);

	    if ( ASDCP_SUCCESS(result) )
	      {
		// skip over the companion SP_LEFT frame
		Kumu::fpos_t new_pos = FilePosition + SMPTE_UL_LENGTH + Reader.KLLength() + Reader.Length();
		result = m_File.Seek(new_pos);
	      }
	  }

	// the call to ReadEKLVPacket() will leave the file not on an R frame
	m_StereoFrameReady = 0xffffffff;
      }
    else
      {
	DefaultLogSink().Error("Unexpected stereoscopic phase value: %u\n", phase);
	return RESULT_STATE;
      }

    if( ASDCP_SUCCESS(result) )
      {
	ui32_t SequenceNum = FrameNum * 2;
	SequenceNum += ( phase == SP_RIGHT ) ? 2 : 1;
	assert(m_Dict);
	result = ReadEKLVPacket(FrameNum, SequenceNum, FrameBuf, m_Dict->ul(MDD_JPEG2000Essence), Ctx, HMAC);
      }

    return result;
  }
};



ASDCP::JP2K::MXFSReader::MXFSReader()
{
  m_Reader = new h__SReader(DefaultCompositeDict());
}


ASDCP::JP2K::MXFSReader::~MXFSReader()
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    m_Reader->Close();
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::JP2K::MXFSReader::OP1aHeader()
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
ASDCP::JP2K::MXFSReader::OPAtomIndexFooter()
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
ASDCP::JP2K::MXFSReader::RIP()
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
ASDCP::JP2K::MXFSReader::OpenRead(const std::string& filename) const
{
  return m_Reader->OpenRead(filename, ASDCP::ESS_JPEG_2000_S);
}

//
ASDCP::Result_t
ASDCP::JP2K::MXFSReader::ReadFrame(ui32_t FrameNum, SFrameBuffer& FrameBuf, AESDecContext* Ctx, HMACContext* HMAC) const
{
  Result_t result = RESULT_INIT;

  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      result = m_Reader->ReadFrame(FrameNum, SP_LEFT, FrameBuf.Left, Ctx, HMAC);

      if ( ASDCP_SUCCESS(result) )
	result = m_Reader->ReadFrame(FrameNum, SP_RIGHT, FrameBuf.Right, Ctx, HMAC);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::JP2K::MXFSReader::ReadFrame(ui32_t FrameNum, StereoscopicPhase_t phase, FrameBuffer& FrameBuf,
				   AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrame(FrameNum, phase, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}

ASDCP::Result_t
ASDCP::JP2K::MXFSReader::LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset, i8_t& temporalOffset, i8_t& keyFrameOffset) const
{
    return m_Reader->LocateFrame(FrameNum, streamOffset, temporalOffset, keyFrameOffset);
}

// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::JP2K::MXFSReader::FillPictureDescriptor(PictureDescriptor& PDesc) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      PDesc = m_Reader->m_PDesc;
      return RESULT_OK;
    }

  return RESULT_INIT;
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::JP2K::MXFSReader::FillWriterInfo(WriterInfo& Info) const
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
ASDCP::JP2K::MXFSReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_HeaderPart.Dump(stream);
}


//
void
ASDCP::JP2K::MXFSReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_IndexAccess.Dump(stream);
}

//
ASDCP::Result_t
ASDCP::JP2K::MXFSReader::Close() const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      m_Reader->Close();
      return RESULT_OK;
    }

  return RESULT_INIT;
}


//------------------------------------------------------------------------------------------


//
class lh__Writer : public ASDCP::h__ASDCPWriter
{
  ASDCP_NO_COPY_CONSTRUCT(lh__Writer);
  lh__Writer();

  JPEG2000PictureSubDescriptor* m_EssenceSubDescriptor;

public:
  PictureDescriptor m_PDesc;
  byte_t            m_EssenceUL[SMPTE_UL_LENGTH];

  lh__Writer(const Dictionary& d) : ASDCP::h__ASDCPWriter(d), m_EssenceSubDescriptor(0) {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  virtual ~lh__Writer(){}

  Result_t OpenWrite(const std::string&, EssenceType_t type, ui32_t HeaderSize);
  Result_t SetSourceStream(const PictureDescriptor&, const std::string& label,
			   ASDCP::Rational LocalEditRate = ASDCP::Rational(0,0));
  Result_t WriteFrame(const JP2K::FrameBuffer&, bool add_index, AESEncContext*, HMACContext*);
  Result_t Finalize();
};

// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
lh__Writer::OpenWrite(const std::string& filename, EssenceType_t type, ui32_t HeaderSize)
{
  if ( ! m_State.Test_BEGIN() )
    return RESULT_STATE;

  Result_t result = m_File.OpenWrite(filename);

  if ( ASDCP_SUCCESS(result) )
    {
      m_HeaderSize = HeaderSize;
      RGBAEssenceDescriptor* tmp_rgba = new RGBAEssenceDescriptor(m_Dict);
      tmp_rgba->ComponentMaxRef = 4095;
      tmp_rgba->ComponentMinRef = 0;

      m_EssenceDescriptor = tmp_rgba;
      m_EssenceSubDescriptor = new JPEG2000PictureSubDescriptor(m_Dict);
      m_EssenceSubDescriptorList.push_back((InterchangeObject*)m_EssenceSubDescriptor);

      GenRandomValue(m_EssenceSubDescriptor->InstanceUID);
      m_EssenceDescriptor->SubDescriptors.push_back(m_EssenceSubDescriptor->InstanceUID);

      if ( type == ASDCP::ESS_JPEG_2000_S && m_Info.LabelSetType == LS_MXF_SMPTE )
	{
	  InterchangeObject* StereoSubDesc = new StereoscopicPictureSubDescriptor(m_Dict);
	  m_EssenceSubDescriptorList.push_back(StereoSubDesc);
	  GenRandomValue(StereoSubDesc->InstanceUID);
	  m_EssenceDescriptor->SubDescriptors.push_back(StereoSubDesc->InstanceUID);
	}

      result = m_State.Goto_INIT();
    }

  return result;
}

// Automatically sets the MXF file's metadata from the first jpeg codestream stream.
ASDCP::Result_t
lh__Writer::SetSourceStream(const PictureDescriptor& PDesc, const std::string& label, ASDCP::Rational LocalEditRate)
{
  assert(m_Dict);
  if ( ! m_State.Test_INIT() )
    return RESULT_STATE;

  if ( LocalEditRate == ASDCP::Rational(0,0) )
    LocalEditRate = PDesc.EditRate;

  m_PDesc = PDesc;
  assert(m_Dict);
  assert(m_EssenceDescriptor);
  assert(m_EssenceSubDescriptor);
  Result_t result = JP2K_PDesc_to_MD(m_PDesc, *m_Dict,
				     *static_cast<ASDCP::MXF::GenericPictureEssenceDescriptor*>(m_EssenceDescriptor),
				     *m_EssenceSubDescriptor);

  if ( ASDCP_SUCCESS(result) )
    {
      if ( PDesc.StoredWidth < 2049 )
	{
	  static_cast<ASDCP::MXF::RGBAEssenceDescriptor*>(m_EssenceDescriptor)->PictureEssenceCoding.Set(m_Dict->ul(MDD_JP2KEssenceCompression_2K));
	  m_EssenceSubDescriptor->Rsize = 3;
	}
      else
	{
	  static_cast<ASDCP::MXF::RGBAEssenceDescriptor*>(m_EssenceDescriptor)->PictureEssenceCoding.Set(m_Dict->ul(MDD_JP2KEssenceCompression_4K));
	  m_EssenceSubDescriptor->Rsize = 4;
	}

      memcpy(m_EssenceUL, m_Dict->ul(MDD_JPEG2000Essence), SMPTE_UL_LENGTH);
      m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container
      result = m_State.Goto_READY();
    }

  if ( ASDCP_SUCCESS(result) )
    {
      result = WriteASDCPHeader(label, UL(m_Dict->ul(MDD_JPEG_2000WrappingFrame)),
				PICT_DEF_LABEL, UL(m_EssenceUL), UL(m_Dict->ul(MDD_PictureDataDef)),
				LocalEditRate, derive_timecode_rate_from_edit_rate(m_PDesc.EditRate));
    }

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
//
ASDCP::Result_t
lh__Writer::WriteFrame(const JP2K::FrameBuffer& FrameBuf, bool add_index,
		       AESEncContext* Ctx, HMACContext* HMAC)
{
  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() )
    result = m_State.Goto_RUNNING(); // first time through
 
  ui64_t StreamOffset = m_StreamOffset;

  if ( ASDCP_SUCCESS(result) )
    result = WriteEKLVPacket(FrameBuf, m_EssenceUL, Ctx, HMAC);

  if ( ASDCP_SUCCESS(result) && add_index )
    {  
      IndexTableSegment::IndexEntry Entry;
      Entry.StreamOffset = StreamOffset;
      m_FooterPart.PushIndexEntry(Entry);
    }

  m_FramesWritten++;
  return result;
}


// Closes the MXF file, writing the index and other closing information.
//
ASDCP::Result_t
lh__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;

  m_State.Goto_FINAL();

  return WriteASDCPFooter();
}


//
class ASDCP::JP2K::MXFWriter::h__Writer : public lh__Writer
{
  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

public:
  h__Writer(const Dictionary& d) : lh__Writer(d) {}
};


//------------------------------------------------------------------------------------------



ASDCP::JP2K::MXFWriter::MXFWriter()
{
}

ASDCP::JP2K::MXFWriter::~MXFWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::JP2K::MXFWriter::OP1aHeader()
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
ASDCP::JP2K::MXFWriter::OPAtomIndexFooter()
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
ASDCP::JP2K::MXFWriter::RIP()
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
ASDCP::JP2K::MXFWriter::OpenWrite(const std::string& filename, const WriterInfo& Info,
				  const PictureDescriptor& PDesc, ui32_t HeaderSize)
{
  if ( Info.LabelSetType == LS_MXF_SMPTE )
    m_Writer = new h__Writer(DefaultSMPTEDict());
  else
    m_Writer = new h__Writer(DefaultInteropDict());

  m_Writer->m_Info = Info;

  Result_t result = m_Writer->OpenWrite(filename, ASDCP::ESS_JPEG_2000, HeaderSize);

  if ( ASDCP_SUCCESS(result) )
    result = m_Writer->SetSourceStream(PDesc, JP2K_PACKAGE_LABEL);

  if ( ASDCP_FAILURE(result) )
    m_Writer.release();

  return result;
}


// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
ASDCP::Result_t
ASDCP::JP2K::MXFWriter::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, true, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
ASDCP::Result_t
ASDCP::JP2K::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}


//------------------------------------------------------------------------------------------
//

//
class ASDCP::JP2K::MXFSWriter::h__SWriter : public lh__Writer
{
  ASDCP_NO_COPY_CONSTRUCT(h__SWriter);
  h__SWriter();
  StereoscopicPhase_t m_NextPhase;

public:
  h__SWriter(const Dictionary& d) : lh__Writer(d), m_NextPhase(SP_LEFT) {}

  //
  Result_t WriteFrame(const FrameBuffer& FrameBuf, StereoscopicPhase_t phase,
		      AESEncContext* Ctx, HMACContext* HMAC)
  {
    if ( m_NextPhase != phase )
      return RESULT_SPHASE;

    if ( phase == SP_LEFT )
      {
	m_NextPhase = SP_RIGHT;
	return lh__Writer::WriteFrame(FrameBuf, true, Ctx, HMAC);
      }

    m_NextPhase = SP_LEFT;
    return lh__Writer::WriteFrame(FrameBuf, false, Ctx, HMAC);
  }

  //
  Result_t Finalize()
  {
    if ( m_NextPhase != SP_LEFT )
      return RESULT_SPHASE;

    assert( m_FramesWritten % 2 == 0 );
    m_FramesWritten /= 2;
    return lh__Writer::Finalize();
  }
};


//
ASDCP::JP2K::MXFSWriter::MXFSWriter()
{
}

ASDCP::JP2K::MXFSWriter::~MXFSWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::JP2K::MXFSWriter::OP1aHeader()
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
ASDCP::JP2K::MXFSWriter::OPAtomIndexFooter()
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
ASDCP::JP2K::MXFSWriter::RIP()
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
ASDCP::JP2K::MXFSWriter::OpenWrite(const std::string& filename, const WriterInfo& Info,
				   const PictureDescriptor& PDesc, ui32_t HeaderSize)
{
  if ( Info.LabelSetType == LS_MXF_SMPTE )
    m_Writer = new h__SWriter(DefaultSMPTEDict());
  else
    m_Writer = new h__SWriter(DefaultInteropDict());

  if ( PDesc.EditRate != ASDCP::EditRate_24
       && PDesc.EditRate != ASDCP::EditRate_25
       && PDesc.EditRate != ASDCP::EditRate_30
       && PDesc.EditRate != ASDCP::EditRate_48
       && PDesc.EditRate != ASDCP::EditRate_50
       && PDesc.EditRate != ASDCP::EditRate_60 )
    {
      DefaultLogSink().Error("Stereoscopic wrapping requires 24, 25, 30, 48, 50 or 60 fps input streams.\n");
      return RESULT_FORMAT;
    }

  if ( PDesc.StoredWidth > 2048 )
    DefaultLogSink().Warn("Wrapping non-standard 4K stereoscopic content. I hope you know what you are doing!\n");

  m_Writer->m_Info = Info;

  Result_t result = m_Writer->OpenWrite(filename, ASDCP::ESS_JPEG_2000_S, HeaderSize);

  if ( ASDCP_SUCCESS(result) )
    {
      PictureDescriptor TmpPDesc = PDesc;

      if ( PDesc.EditRate == ASDCP::EditRate_24 )
	TmpPDesc.EditRate = ASDCP::EditRate_48;

      else if ( PDesc.EditRate == ASDCP::EditRate_25 )
	TmpPDesc.EditRate = ASDCP::EditRate_50;

      else if ( PDesc.EditRate == ASDCP::EditRate_30 )
	TmpPDesc.EditRate = ASDCP::EditRate_60;

      else if ( PDesc.EditRate == ASDCP::EditRate_48 )
	TmpPDesc.EditRate = ASDCP::EditRate_96;

      else if ( PDesc.EditRate == ASDCP::EditRate_50 )
	TmpPDesc.EditRate = ASDCP::EditRate_100;

      else if ( PDesc.EditRate == ASDCP::EditRate_60 )
	TmpPDesc.EditRate = ASDCP::EditRate_120;

      result = m_Writer->SetSourceStream(TmpPDesc, JP2K_S_PACKAGE_LABEL, PDesc.EditRate);
    }

  if ( ASDCP_FAILURE(result) )
    m_Writer.release();

  return result;
}

ASDCP::Result_t
ASDCP::JP2K::MXFSWriter::WriteFrame(const SFrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  Result_t result = m_Writer->WriteFrame(FrameBuf.Left, SP_LEFT, Ctx, HMAC);

  if ( ASDCP_SUCCESS(result) )
    result = m_Writer->WriteFrame(FrameBuf.Right, SP_RIGHT, Ctx, HMAC);

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
ASDCP::Result_t
ASDCP::JP2K::MXFSWriter::WriteFrame(const FrameBuffer& FrameBuf, StereoscopicPhase_t phase,
				    AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, phase, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
ASDCP::Result_t
ASDCP::JP2K::MXFSWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}

//
// end AS_DCP_JP2K.cpp
//
