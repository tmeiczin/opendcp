/*
Copyright (c) 2005-2013, John Hurst
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
/*! \file    MXF.cpp
    \version $Id: MXF.cpp,v 1.65.2.3 2013/12/20 19:51:40 jhurst Exp $
    \brief   MXF objects
*/

#include "MXF.h"
#include "Metadata.h"
#include <KM_log.h>

using Kumu::DefaultLogSink;
using Kumu::GenRandomValue;

// index segments must be < 64K
// NOTE: this value may too high if advanced index entry elements are used.
const ui32_t CBRIndexEntriesPerSegment = 5000;

//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::SeekToRIP(const Kumu::FileReader& Reader)
{
  Kumu::fpos_t end_pos;

  // go to the end - 4 bytes
  Result_t result = Reader.Seek(0, Kumu::SP_END);

  if ( ASDCP_SUCCESS(result) )
    result = Reader.Tell(&end_pos);

  if ( ASDCP_SUCCESS(result)
       && end_pos < (SMPTE_UL_LENGTH+MXF_BER_LENGTH) )
    {
      DefaultLogSink().Error("File is smaller than an KLV empty packet.\n");
      result = RESULT_FAIL;
    }

  if ( ASDCP_SUCCESS(result) )
    result = Reader.Seek(end_pos - 4);

  // get the ui32_t RIP length
  ui32_t read_count;
  byte_t intbuf[MXF_BER_LENGTH];
  ui32_t rip_size = 0;

  if ( ASDCP_SUCCESS(result) )
    {
      result = Reader.Read(intbuf, MXF_BER_LENGTH, &read_count);

      if ( ASDCP_SUCCESS(result) && read_count != 4 )
	{
	  DefaultLogSink().Error("RIP contains fewer than four bytes.\n");
	  result = RESULT_FAIL;
	}
    }

  if ( ASDCP_SUCCESS(result) )
    {
      rip_size = KM_i32_BE(Kumu::cp2i<ui32_t>(intbuf));

      if ( rip_size > end_pos ) // RIP can't be bigger than the file
	{
	  DefaultLogSink().Error("RIP size impossibly large.\n");
	  return RESULT_FAIL;
	}
    }

  // reposition to start of RIP
  if ( ASDCP_SUCCESS(result) )
    result = Reader.Seek(end_pos - rip_size);

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::RIP::GetPairBySID(ui32_t SID, Pair& outPair) const
{
  Array<Pair>::const_iterator pi = PairArray.begin();
  for ( ; pi != PairArray.end(); pi++ )
    {
      if ( (*pi).BodySID == SID )
	{
	  outPair = *pi;
	  return RESULT_OK;
	}
    }

  return RESULT_FAIL;
}

//
ASDCP::Result_t
ASDCP::MXF::RIP::InitFromFile(const Kumu::FileReader& Reader)
{
  assert(m_Dict);
  Result_t result = KLVFilePacket::InitFromFile(Reader, m_Dict->ul(MDD_RandomIndexMetadata));

  if ( ASDCP_SUCCESS(result) )
    {
      Kumu::MemIOReader MemRDR(m_ValueStart, m_ValueLength - 4);
      result = PairArray.Unarchive(&MemRDR) ? RESULT_OK : RESULT_KLV_CODING;
    }

  if ( ASDCP_FAILURE(result) )
    DefaultLogSink().Error("Failed to initialize RIP\n");

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::RIP::WriteToFile(Kumu::FileWriter& Writer)
{
  assert(m_Dict);
  ASDCP::FrameBuffer Buffer;
  ui32_t RIPSize = ( PairArray.size() * (sizeof(ui32_t) + sizeof(ui64_t)) ) + 4;
  Result_t result = Buffer.Capacity(RIPSize);

  if ( ASDCP_SUCCESS(result) )
    result = WriteKLToFile(Writer, m_Dict->ul(MDD_RandomIndexMetadata), RIPSize);

  if ( ASDCP_SUCCESS(result) )
    {
      result = RESULT_KLV_CODING;

      Kumu::MemIOWriter MemWRT(Buffer.Data(), Buffer.Capacity());
      if ( PairArray.Archive(&MemWRT) )
	if ( MemWRT.WriteUi32BE(RIPSize + 20) )
	  {
	    Buffer.Size(MemWRT.Length());
	    result = RESULT_OK;
	  }
    }

  if ( ASDCP_SUCCESS(result) )
    result = Writer.Write(Buffer.RoData(), Buffer.Size());

  return result;
}

//
void
ASDCP::MXF::RIP::Dump(FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  KLVFilePacket::Dump(stream, *m_Dict, false);
  PairArray.Dump(stream, false);
}

//------------------------------------------------------------------------------------------
//

//
ASDCP::MXF::Partition::PacketList::~PacketList() {
  while ( ! m_List.empty() )
    {
      delete m_List.back();
      m_List.pop_back();
    }
}

//
void
ASDCP::MXF::Partition::PacketList::AddPacket(InterchangeObject* ThePacket) // takes ownership
{
  assert(ThePacket);
  m_Map.insert(std::map<UUID, InterchangeObject*>::value_type(ThePacket->InstanceUID, ThePacket));
  m_List.push_back(ThePacket);
}

//
ASDCP::Result_t
ASDCP::MXF::Partition::PacketList::GetMDObjectByID(const UUID& ObjectID, InterchangeObject** Object)
{
  ASDCP_TEST_NULL(Object);

  std::map<UUID, InterchangeObject*>::iterator mi = m_Map.find(ObjectID);
  
  if ( mi == m_Map.end() )
    {
      *Object = 0;
      return RESULT_FAIL;
    }

  *Object = (*mi).second;
  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::MXF::Partition::PacketList::GetMDObjectByType(const byte_t* ObjectID, InterchangeObject** Object)
{
  ASDCP_TEST_NULL(ObjectID);
  ASDCP_TEST_NULL(Object);
  std::list<InterchangeObject*>::iterator li;
  *Object = 0;

  for ( li = m_List.begin(); li != m_List.end(); li++ )
    {
      if ( (*li)->HasUL(ObjectID) )
	{
	  *Object = *li;
	  return RESULT_OK;
	}
    }

  return RESULT_FAIL;
}

//
ASDCP::Result_t
ASDCP::MXF::Partition::PacketList::GetMDObjectsByType(const byte_t* ObjectID, std::list<InterchangeObject*>& ObjectList)
{
  ASDCP_TEST_NULL(ObjectID);
  std::list<InterchangeObject*>::iterator li;

  for ( li = m_List.begin(); li != m_List.end(); li++ )
    {
      if ( (*li)->HasUL(ObjectID) )
	ObjectList.push_back(*li);
    }

  return ObjectList.empty() ? RESULT_FAIL : RESULT_OK;
}

//------------------------------------------------------------------------------------------
//


ASDCP::MXF::Partition::Partition(const Dictionary*& d) :
  m_Dict(d),
  MajorVersion(1), MinorVersion(2),
  KAGSize(1), ThisPartition(0), PreviousPartition(0),
  FooterPartition(0), HeaderByteCount(0), IndexByteCount(0), IndexSID(0),
  BodyOffset(0), BodySID(0)
{
  m_PacketList = new PacketList;
}

ASDCP::MXF::Partition::~Partition()
{
}

// takes ownership
void
ASDCP::MXF::Partition::AddChildObject(InterchangeObject* Object)
{
  assert(Object);

  if ( ! Object->InstanceUID.HasValue() )
    GenRandomValue(Object->InstanceUID);

  m_PacketList->AddPacket(Object);
}

//
ASDCP::Result_t
ASDCP::MXF::Partition::InitFromFile(const Kumu::FileReader& Reader)
{
  Result_t result = KLVFilePacket::InitFromFile(Reader);
  // test the UL
  // could be one of several values
  if ( ASDCP_SUCCESS(result) )
    result = ASDCP::MXF::Partition::InitFromBuffer(m_ValueStart, m_ValueLength);
  
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Partition::InitFromBuffer(const byte_t* p, ui32_t l)
{
  Kumu::MemIOReader MemRDR(p, l);
  Result_t result = RESULT_KLV_CODING;

  if ( MemRDR.ReadUi16BE(&MajorVersion) )
    if ( MemRDR.ReadUi16BE(&MinorVersion) )
      if ( MemRDR.ReadUi32BE(&KAGSize) )
	if ( MemRDR.ReadUi64BE(&ThisPartition) )
	  if ( MemRDR.ReadUi64BE(&PreviousPartition) )
	    if ( MemRDR.ReadUi64BE(&FooterPartition) )
	      if ( MemRDR.ReadUi64BE(&HeaderByteCount) )
		if ( MemRDR.ReadUi64BE(&IndexByteCount) )
		  if ( MemRDR.ReadUi32BE(&IndexSID) )
		    if ( MemRDR.ReadUi64BE(&BodyOffset) )
		      if ( MemRDR.ReadUi32BE(&BodySID) )
			if ( OperationalPattern.Unarchive(&MemRDR) )
			  if ( EssenceContainers.Unarchive(&MemRDR) )
			    result = RESULT_OK;

  if ( ASDCP_FAILURE(result) )
    DefaultLogSink().Error("Failed to initialize Partition\n");

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Partition::WriteToFile(Kumu::FileWriter& Writer, UL& PartitionLabel)
{
  ASDCP::FrameBuffer Buffer;
  Result_t result = Buffer.Capacity(1024);

  if ( ASDCP_SUCCESS(result) )
    {
      Kumu::MemIOWriter MemWRT(Buffer.Data(), Buffer.Capacity());
      result = RESULT_KLV_CODING;
      if ( MemWRT.WriteUi16BE(MajorVersion) )
	if ( MemWRT.WriteUi16BE(MinorVersion) )
	  if ( MemWRT.WriteUi32BE(KAGSize) )
	    if ( MemWRT.WriteUi64BE(ThisPartition) )
	      if ( MemWRT.WriteUi64BE(PreviousPartition) )
		if ( MemWRT.WriteUi64BE(FooterPartition) )
		  if ( MemWRT.WriteUi64BE(HeaderByteCount) )
		    if ( MemWRT.WriteUi64BE(IndexByteCount) )
		      if ( MemWRT.WriteUi32BE(IndexSID) )
			if ( MemWRT.WriteUi64BE(BodyOffset) )
			  if ( MemWRT.WriteUi32BE(BodySID) )
			    if ( OperationalPattern.Archive(&MemWRT) )
			      if ( EssenceContainers.Archive(&MemWRT) )
				{
				  Buffer.Size(MemWRT.Length());
				  result = RESULT_OK;
				}
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t write_count;
      result = WriteKLToFile(Writer, PartitionLabel.Value(), Buffer.Size());

      if ( ASDCP_SUCCESS(result) )
	result = Writer.Write(Buffer.RoData(), Buffer.Size(), &write_count);
    }

  return result;
}

//
ui32_t
ASDCP::MXF::Partition::ArchiveSize()
{
  return ( kl_length
	   + sizeof(ui16_t) + sizeof(ui16_t)
	   + sizeof(ui32_t)
	   + sizeof(ui64_t) + sizeof(ui64_t) + sizeof(ui64_t) + sizeof(ui64_t) + sizeof(ui64_t)
	   + sizeof(ui32_t)
	   + sizeof(ui64_t)
	   + sizeof(ui32_t)
	   + SMPTE_UL_LENGTH
	   + sizeof(ui32_t) + sizeof(ui32_t) + ( UUIDlen * EssenceContainers.size() ) );
}

//
void
ASDCP::MXF::Partition::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVFilePacket::Dump(stream, *m_Dict, false);
  fprintf(stream, "  MajorVersion       = %hu\n", MajorVersion);
  fprintf(stream, "  MinorVersion       = %hu\n", MinorVersion);
  fprintf(stream, "  KAGSize            = %u\n",  KAGSize);
  fprintf(stream, "  ThisPartition      = %s\n",  ui64sz(ThisPartition, identbuf));
  fprintf(stream, "  PreviousPartition  = %s\n",  ui64sz(PreviousPartition, identbuf));
  fprintf(stream, "  FooterPartition    = %s\n",  ui64sz(FooterPartition, identbuf));
  fprintf(stream, "  HeaderByteCount    = %s\n",  ui64sz(HeaderByteCount, identbuf));
  fprintf(stream, "  IndexByteCount     = %s\n",  ui64sz(IndexByteCount, identbuf));
  fprintf(stream, "  IndexSID           = %u\n",  IndexSID);
  fprintf(stream, "  BodyOffset         = %s\n",  ui64sz(BodyOffset, identbuf));
  fprintf(stream, "  BodySID            = %u\n",  BodySID);
  fprintf(stream, "  OperationalPattern = %s\n",  OperationalPattern.EncodeString(identbuf, IdentBufferLen));
  fputs("Essence Containers:\n", stream); EssenceContainers.Dump(stream);
}


//------------------------------------------------------------------------------------------
//

class ASDCP::MXF::Primer::h__PrimerLookup : public std::map<UL, TagValue>
{
public:
  void InitWithBatch(ASDCP::MXF::Batch<ASDCP::MXF::Primer::LocalTagEntry>& Batch)
  {
    ASDCP::MXF::Batch<ASDCP::MXF::Primer::LocalTagEntry>::iterator i = Batch.begin();

    for ( ; i != Batch.end(); i++ )
      insert(std::map<UL, TagValue>::value_type((*i).UL, (*i).Tag));
  }
};


//
ASDCP::MXF::Primer::Primer(const Dictionary*& d) : m_LocalTag(0xff), m_Dict(d) {
  m_UL = m_Dict->ul(MDD_Primer);
}

//
ASDCP::MXF::Primer::~Primer() {}

//
void
ASDCP::MXF::Primer::ClearTagList()
{
  LocalTagEntryBatch.clear();
  m_Lookup = new h__PrimerLookup;
}

//
ASDCP::Result_t
ASDCP::MXF::Primer::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  Result_t result = KLVPacket::InitFromBuffer(p, l, m_Dict->ul(MDD_Primer));

  if ( ASDCP_SUCCESS(result) )
    {
      Kumu::MemIOReader MemRDR(m_ValueStart, m_ValueLength);
      result = LocalTagEntryBatch.Unarchive(&MemRDR) ? RESULT_OK : RESULT_KLV_CODING;
    }

  if ( ASDCP_SUCCESS(result) )
    {
      m_Lookup = new h__PrimerLookup;
      m_Lookup->InitWithBatch(LocalTagEntryBatch);
    }

  if ( ASDCP_FAILURE(result) )
    DefaultLogSink().Error("Failed to initialize Primer\n");

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Primer::WriteToFile(Kumu::FileWriter& Writer)
{
  ASDCP::FrameBuffer Buffer;
  Result_t result = Buffer.Capacity(128*1024);

  if ( ASDCP_SUCCESS(result) )
    result = WriteToBuffer(Buffer);

  if ( ASDCP_SUCCESS(result) )
  result = Writer.Write(Buffer.RoData(), Buffer.Size());

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Primer::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  assert(m_Dict);
  ASDCP::FrameBuffer LocalTagBuffer;
  Kumu::MemIOWriter MemWRT(Buffer.Data() + kl_length, Buffer.Capacity() - kl_length);
  Result_t result = LocalTagEntryBatch.Archive(&MemWRT) ? RESULT_OK : RESULT_KLV_CODING;

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t packet_length = MemWRT.Length();
      result = WriteKLToBuffer(Buffer, packet_length);

      if ( ASDCP_SUCCESS(result) )
	Buffer.Size(Buffer.Size() + packet_length);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Primer::InsertTag(const MDDEntry& Entry, ASDCP::TagValue& Tag)
{
  assert(m_Lookup);
  UL TestUL(Entry.ul);
  std::map<UL, TagValue>::iterator i = m_Lookup->find(TestUL);

  if ( i == m_Lookup->end() )
    {
      if ( Entry.tag.a == 0 && Entry.tag.b == 0 )
	{
	  Tag.a = 0xff;
	  Tag.b = m_LocalTag--;
	}
      else
	{
	  Tag.a = Entry.tag.a;
	  Tag.b = Entry.tag.b;
	}

      LocalTagEntry TmpEntry;
      TmpEntry.UL = TestUL;
      TmpEntry.Tag = Tag;

      LocalTagEntryBatch.push_back(TmpEntry);
      m_Lookup->insert(std::map<UL, TagValue>::value_type(TmpEntry.UL, TmpEntry.Tag));
    }
  else
    {
      Tag = (*i).second;
    }
   
  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::MXF::Primer::TagForKey(const ASDCP::UL& Key, ASDCP::TagValue& Tag)
{
  assert(m_Lookup);
  if ( m_Lookup.empty() )
    {
      DefaultLogSink().Error("Primer lookup is empty\n");
      return RESULT_FAIL;
    }

  std::map<UL, TagValue>::iterator i = m_Lookup->find(Key);

  if ( i == m_Lookup->end() )
    return RESULT_FALSE;

  Tag = (*i).second;
  return RESULT_OK;
}

//
void
ASDCP::MXF::Primer::Dump(FILE* stream)
{
  assert(m_Dict);
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVPacket::Dump(stream, *m_Dict, false);
  fprintf(stream, "Primer: %u %s\n",
	  (ui32_t)LocalTagEntryBatch.size(),
	  ( LocalTagEntryBatch.size() == 1 ? "entry" : "entries" ));
  
  Batch<LocalTagEntry>::iterator i = LocalTagEntryBatch.begin();
  for ( ; i != LocalTagEntryBatch.end(); i++ )
    {
      const MDDEntry* Entry = m_Dict->FindUL((*i).UL.Value());
      fprintf(stream, "  %s %s\n", (*i).EncodeString(identbuf, IdentBufferLen), (Entry ? Entry->name : "Unknown"));
    }
}


//------------------------------------------------------------------------------------------
//

//
ASDCP::MXF::Preface::Preface(const Dictionary*& d) :
  InterchangeObject(d), m_Dict(d), Version(258), ObjectModelVersion(0)
{
  assert(m_Dict);
  m_UL = m_Dict->Type(MDD_Preface).ul;
}

//
void
ASDCP::MXF::Preface::Copy(const Preface& rhs)
{
  InterchangeObject::Copy(rhs);

  LastModifiedDate = rhs.LastModifiedDate;
  Version = rhs.Version;
  ObjectModelVersion = rhs.ObjectModelVersion;
  PrimaryPackage = rhs.PrimaryPackage;
  Identifications = rhs.Identifications;
  ContentStorage = rhs.ContentStorage;
  OperationalPattern = rhs.OperationalPattern;
  EssenceContainers = rhs.EssenceContainers;
  DMSchemes = rhs.DMSchemes;
}

//
ASDCP::Result_t
ASDCP::MXF::Preface::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Preface, LastModifiedDate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(Preface, Version));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(Preface, ObjectModelVersion));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Preface, PrimaryPackage));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Preface, Identifications));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Preface, ContentStorage));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Preface, OperationalPattern));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Preface, EssenceContainers));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Preface, DMSchemes));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Preface::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Preface, LastModifiedDate));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(Preface, Version));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(Preface, ObjectModelVersion));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Preface, PrimaryPackage));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Preface, Identifications));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Preface, ContentStorage));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Preface, OperationalPattern));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Preface, EssenceContainers));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Preface, DMSchemes));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Preface::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::Preface::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//
void
ASDCP::MXF::Preface::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "LastModifiedDate", LastModifiedDate.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %hu\n", "Version", Version);
  fprintf(stream, "  %22s = %u\n",  "ObjectModelVersion", ObjectModelVersion);
  fprintf(stream, "  %22s = %s\n",  "PrimaryPackage", PrimaryPackage.EncodeHex(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s:\n", "Identifications");  Identifications.Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "ContentStorage", ContentStorage.EncodeHex(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "OperationalPattern", OperationalPattern.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s:\n", "EssenceContainers");  EssenceContainers.Dump(stream);
  fprintf(stream, "  %22s:\n", "DMSchemes");  DMSchemes.Dump(stream);
}

//------------------------------------------------------------------------------------------
//

ASDCP::MXF::OPAtomHeader::OPAtomHeader(const Dictionary*& d) : Partition(d), m_Dict(d), m_RIP(d), m_Primer(d), m_Preface(0), m_HasRIP(false) {}
ASDCP::MXF::OPAtomHeader::~OPAtomHeader() {}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomHeader::InitFromFile(const Kumu::FileReader& Reader)
{
  m_HasRIP = false;
  Result_t result = SeekToRIP(Reader);

  if ( ASDCP_SUCCESS(result) )
    {
      result = m_RIP.InitFromFile(Reader);
      ui32_t test_s = m_RIP.PairArray.size();

      if ( ASDCP_FAILURE(result) )
	{
	  DefaultLogSink().Error("File contains no RIP\n");
	  result = RESULT_OK;
	}
      else if ( test_s == 0 )
	{
	  DefaultLogSink().Error("RIP contains no Pairs.\n");
	  result = RESULT_FORMAT;
	}
      else
	{
	  if ( test_s < 2 )
	    {
	      // OP-Atom states that there will be either two or three partitions:
	      // one closed header and one closed footer with an optional body
	      // SMPTE 429-5 files may have many partitions, see SMPTE 410M
	      DefaultLogSink().Warn("RIP count is less than 2: %u\n", test_s);
	    }

	  m_HasRIP = true;
      
	  if ( m_RIP.PairArray.front().ByteOffset !=  0 )
	    {
	      DefaultLogSink().Error("First Partition in RIP is not at offset 0.\n");
	      result = RESULT_FORMAT;
	    }
	}
    }
  else
  {
    DefaultLogSink().Error("OPAtomHeader::InitFromFile, SeekToRIP failed\n");
  }

  if ( ASDCP_SUCCESS(result) )
    result = Reader.Seek(0);
  else
    DefaultLogSink().Error("OPAtomHeader::InitFromFile, Seek failed\n");

  if ( ASDCP_SUCCESS(result) )
    result = Partition::InitFromFile(Reader); // test UL and OP
  else
    DefaultLogSink().Error("OPAtomHeader::InitFromFile, Partition::InitFromFile failed\n");

  if ( ASDCP_FAILURE(result) )
    return result;

  // is it really OP-Atom?
  assert(m_Dict);
  UL OPAtomUL(SMPTE_390_OPAtom_Entry().ul);
  UL InteropOPAtomUL(MXFInterop_OPAtom_Entry().ul);

  if ( OperationalPattern.ExactMatch(OPAtomUL) ) // SMPTE
    {
      if ( m_Dict == &DefaultCompositeDict() )
	m_Dict = &DefaultSMPTEDict();
    }
  else if ( OperationalPattern.ExactMatch(InteropOPAtomUL) ) // Interop
    {
      if ( m_Dict == &DefaultCompositeDict() )
	m_Dict = &DefaultInteropDict();
    }
  else
    {
      char strbuf[IdentBufferLen];
      const MDDEntry* Entry = m_Dict->FindUL(OperationalPattern.Value());
      if ( Entry == 0 )
	DefaultLogSink().Warn("Operational pattern is not OP-Atom: %s\n",
			      OperationalPattern.EncodeString(strbuf, IdentBufferLen));
      else
	DefaultLogSink().Warn("Operational pattern is not OP-Atom: %s\n", Entry->name);
    }

  // slurp up the remainder of the header
  if ( HeaderByteCount < 1024 )
    DefaultLogSink().Warn("Improbably small HeaderByteCount value: %u\n", HeaderByteCount);

  assert (HeaderByteCount <= 0xFFFFFFFFL);
  result = m_Buffer.Capacity((ui32_t) HeaderByteCount);

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t read_count;
      result = Reader.Read(m_Buffer.Data(), m_Buffer.Capacity(), &read_count);

      if ( ASDCP_FAILURE(result) )
        {
	  DefaultLogSink().Error("OPAtomHeader::InitFromFile, Read failed\n");
	  return result;
        }

      if ( read_count != m_Buffer.Capacity() )
	{
	  DefaultLogSink().Error("Short read of OP-Atom header metadata; wanted %u, got %u\n",
				 m_Buffer.Capacity(), read_count);
	  return RESULT_KLV_CODING;
	}
    }

  if ( ASDCP_SUCCESS(result) )
    result = InitFromBuffer(m_Buffer.RoData(), m_Buffer.Capacity());

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomHeader::InitFromPartitionBuffer(const byte_t* p, ui32_t l)
{
  Result_t result = KLVPacket::InitFromBuffer(p, l);

  if ( ASDCP_SUCCESS(result) )
    result = Partition::InitFromBuffer(m_ValueStart, m_ValueLength); // test UL and OP

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t pp_len = KLVPacket::PacketLength();
      result = InitFromBuffer(p + pp_len, l - pp_len);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomHeader::InitFromBuffer(const byte_t* p, ui32_t l)
{
  assert(m_Dict);
  Result_t result = RESULT_OK;
  const byte_t* end_p = p + l;

  while ( ASDCP_SUCCESS(result) && p < end_p )
    {
      // parse the packets and index them by uid, discard KLVFill items
      InterchangeObject* object = CreateObject(m_Dict, p);
      assert(object);

      object->m_Lookup = &m_Primer;
      result = object->InitFromBuffer(p, end_p - p);
      const byte_t* redo_p = p;
      p += object->PacketLength();
      //      hexdump(p, object->PacketLength());

      if ( ASDCP_SUCCESS(result) )
	{
	  if ( object->IsA(m_Dict->ul(MDD_KLVFill)) )
	    {
	      delete object;
	    }
	  else if ( object->IsA(m_Dict->ul(MDD_Primer)) ) // TODO: only one primer should be found
	    {
	      delete object;
	      result = m_Primer.InitFromBuffer(redo_p, end_p - redo_p);
	    }
	  else
	    {
	      m_PacketList->AddPacket(object); // takes ownership

	      if ( object->IsA(m_Dict->ul(MDD_Preface)) && m_Preface == 0 )
		m_Preface = (Preface*)object;
	    }
	}
      else
	{
	  DefaultLogSink().Error("Error initializing packet\n");
	  delete object;
	}
    }

  return result;
}

ASDCP::Result_t
ASDCP::MXF::OPAtomHeader::GetMDObjectByID(const UUID& ObjectID, InterchangeObject** Object)
{
  return m_PacketList->GetMDObjectByID(ObjectID, Object);
}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomHeader::GetMDObjectByType(const byte_t* ObjectID, InterchangeObject** Object)
{
  InterchangeObject* TmpObject;

  if ( Object == 0 )
    Object = &TmpObject;

  return m_PacketList->GetMDObjectByType(ObjectID, Object);
}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomHeader::GetMDObjectsByType(const byte_t* ObjectID, std::list<InterchangeObject*>& ObjectList)
{
  return m_PacketList->GetMDObjectsByType(ObjectID, ObjectList);
}

//
ASDCP::MXF::Identification*
ASDCP::MXF::OPAtomHeader::GetIdentification()
{
  InterchangeObject* Object;

  if ( ASDCP_SUCCESS(GetMDObjectByType(OBJ_TYPE_ARGS(Identification), &Object)) )
    return (Identification*)Object;

  return 0;
}

//
ASDCP::MXF::SourcePackage*
ASDCP::MXF::OPAtomHeader::GetSourcePackage()
{
  InterchangeObject* Object;

  if ( ASDCP_SUCCESS(GetMDObjectByType(OBJ_TYPE_ARGS(SourcePackage), &Object)) )
    return (SourcePackage*)Object;

  return 0;
}

//
ASDCP::MXF::RIP&
ASDCP::MXF::OPAtomHeader::GetRIP() { return m_RIP; }

//
ASDCP::Result_t
ASDCP::MXF::OPAtomHeader::WriteToFile(Kumu::FileWriter& Writer, ui32_t HeaderSize)
{
  assert(m_Dict);
  if ( m_Preface == 0 )
    return RESULT_STATE;

  if ( HeaderSize < 4096 ) 
    {
      DefaultLogSink().Error("HeaderSize %u is too small. Must be >= 4096\n", HeaderSize);
      return RESULT_FAIL;
    }

  ASDCP::FrameBuffer HeaderBuffer;
  HeaderByteCount = HeaderSize - ArchiveSize();
  assert (HeaderByteCount <= 0xFFFFFFFFL);
  Result_t result = HeaderBuffer.Capacity((ui32_t) HeaderByteCount); 
  m_Preface->m_Lookup = &m_Primer;

  std::list<InterchangeObject*>::iterator pl_i = m_PacketList->m_List.begin();
  for ( ; pl_i != m_PacketList->m_List.end() && ASDCP_SUCCESS(result); pl_i++ )
    {
      InterchangeObject* object = *pl_i;
      object->m_Lookup = &m_Primer;

      ASDCP::FrameBuffer WriteWrapper;
      WriteWrapper.SetData(HeaderBuffer.Data() + HeaderBuffer.Size(),
			   HeaderBuffer.Capacity() - HeaderBuffer.Size());
      result = object->WriteToBuffer(WriteWrapper);
      HeaderBuffer.Size(HeaderBuffer.Size() + WriteWrapper.Size());
    }

  if ( ASDCP_SUCCESS(result) )
    {
      UL TmpUL(m_Dict->ul(MDD_ClosedCompleteHeader));
      result = Partition::WriteToFile(Writer, TmpUL);
    }

  if ( ASDCP_SUCCESS(result) )
    result = m_Primer.WriteToFile(Writer);

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t write_count;
      Writer.Write(HeaderBuffer.RoData(), HeaderBuffer.Size(), &write_count);
      assert(write_count == HeaderBuffer.Size());
    }

  // KLV Fill
  if ( ASDCP_SUCCESS(result) )
    {
      Kumu::fpos_t pos = Writer.Tell();

      if ( pos > (Kumu::fpos_t)HeaderByteCount )
	{
	  char intbuf[IntBufferLen];
	  DefaultLogSink().Error("Header size %s exceeds specified value %u\n",
				 ui64sz(pos, intbuf),
				 HeaderSize);
	  return RESULT_FAIL;
	}

      ASDCP::FrameBuffer NilBuf;
      ui32_t klv_fill_length = HeaderSize - (ui32_t)pos;

      if ( klv_fill_length < kl_length )
	{
	  DefaultLogSink().Error("Remaining region too small for KLV Fill header\n");
	  return RESULT_FAIL;
	}

      klv_fill_length -= kl_length;
      result = WriteKLToFile(Writer, m_Dict->ul(MDD_KLVFill), klv_fill_length);

      if ( ASDCP_SUCCESS(result) )
	result = NilBuf.Capacity(klv_fill_length);

      if ( ASDCP_SUCCESS(result) )
	{
	  memset(NilBuf.Data(), 0, klv_fill_length);
	  ui32_t write_count;
	  Writer.Write(NilBuf.RoData(), klv_fill_length, &write_count);
	  assert(write_count == klv_fill_length);
	}
    }

  return result;
}

//
void
ASDCP::MXF::OPAtomHeader::Dump(FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  Partition::Dump(stream);
  m_Primer.Dump(stream);

  if ( m_Preface == 0 )
    fputs("No Preface loaded\n", stream);

  std::list<InterchangeObject*>::iterator i = m_PacketList->m_List.begin();
  for ( ; i != m_PacketList->m_List.end(); i++ )
    (*i)->Dump(stream);
}

//------------------------------------------------------------------------------------------
//

ASDCP::MXF::OPAtomIndexFooter::OPAtomIndexFooter(const Dictionary*& d) :
  Partition(d), m_Dict(d),
  m_CurrentSegment(0), m_BytesPerEditUnit(0), m_BodySID(0),
  m_ECOffset(0), m_Lookup(0)
{
  BodySID = 0;
  IndexSID = 129;
}

ASDCP::MXF::OPAtomIndexFooter::~OPAtomIndexFooter() {}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomIndexFooter::InitFromFile(const Kumu::FileReader& Reader)
{
  Result_t result = Partition::InitFromFile(Reader); // test UL and OP

	// slurp up the remainder of the footer
	ui32_t read_count = 0;

	if ( ASDCP_SUCCESS(result) )
    {
		assert (IndexByteCount <= 0xFFFFFFFFL);
		// At this point, m_Buffer may not have been initialized
		// so it's capacity is zero and data pointer is NULL
		// However, if IndexByteCount is zero then the capacity
		// doesn't change and the data pointer is not set.
		result = m_Buffer.Capacity((ui32_t) IndexByteCount);
    }

	if ( ASDCP_SUCCESS(result) && m_Buffer.Data() )
		result = Reader.Read(m_Buffer.Data(), m_Buffer.Capacity(), &read_count);

  if ( ASDCP_SUCCESS(result) && read_count != m_Buffer.Capacity() )
    {
      DefaultLogSink().Error("Short read of footer partition: got %u, expecting %u\n",
			     read_count, m_Buffer.Capacity());
      return RESULT_FAIL;
    }
	else if( ASDCP_SUCCESS(result) && !m_Buffer.Data() )
	{
		DefaultLogSink().Error( "Buffer for footer partition not created: IndexByteCount = %u\n",
								IndexByteCount );
		return RESULT_FAIL;
	}

  if ( ASDCP_SUCCESS(result) )
    result = InitFromBuffer(m_Buffer.RoData(), m_Buffer.Capacity());

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomIndexFooter::InitFromPartitionBuffer(const byte_t* p, ui32_t l)
{
  Result_t result = KLVPacket::InitFromBuffer(p, l);

  if ( ASDCP_SUCCESS(result) )
    result = Partition::InitFromBuffer(m_ValueStart, m_ValueLength); // test UL and OP

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t pp_len = KLVPacket::PacketLength();
      result = InitFromBuffer(p + pp_len, l - pp_len);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomIndexFooter::InitFromBuffer(const byte_t* p, ui32_t l)
{
  Result_t result = RESULT_OK;
  const byte_t* end_p = p + l;
  
  while ( ASDCP_SUCCESS(result) && p < end_p )
    {
      // parse the packets and index them by uid, discard KLVFill items
      InterchangeObject* object = CreateObject(m_Dict, p);
      assert(object);

      object->m_Lookup = m_Lookup;
      result = object->InitFromBuffer(p, end_p - p);
      p += object->PacketLength();

      if ( ASDCP_SUCCESS(result) )
	{
	  m_PacketList->AddPacket(object); // takes ownership
	}
      else
	{
	  DefaultLogSink().Error("Error initializing packet\n");
	  delete object;
	}
    }

  if ( ASDCP_FAILURE(result) )
    DefaultLogSink().Error("Failed to initialize OPAtomIndexFooter\n");

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomIndexFooter::WriteToFile(Kumu::FileWriter& Writer, ui64_t duration)
{
  assert(m_Dict);
  ASDCP::FrameBuffer FooterBuffer;
  ui32_t   footer_size = m_PacketList->m_List.size() * MaxIndexSegmentSize; // segment-count * max-segment-size
  Result_t result = FooterBuffer.Capacity(footer_size); 
  ui32_t   iseg_count = 0;

  if ( m_CurrentSegment != 0 )
    {
      m_CurrentSegment->IndexDuration = m_CurrentSegment->IndexEntryArray.size();
      m_CurrentSegment = 0;
    }

  std::list<InterchangeObject*>::iterator pl_i = m_PacketList->m_List.begin();
  for ( ; pl_i != m_PacketList->m_List.end() && ASDCP_SUCCESS(result); pl_i++ )
    {
      if ( (*pl_i)->IsA(OBJ_TYPE_ARGS(IndexTableSegment)) )
	{
	  iseg_count++;
	  IndexTableSegment* Segment = (IndexTableSegment*)(*pl_i);

	  if ( m_BytesPerEditUnit != 0 )
	    {
	      if ( iseg_count != 1 )
		return RESULT_STATE;

	      Segment->IndexDuration = duration;
	    }
	}

      InterchangeObject* object = *pl_i;
      object->m_Lookup = m_Lookup;

      ASDCP::FrameBuffer WriteWrapper;
      WriteWrapper.SetData(FooterBuffer.Data() + FooterBuffer.Size(),
			   FooterBuffer.Capacity() - FooterBuffer.Size());
      result = object->WriteToBuffer(WriteWrapper);
      FooterBuffer.Size(FooterBuffer.Size() + WriteWrapper.Size());
    }

  if ( ASDCP_SUCCESS(result) )
    {
      IndexByteCount = FooterBuffer.Size();
      UL FooterUL(m_Dict->ul(MDD_CompleteFooter));
      result = Partition::WriteToFile(Writer, FooterUL);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t write_count = 0;
      result = Writer.Write(FooterBuffer.RoData(), FooterBuffer.Size(), &write_count);
      assert(write_count == FooterBuffer.Size());
    }

  return result;
}

//
void
ASDCP::MXF::OPAtomIndexFooter::Dump(FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  Partition::Dump(stream);

  std::list<InterchangeObject*>::iterator i = m_PacketList->m_List.begin();
  for ( ; i != m_PacketList->m_List.end(); i++ )
    (*i)->Dump(stream);
}

ASDCP::Result_t
ASDCP::MXF::OPAtomIndexFooter::GetMDObjectByID(const UUID& ObjectID, InterchangeObject** Object)
{
  return m_PacketList->GetMDObjectByID(ObjectID, Object);
}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomIndexFooter::GetMDObjectByType(const byte_t* ObjectID, InterchangeObject** Object)
{
  InterchangeObject* TmpObject;

  if ( Object == 0 )
    Object = &TmpObject;

  return m_PacketList->GetMDObjectByType(ObjectID, Object);
}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomIndexFooter::GetMDObjectsByType(const byte_t* ObjectID, std::list<InterchangeObject*>& ObjectList)
{
  return m_PacketList->GetMDObjectsByType(ObjectID, ObjectList);
}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomIndexFooter::Lookup(ui32_t frame_num, IndexTableSegment::IndexEntry& Entry) const
{
  std::list<InterchangeObject*>::iterator li;
  for ( li = m_PacketList->m_List.begin(); li != m_PacketList->m_List.end(); li++ )
    {
      if ( (*li)->IsA(OBJ_TYPE_ARGS(IndexTableSegment)) )
	{
	  IndexTableSegment* Segment = (IndexTableSegment*)(*li);
	  ui64_t start_pos = Segment->IndexStartPosition;

	  if ( Segment->EditUnitByteCount > 0 )
	    {
	      if ( m_PacketList->m_List.size() > 1 )
		DefaultLogSink().Error("Unexpected multiple IndexTableSegment in CBR file\n");

	      if ( ! Segment->IndexEntryArray.empty() )
		DefaultLogSink().Error("Unexpected IndexEntryArray contents in CBR file\n");

	      Entry.StreamOffset = (ui64_t)frame_num * Segment->EditUnitByteCount;
	      return RESULT_OK;
	    }
	  else if ( (ui64_t)frame_num >= start_pos
		    && (ui64_t)frame_num < (start_pos + Segment->IndexDuration) )
	    {
	      ui64_t tmp = frame_num - start_pos;
	      assert(tmp <= 0xFFFFFFFFL);
	      Entry = Segment->IndexEntryArray[(ui32_t) tmp];
	      return RESULT_OK;
	    }
	}
    }

  return RESULT_FAIL;
}

//
void
ASDCP::MXF::OPAtomIndexFooter::SetIndexParamsCBR(IPrimerLookup* lookup, ui32_t size, const Rational& Rate)
{
  assert(lookup);
  m_Lookup = lookup;
  m_BytesPerEditUnit = size;
  m_EditRate = Rate;

  IndexTableSegment* Index = new IndexTableSegment(m_Dict);
  AddChildObject(Index);
  Index->EditUnitByteCount = m_BytesPerEditUnit;
  Index->IndexEditRate = Rate;
}

//
void
ASDCP::MXF::OPAtomIndexFooter::SetIndexParamsVBR(IPrimerLookup* lookup, const Rational& Rate, Kumu::fpos_t offset)
{
  assert(lookup);
  m_Lookup = lookup;
  m_BytesPerEditUnit = 0;
  m_EditRate = Rate;
  m_ECOffset = offset;
}

//
void
ASDCP::MXF::OPAtomIndexFooter::PushIndexEntry(const IndexTableSegment::IndexEntry& Entry)
{
  if ( m_BytesPerEditUnit != 0 )  // are we CBR? that's bad 
    {
      DefaultLogSink().Error("Call to PushIndexEntry() failed: index is CBR\n");
      return;
    }

  // do we have an available segment?
  if ( m_CurrentSegment == 0 )
    { // no, set up a new segment
      m_CurrentSegment = new IndexTableSegment(m_Dict);
      assert(m_CurrentSegment);
      AddChildObject(m_CurrentSegment);
      m_CurrentSegment->DeltaEntryArray.push_back(IndexTableSegment::DeltaEntry());
      m_CurrentSegment->IndexEditRate = m_EditRate;
      m_CurrentSegment->IndexStartPosition = 0;
    }
  else if ( m_CurrentSegment->IndexEntryArray.size() >= CBRIndexEntriesPerSegment )
    { // no, this one is full, start another
      m_CurrentSegment->IndexDuration = m_CurrentSegment->IndexEntryArray.size();
      ui64_t StartPosition = m_CurrentSegment->IndexStartPosition + m_CurrentSegment->IndexDuration;

      m_CurrentSegment = new IndexTableSegment(m_Dict);
      assert(m_CurrentSegment);
      AddChildObject(m_CurrentSegment);
      m_CurrentSegment->DeltaEntryArray.push_back(IndexTableSegment::DeltaEntry());
      m_CurrentSegment->IndexEditRate = m_EditRate;
      m_CurrentSegment->IndexStartPosition = StartPosition;
    }

  m_CurrentSegment->IndexEntryArray.push_back(Entry);
}

//------------------------------------------------------------------------------------------
//

//
void
ASDCP::MXF::InterchangeObject::Copy(const InterchangeObject& rhs)
{
  m_UL = rhs.m_UL;
  InstanceUID = rhs.InstanceUID;
  GenerationUID = rhs.GenerationUID;
}

//
ASDCP::Result_t
ASDCP::MXF::InterchangeObject::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = TLVSet.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
  if ( ASDCP_SUCCESS(result) )
    result = TLVSet.ReadObject(OBJ_READ_ARGS(GenerationInterchangeObject, GenerationUID));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::InterchangeObject::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = TLVSet.WriteObject(OBJ_WRITE_ARGS(InterchangeObject, InstanceUID));
  if ( ASDCP_SUCCESS(result) )
    result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenerationInterchangeObject, GenerationUID));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::InterchangeObject::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);
  Result_t result = RESULT_FALSE;

  if ( m_UL.HasValue() )
    {
      result = KLVPacket::InitFromBuffer(p, l, m_UL);

      if ( ASDCP_SUCCESS(result) )
	{
	  TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
	  result = InitFromTLVSet(MemRDR);
	}
    }
  else
    {
      result = KLVPacket::InitFromBuffer(p, l);
    }
  
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::InterchangeObject::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  if ( ! m_UL.HasValue() )
    return RESULT_STATE;

  TLVWriter MemWRT(Buffer.Data() + kl_length, Buffer.Capacity() - kl_length, m_Lookup);
  Result_t result = WriteToTLVSet(MemWRT);

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t packet_length = MemWRT.Length();
      result = WriteKLToBuffer(Buffer, packet_length);

      if ( ASDCP_SUCCESS(result) )
	Buffer.Size(Buffer.Size() + packet_length);
    }

  return result;
}

//
void
ASDCP::MXF::InterchangeObject::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];

  fputc('\n', stream);
  KLVPacket::Dump(stream, *m_Dict, false);
  fprintf(stream, "             InstanceUID = %s\n",  InstanceUID.EncodeHex(identbuf, IdentBufferLen));
  fprintf(stream, "           GenerationUID = %s\n",  GenerationUID.EncodeHex(identbuf, IdentBufferLen));
}

//
bool
ASDCP::MXF::InterchangeObject::IsA(const byte_t* label)
{
  if ( m_KLLength == 0 )
    return false;

  return ( memcmp(label, m_KeyStart, SMPTE_UL_LENGTH) == 0 );
}


//------------------------------------------------------------------------------------------


typedef std::map<ASDCP::UL, ASDCP::MXF::MXFObjectFactory_t>FactoryMap_t;
typedef FactoryMap_t::iterator FLi_t;

//
class FactoryList : public FactoryMap_t
{
  Kumu::Mutex m_Lock;

public:
  FactoryList() {}
  ~FactoryList() {}

  bool Empty() {
    Kumu::AutoMutex BlockLock(m_Lock);
    return empty();
  }

  FLi_t Find(const byte_t* label) {
    Kumu::AutoMutex BlockLock(m_Lock);
    return find(label);
  }

  FLi_t End() {
    Kumu::AutoMutex BlockLock(m_Lock);
    return end();
  }

  void Insert(ASDCP::UL label, ASDCP::MXF::MXFObjectFactory_t factory) {
    Kumu::AutoMutex BlockLock(m_Lock);
    insert(FactoryList::value_type(label, factory));
  }
};

//
static FactoryList s_FactoryList;
static Kumu::Mutex s_InitLock;
static bool        s_TypesInit = false;


//
void
ASDCP::MXF::SetObjectFactory(ASDCP::UL label, ASDCP::MXF::MXFObjectFactory_t factory)
{
  s_FactoryList.Insert(label, factory);
}

//
ASDCP::MXF::InterchangeObject*
ASDCP::MXF::CreateObject(const Dictionary*& Dict, const UL& label)
{
  if ( ! s_TypesInit )
    {
      Kumu::AutoMutex BlockLock(s_InitLock);

      if ( ! s_TypesInit )
	{
	  MXF::Metadata_InitTypes(Dict);
	  s_TypesInit = true;
	}
    }

  FLi_t i = s_FactoryList.find(label.Value());

  if ( i == s_FactoryList.end() )
    return new InterchangeObject(Dict);

  return i->second(Dict);
}



//------------------------------------------------------------------------------------------

//
bool
ASDCP::MXF::decode_mca_string(const std::string& s, const mca_label_map_t& labels, const Dictionary*& dict, const std::string& language,
			      InterchangeObject_list_t& descriptor_list, ui32_t& channel_count)
{
  std::string symbol_buf;
  channel_count = 0;
  ASDCP::MXF::SoundfieldGroupLabelSubDescriptor *current_soundfield = 0;
  std::string::const_iterator i;

  for ( i = s.begin(); i != s.end(); ++i )
    {
      if ( *i == '(' )
	{
	  if ( current_soundfield != 0 )
	    {
	      DefaultLogSink().Error("Encountered '(', already processing a soundfield group.\n");
	      return false;
	    }

	  if ( symbol_buf.empty() )
	    {
	      DefaultLogSink().Error("Encountered '(', without leading soundfield group symbol.\n");
	      return false;
	    }

	  mca_label_map_t::const_iterator i = labels.find(symbol_buf);
      
	  if ( i == labels.end() )
	    {
	      DefaultLogSink().Error("Unknown symbol: '%s'\n", symbol_buf.c_str());
	      return false;
	    }
      
	  if ( i->second.Value()[10] != 2 ) // magic depends on UL "Essence Facet" byte (see ST 428-12)
	    {
	      DefaultLogSink().Error("Not a soundfield group symbol: '%s'\n", symbol_buf.c_str());
	      return false;
	    }

	  current_soundfield = new ASDCP::MXF::SoundfieldGroupLabelSubDescriptor(dict);

	  GenRandomValue(current_soundfield->InstanceUID);
	  GenRandomValue(current_soundfield->MCALinkID);
	  current_soundfield->MCATagSymbol = "sg" + i->first;
	  current_soundfield->MCATagName = i->first;
	  current_soundfield->RFC5646SpokenLanguage = language;
	  current_soundfield->MCALabelDictionaryID = i->second;
	  descriptor_list.push_back(reinterpret_cast<ASDCP::MXF::InterchangeObject*>(current_soundfield));
	  symbol_buf.clear();
	}
      else if ( *i == ')' )
	{
	  if ( current_soundfield == 0 )
	    {
	      DefaultLogSink().Error("Encountered ')', not currently processing a soundfield group.\n");
	      return false;
	    }

	  if ( symbol_buf.empty() )
	    {
	      DefaultLogSink().Error("Soundfield group description contains an empty channel.\n");
	      return false;
	    }

	  mca_label_map_t::const_iterator i = labels.find(symbol_buf);
      
	  if ( i == labels.end() )
	    {
	      DefaultLogSink().Error("Unknown symbol: '%s'\n", symbol_buf.c_str());
	      return false;
	    }

	  ASDCP::MXF::AudioChannelLabelSubDescriptor *channel_descr =
	    new ASDCP::MXF::AudioChannelLabelSubDescriptor(dict);

	  GenRandomValue(channel_descr->InstanceUID);
	  assert(current_soundfield);
	  channel_descr->MCALinkID = current_soundfield->MCALinkID;
	  channel_descr->MCAChannelID = channel_count++;
	  channel_descr->MCATagSymbol = "ch" + i->first;
	  channel_descr->MCATagName = i->first;
	  channel_descr->RFC5646SpokenLanguage = language;
	  channel_descr->MCALabelDictionaryID = i->second;
	  descriptor_list.push_back(reinterpret_cast<ASDCP::MXF::InterchangeObject*>(channel_descr));
	  symbol_buf.clear();
	  current_soundfield = 0;
	}
      else if ( *i == ',' )
	{
	  if ( ! symbol_buf.empty() )
	    {
	      mca_label_map_t::const_iterator i = labels.find(symbol_buf);

	      if ( i == labels.end() )
		{
		  DefaultLogSink().Error("Unknown symbol: '%s'\n", symbol_buf.c_str());
		  return false;
		}

	      if ( i->second.Value()[10] != 1 ) // magic depends on UL "Essence Facet" byte (see ST 428-12)
		{
		  DefaultLogSink().Error("Not a channel symbol: '%s'\n", symbol_buf.c_str());
		  return false;
		}

	      ASDCP::MXF::AudioChannelLabelSubDescriptor *channel_descr =
		new ASDCP::MXF::AudioChannelLabelSubDescriptor(dict);

	      GenRandomValue(channel_descr->InstanceUID);

	      if ( current_soundfield != 0 )
		{
		  channel_descr->MCALinkID = current_soundfield->MCALinkID;
		}

	      channel_descr->MCAChannelID = channel_count++;
	      channel_descr->MCATagSymbol = "ch" + i->first;
	      channel_descr->MCATagName = i->first;
	      channel_descr->RFC5646SpokenLanguage = language;
	      channel_descr->MCALabelDictionaryID = i->second;
	      descriptor_list.push_back(reinterpret_cast<ASDCP::MXF::InterchangeObject*>(channel_descr));
	      symbol_buf.clear();
	    }
	}
      else if ( isalnum(*i) )
	{
	  symbol_buf += *i;
	}
      else if ( ! isspace(*i) )
	{
	  DefaultLogSink().Error("Unexpected character '%c'.\n", *i);
	  return false;
	}
    }

  if ( ! symbol_buf.empty() )
    {
      mca_label_map_t::const_iterator i = labels.find(symbol_buf);
      
      if ( i == labels.end() )
	{
	  DefaultLogSink().Error("Unknown symbol: '%s'\n", symbol_buf.c_str());
	  return false;
	}

      ASDCP::MXF::AudioChannelLabelSubDescriptor *channel_descr =
	new ASDCP::MXF::AudioChannelLabelSubDescriptor(dict);

      GenRandomValue(channel_descr->InstanceUID);

      if ( current_soundfield != 0 )
	{
	  channel_descr->MCALinkID = current_soundfield->MCALinkID;
	}

      channel_descr->MCAChannelID = channel_count++;
      channel_descr->MCATagSymbol = "ch" + i->first;
      channel_descr->MCATagName = i->first;
      channel_descr->RFC5646SpokenLanguage = language;
      channel_descr->MCALabelDictionaryID = i->second;
      descriptor_list.push_back(reinterpret_cast<ASDCP::MXF::InterchangeObject*>(channel_descr));
    }

  return true;
}

//
ASDCP::MXF::ASDCP_MCAConfigParser::ASDCP_MCAConfigParser(const Dictionary*& d) : m_Dict(d), m_ChannelCount(0)
{
  m_LabelMap.insert(mca_label_map_t::value_type("L", m_Dict->ul(MDD_DCAudioChannel_L)));
  m_LabelMap.insert(mca_label_map_t::value_type("R", m_Dict->ul(MDD_DCAudioChannel_R)));
  m_LabelMap.insert(mca_label_map_t::value_type("C", m_Dict->ul(MDD_DCAudioChannel_C)));
  m_LabelMap.insert(mca_label_map_t::value_type("LFE", m_Dict->ul(MDD_DCAudioChannel_LFE)));
  m_LabelMap.insert(mca_label_map_t::value_type("Ls", m_Dict->ul(MDD_DCAudioChannel_Ls)));
  m_LabelMap.insert(mca_label_map_t::value_type("Rs", m_Dict->ul(MDD_DCAudioChannel_Rs)));
  m_LabelMap.insert(mca_label_map_t::value_type("Lss", m_Dict->ul(MDD_DCAudioChannel_Lss)));
  m_LabelMap.insert(mca_label_map_t::value_type("Rss", m_Dict->ul(MDD_DCAudioChannel_Rss)));
  m_LabelMap.insert(mca_label_map_t::value_type("Lrs", m_Dict->ul(MDD_DCAudioChannel_Lrs)));
  m_LabelMap.insert(mca_label_map_t::value_type("Rrs", m_Dict->ul(MDD_DCAudioChannel_Rrs)));
  m_LabelMap.insert(mca_label_map_t::value_type("Lc", m_Dict->ul(MDD_DCAudioChannel_Lc)));
  m_LabelMap.insert(mca_label_map_t::value_type("Rc", m_Dict->ul(MDD_DCAudioChannel_Rc)));
  m_LabelMap.insert(mca_label_map_t::value_type("Cs", m_Dict->ul(MDD_DCAudioChannel_Cs)));
  m_LabelMap.insert(mca_label_map_t::value_type("HI", m_Dict->ul(MDD_DCAudioChannel_HI)));
  m_LabelMap.insert(mca_label_map_t::value_type("VIN", m_Dict->ul(MDD_DCAudioChannel_VIN)));
  m_LabelMap.insert(mca_label_map_t::value_type("51", m_Dict->ul(MDD_DCAudioSoundfield_51)));
  m_LabelMap.insert(mca_label_map_t::value_type("71", m_Dict->ul(MDD_DCAudioSoundfield_71)));
  m_LabelMap.insert(mca_label_map_t::value_type("SDS", m_Dict->ul(MDD_DCAudioSoundfield_SDS)));
  m_LabelMap.insert(mca_label_map_t::value_type("61", m_Dict->ul(MDD_DCAudioSoundfield_61)));
  m_LabelMap.insert(mca_label_map_t::value_type("M", m_Dict->ul(MDD_DCAudioSoundfield_M)));
}

//
ui32_t
ASDCP::MXF::ASDCP_MCAConfigParser::ChannelCount() const
{
  return m_ChannelCount;
}

// 51(L,R,C,LFE,Ls,Rs),HI,VIN
bool
ASDCP::MXF::ASDCP_MCAConfigParser::DecodeString(const std::string& s, const std::string& language)
{
  return decode_mca_string(s, m_LabelMap, m_Dict, language, *this, m_ChannelCount);
}

//
// end MXF.cpp
//
