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
/*! \file    MXF.cpp
    \version $Id: MXF.cpp,v 1.79 2015/10/12 15:30:46 jhurst Exp $
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
      DefaultLogSink().Error("File is smaller than an empty KLV packet.\n");
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
bool
ASDCP::MXF::RIP::GetPairBySID(ui32_t SID, PartitionPair& outPair) const
{
  RIP::const_pair_iterator i;
  for ( i = PairArray.begin(); i != PairArray.end(); ++i )
    {
      if ( i->BodySID == SID )
	{
	  outPair = *i;
	  return true;
	}
    }

  return false;
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
      result = PairArray.Unarchive(&MemRDR) ? RESULT_OK : RESULT_KLV_CODING(__LINE__, __FILE__);
    }

  if ( ASDCP_FAILURE(result) )
    DefaultLogSink().Error("Failed to initialize RIP.\n");

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
      result = RESULT_KLV_CODING(__LINE__, __FILE__);

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
  Result_t result = RESULT_KLV_CODING(__LINE__, __FILE__);

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
    DefaultLogSink().Error("Failed to initialize Partition.\n");

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
      result = RESULT_KLV_CODING(__LINE__, __FILE__);
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
      result = LocalTagEntryBatch.Unarchive(&MemRDR) ? RESULT_OK : RESULT_KLV_CODING(__LINE__, __FILE__);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      m_Lookup = new h__PrimerLookup;
      m_Lookup->InitWithBatch(LocalTagEntryBatch);
    }

  if ( ASDCP_FAILURE(result) )
    DefaultLogSink().Error("Failed to initialize Primer.\n");

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
  Result_t result = LocalTagEntryBatch.Archive(&MemWRT) ? RESULT_OK : RESULT_KLV_CODING(__LINE__, __FILE__);

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

      LocalTagEntryBatch.insert(TmpEntry);
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
  InterchangeObject(d), m_Dict(d), Version(258)
{
  assert(m_Dict);
  m_UL = m_Dict->Type(MDD_Preface).ul;
  ObjectModelVersion = 0;
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
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS_OPT(Preface, ObjectModelVersion));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(Preface, PrimaryPackage));
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
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteUi32(OBJ_WRITE_ARGS_OPT(Preface, ObjectModelVersion));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(Preface, PrimaryPackage));
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

  if ( ! ObjectModelVersion.empty() )
    fprintf(stream, "  %22s = %u\n",  "ObjectModelVersion", ObjectModelVersion.get());

  if ( ! PrimaryPackage.empty() )
    fprintf(stream, "  %22s = %s\n",  "PrimaryPackage", PrimaryPackage.get().EncodeHex(identbuf, IdentBufferLen));

  fprintf(stream, "  %22s:\n", "Identifications");  Identifications.Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "ContentStorage", ContentStorage.EncodeHex(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s = %s\n",  "OperationalPattern", OperationalPattern.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  %22s:\n", "EssenceContainers");  EssenceContainers.Dump(stream);
  fprintf(stream, "  %22s:\n", "DMSchemes");  DMSchemes.Dump(stream);
}

//------------------------------------------------------------------------------------------
//

ASDCP::MXF::OP1aHeader::OP1aHeader(const Dictionary*& d) : Partition(d), m_Dict(d), m_Primer(d), m_Preface(0) {}
ASDCP::MXF::OP1aHeader::~OP1aHeader() {}

//
ASDCP::Result_t
ASDCP::MXF::OP1aHeader::InitFromFile(const Kumu::FileReader& Reader)
{
  Result_t result = Partition::InitFromFile(Reader);

  if ( ASDCP_FAILURE(result) )
    return result;

  if ( m_Dict == &DefaultCompositeDict() )
    {
      // select more explicit dictionary if one is available
      if ( OperationalPattern.ExactMatch(MXFInterop_OPAtom_Entry().ul) )
	{
	  m_Dict = &DefaultInteropDict();
	}
      else if ( OperationalPattern.ExactMatch(SMPTE_390_OPAtom_Entry().ul) )
	{
	  m_Dict = &DefaultSMPTEDict();
	}
    }

  // slurp up the remainder of the header
  if ( HeaderByteCount < 1024 )
    {
      DefaultLogSink().Warn("Improbably small HeaderByteCount value: %qu\n", HeaderByteCount);
    }
  else if (HeaderByteCount > ( 4 * Kumu::Megabyte ) )
    {
      DefaultLogSink().Warn("Improbably huge HeaderByteCount value: %qu\n", HeaderByteCount);
    }
  
  result = m_HeaderData.Capacity(Kumu::xmin(4*Kumu::Megabyte, static_cast<ui32_t>(HeaderByteCount)));

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t read_count;
      result = Reader.Read(m_HeaderData.Data(), m_HeaderData.Capacity(), &read_count);

      if ( ASDCP_FAILURE(result) )
        {
	  DefaultLogSink().Error("OP1aHeader::InitFromFile, Read failed\n");
	  return result;
        }

      if ( read_count != m_HeaderData.Capacity() )
	{
	  DefaultLogSink().Error("Short read of OP-Atom header metadata; wanted %u, got %u.\n",
				 m_HeaderData.Capacity(), read_count);
	  return RESULT_KLV_CODING(__LINE__, __FILE__);
	}
    }

  if ( ASDCP_SUCCESS(result) )
    result = InitFromBuffer(m_HeaderData.RoData(), m_HeaderData.Capacity());

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::OP1aHeader::InitFromPartitionBuffer(const byte_t* p, ui32_t l)
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
ASDCP::MXF::OP1aHeader::InitFromBuffer(const byte_t* p, ui32_t l)
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

      if ( ASDCP_SUCCESS(result) )
	{
	  if ( object->IsA(m_Dict->ul(MDD_KLVFill)) )
	    {
	      delete object;

	      if ( p > end_p )
		{
		  DefaultLogSink().Error("Fill item short read: %d.\n", p - end_p);
		}
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
	  DefaultLogSink().Error("Error initializing OP1a header packet.\n");
	  //	  Kumu::hexdump(p-object->PacketLength(), object->PacketLength());
	  delete object;
	}
    }

  return result;
}

ASDCP::Result_t
ASDCP::MXF::OP1aHeader::GetMDObjectByID(const UUID& ObjectID, InterchangeObject** Object)
{
  return m_PacketList->GetMDObjectByID(ObjectID, Object);
}

//
ASDCP::Result_t
ASDCP::MXF::OP1aHeader::GetMDObjectByType(const byte_t* ObjectID, InterchangeObject** Object)
{
  InterchangeObject* TmpObject;

  if ( Object == 0 )
    Object = &TmpObject;

  return m_PacketList->GetMDObjectByType(ObjectID, Object);
}

//
ASDCP::Result_t
ASDCP::MXF::OP1aHeader::GetMDObjectsByType(const byte_t* ObjectID, std::list<InterchangeObject*>& ObjectList)
{
  return m_PacketList->GetMDObjectsByType(ObjectID, ObjectList);
}

//
ASDCP::MXF::Identification*
ASDCP::MXF::OP1aHeader::GetIdentification()
{
  InterchangeObject* Object;

  if ( ASDCP_SUCCESS(GetMDObjectByType(OBJ_TYPE_ARGS(Identification), &Object)) )
    return (Identification*)Object;

  return 0;
}

//
ASDCP::MXF::SourcePackage*
ASDCP::MXF::OP1aHeader::GetSourcePackage()
{
  InterchangeObject* Object;

  if ( ASDCP_SUCCESS(GetMDObjectByType(OBJ_TYPE_ARGS(SourcePackage), &Object)) )
    return (SourcePackage*)Object;

  return 0;
}

//
ASDCP::Result_t
ASDCP::MXF::OP1aHeader::WriteToFile(Kumu::FileWriter& Writer, ui32_t HeaderSize)
{
  assert(m_Dict);
  if ( m_Preface == 0 )
    return RESULT_STATE;

  if ( HeaderSize < 4096 ) 
    {
      DefaultLogSink().Error("HeaderSize %u is too small. Must be >= 4096\n", HeaderSize);
      return RESULT_PARAM;
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
ASDCP::MXF::OP1aHeader::Dump(FILE* stream)
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

  if ( ASDCP_SUCCESS(result) && IndexByteCount > 0 )
    {
      assert (IndexByteCount <= 0xFFFFFFFFL);
      // At this point, m_FooterData may not have been initialized
      // so it's capacity is zero and data pointer is NULL
      // However, if IndexByteCount is zero then the capacity
      // doesn't change and the data pointer is not set.
      result = m_FooterData.Capacity((ui32_t) IndexByteCount);

      if ( ASDCP_SUCCESS(result) )
	result = Reader.Read(m_FooterData.Data(), m_FooterData.Capacity(), &read_count);

      if ( ASDCP_SUCCESS(result) && read_count != m_FooterData.Capacity() )
	{
	  DefaultLogSink().Error("Short read of footer partition: got %u, expecting %u\n",
				 read_count, m_FooterData.Capacity());
	  return RESULT_FAIL;
	}
      else if( ASDCP_SUCCESS(result) && !m_FooterData.Data() )
	{
	  DefaultLogSink().Error( "Buffer for footer partition not created: IndexByteCount = %u\n",
				  IndexByteCount );
	  return RESULT_FAIL;
	}

      if ( ASDCP_SUCCESS(result) )
	result = InitFromBuffer(m_FooterData.RoData(), m_FooterData.Capacity());
    }

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
	  DefaultLogSink().Error("Error initializing OPAtom footer packet.\n");
	  delete object;
	}
    }

  if ( ASDCP_FAILURE(result) )
    {
      DefaultLogSink().Error("Failed to initialize OPAtomIndexFooter.\n");
    }

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
      IndexTableSegment *segment = dynamic_cast<IndexTableSegment*>(*pl_i);

      if ( segment != 0 )
	{
	  iseg_count++;
	  if ( m_BytesPerEditUnit != 0 )
	    {
	      if ( iseg_count != 1 )
		return RESULT_STATE;

	      segment->IndexDuration = duration;
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
      IndexTableSegment *segment = dynamic_cast<IndexTableSegment*>(*li);

      if ( segment != 0 )
	{
	  ui64_t start_pos = segment->IndexStartPosition;

	  if ( segment->EditUnitByteCount > 0 )
	    {
	      if ( m_PacketList->m_List.size() > 1 )
		DefaultLogSink().Error("Unexpected multiple IndexTableSegment in CBR file\n");

	      if ( ! segment->IndexEntryArray.empty() )
		DefaultLogSink().Error("Unexpected IndexEntryArray contents in CBR file\n");

	      Entry.StreamOffset = (ui64_t)frame_num * segment->EditUnitByteCount;
	      return RESULT_OK;
	    }
	  else if ( (ui64_t)frame_num >= start_pos
		    && (ui64_t)frame_num < (start_pos + segment->IndexDuration) )
	    {
	      ui64_t tmp = frame_num - start_pos;
	      assert(tmp <= 0xFFFFFFFFL);
	      Entry = segment->IndexEntryArray[(ui32_t) tmp];
	      return RESULT_OK;
	    }
	}
    }

  return RESULT_FAIL;
}

//
void
ASDCP::MXF::OPAtomIndexFooter::SetDeltaParams(const IndexTableSegment::DeltaEntry& delta)
{
  m_DefaultDeltaEntry = delta;
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
      m_CurrentSegment->DeltaEntryArray.push_back(m_DefaultDeltaEntry);
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
      m_CurrentSegment->DeltaEntryArray.push_back(m_DefaultDeltaEntry);
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
    result = TLVSet.ReadObject(OBJ_READ_ARGS_OPT(GenerationInterchangeObject, GenerationUID));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::InterchangeObject::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = TLVSet.WriteObject(OBJ_WRITE_ARGS(InterchangeObject, InstanceUID));
  if ( ASDCP_SUCCESS(result) )
    result = TLVSet.WriteObject(OBJ_WRITE_ARGS_OPT(GenerationInterchangeObject, GenerationUID));
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

  if ( ! GenerationUID.empty() )
    fprintf(stream, "           GenerationUID = %s\n",  GenerationUID.get().EncodeHex(identbuf, IdentBufferLen));
}

//
bool
ASDCP::MXF::InterchangeObject::IsA(const byte_t* label)
{
  if ( m_KLLength == 0 || m_KeyStart == 0 )
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
ASDCP::MXF::SetObjectFactory(const ASDCP::UL& label, ASDCP::MXF::MXFObjectFactory_t factory)
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
      
	  if ( i->second.ul.Value()[10] != 2 ) // magic depends on UL "Essence Facet" byte (see ST 428-12)
	    {
	      DefaultLogSink().Error("Not a soundfield group symbol: '%s'\n", symbol_buf.c_str());
	      return false;
	    }

	  current_soundfield = new ASDCP::MXF::SoundfieldGroupLabelSubDescriptor(dict);
	  GenRandomValue(current_soundfield->MCALinkID);

	  current_soundfield->MCATagSymbol = (i->second.requires_prefix ? "sg" : "") + i->first;
	  current_soundfield->MCATagName = i->second.tag_name;
	  current_soundfield->RFC5646SpokenLanguage = language;
	  current_soundfield->MCALabelDictionaryID = i->second.ul;
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
	      DefaultLogSink().Error("Soundfield group description contains no channels.\n");
	      return false;
	    }

	  mca_label_map_t::const_iterator i = labels.find(symbol_buf);
      
	  if ( i == labels.end() )
	    {
	      DefaultLogSink().Error("Unknown symbol: '%s'\n", symbol_buf.c_str());
	      return false;
	    }

	  assert(current_soundfield);

	  ASDCP::MXF::AudioChannelLabelSubDescriptor *channel_descr =
	    new ASDCP::MXF::AudioChannelLabelSubDescriptor(dict);
	  GenRandomValue(channel_descr->MCALinkID);

	  channel_descr->SoundfieldGroupLinkID = current_soundfield->MCALinkID;
	  channel_descr->MCAChannelID = channel_count++ + 1;
	  channel_descr->MCATagSymbol = (i->second.requires_prefix ? "ch" : "") + i->first;
	  channel_descr->MCATagName = i->second.tag_name;
	  channel_descr->RFC5646SpokenLanguage = language;
	  channel_descr->MCALabelDictionaryID = i->second.ul;
	  descriptor_list.push_back(reinterpret_cast<ASDCP::MXF::InterchangeObject*>(channel_descr));
	  symbol_buf.clear();
	  current_soundfield = 0;
	}
      else if ( *i == ',' )
	{
	  if ( ! symbol_buf.empty() && ! symbol_buf.compare("-") )
	    {
	      channel_count++;
	      symbol_buf.clear();
	    }
	  else if ( ! symbol_buf.empty() )
	    {
	      mca_label_map_t::const_iterator i = labels.find(symbol_buf);

	      if ( i == labels.end() )
		{
		  DefaultLogSink().Error("Unknown symbol: '%s'\n", symbol_buf.c_str());
		  return false;
		}

	      if ( i->second.ul.Value()[10] != 1 ) // magic depends on UL "Essence Facet" byte (see ST 428-12)
		{
		  DefaultLogSink().Error("Not a channel symbol: '%s'\n", symbol_buf.c_str());
		  return false;
		}

	      ASDCP::MXF::AudioChannelLabelSubDescriptor *channel_descr =
		new ASDCP::MXF::AudioChannelLabelSubDescriptor(dict);
	      GenRandomValue(channel_descr->MCALinkID);

	      if ( current_soundfield != 0 )
		{
		  channel_descr->SoundfieldGroupLinkID = current_soundfield->MCALinkID;
		}

	      channel_descr->MCAChannelID = channel_count++ + 1;
	      channel_descr->MCATagSymbol = (i->second.requires_prefix ? "ch" : "") + i->first;
	      channel_descr->MCATagName = i->second.tag_name;
	      channel_descr->RFC5646SpokenLanguage = language;
	      channel_descr->MCALabelDictionaryID = i->second.ul;
	      descriptor_list.push_back(reinterpret_cast<ASDCP::MXF::InterchangeObject*>(channel_descr));
	      symbol_buf.clear();
	    }
	}
      else if ( *i == '-' || isalnum(*i) )
	{
	  symbol_buf += *i;
	}
      else if ( ! isspace(*i) )
	{
	  DefaultLogSink().Error("Unexpected character '%c'.\n", *i);
	  return false;
	}
    }

  if ( ! symbol_buf.empty() && ! symbol_buf.compare("-")  )
    {
      channel_count++;
    }
  else if ( ! symbol_buf.empty() )
    {
      mca_label_map_t::const_iterator i = labels.find(symbol_buf);
      
      if ( i == labels.end() )
	{
	  DefaultLogSink().Error("Unknown symbol: '%s'\n", symbol_buf.c_str());
	  return false;
	}

      ASDCP::MXF::AudioChannelLabelSubDescriptor *channel_descr =
	new ASDCP::MXF::AudioChannelLabelSubDescriptor(dict);
      GenRandomValue(channel_descr->MCALinkID);

      if ( current_soundfield != 0 )
	{
	  channel_descr->SoundfieldGroupLinkID = current_soundfield->MCALinkID;
	}

      channel_descr->MCAChannelID = channel_count++ + 1;
      channel_descr->MCATagSymbol = (i->second.requires_prefix ? "ch" : "") + i->first;
      channel_descr->MCATagName = i->second.tag_name;
      channel_descr->RFC5646SpokenLanguage = language;
      channel_descr->MCALabelDictionaryID = i->second.ul;
      descriptor_list.push_back(reinterpret_cast<ASDCP::MXF::InterchangeObject*>(channel_descr));
    }

  return true;
}

//
ASDCP::MXF::ASDCP_MCAConfigParser::ASDCP_MCAConfigParser(const Dictionary*& d) : m_Dict(d), m_ChannelCount(0)
{
  typedef mca_label_map_t::value_type pair;
  m_LabelMap.insert(pair("L",     label_traits("Left"                              , true,  m_Dict->ul(MDD_DCAudioChannel_L))));
  m_LabelMap.insert(pair("R",     label_traits("Right"                             , true,  m_Dict->ul(MDD_DCAudioChannel_R))));
  m_LabelMap.insert(pair("C",     label_traits("Center"                            , true,  m_Dict->ul(MDD_DCAudioChannel_C))));
  m_LabelMap.insert(pair("LFE",   label_traits("LFE"                               , true,  m_Dict->ul(MDD_DCAudioChannel_LFE))));
  m_LabelMap.insert(pair("Ls",    label_traits("Left Surround"                     , true,  m_Dict->ul(MDD_DCAudioChannel_Ls))));
  m_LabelMap.insert(pair("Rs",    label_traits("Right Surround"                    , true,  m_Dict->ul(MDD_DCAudioChannel_Rs))));
  m_LabelMap.insert(pair("Lss",   label_traits("Left Side Surround"                , true,  m_Dict->ul(MDD_DCAudioChannel_Lss))));
  m_LabelMap.insert(pair("Rss",   label_traits("Right Side Surround"               , true,  m_Dict->ul(MDD_DCAudioChannel_Rss))));
  m_LabelMap.insert(pair("Lrs",   label_traits("Left Rear Surround"                , true,  m_Dict->ul(MDD_DCAudioChannel_Lrs))));
  m_LabelMap.insert(pair("Rrs",   label_traits("Right Rear Surround"               , true,  m_Dict->ul(MDD_DCAudioChannel_Rrs))));
  m_LabelMap.insert(pair("Lc",    label_traits("Left Center"                       , true,  m_Dict->ul(MDD_DCAudioChannel_Lc))));
  m_LabelMap.insert(pair("Rc",    label_traits("Right Center"                      , true,  m_Dict->ul(MDD_DCAudioChannel_Rc))));
  m_LabelMap.insert(pair("Cs",    label_traits("Center Surround"                   , true,  m_Dict->ul(MDD_DCAudioChannel_Cs))));
  m_LabelMap.insert(pair("HI",    label_traits("Hearing Impaired"                  , true,  m_Dict->ul(MDD_DCAudioChannel_HI))));
  m_LabelMap.insert(pair("VIN",   label_traits("Visually Impaired-Narrative"       , true,  m_Dict->ul(MDD_DCAudioChannel_VIN))));
  m_LabelMap.insert(pair("51",    label_traits("5.1"                               , true,  m_Dict->ul(MDD_DCAudioSoundfield_51))));
  m_LabelMap.insert(pair("71",    label_traits("7.1DS"                             , true,  m_Dict->ul(MDD_DCAudioSoundfield_71))));
  m_LabelMap.insert(pair("SDS",   label_traits("7.1SDS"                            , true,  m_Dict->ul(MDD_DCAudioSoundfield_SDS))));
  m_LabelMap.insert(pair("61",    label_traits("6.1"                               , true,  m_Dict->ul(MDD_DCAudioSoundfield_61))));
  m_LabelMap.insert(pair("M",     label_traits("1.0 Monaural"                      , true,  m_Dict->ul(MDD_DCAudioSoundfield_M))));
  m_LabelMap.insert(pair("DBOX",  label_traits("D-BOX Motion Code Primary Stream"  , false, m_Dict->ul(MDD_DBOXMotionCodePrimaryStream))));
  m_LabelMap.insert(pair("DBOX2", label_traits("D-BOX Motion Code Secondary Stream", false, m_Dict->ul(MDD_DBOXMotionCodeSecondaryStream))));
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



ASDCP::MXF::AS02_MCAConfigParser::AS02_MCAConfigParser(const Dictionary*& d) : ASDCP::MXF::ASDCP_MCAConfigParser(d)
{
  typedef mca_label_map_t::value_type pair;
  m_LabelMap.insert(pair("M1",    label_traits("M1",  true,  m_Dict->ul(MDD_IMFAudioChannel_M1))));
  m_LabelMap.insert(pair("M2",    label_traits("M2",  true,  m_Dict->ul(MDD_IMFAudioChannel_M2))));
  m_LabelMap.insert(pair("Lt",    label_traits("Lt",  true,  m_Dict->ul(MDD_IMFAudioChannel_Lt))));
  m_LabelMap.insert(pair("Rt",    label_traits("Rt",  true,  m_Dict->ul(MDD_IMFAudioChannel_Rt))));
  m_LabelMap.insert(pair("Lst",   label_traits("Lst", true,  m_Dict->ul(MDD_IMFAudioChannel_Lst))));
  m_LabelMap.insert(pair("Rst",   label_traits("Rst", true,  m_Dict->ul(MDD_IMFAudioChannel_Rst))));
  m_LabelMap.insert(pair("S",     label_traits("S",   true,  m_Dict->ul(MDD_IMFAudioChannel_S))));
  m_LabelMap.insert(pair("ST",    label_traits("ST",  true,  m_Dict->ul(MDD_IMFAudioSoundfield_ST))));
  m_LabelMap.insert(pair("DM",    label_traits("DM",  true,  m_Dict->ul(MDD_IMFAudioSoundfield_DM))));
  m_LabelMap.insert(pair("DNS",   label_traits("DNS", true,  m_Dict->ul(MDD_IMFAudioSoundfield_DNS))));
  m_LabelMap.insert(pair("30",    label_traits("30",  true,  m_Dict->ul(MDD_IMFAudioSoundfield_30))));
  m_LabelMap.insert(pair("40",    label_traits("40",  true,  m_Dict->ul(MDD_IMFAudioSoundfield_40))));
  m_LabelMap.insert(pair("50",    label_traits("50",  true,  m_Dict->ul(MDD_IMFAudioSoundfield_50))));
  m_LabelMap.insert(pair("60",    label_traits("60",  true,  m_Dict->ul(MDD_IMFAudioSoundfield_60))));
  m_LabelMap.insert(pair("70",    label_traits("70",  true,  m_Dict->ul(MDD_IMFAudioSoundfield_70))));
  m_LabelMap.insert(pair("LtRt",  label_traits("LtRt",true,  m_Dict->ul(MDD_IMFAudioSoundfield_LtRt))));
  m_LabelMap.insert(pair("51Ex",  label_traits("51Ex",true,  m_Dict->ul(MDD_IMFAudioSoundfield_51Ex))));
  m_LabelMap.insert(pair("HI",    label_traits("HI",  true,  m_Dict->ul(MDD_IMFAudioSoundfield_HI))));
  m_LabelMap.insert(pair("VIN",   label_traits("VIN", true,  m_Dict->ul(MDD_IMFAudioSoundfield_VIN))));
}



//
bool
ASDCP::MXF::GetEditRateFromFP(ASDCP::MXF::OP1aHeader& header, ASDCP::Rational& edit_rate)
{
  bool has_first_item = false;

  MXF::InterchangeObject* temp_item;
  std::list<MXF::InterchangeObject*> temp_items;

  Result_t result = header.GetMDObjectsByType(DefaultCompositeDict().ul(MDD_SourcePackage), temp_items);

  if ( KM_FAILURE(result) )
    {
      DefaultLogSink().Error("The MXF header does not contain a FilePackage item.\n");
      return false;
    }

  if ( temp_items.size() != 1 )
    {
      DefaultLogSink().Error("The MXF header must contain one FilePackage item, found %d.\n", temp_items.size());
      return false;
    }

  char buf[64];
  MXF::Array<UUID>::const_iterator i;
  MXF::SourcePackage *source_package = dynamic_cast<MXF::SourcePackage*>(temp_items.front());
  assert(source_package);

  for ( i = source_package->Tracks.begin(); i != source_package->Tracks.end(); ++i )
    {
      // Track
      result = header.GetMDObjectByID(*i, &temp_item);

      if ( KM_FAILURE(result) )
	{
	  DefaultLogSink().Error("The MXF header is incomplete: strong reference %s leads nowhere.\n",
				 i->EncodeHex(buf, 64));
	  return false;
	}

      MXF::Track *track = dynamic_cast<MXF::Track*>(temp_item);

      if ( track == 0 )
	{
	  DefaultLogSink().Error("The MXF header is incomplete: %s is not a Track item.\n",
				 i->EncodeHex(buf, 64));
	  return false;
	}

      // Sequence
      result = header.GetMDObjectByID(track->Sequence, &temp_item);

      if ( KM_FAILURE(result) )
	{
	  DefaultLogSink().Error("The MXF header is incomplete: strong reference %s leads nowhere.\n",
				 i->EncodeHex(buf, 64));
	  return false;
	}

      MXF::Sequence *sequence = dynamic_cast<MXF::Sequence*>(temp_item);

      if ( sequence == 0 )
	{
	  DefaultLogSink().Error("The MXF header is incomplete: %s is not a Sequence item.\n",
				 track->Sequence.get().EncodeHex(buf, 64));
	  return false;
	}

      if ( sequence->StructuralComponents.size() != 1 )
	{
	  DefaultLogSink().Error("The Sequence item must contain one reference to an esence item, found %d.\n",
				 sequence->StructuralComponents.size());
	  return false;
	}

      // SourceClip
      result = header.GetMDObjectByID(sequence->StructuralComponents.front(), &temp_item);

      if ( KM_FAILURE(result) )
	{
	  DefaultLogSink().Error("The MXF header is incomplete: strong reference %s leads nowhere.\n",
				 sequence->StructuralComponents.front().EncodeHex(buf, 64));
	  return false;
	}

      if ( temp_item->IsA(DefaultCompositeDict().ul(MDD_SourceClip)) )
	{
	  MXF::SourceClip *source_clip = dynamic_cast<MXF::SourceClip*>(temp_item);

	  if ( source_clip == 0 )
	    {
	      DefaultLogSink().Error("The MXF header is incomplete: %s is not a SourceClip item.\n",
				     sequence->StructuralComponents.front().EncodeHex(buf, 64));
	      return false;
	    }

	  if ( ! has_first_item )
	    {
	      edit_rate = track->EditRate;
	      has_first_item = true;
	    }
	  else if ( edit_rate != track->EditRate )
	    {
	      DefaultLogSink().Error("The MXF header is incomplete: %s EditRate value does not match others in the file.\n",
				     sequence->StructuralComponents.front().EncodeHex(buf, 64));
	      return false;
	    }
	}
      else if ( ! temp_item->IsA(DefaultCompositeDict().ul(MDD_TimecodeComponent)) )
	{
	  DefaultLogSink().Error("Reference from Sequence to an unexpected type: %s.\n", temp_item->ObjectName());
	  return false;
	}
    }

  return true;
}


//
// end MXF.cpp
//
