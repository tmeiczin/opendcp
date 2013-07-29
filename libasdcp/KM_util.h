/*
Copyright (c) 2005-2012, John Hurst
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
  /*! \file    KM_util.h
    \version $Id: KM_util.h,v 1.34.2.1 2013/07/02 16:54:23 mikey Exp $
    \brief   Utility functions
  */

#ifndef _KM_UTIL_H_
#define _KM_UTIL_H_

#include <KM_memio.h>
#include <KM_error.h>
#include <KM_tai.h>
#include <string.h>
#include <string>
#include <list>

namespace Kumu
{
  // The version number declaration and explanation are in ../configure.ac
  const char* Version();

  // a class that represents the string form of a value
  template <class T, int SIZE = 16>
    class IntPrinter : public std::string
  {
    KM_NO_COPY_CONSTRUCT(IntPrinter);
    IntPrinter();

    protected:
    const char* m_format;
    char m_strbuf[SIZE];
    
    public:
    IntPrinter(const char* format, T value) {
      assert(format);
      m_format = format;
      snprintf(m_strbuf, SIZE, m_format, value);
    }

    inline operator const char*() { return m_strbuf; }
    inline const char* c_str() { return m_strbuf; }
    inline const char* set_value(T value) {
      snprintf(m_strbuf, SIZE, m_format, value);
      return m_strbuf;
    }
  };

  struct i8Printer : public IntPrinter<i8_t> {
    i8Printer(i8_t value) : IntPrinter<i8_t>("%hd", value) {}
  };

  struct ui8Printer : public IntPrinter<ui8_t> {
    ui8Printer(ui8_t value) : IntPrinter<ui8_t>("%hu", value) {}
  };

  struct i16Printer : public IntPrinter<i16_t> {
    i16Printer(i16_t value) : IntPrinter<i16_t>("%hd", value) {}
  };

  struct ui16Printer : public IntPrinter<ui16_t> {
    ui16Printer(ui16_t value) : IntPrinter<ui16_t>("%hu", value) {}
  };

  struct i32Printer : public IntPrinter<i32_t> {
    i32Printer(i32_t value) : IntPrinter<i32_t>("%d", value) {}
  };

  struct ui32Printer : public IntPrinter<ui32_t> {
    ui32Printer(ui32_t value) : IntPrinter<ui32_t>("%u", value) {}
  };

#ifdef KM_WIN32
  struct i64Printer : public IntPrinter<i64_t, 32> {
    i64Printer(i64_t value) : IntPrinter<i64_t, 32>("%I64d", value) {}
  };

  struct ui64Printer : public IntPrinter<ui64_t, 32> {
    ui64Printer(ui64_t value) : IntPrinter<ui64_t, 32>("%I64u", value) {}
  };
#else
  struct i64Printer : public IntPrinter<i64_t, 32> {
    i64Printer(i64_t value) : IntPrinter<i64_t, 32>("%qd", value) {}
  };

  struct ui64Printer : public IntPrinter<ui64_t, 32> {
    ui64Printer(ui64_t value) : IntPrinter<ui64_t, 32>("%qu", value) {}
  };
#endif

  // Convert NULL-terminated UTF-8 hexadecimal string to binary, returns 0 if
  // the binary buffer was large enough to hold the result. The output parameter
  // 'char_count' will contain the length of the converted string. If the output
  // buffer is too small or any of the pointer arguments are NULL, the subroutine
  // will return -1 and set 'char_count' to the required buffer size. No data will
  // be written to 'buf' if the subroutine fails.
  i32_t       hex2bin(const char* str, byte_t* buf, ui32_t buf_len, ui32_t* char_count);

  // Convert a binary string to NULL-terminated UTF-8 hexadecimal, returns the buffer
  // if the output buffer was large enough to hold the result. If the output buffer
  // is too small or any of the pointer arguments are NULL, the subroutine will
  // return 0.
  //
  const char* bin2hex(const byte_t* bin_buf, ui32_t bin_len, char* str_buf, ui32_t str_len);

  const char* bin2UUIDhex(const byte_t* bin_buf, ui32_t bin_len, char* str_buf, ui32_t str_len);

  // same as above for base64 text
  i32_t       base64decode(const char* str, byte_t* buf, ui32_t buf_len, ui32_t* char_count);
  const char* base64encode(const byte_t* bin_buf, ui32_t bin_len, char* str_buf, ui32_t str_len);

  // returns the length of a Base64 encoding of a buffer of the given length
  inline ui32_t base64_encode_length(ui32_t length) {
    while ( ( length % 3 ) != 0 )
      length++;

    return ( length / 3 ) * 4;
  }

  // print buffer contents to a stream as hexadecimal values in numbered
  // rows of 16-bytes each.
  //
  void hexdump(const byte_t* buf, ui32_t dump_len, FILE* stream = 0);

  // Return the length in bytes of a BER encoded value
  inline ui32_t BER_length(const byte_t* buf)
    {
      if ( buf == 0 || (*buf & 0xf0) != 0x80 )
	return 0;

      return (*buf & 0x0f) + 1;
    }

  // Return the BER length required to encode value. A return value of zero
  // indicates a value too large for this library.
  ui32_t get_BER_length_for_value(ui64_t valuse);

  // read a BER value
  bool read_BER(const byte_t* buf, ui64_t* val);

  // decode a ber value and compare it to a test value
  bool read_test_BER(byte_t **buf, ui64_t test_value);

  // create BER encoding of integer value
  bool write_BER(byte_t* buf, ui64_t val, ui32_t ber_len = 0);

  //----------------------------------------------------------------
  //

  // an abstract base class that objects implement to serialize state
  // to and from a binary stream.
  class IArchive
    {
    public:
      virtual ~IArchive(){}
      virtual bool   HasValue() const = 0;
      virtual ui32_t ArchiveLength() const = 0;
      virtual bool   Archive(MemIOWriter* Writer) const = 0;
      virtual bool   Unarchive(MemIOReader* Reader) = 0;
    };

  //
  template <class T>
  class ArchivableList : public std::list<T>, public IArchive
    {
    public:
      ArchivableList() {}
      virtual ~ArchivableList() {}

      bool HasValue() const { return ! this->empty(); }

      ui32_t ArchiveLength() const
      {
	ui32_t arch_size = sizeof(ui32_t);

	typename ArchivableList<T>::const_iterator i = this->begin();
	for ( ; i != this->end(); i++ )
	  arch_size += i->ArchiveLength();

	return arch_size;
      }

      bool Unarchive(Kumu::MemIOReader* Reader)
	{
	  if ( Reader == 0 ) return false;
	  ui32_t read_size = 0;
	  if ( ! Reader->ReadUi32BE(&read_size) ) return false;
	  for ( ui32_t i = 0; i < read_size; i++ )
	    {
	      T TmpTP;
	      if ( ! TmpTP.Unarchive(Reader) ) return false;
	      this->push_back(TmpTP);
	    }

	  return true;
	}

      bool Archive(Kumu::MemIOWriter* Writer) const
	{
	  if ( Writer == 0 ) return false;
	  if ( ! Writer->WriteUi32BE(static_cast<ui32_t>(this->size())) ) return false;
	  typename ArchivableList<T>::const_iterator i = this->begin();
	  for ( ; i != this->end(); i++ )
	    if ( ! i->Archive(Writer) ) return false;

	  return true;
	}
    };

  // archivable version of std::string

  //
  class ArchivableString : public std::string, public Kumu::IArchive
    {

    public:
      ArchivableString() {}
      ArchivableString(const char* sz) : std::string(sz) {}
      ArchivableString(const std::string& s) : std::string(s) {}
      virtual ~ArchivableString() {}

      bool   HasValue() const { return ! this->empty(); }
      ui32_t ArchiveLength() const { return sizeof(ui32_t) + static_cast<ui32_t>(this->size()); }

      bool   Archive(MemIOWriter* Writer) const {
	if ( Writer == 0 ) return false;
	return Writer->WriteString(*this);
      }

      bool   Unarchive(MemIOReader* Reader) {
	if ( Reader == 0 ) return false;
	return Reader->ReadString(*this);
      }
    };

  //
  typedef Kumu::ArchivableList<ArchivableString> StringList;

  //
  // the base of all identifier classes, Identifier is not usually used directly
  // see UUID and SymmetricKey below for more detail.
  //
  template <ui32_t SIZE>
    class Identifier : public IArchive
    {
    protected:
      bool   m_HasValue;
      byte_t m_Value[SIZE];

    public:
      Identifier() : m_HasValue(false) { memset(m_Value, 0, SIZE); }
      Identifier(const byte_t* value) : m_HasValue(true) { memcpy(m_Value, value, SIZE); }
      Identifier(const Identifier& rhs) : IArchive() {
	m_HasValue = rhs.m_HasValue;
	memcpy(m_Value, rhs.m_Value, SIZE);
      }

      virtual ~Identifier() {}

      const Identifier& operator=(const Identifier& rhs) {
	m_HasValue = rhs.m_HasValue;
	memcpy(m_Value, rhs.m_Value, SIZE);
        return *this;
      }

      inline void Set(const byte_t* value) { m_HasValue = true; memcpy(m_Value, value, SIZE); }
      inline void Reset() { m_HasValue = false; memset(m_Value, 0, SIZE); }
      inline const byte_t* Value() const { return m_Value; }
      inline ui32_t Size() const { return SIZE; }

      inline bool operator<(const Identifier& rhs) const {
	ui32_t test_size = xmin(rhs.Size(), SIZE);

	for ( ui32_t i = 0; i < test_size; i++ )
	  {
	    if ( m_Value[i] != rhs.m_Value[i] )
	      return m_Value[i] < rhs.m_Value[i];
	  }
	
	return false;
      }

      inline bool operator==(const Identifier& rhs) const {
	if ( rhs.Size() != SIZE ) return false;
	return ( memcmp(m_Value, rhs.m_Value, SIZE) == 0 );
      }

      inline bool operator!=(const Identifier& rhs) const {
	if ( rhs.Size() != SIZE ) return true;
	return ( memcmp(m_Value, rhs.m_Value, SIZE) != 0 );
      }

      inline bool DecodeHex(const char* str) {
	ui32_t char_count;
	m_HasValue = ( hex2bin(str, m_Value, SIZE, &char_count) == 0 );
	if ( m_HasValue && char_count != SIZE )
	  m_HasValue = false;
	return m_HasValue;
      }

      inline const char* EncodeHex(char* buf, ui32_t buf_len) const {
	return bin2hex(m_Value, SIZE, buf, buf_len);
      }

      inline const char* EncodeString(char* str_buf, ui32_t buf_len) const {
	return EncodeHex(str_buf, buf_len);
      }

      inline bool DecodeBase64(const char* str) {
	ui32_t char_count;
	m_HasValue = ( base64decode(str, m_Value, SIZE, &char_count) == 0 );
	if ( m_HasValue && char_count != SIZE )
	  m_HasValue = false;
	return m_HasValue;
      }

      inline const char* EncodeBase64(char* buf, ui32_t buf_len) const {
	return base64encode(m_Value, SIZE, buf, buf_len);
      }

      inline bool HasValue() const { return m_HasValue; }

      inline ui32_t ArchiveLength() const { return SIZE; }

      inline bool Unarchive(Kumu::MemIOReader* Reader) {
	m_HasValue = Reader->ReadRaw(m_Value, SIZE);
	return m_HasValue;
      }

      inline bool Archive(Kumu::MemIOWriter* Writer) const {
	return Writer->WriteRaw(m_Value, SIZE);
      }
    };


  // UUID
  //
  const ui32_t UUID_Length = 16;
  class UUID : public Identifier<UUID_Length>
    {
    public:
      UUID() {}
      UUID(const byte_t* value) : Identifier<UUID_Length>(value) {}
      UUID(const UUID& rhs) : Identifier<UUID_Length>(rhs) {}
      virtual ~UUID() {}

      inline const char* EncodeString(char* buf, ui32_t buf_len) const {
	return bin2UUIDhex(m_Value, Size(), buf, buf_len);
      }

      inline const char* EncodeHex(char* buf, ui32_t buf_len) const {
	return bin2UUIDhex(m_Value, Size(), buf, buf_len);
      }
    };
  
  void GenRandomUUID(byte_t* buf); // buf must be UUID_Length or longer
  void GenRandomValue(UUID&);
  
  typedef ArchivableList<UUID> UUIDList;

  // a self-wiping key container
  //
  const ui32_t SymmetricKey_Length = 16;
  const byte_t NilKey[SymmetricKey_Length] = {
    0xfa, 0xce, 0xfa, 0xce, 0xfa, 0xce, 0xfa, 0xce,
    0xfa, 0xce, 0xfa, 0xce, 0xfa, 0xce, 0xfa, 0xce
  };

  class SymmetricKey : public Identifier<SymmetricKey_Length>
    {
    public:
      SymmetricKey() {}
      SymmetricKey(const byte_t* value) : Identifier<SymmetricKey_Length>(value) {}
      SymmetricKey(const UUID& rhs) : Identifier<SymmetricKey_Length>(rhs) {}
      virtual ~SymmetricKey() { memcpy(m_Value, NilKey, 16); m_HasValue = false; }
    };

  void GenRandomValue(SymmetricKey&);

  //
  // 2004-05-01T13:20:00+00:00
  const ui32_t DateTimeLen = 25; //  the number of chars in the xs:dateTime format (sans milliseconds)

  // UTC time+date representation
  class Timestamp : public IArchive
    {
      TAI::tai m_Timestamp; // always UTC
      i32_t m_TZOffsetMinutes;

   public:
      Timestamp();
      Timestamp(const Timestamp& rhs);
      Timestamp(const char* datestr);
      Timestamp(const ui16_t& Year, const ui8_t&  Month, const ui8_t&  Day);
      Timestamp(const ui16_t& Year, const ui8_t&  Month, const ui8_t&  Day,
		const ui8_t&  Hour, const ui8_t&  Minute, const ui8_t&  Second);
      virtual ~Timestamp();

      const Timestamp& operator=(const Timestamp& rhs);
      bool operator<(const Timestamp& rhs) const;
      bool operator>(const Timestamp& rhs) const;
      bool operator==(const Timestamp& rhs) const;
      bool operator!=(const Timestamp& rhs) const;

      // always UTC
      void GetComponents(ui16_t& Year, ui8_t&  Month, ui8_t&  Day,
			 ui8_t&  Hour, ui8_t&  Minute, ui8_t&  Second) const;      
      void SetComponents(const ui16_t& Year, const ui8_t&  Month, const ui8_t&  Day,
			 const ui8_t&  Hour, const ui8_t&  Minute, const ui8_t&  Second);

      // Write the timestamp value to the given buffer in the form 2004-05-01T13:20:00+00:00
      // returns 0 if the buffer is smaller than DateTimeLen
      const char* EncodeString(char* str_buf, ui32_t buf_len) const;

      // decode and set value from string formatted by EncodeString
      bool        DecodeString(const char* datestr);

      // Add the given number of days, hours, minutes, or seconds to the timestamp value.
      // Values less than zero will cause the timestamp to decrease
      inline void AddDays(const i32_t& d) { m_Timestamp.add_days(d); }
      inline  void AddHours(const i32_t& h) { m_Timestamp.add_hours(h); }
      inline  void AddMinutes(const i32_t& m) { m_Timestamp.add_minutes(m); }
      inline  void AddSeconds(const i32_t& s) { m_Timestamp.add_seconds(s); }

      // returns false if the requested adjustment is out of range
      bool SetTZOffsetMinutes(const i32_t& minutes);
      inline i32_t GetTZOffsetMinutes() const { return m_TZOffsetMinutes; }

      // Return the number of seconds since the Unix epoch UTC (1970-01-01T00:00:00+00:00)
      ui64_t GetCTime() const;

      // Set internal time to the number of seconds since the Unix epoch UTC
      void SetCTime(const ui64_t& ctime);

      // Read and write the timestamp (always UTC) value as a byte string having
      // the following format:
      // | 16 bits int, big-endian |    8 bits   |   8 bits  |   8 bits   |    8 bits    |    8 bits    |
      // |        Year A.D         | Month(1-12) | Day(1-31) | Hour(0-23) | Minute(0-59) | Second(0-59) |
      //
      virtual bool   HasValue() const;
      virtual ui32_t ArchiveLength() const { return 8L; }
      virtual bool   Archive(MemIOWriter* Writer) const;
      virtual bool   Unarchive(MemIOReader* Reader);
    };

  //
  class ByteString : public IArchive
    {
      KM_NO_COPY_CONSTRUCT(ByteString);
	
    protected:
      byte_t* m_Data;          // pointer to memory area containing frame data
      ui32_t  m_Capacity;      // size of memory area pointed to by m_Data
      ui32_t  m_Length;        // length of byte string in memory area pointed to by m_Data
	
    public:
      ByteString();
      ByteString(ui32_t cap);
      virtual ~ByteString();

      // Sets or resets the size of the internally allocated buffer.
      Result_t Capacity(ui32_t cap);

      Result_t Append(const ByteString&);
      Result_t Append(const byte_t* buf, ui32_t buf_len);
	
      // returns the size of the buffer
      inline ui32_t  Capacity() const { return m_Capacity; }

      // returns a const pointer to the essence data
      inline const byte_t* RoData() const { assert(m_Data); return m_Data; }
	
      // returns a non-const pointer to the essence data
      inline byte_t* Data() { assert(m_Data); return m_Data; }
	
      // set the length of the buffer's contents
      inline ui32_t  Length(ui32_t l) { return m_Length = l; }
	
      // returns the length of the buffer's contents
      inline ui32_t  Length() const { return m_Length; }

      // copy the given data into the ByteString, set Length value.
      // Returns error if the ByteString is too small.
      Result_t Set(const byte_t* buf, ui32_t buf_len);
      Result_t Set(const ByteString& Buf);

      inline virtual bool HasValue() const { return m_Length > 0; }

      inline virtual ui32_t ArchiveLength() const { return m_Length; }

      inline virtual bool Archive(MemIOWriter* Writer) const {
	assert(Writer);
	if ( ! Writer->WriteUi32BE(m_Length) ) return false;
	if ( ! Writer->WriteRaw(m_Data, m_Length) ) return false;
	return true;
      }

      inline virtual bool Unarchive(MemIOReader* Reader) {
	assert(Reader);
	ui32_t tmp_len;
	if ( ! Reader->ReadUi32BE(&tmp_len) ) return false;
	if ( KM_FAILURE(Capacity(tmp_len)) ) return false;
	if ( ! Reader->ReadRaw(m_Data, tmp_len) ) return false;
	m_Length = tmp_len;
	return true;
      }
    };

  inline void hexdump(const ByteString& buf, FILE* stream = 0) {
    hexdump(buf.RoData(), buf.Length());
  }

  // Locates the first occurrence of the null-terminated string s2 in the string s1, where not more
  // than n characters are searched.  Characters that appear after a `\0' character are not searched.
  // Reproduced here from BSD for portability.
  const char *km_strnstr(const char *s1, const char *s2, size_t n);

  // Split the input string into tokens using the given separator. If the separator is not found the
  // entire string will be returned as a single-item list.
  std::list<std::string> km_token_split(const std::string& str, const std::string& separator);

} // namespace Kumu


#endif // _KM_UTIL_H_

//
// end KM_util.h
//
