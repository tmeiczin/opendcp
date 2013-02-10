/*
Copyright (c) 2005-2011, John Hurst
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
/*! \file    MPEG.h
    \version $Id: MPEG.h,v 1.5 2011/08/30 17:04:25 jhurst Exp $
    \brief   MPEG2 VES parser interface
*/

#ifndef _MPEG_H_
#define _MPEG_H_

#include <KM_platform.h>
#include "AS_DCP.h"
#include <stdio.h>
#include <assert.h>


namespace ASDCP
{
  namespace MPEG2
    {
      //
      enum StartCode_t {
	PIC_START   = 0x00,
	SEQ_START   = 0xb3,
	EXT_START   = 0xb5,
	GOP_START   = 0xb8,
	FIRST_SLICE = 0x01,
	LAST_SLICE  = 0xaf,
	INVALID     = 0xff
      };

      //
      enum RateCode_t {
	RATE_23_976 = 0x01,
	RATE_24     = 0x02,
	RATE_25     = 0x03,
	RATE_29_97  = 0x04,
	RATE_30     = 0x05
      };

      //
      enum ExtCode_t {
	EXT_SEQ = 0x01
      };

      //------------------------------------------------------------------------------------------
      // VES Parser

      // find the location in the buffer of the next VES packet, returns RESULT_FAIL
      // if no start code is found
      Result_t FindVESStartCode(const byte_t* buf, ui32_t buf_len, StartCode_t* sc, const byte_t** new_pos);

      // return the extension code of an extension header
      inline ExtCode_t ParseExtensionCode(const byte_t* buf)
	{
	  assert(buf);
	  return (ExtCode_t)(buf[4] >> 4);
	}

      //
      class VESParserDelegate; // the delegate is declared later
      const ui32_t VESHeaderBufSize = 1024*32; // should be larger than any expected header

      // MPEG VES parser class - call Parse() as many times as you want with buffers
      // of any size. State is maintained between calls. When complete headers are
      // available for examination, the respective delegate method will be called.
      // All other data is given to the delegate's Data() method.
      class VESParser
	{
	  class h__StreamState;
	  Kumu::mem_ptr<h__StreamState> m_State;
	  VESParserDelegate*      m_Delegate;

	  ui32_t         m_HBufLen; // temp space for partial header contents
	  byte_t         m_HBuf[VESHeaderBufSize];
	  ui32_t         m_ZeroCount;
	  bool           m_Partial;

	  ASDCP_NO_COPY_CONSTRUCT(VESParser);

	public:
	  VESParser();
	  ~VESParser();

	  void     SetDelegate(VESParserDelegate*); // you must call this before Parse()
	  Result_t Parse(const byte_t*, ui32_t);  // call repeatedly
	  void     Reset(); // resets the internal state machine and counters, return to the top of the file
	};

      // Parser Event Delegate Interface
      //
      // Create a concrete subclass and give it to the parser by calling SetDelegate().
      // The respective method will be called when a header of the named type is found.
      // Handler methods should return RESULT_OK to continue processing or RESULT_FALSE
      // to terminate parsing without signaling an error.
      //
      class VESParserDelegate
	{
	public:
	  virtual ~VESParserDelegate() {}

	  // header handlers
	  virtual Result_t Picture(VESParser* Caller, const byte_t* header_buf, ui32_t header_len) = 0;
	  virtual Result_t Extension(VESParser*, const byte_t*, ui32_t) = 0;
	  virtual Result_t Sequence(VESParser*, const byte_t*, ui32_t) = 0;
	  virtual Result_t GOP(VESParser*, const byte_t*, ui32_t) = 0;

	  // this is not a header handler, it is a signal that actual picture data
	  // has started. All Slice data is reported via the Data() method.
	  virtual Result_t Slice(VESParser*, byte_t slice_id) = 0;

	  // Any data not given to the header handlers above is reported here
	  // This method may be called with a value of -1 or -2. This will happen
	  // when processing a start code that has one or two leading zeros
	  // in the preceding buffer
	  virtual Result_t Data(VESParser*, const byte_t*, i32_t) = 0;
	};


      //------------------------------------------------------------------------------------------
      // VES header accessor objects
      //
      // For use within parser delegate methods. The constructor expects a pointer to a buffer
      // containing two zero bytes, a one byte, a start code and some number of header bytes.
      // They are not documented further as it is hoped that they are self-explanatory.

      //
      namespace Accessor
	{
	  // decoding tables
	  static i16_t FrameRateLUT[] = { 0, 24, 24, 25, 30, 30, 50, 60, 60};
	  static bool  PulldownLUT[]  = { false,  true,  false,  false,  true,  false,  false,  true,  false};

	  //
	  class Sequence
	    {
	      const byte_t* m_p;
	      ASDCP_NO_COPY_CONSTRUCT(Sequence);

	    public:
	      Sequence(const byte_t* p) { assert(p); m_p = p + 4; }
	      inline ui16_t      HorizontalSize() { return (ui16_t)( ( m_p[0] << 4 ) | ( m_p[1] >> 4 ) ); }
	      inline ui16_t      VerticalSize()   { return (ui16_t)( ( ( m_p[1] & 0x0f ) << 8 ) | m_p[2] ); }
	      inline RateCode_t  RateCode()       { return (RateCode_t)( m_p[3] & 0x0f ); }
	      inline ui16_t      FrameRate()      { return FrameRateLUT[RateCode()]; }
	      inline bool        Pulldown()       { return PulldownLUT[RateCode()] != 0; }
	      inline i32_t       BitRate() {
		return ( ( (i32_t)m_p[4] << 10 ) + ( (i32_t)m_p[5] << 2 ) + ( m_p[6] >> 6 ) ) * 400;
	      }

	      Rational   AspectRatio();
	    };

	  //
	  class SequenceEx // tension
	    {
	      const byte_t* m_p;
	      ASDCP_NO_COPY_CONSTRUCT(SequenceEx);

	    public:
	      SequenceEx(const byte_t* p)
		{
		  assert(p);
		  assert(ParseExtensionCode(p) == EXT_SEQ);
		  m_p = p + 4;
		}

	      inline ui16_t      ProfileAndLevel()   { return ( m_p[0] << 4) | ( m_p[1] >> 4 ); }
	      inline ui8_t       ChromaFormat()      { return ( m_p[1] >> 1 ) & 0x03; }
	      inline bool        Progressive()       { return ( ( m_p[1] >> 3 ) & 0x01 ) > 0; }
	      inline ui32_t      HorizontalSizeExt() {
		return ( ( m_p[1] & 0x01 ) << 13 ) | ( ( m_p[2] & 0x80 ) << 5 );
	      }
	      inline ui32_t      VerticalSizeExt()   { return ( m_p[2] & 0x60 ) << 7; }
	      inline ui32_t      BitRateExt()        {
		return ( ( m_p[2] & 0x1f ) << 25 ) | ( ( m_p[3] & 0xfe ) << 17 );
	      }

	      inline bool        LowDelay()          { return ( m_p[5] & 0x80 ) > 0; }
	    };

	  //
	  class GOP
	    {
	      const byte_t* m_p;
	      ASDCP_NO_COPY_CONSTRUCT(GOP);

	    public:
	      GOP(const byte_t* p) { assert(p); m_p = p + 4; }
	      inline bool        Closed() { return ( ( m_p[3] ) >> 6 ) & 0x01; }
	    };

	  //
	  class Picture
	    {
	      const byte_t*  m_p;
	      ASDCP_NO_COPY_CONSTRUCT(Picture);
	      
	    public:
	      Picture(const byte_t* p) { assert(p); m_p = p + 4; }
	      inline i16_t       TemporalRef() {
		return ( (i16_t)( m_p[0] << 2 ) ) | ( ( (i16_t)m_p[1] & 0x00c0 ) >> 6 );
	      }

	      inline FrameType_t FrameType()   {
		return (FrameType_t)( ( m_p[1] & 0x38 ) >> 3 );
	      }
	    };
	  
	} // namespace Accessor

    } // namespace MPEG2

} // namespace ASDCP

#endif // _MPEG_H_

//
// end MPEG.h
//
