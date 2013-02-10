/*
Copyright (c) 2006-2011, John Hurst
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
  /*! \file  KM_memio.h
    \version $Id: KM_memio.h,v 1.9 2011/03/07 06:46:36 jhurst Exp $
    \brief   abstraction for byte-oriented conversion of integers and objects
  */

#ifndef _KM_MEMIO_H_
#define _KM_MEMIO_H_

#include <KM_platform.h>
#include <string>
#include <cstring>

namespace Kumu
{
  class ByteString;

  //
  class MemIOWriter
    {
      KM_NO_COPY_CONSTRUCT(MemIOWriter);
      MemIOWriter();
      
    protected:
      byte_t* m_p;
      ui32_t  m_capacity;
      ui32_t  m_size;

    public:
      MemIOWriter(byte_t* p, ui32_t c) : m_p(p), m_capacity(c), m_size(0) {
	assert(m_p); assert(m_capacity);
      }

      MemIOWriter(ByteString* Buf);
      ~MemIOWriter() {}

      inline void    Reset() { m_size = 0; }
      inline byte_t* Data() { return m_p; }
      inline const byte_t* RoData() const { return m_p; }
      inline byte_t* CurrentData() { return m_p + m_size; }
      inline ui32_t  Length() const { return m_size; }
      inline ui32_t  Remainder() const { return m_capacity - m_size; }

      inline bool AddOffset(ui32_t offset) {
	if ( ( m_size + offset ) > m_capacity )
	  return false;

	m_size += offset;
	return true;

      }

      inline bool WriteRaw(const byte_t* p, ui32_t buf_len) {
	if ( ( m_size + buf_len ) > m_capacity )
	  return false;

	memcpy(m_p + m_size, p, buf_len);
	m_size += buf_len;
	return true;
      }

      bool WriteBER(ui64_t i, ui32_t ber_len);

      inline bool WriteUi8(ui8_t i) {
	if ( ( m_size + 1 ) > m_capacity )
	  return false;

	*(m_p + m_size) = i;
	m_size++;
	return true;
      }

      inline bool WriteUi16BE(ui16_t i) {
	if ( ( m_size + sizeof(ui16_t) ) > m_capacity )
	  return false;
	
	i2p<ui16_t>(KM_i16_BE(i), m_p + m_size);
	m_size += sizeof(ui16_t);
	return true;
      }

      inline bool WriteUi32BE(ui32_t i) {
	if ( ( m_size + sizeof(ui32_t) ) > m_capacity )
	  return false;
	
	i2p<ui32_t>(KM_i32_BE(i), m_p + m_size);
	m_size += sizeof(ui32_t);
	return true;
      }

      inline bool WriteUi64BE(ui64_t i) {
	if ( ( m_size + sizeof(ui64_t) ) > m_capacity )
	  return false;
	
	i2p<ui64_t>(KM_i64_BE(i), m_p + m_size);
	m_size += sizeof(ui64_t);
	return true;
      }

      inline bool WriteString(const std::string& str) {
	ui32_t len = static_cast<ui32_t>(str.length());
	if ( ! WriteUi32BE(len) ) return false;
	if ( ! WriteRaw((const byte_t*)str.c_str(), len) ) return false;
	return true;
      }
    };

  //
  class MemIOReader
    {
      KM_NO_COPY_CONSTRUCT(MemIOReader);
      MemIOReader();
      
    protected:
      const byte_t* m_p;
      ui32_t  m_capacity;
      ui32_t  m_size; // this is sort of a misnomer, when we are reading it measures offset

    public:
      MemIOReader(const byte_t* p, ui32_t c) :
	m_p(p), m_capacity(c), m_size(0) {
	assert(m_p); assert(m_capacity);
      }

      MemIOReader(const ByteString* Buf);
      ~MemIOReader() {}

      inline void          Reset() { m_size = 0; }
      inline const byte_t* Data() const { return m_p; }
      inline const byte_t* CurrentData() const { return m_p + m_size; }
      inline ui32_t        Offset() const { return m_size; }
      inline ui32_t        Remainder() const { return m_capacity - m_size; }

      inline bool SkipOffset(ui32_t offset) {
	if ( ( m_size + offset ) > m_capacity )
	  return false;

	m_size += offset;
	return true;
      }

      inline bool ReadRaw(byte_t* p, ui32_t buf_len) {
	if ( ( m_size + buf_len ) > m_capacity )
	  return false;

	memcpy(p, m_p + m_size, buf_len);
	m_size += buf_len;
	return true;
      }

      bool ReadBER(ui64_t* i, ui32_t* ber_len);

      inline bool ReadUi8(ui8_t* i) {
	assert(i);
	if ( ( m_size + 1 ) > m_capacity )
	  return false;

	*i = *(m_p + m_size);
	m_size++;
	return true;
      }

      inline bool ReadUi16BE(ui16_t* i) {
	assert(i);
	if ( ( m_size + sizeof(ui16_t) ) > m_capacity )
	  return false;

	*i = KM_i16_BE(cp2i<ui16_t>(m_p + m_size));
	m_size += sizeof(ui16_t);
	return true;
      }

      inline bool ReadUi32BE(ui32_t* i) {
	assert(i);
	if ( ( m_size + sizeof(ui32_t) ) > m_capacity )
	  return false;

	*i = KM_i32_BE(cp2i<ui32_t>(m_p + m_size));
	m_size += sizeof(ui32_t);
	return true;
      }

      inline bool ReadUi64BE(ui64_t* i) {
	assert(i);
	if ( ( m_size + sizeof(ui64_t) ) > m_capacity )
	  return false;

	*i = KM_i64_BE(cp2i<ui64_t>(m_p + m_size));
	m_size += sizeof(ui64_t);
	return true;
      }

      inline bool ReadString(std::string& str)
      {
	ui32_t str_length;
	if ( ! ReadUi32BE(&str_length) ) return false;
	if ( ( m_size + str_length ) > m_capacity ) return false;
	str.assign((const char*)CurrentData(), str_length);
	if ( ! SkipOffset(str_length) ) return false;
	return true;
      }
    };

  //
  inline bool
  UnarchiveString(MemIOReader& Reader, std::string& str) {
    return Reader.ReadString(str);
  }

  //
  inline bool
  ArchiveString(MemIOWriter& Writer, const std::string& str)
  {
    return Writer.WriteString(str);
  }


} // namespace Kumu

#endif // _KM_MEMIO_H_

//
// end KM_memio.h
//
