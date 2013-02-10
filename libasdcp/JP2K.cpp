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
/*! \file    JP2K.cpp
    \version $Id: JP2K.cpp,v 1.8 2010/06/17 03:33:17 jhurst Exp $
    \brief   JPEG 2000 parser implementation

    This is not a complete implementation of all things JP2K.  There is just enough here to
    support parsing picture metadata from a codestream header.
*/

#include <JP2K.h>
#include <KM_log.h>
using Kumu::DefaultLogSink;


// when indexed with the second byte of a marker code, this table will procuce one of
// two values:
//   0 - the marker is a standalone marker
//   1 - the marker designates a marker segment
//
const byte_t MarkerSegmentMap[] =
  {
    /*      0   1   2   3   4   5   6   7      8   9   a   b   c   d   e   f */
    /* 0 */ 0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, // 0
    /* 1 */ 0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, // 1
    /* 2 */ 0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, // 2
    /* 3 */ 0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, // 3
    /* 4 */ 0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, // 4
    /* 5 */ 0,  1,  1,  1,  1,  1,  0,  1,     1,  0,  0,  0,  1,  1,  1,  1, // 5
    /* 6 */ 1,  1,  0,  1,  1,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, // 6
    /* 7 */ 0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, // 7
    /* 8 */ 0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, // 8
    /* 9 */ 1,  1,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, // 9
    /* a */ 0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, // a
    /* b */ 0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, // b
    /* c */ 0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, // c
    /* d */ 0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, // d
    /* e */ 0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, // e
    /* f */ 0,  0,  0,  0,  0,  0,  0,  0,     0,  0,  0,  0,  0,  0,  0,  0, // f
    /*      0   1   2   3   4   5   6   7      8   9   a   b   c   d   e   f */
  };


//
ASDCP::Result_t
ASDCP::JP2K::GetNextMarker(const byte_t** buf, JP2K::Marker& Marker)
{
  assert((buf != 0) && (*buf != 0 ));
  
  if ( **buf != 0xff )
    return ASDCP::RESULT_FAIL;

  Marker.m_IsSegment = (MarkerSegmentMap[*(++(*buf))] == 1);
  Marker.m_Type = (Marker_t)(0xff00 | *(*buf)++);

  if ( Marker.m_IsSegment )
    {
      Marker.m_DataSize = *(*buf)++ << 8;
      Marker.m_DataSize |= *(*buf)++;
      Marker.m_DataSize -= 2;
      Marker.m_Data = *buf;
      *buf += Marker.m_DataSize;
    }


  if ( Marker.m_DataSize != 0 && Marker.m_DataSize < 3 )
    {
      DefaultLogSink().Error("Illegal data size: %u\n", Marker.m_DataSize);
      return ASDCP::RESULT_FAIL;
    }

  return ASDCP::RESULT_OK;
}


//-------------------------------------------------------------------------------------------------------
//

//
void
ASDCP::JP2K::Accessor::SIZ::ReadComponent(ui32_t index, ASDCP::JP2K::ImageComponent_t& IC)
{
  assert ( index < Csize() );
  const byte_t* p = m_MarkerData + 36 + (index * 3);
  IC.Ssize = *p++;
  IC.XRsize = *p++;
  IC.YRsize = *p;
}

//
void
ASDCP::JP2K::Accessor::SIZ::Dump(FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "SIZ: \n");
  fprintf(stream, "  Rsize: %hu\n", Rsize());
  fprintf(stream, "  Xsize: %u\n",  Xsize());
  fprintf(stream, "  Ysize: %u\n",  Ysize());
  fprintf(stream, " XOsize: %u\n",  XOsize());
  fprintf(stream, " YOsize: %u\n",  YOsize());
  fprintf(stream, " XTsize: %u\n",  XTsize());
  fprintf(stream, " YTsize: %u\n",  YTsize());
  fprintf(stream, "XTOsize: %u\n",  XTOsize());
  fprintf(stream, "YTOsize: %u\n",  YTOsize());
  fprintf(stream, "  Csize: %u\n",  Csize());

  if ( Csize() > 0 )
    {
      fprintf(stream, "Components\n");

      for ( ui32_t i = 0; i < Csize(); i++ )
	{
	  ImageComponent_t TmpComp;
	  ReadComponent(i, TmpComp);
	  fprintf(stream, "%u: ", i);
	  fprintf(stream, "%u, %u, %u\n", TmpComp.Ssize, TmpComp.XRsize, TmpComp.YRsize);
	}
    }
}

//
void
ASDCP::JP2K::Accessor::COM::Dump(FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  if ( IsText() )
    {
      char* t_str = (char*)malloc(CommentSize() + 1);
      assert( t_str != 0 );
      ui32_t cs = CommentSize();
      memcpy(t_str, CommentData(), cs);
      t_str[cs] = 0;
      fprintf(stream, "COM:%s\n", t_str);
    }
  else
    {
      fprintf(stream, "COM:\n");
      Kumu::hexdump(CommentData(), CommentSize(), stream);
    }
}


//-------------------------------------------------------------------------------------------------------
//


//
void
ASDCP::JP2K::Marker::Dump(FILE* stream) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "Marker%s 0x%04x: %s", (m_IsSegment ? " segment" : ""), m_Type, GetMarkerString(m_Type));  

  if ( m_IsSegment )
    fprintf(stream, ", 0x%0x bytes", m_DataSize);

  fputc('\n', stream);
}

//
const char*
ASDCP::JP2K::GetMarkerString(Marker_t m)
{
  switch ( m )
    {
    case MRK_NIL: return "NIL"; break;
    case MRK_SOC: return "SOC: Start of codestream"; break;
    case MRK_SOT: return "SOT: Start of tile-part"; break;
    case MRK_SOD: return "SOD: Start of data"; break;
    case MRK_EOC: return "EOC: End of codestream"; break;
    case MRK_SIZ: return "SIZ: Image and tile size"; break;
    case MRK_COD: return "COD: Coding style default"; break;
    case MRK_COC: return "COC: Coding style component"; break;
    case MRK_RGN: return "RGN: Region of interest"; break;
    case MRK_QCD: return "QCD: Quantization default"; break;
    case MRK_QCC: return "QCC: Quantization component"; break;
    case MRK_POC: return "POC: Progression order change"; break;
    case MRK_TLM: return "TLM: Tile-part lengths"; break;
    case MRK_PLM: return "PLM: Packet length, main header"; break;
    case MRK_PLT: return "PLT: Packet length, tile-part header"; break;
    case MRK_PPM: return "PPM: Packed packet headers, main header"; break;
    case MRK_PPT: return "PPT: Packed packet headers, tile-part header"; break;
    case MRK_SOP: return "SOP: Start of packet"; break;
    case MRK_EPH: return "EPH: End of packet header"; break;
    case MRK_CRG: return "CRG: Component registration"; break;
    case MRK_COM: return "COM: Comment"; break;
    }

  return "Unknown marker code";
}

//
// end JP2K.cpp
//
