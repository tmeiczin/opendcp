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
/*! \file    MXFTypes.cpp
    \version $Id: MXFTypes.cpp,v 1.27 2012/02/21 02:09:31 jhurst Exp $
    \brief   MXF objects
*/

#include <KM_prng.h>
#include <KM_tai.h>
#include "MXFTypes.h"
#include <KM_log.h>

using Kumu::DefaultLogSink;

//------------------------------------------------------------------------------------------
//

//
bool
ASDCP::UL::operator==(const UL& rhs) const
{
  if ( m_Value[0] == rhs.m_Value[0] &&
       m_Value[1] == rhs.m_Value[1] &&
       m_Value[2] == rhs.m_Value[2] &&
       m_Value[3] == rhs.m_Value[3] &&
       m_Value[4] == rhs.m_Value[4] &&
       m_Value[5] == rhs.m_Value[5] &&
       m_Value[6] == rhs.m_Value[6] &&
       //       m_Value[7] == rhs.m_Value[7] &&  // version is ignored when performing lookups
       m_Value[8] == rhs.m_Value[8] &&
       m_Value[9] == rhs.m_Value[9] &&
       m_Value[10] == rhs.m_Value[10] &&
       m_Value[11] == rhs.m_Value[11] &&
       m_Value[12] == rhs.m_Value[12] &&
       m_Value[13] == rhs.m_Value[13] &&
       m_Value[14] == rhs.m_Value[14] &&
       m_Value[15] == rhs.m_Value[15]
       )
    return true;

  return false;
}

//
bool
ASDCP::UL::MatchIgnoreStream(const UL& rhs) const
{
  if ( m_Value[0] == rhs.m_Value[0] &&
       m_Value[1] == rhs.m_Value[1] &&
       m_Value[2] == rhs.m_Value[2] &&
       m_Value[3] == rhs.m_Value[3] &&
       m_Value[4] == rhs.m_Value[4] &&
       m_Value[5] == rhs.m_Value[5] &&
       m_Value[6] == rhs.m_Value[6] &&
       //       m_Value[7] == rhs.m_Value[7] &&  // version is ignored when performing lookups
       m_Value[8] == rhs.m_Value[8] &&
       m_Value[9] == rhs.m_Value[9] &&
       m_Value[10] == rhs.m_Value[10] &&
       m_Value[11] == rhs.m_Value[11] &&
       m_Value[12] == rhs.m_Value[12] &&
       m_Value[13] == rhs.m_Value[13] &&
       m_Value[14] == rhs.m_Value[14]
       //       m_Value[15] == rhs.m_Value[15] // ignore stream number
       )
    return true;

  return false;
}

//
bool
ASDCP::UL::ExactMatch(const UL& rhs) const
{
  if ( m_Value[0] == rhs.m_Value[0] &&
       m_Value[1] == rhs.m_Value[1] &&
       m_Value[2] == rhs.m_Value[2] &&
       m_Value[3] == rhs.m_Value[3] &&
       m_Value[4] == rhs.m_Value[4] &&
       m_Value[5] == rhs.m_Value[5] &&
       m_Value[6] == rhs.m_Value[6] &&
       m_Value[7] == rhs.m_Value[7] &&
       m_Value[8] == rhs.m_Value[8] &&
       m_Value[9] == rhs.m_Value[9] &&
       m_Value[10] == rhs.m_Value[10] &&
       m_Value[11] == rhs.m_Value[11] &&
       m_Value[12] == rhs.m_Value[12] &&
       m_Value[13] == rhs.m_Value[13] &&
       m_Value[14] == rhs.m_Value[14] &&
       m_Value[15] == rhs.m_Value[15]
       )
    return true;

  return false;
}

const char*
ASDCP::UL::EncodeString(char* str_buf, ui32_t buf_len) const
{
  if ( buf_len > 38 ) // room for dotted notation?
    {
      snprintf(str_buf, buf_len,
	       "%02x%02x%02x%02x.%02x%02x.%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x",
	       m_Value[0],  m_Value[1],  m_Value[2],  m_Value[3],
	       m_Value[4],  m_Value[5],  m_Value[6],  m_Value[7],
	       m_Value[8],  m_Value[9],  m_Value[10], m_Value[11],
	       m_Value[12], m_Value[13], m_Value[14], m_Value[15]
               );

      return str_buf;
    }
  else if ( buf_len > 32 ) // room for compact?
    {
      snprintf(str_buf, buf_len,
	       "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
	       m_Value[0],  m_Value[1],  m_Value[2],  m_Value[3],
	       m_Value[4],  m_Value[5],  m_Value[6],  m_Value[7],
	       m_Value[8],  m_Value[9],  m_Value[10], m_Value[11],
	       m_Value[12], m_Value[13], m_Value[14], m_Value[15]
               );

      return str_buf;
    }

  return 0;
}

//
void
ASDCP::UMID::MakeUMID(int Type)
{
  UUID AssetID;
  Kumu::GenRandomValue(AssetID);
  MakeUMID(Type, AssetID);
}

//
void
ASDCP::UMID::MakeUMID(int Type, const UUID& AssetID)
{
  // Set the non-varying base of the UMID
  static const byte_t UMIDBase[10] = { 0x06, 0x0a, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
  memcpy(m_Value, UMIDBase, 10);
  m_Value[10] = Type;  // Material Type
  m_Value[12] = 0x13;  // length

  // preserved for compatibility with mfxlib
  if( Type > 4 ) m_Value[7] = 5;
  m_Value[11] = 0x20; // UUID/UL method, number gen undefined

  // Instance Number
  m_Value[13] = m_Value[14] = m_Value[15] = 0;
  
  memcpy(&m_Value[16], AssetID.Value(), AssetID.Size());
  m_HasValue = true;
}


// Write the UMID value to the given buffer in the form
//   [00000000.0000.0000.00000000],00,00,00,00,00000000.0000.0000.00000000.00000000]
// or
//   [00000000.0000.0000.00000000],00,00,00,00,00000000-0000-0000-0000-000000000000]
// returns 0 if the buffer is smaller than DateTimeLen
const char*
ASDCP::UMID::EncodeString(char* str_buf, ui32_t buf_len) const
{
  assert(str_buf);

  snprintf(str_buf, buf_len, "[%02x%02x%02x%02x.%02x%02x.%02x%02x.%02x%02x%02x%02x],%02x,%02x,%02x,%02x,",
	   m_Value[0],  m_Value[1],  m_Value[2],  m_Value[3],
	   m_Value[4],  m_Value[5],  m_Value[6],  m_Value[7],
	   m_Value[8],  m_Value[9],  m_Value[10], m_Value[11],
	   m_Value[12], m_Value[13], m_Value[14], m_Value[15]
	   );

  ui32_t offset = strlen(str_buf);

  if ( ( m_Value[8] & 0x80 ) == 0 )
    {
      // half-swapped UL, use [bbaa9988.ddcc.ffee.00010203.04050607]
      snprintf(str_buf + offset, buf_len - offset,
	       "[%02x%02x%02x%02x.%02x%02x.%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x]",
               m_Value[24], m_Value[25], m_Value[26], m_Value[27],
	       m_Value[28], m_Value[29], m_Value[30], m_Value[31],
               m_Value[16], m_Value[17], m_Value[18], m_Value[19],
	       m_Value[20], m_Value[21], m_Value[22], m_Value[23]
               );
    }
  else
    {
      // UUID, use {00112233-4455-6677-8899-aabbccddeeff}
      snprintf(str_buf + offset, buf_len - offset,
	       "{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
               m_Value[16], m_Value[17], m_Value[18], m_Value[19],
	       m_Value[20], m_Value[21], m_Value[22], m_Value[23],
               m_Value[24], m_Value[25], m_Value[26], m_Value[27],
	       m_Value[28], m_Value[29], m_Value[30], m_Value[31]
               );
    }

  return str_buf;
}

//------------------------------------------------------------------------------------------
//

//
const ASDCP::MXF::UTF16String&
ASDCP::MXF::UTF16String::operator=(const char* sz)
{
  if ( sz == 0 || *sz == 0 )
    erase();

  else
    this->assign(sz);
  
  return *this;
}

//
const ASDCP::MXF::UTF16String&
ASDCP::MXF::UTF16String::operator=(const std::string& str)
{
  this->assign(str);
  return *this;
}

//
const char*
ASDCP::MXF::UTF16String::EncodeString(char* str_buf, ui32_t buf_len) const
{
  ui32_t write_len = Kumu::xmin(buf_len - 1, (ui32_t)size());
  strncpy(str_buf, c_str(), write_len);
  str_buf[write_len] = 0;
  return str_buf;
}

//
bool
ASDCP::MXF::UTF16String::Unarchive(Kumu::MemIOReader* Reader)
{
  erase();
  const ui16_t* p = (ui16_t*)Reader->CurrentData();
  ui32_t length = Reader->Remainder() / 2;
  char mb_buf[MB_LEN_MAX+1];

  for ( ui32_t i = 0; i < length; i++ )
    {
      int count = wctomb(mb_buf, KM_i16_BE(p[i]));

      if ( count == -1 )
	{
	  DefaultLogSink().Error("Unable to decode wide character 0x%04hx\n", p[i]);
	  return false;
	}

      assert(count <= MB_LEN_MAX);
      mb_buf[count] = 0;
      this->append(mb_buf);
    }

  Reader->SkipOffset(length*2);
  return true;
}

//
bool
ASDCP::MXF::UTF16String::Archive(Kumu::MemIOWriter* Writer) const
{
  if ( size() > IdentBufferLen )
    {
      DefaultLogSink().Error("String length exceeds maximum %u bytes\n", IdentBufferLen);
      return false;
    }

  const char* mbp = c_str();
  wchar_t wcp;
  ui32_t remainder = size();
  ui32_t length = size();
  ui32_t i = 0;

  while ( i < length )
    {
      int count = mbtowc(&wcp, mbp+i, remainder);

      if ( count == -1 )
	{
	  DefaultLogSink().Error("Error decoding multi-byte sequence starting at offset %u\n", i);
	  return false;
	}
      else if ( count  == 0 )
	break;

      bool result = Writer->WriteUi16BE((ui16_t)wcp);

      if ( result == false )
	{
	  DefaultLogSink().Error("No more space in memory IO writer\n");
	  return false;
	}

      i += count;
      remainder -= count;
    }

  return true;
}

//------------------------------------------------------------------------------------------
//

//
const ASDCP::MXF::ISO8String&
ASDCP::MXF::ISO8String::operator=(const char* sz)
{
  if ( sz == 0 || *sz == 0 )
    erase();

  else
    this->assign(sz);
  
  return *this;
}

//
const ASDCP::MXF::ISO8String&
ASDCP::MXF::ISO8String::operator=(const std::string& str)
{
  this->assign(str);
  return *this;
}

//
const char*
ASDCP::MXF::ISO8String::EncodeString(char* str_buf, ui32_t buf_len) const
{
  ui32_t write_len = Kumu::xmin(buf_len - 1, (ui32_t)size());
  strncpy(str_buf, c_str(), write_len);
  str_buf[write_len] = 0;
  return str_buf;
}

//
bool
ASDCP::MXF::ISO8String::Unarchive(Kumu::MemIOReader* Reader)
{
  assign((char*)Reader->CurrentData(), Reader->Remainder());
  return true;
}

//
bool
ASDCP::MXF::ISO8String::Archive(Kumu::MemIOWriter* Writer) const
{
  if ( size() > IdentBufferLen )
    {
      DefaultLogSink().Error("String length exceeds maximum %u bytes\n", IdentBufferLen);
      return false;
    }

  return Writer->WriteString(*this);
}

//------------------------------------------------------------------------------------------
//

ASDCP::MXF::TLVReader::TLVReader(const byte_t* p, ui32_t c, IPrimerLookup* PrimerLookup) :
  MemIOReader(p, c), m_Lookup(PrimerLookup)
{
  Result_t result = RESULT_OK;

  while ( Remainder() > 0 && ASDCP_SUCCESS(result) )
    {
      TagValue Tag;
      ui16_t pkt_len = 0;

      if ( MemIOReader::ReadUi8(&Tag.a) )
	if ( MemIOReader::ReadUi8(&Tag.b) )
	  if ( MemIOReader::ReadUi16BE(&pkt_len) )
	    {
	      m_ElementMap.insert(TagMap::value_type(Tag, ItemInfo(m_size, pkt_len)));
	      if ( SkipOffset(pkt_len) )
		continue;;
	    }

      DefaultLogSink().Error("Malformed Set\n");
      m_ElementMap.clear();
      result = RESULT_KLV_CODING;
    }
}

//
bool
ASDCP::MXF::TLVReader::FindTL(const MDDEntry& Entry)
{
  if ( m_Lookup == 0 )
    {
      DefaultLogSink().Error("No Lookup service\n");
      return false;
    }
  
  TagValue TmpTag;

  if ( m_Lookup->TagForKey(Entry.ul, TmpTag) != RESULT_OK )
    {
      if ( Entry.tag.a == 0 )
	{
	  //	  DefaultLogSink().Debug("No such UL in this TL list: %s (%02x %02x)\n",
	  //				 Entry.name, Entry.tag.a, Entry.tag.b);
	  return false;
	}

      TmpTag = Entry.tag;
    }

  TagMap::iterator e_i = m_ElementMap.find(TmpTag);

  if ( e_i != m_ElementMap.end() )
    {
      m_size = (*e_i).second.first;
      m_capacity = m_size + (*e_i).second.second;
      return true;
    }

  //  DefaultLogSink().Debug("Not Found (%02x %02x): %s\n", TmpTag.a, TmpTag.b, Entry.name);
  return false;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVReader::ReadObject(const MDDEntry& Entry, Kumu::IArchive* Object)
{
  ASDCP_TEST_NULL(Object);

  if ( FindTL(Entry) )
    {
      if ( m_size < m_capacity ) // don't try to unarchive an empty item
	return Object->Unarchive(this) ? RESULT_OK : RESULT_KLV_CODING;
    }

  return RESULT_FALSE;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVReader::ReadUi8(const MDDEntry& Entry, ui8_t* value)
{
  ASDCP_TEST_NULL(value);

  if ( FindTL(Entry) )
    return MemIOReader::ReadUi8(value) ? RESULT_OK : RESULT_KLV_CODING;

  return RESULT_FALSE;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVReader::ReadUi16(const MDDEntry& Entry, ui16_t* value)
{
  ASDCP_TEST_NULL(value);

  if ( FindTL(Entry) )
    return MemIOReader::ReadUi16BE(value) ? RESULT_OK : RESULT_KLV_CODING;

  return RESULT_FALSE;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVReader::ReadUi32(const MDDEntry& Entry, ui32_t* value)
{
  ASDCP_TEST_NULL(value);

  if ( FindTL(Entry) )
    return MemIOReader::ReadUi32BE(value) ? RESULT_OK : RESULT_KLV_CODING;

  return RESULT_FALSE;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVReader::ReadUi64(const MDDEntry& Entry, ui64_t* value)
{
  ASDCP_TEST_NULL(value);

  if ( FindTL(Entry) )
    return MemIOReader::ReadUi64BE(value) ? RESULT_OK : RESULT_KLV_CODING;

  return RESULT_FALSE;
}

//------------------------------------------------------------------------------------------
//

ASDCP::MXF::TLVWriter::TLVWriter(byte_t* p, ui32_t c, IPrimerLookup* PrimerLookup) :
  MemIOWriter(p, c), m_Lookup(PrimerLookup)
{
  assert(c > 3);
}

//
ASDCP::Result_t
ASDCP::MXF::TLVWriter::WriteTag(const MDDEntry& Entry)
{
  if ( m_Lookup == 0 )
    {
      DefaultLogSink().Error("No Primer object available\n");
      return RESULT_FAIL;
    }

  TagValue TmpTag;

  if ( m_Lookup->InsertTag(Entry, TmpTag) != RESULT_OK )
    {
      DefaultLogSink().Error("No tag for entry %s\n", Entry.name);
      return RESULT_FAIL;
    }

  if ( ! MemIOWriter::WriteUi8(TmpTag.a) ) return RESULT_KLV_CODING;
  if ( ! MemIOWriter::WriteUi8(TmpTag.b) ) return RESULT_KLV_CODING;
  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVWriter::WriteObject(const MDDEntry& Entry, Kumu::IArchive* Object)
{
  ASDCP_TEST_NULL(Object);

  if ( Entry.optional && ! Object->HasValue() )
    return RESULT_OK;

  Result_t result = WriteTag(Entry);

  if ( ASDCP_SUCCESS(result) )
    {
      // write a temp length
      byte_t* l_p = CurrentData();

      if ( ! MemIOWriter::WriteUi16BE(0) ) return RESULT_KLV_CODING;

      ui32_t before = Length();
      if ( ! Object->Archive(this) ) return RESULT_KLV_CODING;
      if ( (Length() - before) > 0xffffL ) return RESULT_KLV_CODING;
      Kumu::i2p<ui16_t>(KM_i16_BE(Length() - before), l_p);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVWriter::WriteUi8(const MDDEntry& Entry, ui8_t* value)
{
  ASDCP_TEST_NULL(value);
  Result_t result = WriteTag(Entry);

  if ( ASDCP_SUCCESS(result) )
    {
      if ( ! MemIOWriter::WriteUi16BE(sizeof(ui8_t)) ) return RESULT_KLV_CODING;
      if ( ! MemIOWriter::WriteUi8(*value) ) return RESULT_KLV_CODING;
    }
  
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVWriter::WriteUi16(const MDDEntry& Entry, ui16_t* value)
{
  ASDCP_TEST_NULL(value);
  Result_t result = WriteTag(Entry);

  if ( KM_SUCCESS(result) )
    {
      if ( ! MemIOWriter::WriteUi16BE(sizeof(ui16_t)) ) return RESULT_KLV_CODING;
      if ( ! MemIOWriter::WriteUi16BE(*value) ) return RESULT_KLV_CODING;
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVWriter::WriteUi32(const MDDEntry& Entry, ui32_t* value)
{
  ASDCP_TEST_NULL(value);
  Result_t result = WriteTag(Entry);

  if ( KM_SUCCESS(result) )
    {
      if ( ! MemIOWriter::WriteUi16BE(sizeof(ui32_t)) ) return RESULT_KLV_CODING;
      if ( ! MemIOWriter::WriteUi32BE(*value) ) return RESULT_KLV_CODING;
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVWriter::WriteUi64(const MDDEntry& Entry, ui64_t* value)
{
  ASDCP_TEST_NULL(value);
  Result_t result = WriteTag(Entry);

  if ( KM_SUCCESS(result) )
    {
      if ( ! MemIOWriter::WriteUi16BE(sizeof(ui64_t)) ) return RESULT_KLV_CODING;
      if ( ! MemIOWriter::WriteUi64BE(*value) ) return RESULT_KLV_CODING;
    }

  return result;
}


//----------------------------------------------------------------------------------------------------
//

ASDCP::MXF::Raw::Raw()
{
  Capacity(256);
}

ASDCP::MXF::Raw::~Raw()
{
}

//
bool
ASDCP::MXF::Raw::Unarchive(Kumu::MemIOReader* Reader)
{
  ui32_t payload_size = Reader->Remainder();
  if ( payload_size == 0 ) return false;
  if ( KM_FAILURE(Capacity(payload_size)) ) return false;

  memcpy(Data(), Reader->CurrentData(), payload_size);
  Length(payload_size);
  return true;
}

//
bool
ASDCP::MXF::Raw::Archive(Kumu::MemIOWriter* Writer) const
{
  return Writer->WriteRaw(RoData(), Length());
}

//
const char*
ASDCP::MXF::Raw::EncodeString(char* str_buf, ui32_t buf_len) const
{
  *str_buf = 0;
  Kumu::bin2hex(RoData(), Length(), str_buf, buf_len);
  return str_buf;
}

//
// end MXFTypes.cpp
//
