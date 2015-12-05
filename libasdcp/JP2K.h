/*
Copyright (c) 2005-2014, John Hurst
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
/*! \file    JP2K.h
    \version $Id: JP2K.h,v 1.6 2014/06/02 22:07:51 jhurst Exp $
    \brief   JPEG 2000 constants and data structures

    This is not a complete enumeration of all things JP2K.  There is just enough here to
    support parsing picture metadata from a codestream header.
*/

#ifndef _JP2K_H_
#define _JP2K_H_

// AS_DCP.h is included only for it's base type definitions.
#include <KM_platform.h>
#include <KM_util.h>
#include <AS_DCP.h>
#include <assert.h>

namespace ASDCP
{
namespace JP2K
{
  const byte_t Magic[] = {0xff, 0x4f, 0xff};

  enum Marker_t
    {
      MRK_NIL = 0,
      MRK_SOC = 0xff4f, // Start of codestream
      MRK_SOT = 0xff90, // Start of tile-part
      MRK_SOD = 0xff93, // Start of data
      MRK_EOC = 0xffd9, // End of codestream
      MRK_SIZ = 0xff51, // Image and tile size
      MRK_COD = 0xff52, // Coding style default
      MRK_COC = 0xff53, // Coding style component
      MRK_RGN = 0xff5e, // Region of interest
      MRK_QCD = 0xff5c, // Quantization default
      MRK_QCC = 0xff5d, // Quantization component
      MRK_POC = 0xff5f, // Progression order change
      MRK_TLM = 0xff55, // Tile-part lengths
      MRK_PLM = 0xff57, // Packet length, main header
      MRK_PLT = 0xff58, // Packet length, tile-part header
      MRK_PPM = 0xff60, // Packed packet headers, main header
      MRK_PPT = 0xff61, // Packed packet headers, tile-part header
      MRK_SOP = 0xff91, // Start of packet
      MRK_EPH = 0xff92, // End of packet header
      MRK_CRG = 0xff63, // Component registration
      MRK_COM = 0xff64, // Comment
    };

  const char* GetMarkerString(Marker_t m);

  //
  class Marker
    {
      KM_NO_COPY_CONSTRUCT(Marker);

    public:
      Marker_t m_Type;
      bool     m_IsSegment;
      ui32_t   m_DataSize;
      const byte_t* m_Data;

      Marker() : m_Type(MRK_NIL), m_IsSegment(false), m_DataSize(0), m_Data(0) {}
      ~Marker() {}

      void Dump(FILE* stream = 0) const;
    };

  //
  ASDCP::Result_t GetNextMarker(const byte_t**, Marker&);

  // accessor objects for marker segments
  namespace Accessor
    {
      // image size
      class SIZ
	{
	  const byte_t* m_MarkerData;
	  KM_NO_COPY_CONSTRUCT(SIZ);
          SIZ();

        public:
          SIZ(const Marker& M)
            {
	      assert(M.m_Type == MRK_SIZ);
	      m_MarkerData = M.m_Data;
	    }

	  ~SIZ() {}

	  inline ui16_t Rsize()   const { return KM_i16_BE(*(ui16_t*)m_MarkerData); }
	  inline ui32_t Xsize()   const { return KM_i32_BE(*(ui32_t*)(m_MarkerData + 2)); }
	  inline ui32_t Ysize()   const { return KM_i32_BE(*(ui32_t*)(m_MarkerData + 6)); }
	  inline ui32_t XOsize()  const { return KM_i32_BE(*(ui32_t*)(m_MarkerData + 10)); }
	  inline ui32_t YOsize()  const { return KM_i32_BE(*(ui32_t*)(m_MarkerData + 14)); }
	  inline ui32_t XTsize()  const { return KM_i32_BE(*(ui32_t*)(m_MarkerData + 18)); }
	  inline ui32_t YTsize()  const { return KM_i32_BE(*(ui32_t*)(m_MarkerData + 22)); }
	  inline ui32_t XTOsize() const { return KM_i32_BE(*(ui32_t*)(m_MarkerData + 26)); }
	  inline ui32_t YTOsize() const { return KM_i32_BE(*(ui32_t*)(m_MarkerData + 30)); }
	  inline ui16_t Csize()   const { return KM_i16_BE(*(ui16_t*)(m_MarkerData + 34)); }
	  void ReadComponent(const ui32_t index, ImageComponent_t& IC) const;
	  void Dump(FILE* stream = 0) const;
	};

      const int SGcodOFST = 1;
      const int SPcodOFST = 5;

      // coding style
      class COD
	{
	  const byte_t* m_MarkerData;

	  KM_NO_COPY_CONSTRUCT(COD);
	  COD();

	public:
	  COD(const Marker& M)
	    {
	      assert(M.m_Type == MRK_COD);
	      m_MarkerData = M.m_Data;
	    }

	  ~COD() {}
	  
	  inline ui8_t  ProgOrder()        const { return *(m_MarkerData + SGcodOFST ); }
	  inline ui16_t Layers()           const { return KM_i16_BE(*(ui16_t*)(m_MarkerData + SGcodOFST + 1));}
	  inline ui8_t  DecompLevels()     const { return *(m_MarkerData + SPcodOFST); }
	  inline ui8_t  CodeBlockWidth()   const { return *(m_MarkerData + SPcodOFST + 1) + 2; }
	  inline ui8_t  CodeBlockHeight()  const { return *(m_MarkerData + SPcodOFST + 2) + 2; }
	  inline ui8_t  CodeBlockStyle()   const { return *(m_MarkerData + SPcodOFST + 3); }
	  inline ui8_t  Transformation()   const { return *(m_MarkerData + SPcodOFST + 4); }

	  void Dump(FILE* stream = 0) const;
	};

      const int SqcdOFST = 1;
      const int SPqcdOFST = 2;

      enum QuantizationType_t
      {
	QT_NONE,
	QT_DERIVED,
	QT_EXP
      };

      const char* GetQuantizationTypeString(const QuantizationType_t m);

      // Quantization default
      class QCD
        {
	  const byte_t* m_MarkerData;
	  ui32_t m_DataSize;

	  KM_NO_COPY_CONSTRUCT(QCD);
	  QCD();

	public:
	  QCD(const Marker& M)
	    {
	      assert(M.m_Type == MRK_QCD);
	      m_MarkerData = M.m_Data + 2;
	      m_DataSize = M.m_DataSize - 2;
	    }

	  ~QCD() {}
	  inline QuantizationType_t QuantizationType() const { return static_cast<QuantizationType_t>(*(m_MarkerData + SqcdOFST) & 0x03); }
	  inline ui8_t  GuardBits() const { return (*(m_MarkerData + SqcdOFST) & 0xe0) >> 5; }
	  void Dump(FILE* stream = 0) const;
	};

      // a comment
      class COM
	{
	  bool          m_IsText;
	  const byte_t* m_MarkerData;
	  ui32_t        m_DataSize;

	  KM_NO_COPY_CONSTRUCT(COM);
	  COM();

	public:
	  COM(const Marker& M)
	    {
	      assert(M.m_Type == MRK_COM);
	      m_IsText = M.m_Data[1] == 1;
	      m_MarkerData = M.m_Data + 2;
	      m_DataSize = M.m_DataSize - 2;
	    }

	  ~COM() {}
	  
	  inline bool IsText() const { return m_IsText; }
	  inline const byte_t* CommentData() const { return m_MarkerData; }
	  inline ui32_t CommentSize() const { return m_DataSize; }
	  void Dump(FILE* stream = 0) const;
	};
    } // namespace Accessor
} // namespace JP2K
} // namespace ASDCP

#endif // _JP2K_H_

//
// end JP2K.h
//
