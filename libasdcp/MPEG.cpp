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
/*! \file    MPEG.cpp
    \version $Id: MPEG.cpp,v 1.4 2009/04/09 19:16:49 msheby Exp $
    \brief   MPEG2 VES parser
*/

#include <MPEG.h>
#include <KM_log.h>
using Kumu::DefaultLogSink;

// walk a buffer stopping at the end of the buffer or the end of a VES
// start code '00 00 01'.  If successful, returns address of first byte
// of start code
ASDCP::Result_t
ASDCP::MPEG2::FindVESStartCode(const byte_t* buf, ui32_t buf_len, StartCode_t* sc, const byte_t** new_pos)
{
  ASDCP_TEST_NULL(buf);
  ASDCP_TEST_NULL(new_pos);

  ui32_t zero_i = 0;
  const byte_t* p = buf;
  const byte_t* end_p = buf + buf_len;

  for ( ; p < end_p; p++ )
    {
      if ( *p == 0 )
	zero_i++;

      else if ( *p == 1 && zero_i > 1 )
	{
	  // 2 or more 0 bytes followed by a 1, start code is next
	  if ( ++p == end_p )
	    return RESULT_FAIL;

	  *new_pos = p - 3;
	  *sc = (StartCode_t)*p;
	  return RESULT_OK;
	}
      else
	zero_i = 0;
    }

  *new_pos = buf + buf_len;
  return  RESULT_FAIL;
}


//------------------------------------------------------------------------------------------

//
ASDCP::Rational
ASDCP::MPEG2::Accessor::Sequence::AspectRatio()
{
  switch ( m_p[3] & 0xf0 )
    {
    case 0x10: return Rational(1,1);
    case 0x20: return Rational(4,3);
    case 0x30: return Rational(16,9);
    case 0x40: return Rational(221,100);
    }

  DefaultLogSink().Error("Unknown AspectRatio value: %02x\n", m_p[3]);
  return Rational(0,0);
}

//------------------------------------------------------------------------------------------

enum State_t {
    ST_IDLE,
    ST_START_HEADER,
    ST_IN_HEADER,
};


//
class ASDCP::MPEG2::VESParser::h__StreamState
{
public:
  State_t m_State;
  h__StreamState() : m_State(ST_IDLE) {}
  ~h__StreamState() {}

  void Goto_START_HEADER() { m_State = ST_START_HEADER; }
  void Goto_IN_HEADER()    { m_State = ST_IN_HEADER; }
  void Goto_IDLE()         { m_State = ST_IDLE; }
  bool Test_IDLE()         { return m_State == ST_IDLE; }
  bool Test_START_HEADER() { return m_State == ST_START_HEADER; }
  bool Test_IN_HEADER()    { return m_State == ST_IN_HEADER; }
};

//------------------------------------------------------------------------------------------


ASDCP::MPEG2::VESParser::VESParser() :
  m_Delegate(0), m_HBufLen(0), m_ZeroCount(0)
{
  m_State = new h__StreamState;
}

ASDCP::MPEG2::VESParser::~VESParser()
{
}


//
void
ASDCP::MPEG2::VESParser::SetDelegate(VESParserDelegate* Delegate)
{
  m_Delegate = Delegate;
}

//
void
ASDCP::MPEG2::VESParser::Reset()
{
  m_State->Goto_IDLE();
  m_HBufLen = 0;
  m_ZeroCount = 0;
}

//
ASDCP::Result_t
ASDCP::MPEG2::VESParser::Parse(const byte_t* buf, ui32_t buf_len)
{
  ASDCP_TEST_NULL(buf);
  ASDCP_TEST_NULL(m_Delegate);

  Result_t result = RESULT_OK;
  register const byte_t* end_p = buf + buf_len;
  register const byte_t* run_pos = buf; // track runs of uninteresting data using a position and count
  register ui32_t  run_len = 0;

  // search for MPEG2 headers
  // copy interesting data to a buffer and pass to delegate for processing
  for ( register const byte_t* p = buf; p < end_p; p++ )
    {
      if ( m_State->Test_IN_HEADER() )
	{
	  assert(run_len==0);
	  m_HBuf[m_HBufLen++] = *p;
	  assert(m_HBufLen < VESHeaderBufSize);
	}
      else
	{
	  run_len++;
	}

      if ( m_State->Test_START_HEADER() ) // *p is a start code
	{
	  if ( m_HBufLen == 0) // not already collecting a header
	    {
	      m_HBuf[0] = m_HBuf[1] = 0; m_HBuf[2] = 1; m_HBuf[3] = *p;

	      // is this one we want?
	      if ( *p == PIC_START || *p == SEQ_START || *p == EXT_START || *p == GOP_START )
		{
		  m_HBufLen = 4;
		  m_State->Goto_IN_HEADER();

		  switch ( run_len )
		    {
		    case 1: // we suppressed writing 001 when exiting from the last call
		    case 4: // we have exactly 001x
		      break;
		    case 2: // we have 1x
		    case 3: // we have 01x
		      m_Delegate->Data(this, run_pos, (run_len == 2 ? -2 : -1));
		      break;

		    default:
		      m_Delegate->Data(this, run_pos, run_len - 4);
		    }

		  run_len = 0;
		}
	      else
		{
		  m_State->Goto_IDLE();

		  if ( run_len == 1 ) // did we suppress writing 001 when exiting from the last call?
		    {
		      m_Delegate->Data(this, m_HBuf, 4);
		      run_len = 0;
		    }
		}
	    }
	  else // currently collecting a header, requires a flush before handling
	    {
	      m_HBufLen -= 3; // remove the current partial start code

	      // let the delegate handle the header
	      switch( m_HBuf[3] )
		{
		case PIC_START:   result = m_Delegate->Picture(this, m_HBuf, m_HBufLen);   break;	  
		case EXT_START:   result = m_Delegate->Extension(this, m_HBuf, m_HBufLen); break;
		case SEQ_START:   result = m_Delegate->Sequence(this, m_HBuf, m_HBufLen);  break;
		case GOP_START:   result = m_Delegate->GOP(this, m_HBuf, m_HBufLen);       break;

		default:
		  DefaultLogSink().Error("Unexpected start code: %02x at byte %u\n",
					 m_HBuf[3], (ui32_t)(p - buf));
		  result = RESULT_RAW_FORMAT;
		}

	      // Parser handlers return RESULT_FALSE to teriminate without error
	      if ( result != RESULT_OK )
		{
		  m_State->Goto_IDLE();
		  return result;
		}
	      
	      m_HBuf[0] = m_HBuf[1] = 0; m_HBuf[2] = 1; m_HBuf[3] = *p; // 001x
	      run_len = 0;

	      // is this a header we want?
	      if ( *p == PIC_START || *p == SEQ_START || *p == EXT_START || *p == GOP_START )
		{
		  m_HBufLen = 4;
		  m_State->Goto_IN_HEADER();
		}
	      else
		{
		  m_HBufLen = 0;
		  m_State->Goto_IDLE();

		  if ( *p >= FIRST_SLICE && *p <= LAST_SLICE )
		    {
		      result = m_Delegate->Slice(this, *p);

		      if ( result != RESULT_OK )
			return result;
		    }

		  m_Delegate->Data(this, m_HBuf, 4);
		  run_pos = p+1;
		}
	    }
	}
      else if ( *p == 0 )
	{
	  m_ZeroCount++;
	}
      else
	{
	  if ( *p == 1 && m_ZeroCount > 1 )
	    m_State->Goto_START_HEADER();

	  m_ZeroCount = 0;
	}
    }

  if ( run_len > 0 )
    {
      if ( m_State->Test_START_HEADER() && run_len != 0 )
	{
	  assert(run_len > 2);
	  run_len -= 3;
	}

      // flush the current run
      m_Delegate->Data(this, run_pos, run_len);
    }

  return RESULT_OK;
}


//
// end MPEG.cpp
//
