/*
Copyright (c) 2005-2015, John Hurst
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
    \version $Id: MXFTypes.h,v 1.38 2015/10/12 15:30:46 jhurst Exp $
    \brief   MXF objects
*/

#ifndef _MXFTYPES_H_
#define _MXFTYPES_H_

#include "KLV.h"
#include <list>
#include <vector>
#include <set>
#include <map>
#include <wchar.h>

// used with TLVReader::Read*
//
// these are used below to manufacture arguments
#define OBJ_READ_ARGS(s,l) m_Dict->Type(MDD_##s##_##l), &l
#define OBJ_WRITE_ARGS(s,l) m_Dict->Type(MDD_##s##_##l), &l
#define OBJ_READ_ARGS_OPT(s,l) m_Dict->Type(MDD_##s##_##l), &l.get()
#define OBJ_WRITE_ARGS_OPT(s,l) m_Dict->Type(MDD_##s##_##l), &l.get()
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
      template <class ContainerType>
	class FixedSizeItemCollection : public ContainerType, public Kumu::IArchive
	{
	public:
	  FixedSizeItemCollection() {}
	  virtual ~FixedSizeItemCollection() {}

	  ui32_t ItemSize() const {
	    typename ContainerType::value_type tmp_item;
	    return tmp_item.ArchiveLength();
	  }

	  bool HasValue() const { return ! this->empty(); }

	  ui32_t ArchiveLength() const {
	    return ( sizeof(ui32_t) * 2 ) +  ( this->size() * this->ItemSize() );
	  }

	  bool Archive(Kumu::MemIOWriter* Writer) const {
	    if ( ! Writer->WriteUi32BE(this->size()) ) return false;
	    if ( ! Writer->WriteUi32BE(this->ItemSize()) ) return false;
	    if ( this->empty() ) return true;
	    
	    typename ContainerType::const_iterator i;
	    bool result = true;
	    for ( i = this->begin(); i != this->end() && result; ++i )
	      {
		result = i->Archive(Writer);
	      }

	    return result;
	  }

	  //
	  bool Unarchive(Kumu::MemIOReader* Reader) {
	    ui32_t item_count, item_size;
	    if ( ! Reader->ReadUi32BE(&item_count) ) return false;
	    if ( ! Reader->ReadUi32BE(&item_size) ) return false;

	    if ( item_count > 0 )
	      {
		if ( this->ItemSize() != item_size ) return false;
	      }

	    bool result = true;
	    for ( ui32_t i = 0; i < item_count && result; ++i )
	      {
		typename ContainerType::value_type tmp_item;
		result = tmp_item.Unarchive(Reader);

		if ( result )
		  {
		    this->push_back(tmp_item);
		  }
	      }

	    return result;
	  }

	  void Dump(FILE* stream = 0, ui32_t depth = 0) {
	    char identbuf[IdentBufferLen];

	    if ( stream == 0 )
	      {
		stream = stderr;
	      }
	    
	    typename ContainerType::const_iterator i;
	    for ( i = this->begin(); i != this->end(); ++i )
	      {
		fprintf(stream, "  %s\n", (*i).EncodeString(identbuf, IdentBufferLen));
	      }
	  }
	};


      template <class item_type>
	class PushSet : public std::set<item_type>
      {
      public:
	PushSet() {}
	virtual ~PushSet() {}
	void push_back(const item_type& item) { this->insert(item); }
      };

      template <class ItemType>
	class Batch : public FixedSizeItemCollection<PushSet<ItemType> >
      {
      public:
	Batch() {}
	virtual ~Batch() {}
      };

      template <class ItemType>
	class Array : public FixedSizeItemCollection<std::vector<ItemType> >
      {
      public:
	Array() {}
	virtual ~Array() {}
      };

      //
      template <class T>
	class SimpleArray : public std::list<T>, public Kumu::IArchive
	{
	public:
	  SimpleArray() {}
	  virtual ~SimpleArray() {}

	  //
	  bool Unarchive(Kumu::MemIOReader* Reader)
	    {
	      bool result = true;

	      while ( Reader->Remainder() > 0 && result )
		{
		  T Tmp;
		  result = Tmp.Unarchive(Reader);

		  if ( result )
		    {
		      this->push_back(Tmp);
		    }
		}

	      return result;
	    }

	  inline bool HasValue() const { return ! this->empty(); }

	  ui32_t ArchiveLength() const {
	    ui32_t arch_size = 0;

	    typename std::list<T>::const_iterator l_i = this->begin();

	    for ( ; l_i != this->end(); l_i++ )
	      arch_size += l_i->ArchiveLength();
	    
	    return arch_size;
	  }

	  //
	  bool Archive(Kumu::MemIOWriter* Writer) const {
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
	  ISO8String(const char*);
	  ISO8String(const std::string&);
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
	  UTF16String(const char*);
	  UTF16String(const std::string&);
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
	  enum Release_t { RL_UNKNOWN, RL_RELEASE, RL_DEVELOPMENT, RL_PATCHED, RL_BETA, RL_PRIVATE, RL_MAX };
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
	    snprintf(str_buf, buf_len, "%hu.%hu.%hu.%hur%hu", Major, Minor, Patch, Build, ui16_t(Release));
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

      /*
	The RGBALayout type shall be a fixed-size 8 element sequence with a total length
	of 16 bytes, where each element shall consist of the RGBAComponent type with the
	following fields:

	Code (UInt8): Enumerated value specifying component (i.e., component identifier).
	"0" is the layout terminator.

	Depth (UInt8): Integer specifying the number of bits occupied (see also G.2.26) 
	  1->32 indicates integer depth
	  253 = HALF (floating point 16-bit value)
	  254 = IEEE floating point 32-bit value
	  255 = IEEE floating point 64-bit value
	  0 = RGBALayout terminator

	A Fill component indicates unused bits. After the components have been specified,
	the remaining Code and Size fields shall be set to zero (0).

	For each component in the Pixel, one of the following Codes or the terminator
	shall be specified (explained below):

	Code	ASCII

      */
      struct RGBALayoutTableEntry
      {
	byte_t code;
	char symbol;
	const char* label;
      };

      struct RGBALayoutTableEntry const RGBALayoutTable[] = {
	{ 0x52, 'R', "Red component" },
	{ 0x47, 'G', "Green component" },
	{ 0x42, 'B', "Blue component" },
	{ 0x41, 'A', "Alpha component" },
	{ 0x72, 'r', "Red component (LSBs)" },
	{ 0x67, 'g', "Green component (LSBs)" },
	{ 0x62, 'b', "Blue component (LSBs)" },
	{ 0x61, 'a', "Alpha component (LSBs)" },
	{ 0x46, 'F', "Fill component" },
	{ 0x50, 'P', "Palette code" },
	{ 0x55, 'U', "Color Difference Sample (e.g. U, Cb, I etc.)" },
	{ 0x56, 'V', "Color Difference Sample (e.g. V, Cr, Q etc.)" },
	{ 0x57, 'W', "Composite Video" },
	{ 0x58, 'X', "Non co-sited luma component" },
	{ 0x59, 'Y', "Luma component" },
	{ 0x5a, 'Z', "Depth component (SMPTE ST 268 compatible)" },
	{ 0x75, 'u', "Color Difference Sample (e.g. U, Cb, I etc.) (LSBs)" },
	{ 0x76, 'v', "Color Difference Sample (e.g. V, Cr, Q etc.) (LSBs)" },
	{ 0x77, 'w', "Composite Video (LSBs)" },
	{ 0x78, 'x', "Non co-sited luma component (LSBs)" },
	{ 0x79, 'y', "Luma component (LSBs)" },
	{ 0x7a, 'z', "Depth component (LSBs) (SMPTE ST 268 compatible)" },
	{ 0xd8, 'X', "The DCDM X color component (see SMPTE ST 428-1 X')" },
	{ 0xd9, 'Y', "The DCDM Y color component (see SMPTE ST 428-1 Y')" },
	{ 0xda, 'Z', "The DCDM Z color component (see SMPTE ST 428-1 Z')" },
	{ 0x00, '_', "Terminator" }
      };


      size_t const RGBAValueLength = 16;

      byte_t const RGBAValue_RGB_10[RGBAValueLength] = { 'R', 10, 'G', 10, 'B', 10, 0, 0 };
      byte_t const RGBAValue_RGB_8[RGBAValueLength]  = { 'R', 8,  'G', 8,  'B', 8,  0, 0 };
      byte_t const RGBAValue_YUV_10[RGBAValueLength] = { 'Y', 10, 'U', 10, 'V', 10, 0, 0 };
      byte_t const RGBAValue_YUV_8[RGBAValueLength]  = { 'Y', 8,  'U', 8,  'V', 8,  0, 0 };
      byte_t const RGBAValue_DCDM[RGBAValueLength] = { 0xd8, 10, 0xd9, 10, 0xda, 10, 0, 0 };


      class RGBALayout : public Kumu::IArchive
	{
	  byte_t m_value[RGBAValueLength];

	public:
	  RGBALayout();
	  RGBALayout(const byte_t* value);
	  ~RGBALayout();

	  RGBALayout(const RGBALayout& rhs) { Set(rhs.m_value); }
	  const RGBALayout& operator=(const RGBALayout& rhs) { Set(rhs.m_value); return *this; }
	  
	  void Set(const byte_t* value) {
	    memcpy(m_value, value, RGBAValueLength);
	  }

	  const char* EncodeString(char* buf, ui32_t buf_len) const;

	  bool HasValue() const { return true; }
	  ui32_t ArchiveLength() const { return RGBAValueLength; }

	  bool Archive(Kumu::MemIOWriter* Writer) const {
	    return Writer->WriteRaw(m_value, RGBAValueLength);
	  }

	  bool Unarchive(Kumu::MemIOReader* Reader) {
	    if ( Reader->Remainder() < RGBAValueLength )
	      {
		return false;
	      }

	    memcpy(m_value, Reader->CurrentData(), RGBAValueLength);
	    Reader->SkipOffset(RGBAValueLength);
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
