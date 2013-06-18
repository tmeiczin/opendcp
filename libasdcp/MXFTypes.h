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
/*! \file    MXFTypes.h
    \version $Id: MXFTypes.h,v 1.30 2012/03/15 17:54:15 jhurst Exp $
    \brief   MXF objects
*/

#ifndef _MXFTYPES_H_
#define _MXFTYPES_H_

#include "KLV.h"
#include <list>
#include <vector>
#include <map>
#include <wchar.h>

// used with TLVReader::Read*
//
// these are used below to manufacture arguments
#define OBJ_READ_ARGS(s,l) m_Dict->Type(MDD_##s##_##l), &l
#define OBJ_WRITE_ARGS(s,l) m_Dict->Type(MDD_##s##_##l), &l
#define OBJ_TYPE_ARGS(t) m_Dict->Type(MDD_##t).ul


namespace ASDCP
{
  namespace MXF
    {
      typedef std::pair<ui32_t, ui32_t> ItemInfo;
      typedef std::map<TagValue, ItemInfo> TagMap;

      //      
      class TLVReader : public Kumu::MemIOReader
	{

	  TagMap         m_ElementMap;
	  IPrimerLookup* m_Lookup;

	  TLVReader();
	  ASDCP_NO_COPY_CONSTRUCT(TLVReader);
	  bool FindTL(const MDDEntry&);

	public:
	  TLVReader(const byte_t* p, ui32_t c, IPrimerLookup* = 0);
	  Result_t ReadObject(const MDDEntry&, Kumu::IArchive*);
	  Result_t ReadUi8(const MDDEntry&, ui8_t*);
	  Result_t ReadUi16(const MDDEntry&, ui16_t*);
	  Result_t ReadUi32(const MDDEntry&, ui32_t*);
	  Result_t ReadUi64(const MDDEntry&, ui64_t*);
	};

      //      
      class TLVWriter : public Kumu::MemIOWriter
	{

	  TagMap         m_ElementMap;
	  IPrimerLookup* m_Lookup;

	  TLVWriter();
	  ASDCP_NO_COPY_CONSTRUCT(TLVWriter);
	  Result_t WriteTag(const MDDEntry&);

	public:
	  TLVWriter(byte_t* p, ui32_t c, IPrimerLookup* = 0);
	  Result_t WriteObject(const MDDEntry&, Kumu::IArchive*);
	  Result_t WriteUi8(const MDDEntry&, ui8_t*);
	  Result_t WriteUi16(const MDDEntry&, ui16_t*);
	  Result_t WriteUi32(const MDDEntry&, ui32_t*);
	  Result_t WriteUi64(const MDDEntry&, ui64_t*);
	};

      //
      template <class T>
	class Batch : public std::vector<T>, public Kumu::IArchive
	{
	public:
	  Batch() {}
	  virtual ~Batch() {}

	  //
	  virtual bool Unarchive(Kumu::MemIOReader* Reader) {
	    ui32_t ItemCount, ItemSize;
	    if ( ! Reader->ReadUi32BE(&ItemCount) ) return false;
	    if ( ! Reader->ReadUi32BE(&ItemSize) ) return false;

	    if ( ( ItemCount > 65536 ) || ( ItemSize > 1024 ) )
	      return false;

	    bool result = true;
	    for ( ui32_t i = 0; i < ItemCount && result; i++ )
	      {
		T Tmp;
		result = Tmp.Unarchive(Reader);

		if ( result )
		  this->push_back(Tmp);
	      }

	    return result;
	  }

	  inline virtual bool HasValue() const { return ! this->empty(); }

	  virtual ui32_t ArchiveLength() const {
	    ui32_t arch_size = sizeof(ui32_t)*2;

	    typename std::vector<T>::const_iterator l_i = this->begin();
	    assert(l_i != this->end());

	    for ( ; l_i != this->end(); l_i++ )
	      arch_size += l_i->ArchiveLength();
	    
	    return arch_size;
	  }

	  //
	  virtual bool Archive(Kumu::MemIOWriter* Writer) const {
	    if ( ! Writer->WriteUi32BE(this->size()) ) return false;
	    byte_t* p = Writer->CurrentData();

	    if ( ! Writer->WriteUi32BE(0) ) return false;
	    if ( this->empty() ) return true;
	    
	    typename std::vector<T>::const_iterator l_i = this->begin();
	    assert(l_i != this->end());

	    ui32_t ItemSize = Writer->Remainder();
	    if ( ! (*l_i).Archive(Writer) ) return false;
	    ItemSize -= Writer->Remainder();
	    Kumu::i2p<ui32_t>(KM_i32_BE(ItemSize), p);
	    l_i++;

	    bool result = true;
	    for ( ; l_i != this->end() && result; l_i++ )
	      result = (*l_i).Archive(Writer);

	    return result;
	  }

	  //
	  void Dump(FILE* stream = 0, ui32_t depth = 0)
	    {
	      char identbuf[IdentBufferLen];

	      if ( stream == 0 )
		stream = stderr;

	      typename std::vector<T>::iterator i = this->begin();
	      for ( ; i != this->end(); i++ )
		fprintf(stream, "  %s\n", (*i).EncodeString(identbuf, IdentBufferLen));
	    }
	};

      //
      template <class T>
	class Array : public std::list<T>, public Kumu::IArchive
	{
	public:
	  Array() {}
	  virtual ~Array() {}

	  //
	  virtual bool Unarchive(Kumu::MemIOReader* Reader)
	    {
	      bool result = true;

	      while ( Reader->Remainder() > 0 && result )
		{
		  T Tmp;
		  result = Tmp.Unarchive(Reader);
		  this->push_back(Tmp);
		}

	      return result;
	    }

	  inline virtual bool HasValue() const { return ! this->empty(); }

	  virtual ui32_t ArchiveLength() const {
	    ui32_t arch_size = 0;

	    typename std::list<T>::const_iterator l_i = this->begin();

	    for ( ; l_i != this->end(); l_i++ )
	      arch_size += l_i->ArchiveLength();
	    
	    return arch_size;
	  }

	  //
	  virtual bool Archive(Kumu::MemIOWriter* Writer) const {
	    bool result = true;
	    typename std::list<T>::const_iterator l_i = this->begin();

	    for ( ; l_i != this->end() && result; l_i++ )
	      result = (*l_i).Archive(Writer);

	    return result;
	  }

	  //
	  void Dump(FILE* stream = 0, ui32_t depth = 0)
	    {
	      char identbuf[IdentBufferLen];

	      if ( stream == 0 )
		stream = stderr;

	      typename std::list<T>::iterator i = this->begin();
	      for ( ; i != this->end(); i++ )
		fprintf(stream, "  %s\n", (*i).EncodeString(identbuf, IdentBufferLen));
	    }
	};

      //
    class ISO8String : public std::string, public Kumu::IArchive
	{
	public:
	  ISO8String() {}
	  ~ISO8String() {}

	  const ISO8String& operator=(const char*);
	  const ISO8String& operator=(const std::string&);

	  const char* EncodeString(char* str_buf, ui32_t buf_len) const;
	  inline virtual bool HasValue() const { return ! empty(); }
	  inline virtual ui32_t ArchiveLength() const { return sizeof(ui32_t) + size(); }
	  virtual bool Unarchive(Kumu::MemIOReader* Reader);
	  virtual bool Archive(Kumu::MemIOWriter* Writer) const;
	};

      //
    class UTF16String : public std::string, public Kumu::IArchive
	{
	public:
	  UTF16String() {}
	  ~UTF16String() {}

	  const UTF16String& operator=(const char*);
	  const UTF16String& operator=(const std::string&);

	  const char* EncodeString(char* str_buf, ui32_t buf_len) const;
	  inline virtual bool HasValue() const { return ! empty(); }
	  inline virtual ui32_t ArchiveLength() const { return sizeof(ui32_t) + size(); }
	  virtual bool Unarchive(Kumu::MemIOReader* Reader);
	  virtual bool Archive(Kumu::MemIOWriter* Writer) const;
	};

      //
      class Rational : public ASDCP::Rational, public Kumu::IArchive
	{
	public:
	  Rational() {}
	  ~Rational() {}

	  Rational(const Rational& rhs) : ASDCP::Rational(), IArchive() {
	    Numerator = rhs.Numerator;
	    Denominator = rhs.Denominator;
	  }

	  const Rational& operator=(const Rational& rhs) {
	    Numerator = rhs.Numerator;
	    Denominator = rhs.Denominator;
	    return *this;
	  }

	  Rational(const ASDCP::Rational& rhs) {
	    Numerator = rhs.Numerator;
	    Denominator = rhs.Denominator;
	  }

	  const Rational& operator=(const ASDCP::Rational& rhs) {
	    Numerator = rhs.Numerator;
	    Denominator = rhs.Denominator;
	    return *this;
	  }

	  //
	  inline const char* EncodeString(char* str_buf, ui32_t buf_len) const {
	    snprintf(str_buf, buf_len, "%d/%d", Numerator, Denominator);
	    return str_buf;
	  }

	  inline virtual bool Unarchive(Kumu::MemIOReader* Reader) {
	    if ( ! Reader->ReadUi32BE((ui32_t*)&Numerator) ) return false;
	    if ( ! Reader->ReadUi32BE((ui32_t*)&Denominator) ) return false;
	    return true;
	  }

	  inline virtual bool HasValue() const { return true; }
	  inline virtual ui32_t ArchiveLength() const { return sizeof(ui32_t)*2; }

	  inline virtual bool Archive(Kumu::MemIOWriter* Writer) const {
	    if ( ! Writer->WriteUi32BE((ui32_t)Numerator) ) return false;
	    if ( ! Writer->WriteUi32BE((ui32_t)Denominator) ) return false;
	    return true;
	  }
	};

      //
      class VersionType : public Kumu::IArchive
	{
	public:
	  enum Release_t { RL_UNKNOWN, RL_RELEASE, RL_DEVELOPMENT, RL_PATCHED, RL_BETA, RL_PRIVATE };
	  ui16_t Major;
	  ui16_t Minor;
	  ui16_t Patch;
	  ui16_t Build;
	  Release_t Release;

	  VersionType() : Major(0), Minor(0), Patch(0), Build(0), Release(RL_UNKNOWN) {}
	  VersionType(const VersionType& rhs) { Copy(rhs); }
	  virtual ~VersionType() {}

	  const VersionType& operator=(const VersionType& rhs) { Copy(rhs); return *this; }
	  void Copy(const VersionType& rhs) {
	    Major = rhs.Major;
	    Minor = rhs.Minor;
	    Patch = rhs.Patch;
	    Build = rhs.Build;
	    Release = rhs.Release;
	  }

	  void Dump(FILE* = 0);

	  const char* EncodeString(char* str_buf, ui32_t buf_len) const {
	    snprintf(str_buf, buf_len, "%hu.%hu.%hu.%hur%hu", Major, Minor, Patch, Build, Release);
	    return str_buf;
	  }

	  virtual bool Unarchive(Kumu::MemIOReader* Reader) {
	    if ( ! Reader->ReadUi16BE(&Major) ) return false;
	    if ( ! Reader->ReadUi16BE(&Minor) ) return false;
	    if ( ! Reader->ReadUi16BE(&Patch) ) return false;
	    if ( ! Reader->ReadUi16BE(&Build) ) return false;
	    ui16_t tmp_release;
	    if ( ! Reader->ReadUi16BE(&tmp_release) ) return false;
	    Release = (Release_t)tmp_release;
	    return true;
	  }

	  inline virtual bool HasValue() const { return true; }
	  inline virtual ui32_t ArchiveLength() const { return sizeof(ui16_t)*5; }

	  virtual bool Archive(Kumu::MemIOWriter* Writer) const {
	    if ( ! Writer->WriteUi16BE(Major) ) return false;
	    if ( ! Writer->WriteUi16BE(Minor) ) return false;
	    if ( ! Writer->WriteUi16BE(Patch) ) return false;
	    if ( ! Writer->WriteUi16BE(Build) ) return false;
	    if ( ! Writer->WriteUi16BE((ui16_t)(Release & 0x0000ffffL)) ) return false;
	    return true;
	  }
	};

      //
      class Raw : public Kumu::ByteString
	{
	public:
	  Raw();
	  Raw(const Raw& rhs) { Copy(rhs); }
	  virtual ~Raw();

	  const Raw& operator=(const Raw& rhs) { Copy(rhs); return *this; }
	  void Copy(const Raw& rhs) {
	    if ( KM_SUCCESS(Capacity(rhs.Length())) )
	      {
		Set(rhs);
	      }
	  }

	  //
          virtual bool Unarchive(Kumu::MemIOReader* Reader);
	  virtual bool Archive(Kumu::MemIOWriter* Writer) const;
	  const char* EncodeString(char* str_buf, ui32_t buf_len) const;
	};

    } // namespace MXF
} // namespace ASDCP


#endif //_MXFTYPES_H_

//
// end MXFTypes.h
//
