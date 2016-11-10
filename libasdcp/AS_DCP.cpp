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
/*! \file    AS_DCP.cpp
    \version $Id: AS_DCP.cpp,v 1.7 2014/09/21 13:27:43 jhurst Exp $       
    \brief   AS-DCP library, misc classes and subroutines
*/

#include "AS_DCP_internal.h"
#include <assert.h>

const char*
ASDCP::Version()
{
  return PACKAGE_VERSION;
}



//------------------------------------------------------------------------------------------
//

// Encodes a rational number as a string having a single delimiter character between
// numerator and denominator.  Retuns the buffer pointer to allow convenient in-line use.
const char*
ASDCP::EncodeRational(const Rational& rational, char* str_buf, ui32_t buf_len, char delimiter)
{
  assert(str_buf);
  snprintf(str_buf, buf_len, "%u%c%u", rational.Numerator, delimiter, rational.Denominator);
  return str_buf;
}

// Decodes a rational number havng a single non-digit delimiter character between
// the numerator and denominator.  Returns false if the string does not contain
// the expected syntax.
bool
ASDCP::DecodeRational(const char* str_rational, Rational& rational)
{
  assert(str_rational);
  rational.Numerator = strtol(str_rational, 0, 10);

  const char* p = str_rational;
  while ( *p && isdigit(*p) )
    {
      ++p;
    }

  if ( p[0] == 0 || p[1] == 0 )
    {
      return false;
    }

  ++p;
  rational.Denominator = strtol(p, 0, 10);
  return  true;
}


//------------------------------------------------------------------------------------------
//
// frame buffer base class implementation

ASDCP::FrameBuffer::FrameBuffer() :
  m_Data(0), m_Capacity(0), m_OwnMem(false), m_Size(0),
  m_FrameNumber(0), m_SourceLength(0), m_PlaintextOffset(0)
{
}

ASDCP::FrameBuffer::~FrameBuffer()
{
  if ( m_OwnMem && m_Data != 0 )
    free(m_Data);
}

// Instructs the object to use an externally allocated buffer. The external
// buffer will not be cleaned up by the frame buffer when it is destroyed.
// Call with (0,0) to revert to internally allocated buffer.
// Returns error if the buf_addr argument is NULL and either buf_size is
// non-zero or internally allocated memory is in use.
ASDCP::Result_t
ASDCP::FrameBuffer::SetData(byte_t* buf_addr, ui32_t buf_size)
{
  // if buf_addr is null and we have an external memory reference,
  // drop the reference and place the object in the initialized-
  // but-no-buffer-allocated state
  if ( buf_addr == 0 )
    {
      if ( buf_size > 0 || m_OwnMem )
	return RESULT_PTR;

      m_OwnMem = false;
      m_Capacity = m_Size = 0;
      m_Data = 0;
      return RESULT_OK;
    }

  if ( m_OwnMem && m_Data != 0 )
    free(m_Data);

  m_OwnMem = false;
  m_Capacity = buf_size;
  m_Data = buf_addr;
  m_Size = 0;

  return RESULT_OK;
}

// Sets the size of the internally allocate buffer. Returns RESULT_CAPEXTMEM
// if the object is using an externally allocated buffer via SetData();
// Resets content size to zero.
ASDCP::Result_t
ASDCP::FrameBuffer::Capacity(ui32_t cap_size)
{
  if ( ! m_OwnMem && m_Data != 0 )
    return RESULT_CAPEXTMEM; // cannot resize external memory

  if ( m_Capacity < cap_size )
    {
      if ( m_Data != 0 )
	{
	  assert(m_OwnMem);
	  free(m_Data);
	}

      m_Data = (byte_t*)malloc(cap_size);

      if ( m_Data == 0 )
	return RESULT_ALLOC;

      m_Capacity = cap_size;
      m_OwnMem = true;
      m_Size = 0;
    }

  return RESULT_OK;
}


//
// end AS_DCP.cpp
//
