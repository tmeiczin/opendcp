/*
Copyright (c) 2004-2013, John Hurst
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
/*! \file    AS_DCP_internal.h
    \version $Id: AS_DCP_internal.h,v 1.45 2014/09/21 13:27:43 jhurst Exp $
    \brief   AS-DCP library, non-public common elements
*/

#ifndef _AS_DCP_INTERNAL_H_
#define _AS_DCP_INTERNAL_H_

#include <KM_platform.h>
#include <KM_util.h>
#include <KM_log.h>
#include "Metadata.h"

using Kumu::DefaultLogSink;
using namespace ASDCP;
using namespace ASDCP::MXF;

// a magic number identifying asdcplib
#ifndef ASDCP_BUILD_NUMBER
#define ASDCP_BUILD_NUMBER 0x6A68
#endif


#ifdef DEFAULT_MD_DECL
ASDCP::MXF::OP1aHeader *g_OP1aHeader;
ASDCP::MXF::OPAtomIndexFooter *g_OPAtomIndexFooter;
ASDCP::MXF::RIP *g_RIP;
#else
extern MXF::OP1aHeader *g_OP1aHeader;
extern MXF::OPAtomIndexFooter *g_OPAtomIndexFooter;
extern MXF::RIP *g_RIP;
#endif


namespace ASDCP
{

  //
  static std::vector<int>
    version_split(const char* str)
  {
    std::vector<int> result;
    const char* pstr = str;
    const char* r = strchr(pstr, '.');

    while ( r != 0 )
      {
	assert(r >= pstr);
	if ( r > pstr )
	  result.push_back(strtol(pstr, 0, 10));

	pstr = r + 1;
	r = strchr(pstr, '.');
      }

    if( strlen(pstr) > 0 )
      result.push_back(strtol(pstr, 0, 10));

    assert(result.size() == 3);
    return result;
  }

  // constant values used to calculate KLV and EKLV packet sizes
  static const ui32_t klv_cryptinfo_size =
    MXF_BER_LENGTH
    + UUIDlen /* ContextID */
    + MXF_BER_LENGTH
    + sizeof(ui64_t) /* PlaintextOffset */
    + MXF_BER_LENGTH
    + SMPTE_UL_LENGTH /* SourceKey */
    + MXF_BER_LENGTH
    + sizeof(ui64_t) /* SourceLength */
    + MXF_BER_LENGTH /* ESV length */ ;

  static const ui32_t klv_intpack_size =
    MXF_BER_LENGTH
    + UUIDlen /* TrackFileID */
    + MXF_BER_LENGTH
    + sizeof(ui64_t) /* SequenceNumber */
    + MXF_BER_LENGTH
    + 20; /* HMAC length*/

  // calculate size of encrypted essence with IV, CheckValue, and padding
  inline ui32_t
    calc_esv_length(ui32_t source_length, ui32_t plaintext_offset)
    {
      ui32_t ct_size = source_length - plaintext_offset;
      ui32_t diff = ct_size % CBC_BLOCK_SIZE;
      ui32_t block_size = ct_size - diff;
      return plaintext_offset + block_size + (CBC_BLOCK_SIZE * 3);
    }

  // the check value for EKLV packets
  // CHUKCHUKCHUKCHUK
  static const byte_t ESV_CheckValue[CBC_BLOCK_SIZE] =
  { 0x43, 0x48, 0x55, 0x4b, 0x43, 0x48, 0x55, 0x4b,
    0x43, 0x48, 0x55, 0x4b, 0x43, 0x48, 0x55, 0x4b };

  //------------------------------------------------------------------------------------------
  //

  ui32_t derive_timecode_rate_from_edit_rate(const ASDCP::Rational& edit_rate);

  Result_t MD_to_WriterInfo(MXF::Identification*, WriterInfo&);
  Result_t MD_to_CryptoInfo(MXF::CryptographicContext*, WriterInfo&, const Dictionary&);

  Result_t EncryptFrameBuffer(const ASDCP::FrameBuffer&, ASDCP::FrameBuffer&, AESEncContext*);
  Result_t DecryptFrameBuffer(const ASDCP::FrameBuffer&, ASDCP::FrameBuffer&, AESDecContext*);

  Result_t MD_to_JP2K_PDesc(const ASDCP::MXF::GenericPictureEssenceDescriptor&  EssenceDescriptor,
			    const ASDCP::MXF::JPEG2000PictureSubDescriptor& EssenceSubDescriptor,
			    const ASDCP::Rational& EditRate, const ASDCP::Rational& SampleRate,
			    ASDCP::JP2K::PictureDescriptor& PDesc);

  Result_t JP2K_PDesc_to_MD(const JP2K::PictureDescriptor& PDesc,
			    const ASDCP::Dictionary& dict,
			    ASDCP::MXF::GenericPictureEssenceDescriptor& EssenceDescriptor,
			    ASDCP::MXF::JPEG2000PictureSubDescriptor& EssenceSubDescriptor);

  Result_t PCM_ADesc_to_MD(PCM::AudioDescriptor& ADesc, ASDCP::MXF::WaveAudioDescriptor* ADescObj);
  Result_t MD_to_PCM_ADesc(ASDCP::MXF::WaveAudioDescriptor* ADescObj, PCM::AudioDescriptor& ADesc);

  void     AddDMScrypt(Partition& HeaderPart, SourcePackage& Package,
		       WriterInfo& Descr, const UL& WrappingUL, const Dictionary*& Dict);

  Result_t Read_EKLV_Packet(Kumu::FileReader& File, const ASDCP::Dictionary& Dict,
			    const ASDCP::WriterInfo& Info, Kumu::fpos_t& LastPosition, ASDCP::FrameBuffer& CtFrameBuf,
			    ui32_t FrameNum, ui32_t SequenceNum, ASDCP::FrameBuffer& FrameBuf,
			    const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC);

  Result_t Write_EKLV_Packet(Kumu::FileWriter& File, const ASDCP::Dictionary& Dict, const MXF::OP1aHeader& HeaderPart,
			     const ASDCP::WriterInfo& Info, ASDCP::FrameBuffer& CtFrameBuf, ui32_t& FramesWritten,
			     ui64_t & StreamOffset, const ASDCP::FrameBuffer& FrameBuf, const byte_t* EssenceUL,
			     AESEncContext* Ctx, HMACContext* HMAC);

  //
 class KLReader : public ASDCP::KLVPacket
    {
      ASDCP_NO_COPY_CONSTRUCT(KLReader);
      byte_t m_KeyBuf[SMPTE_UL_LENGTH*2];

    public:
      KLReader() {}
      ~KLReader() {}

      inline const byte_t* Key() { return m_KeyBuf; }
      inline const ui64_t  Length() { return m_ValueLength; }
      inline const ui64_t  KLLength() { return m_KLLength; }

      Result_t ReadKLFromFile(Kumu::FileReader& Reader);
    };

  namespace MXF
  {
      //---------------------------------------------------------------------------------
      //

    ///      void default_md_object_init();

      template <class HeaderType, class IndexAccessType>
      class TrackFileReader
      {
	KM_NO_COPY_CONSTRUCT(TrackFileReader);
	TrackFileReader();

      public:
	const Dictionary*  m_Dict;
	Kumu::FileReader   m_File;
	HeaderType         m_HeaderPart;
	IndexAccessType    m_IndexAccess;
	RIP                m_RIP;
	WriterInfo         m_Info;
	ASDCP::FrameBuffer m_CtFrameBuf;
	Kumu::fpos_t       m_LastPosition;

      TrackFileReader(const Dictionary& d) :
	m_HeaderPart(m_Dict), m_IndexAccess(m_Dict), m_RIP(m_Dict), m_Dict(&d)
	  {
	    default_md_object_init();
	  }

	virtual ~TrackFileReader() {
	  Close();
	}

	const MXF::RIP& GetRIP() const { return m_RIP; }

	//
	Result_t OpenMXFRead(const std::string& filename)
	{
	  m_LastPosition = 0;
	  Result_t result = m_File.OpenRead(filename);

	  if ( ASDCP_SUCCESS(result) )
	    result = SeekToRIP(m_File);

	  if ( ASDCP_SUCCESS(result) )
	    {
	      result = m_RIP.InitFromFile(m_File);
	      ui32_t test_s = m_RIP.PairArray.size();

	      if ( ASDCP_FAILURE(result) )
		{
		  DefaultLogSink().Error("File contains no RIP\n");
		}
	      else if ( m_RIP.PairArray.empty() )
		{
		  DefaultLogSink().Error("RIP contains no Pairs.\n");
		}
	    }
	  else
	    {
	      DefaultLogSink().Error("TrackFileReader::OpenMXFRead, SeekToRIP failed\n");
	    }

	  m_File.Seek(0);
	  result = m_HeaderPart.InitFromFile(m_File);

	  if ( KM_FAILURE(result) )
	    {
	      DefaultLogSink().Error("TrackFileReader::OpenMXFRead, header init failed\n");
	    }

	  return result;
	}

	//
	Result_t InitInfo()
	{
	  assert(m_Dict);
	  InterchangeObject* Object;

	  // Identification
	  Result_t result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(Identification), &Object);

	  // Writer Info and SourcePackage
	  if ( KM_SUCCESS(result) )
	    {
	      MD_to_WriterInfo((Identification*)Object, m_Info);
	      result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(SourcePackage), &Object);
	    }

	  if ( KM_SUCCESS(result) )
	    {
	      SourcePackage* SP = (SourcePackage*)Object;
	      memcpy(m_Info.AssetUUID, SP->PackageUID.Value() + 16, UUIDlen);
	    }

	  // optional CryptographicContext
	  if ( KM_SUCCESS(result) )
	    {
	      Result_t cr_result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(CryptographicContext), &Object);

	      if ( KM_SUCCESS(cr_result) )
		MD_to_CryptoInfo((CryptographicContext*)Object, m_Info, *m_Dict);
	    }

	  return result;
	}

	// positions file before reading
	// allows external control of index offset
	Result_t ReadEKLVFrame(const ui64_t& body_offset,
			       ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
			       const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC)
	{
	  // look up frame index node
	  IndexTableSegment::IndexEntry TmpEntry;

	  if ( KM_FAILURE(m_IndexAccess.Lookup(FrameNum, TmpEntry)) )
	    {
	      DefaultLogSink().Error("Frame value out of range: %u\n", FrameNum);
	      return RESULT_RANGE;
	    }

	  // get relative frame position, apply offset and go read the frame's key and length
	  Kumu::fpos_t FilePosition = body_offset + TmpEntry.StreamOffset;
	  Result_t result = RESULT_OK;

	  if ( FilePosition != m_LastPosition )
	    {
	      m_LastPosition = FilePosition;
	      result = m_File.Seek(FilePosition);
	    }

	  if ( KM_SUCCESS(result) )
	    result = ReadEKLVPacket(FrameNum, FrameNum + 1, FrameBuf, EssenceUL, Ctx, HMAC);

	  return result;
	}

	// positions file before reading
	// assumes "processed" index entries have absolute positions
	Result_t ReadEKLVFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
			       const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC)
	{
	  // look up frame index node
	  IndexTableSegment::IndexEntry TmpEntry;

	  if ( KM_FAILURE(m_IndexAccess.Lookup(FrameNum, TmpEntry)) )
	    {
	      DefaultLogSink().Error("Frame value out of range: %u\n", FrameNum);
	      return RESULT_RANGE;
	    }

	  // get absolute frame position and go read the frame's key and length
	  Result_t result = RESULT_OK;

	  if ( TmpEntry.StreamOffset != m_LastPosition )
	    {
	      m_LastPosition = TmpEntry.StreamOffset;
	      result = m_File.Seek(TmpEntry.StreamOffset);
	    }

	  if ( KM_SUCCESS(result) )
	    result = ReadEKLVPacket(FrameNum, FrameNum + 1, FrameBuf, EssenceUL, Ctx, HMAC);

	  return result;
	}

	// reads from current position
	Result_t ReadEKLVPacket(ui32_t FrameNum, ui32_t SequenceNum, ASDCP::FrameBuffer& FrameBuf,
				const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC)
	{
	  assert(m_Dict);
	  return Read_EKLV_Packet(m_File, *m_Dict, m_Info, m_LastPosition, m_CtFrameBuf,
				  FrameNum, SequenceNum, FrameBuf, EssenceUL, Ctx, HMAC);
	}

	// Get the position of a frame from a track file
	Result_t LocateFrame(const ui64_t& body_offset,
			     ui32_t FrameNum, Kumu::fpos_t& streamOffset,
			     i8_t& temporalOffset, i8_t& keyFrameOffset)
	{
	  // look up frame index node
	  IndexTableSegment::IndexEntry TmpEntry;

	  if ( KM_FAILURE(m_IndexAccess.Lookup(FrameNum, TmpEntry)) )
	    {
	      DefaultLogSink().Error("Frame value out of range: %u\n", FrameNum);
	      return RESULT_RANGE;
	    }

	  // get frame position, temporal offset, and key frame ofset
	  streamOffset = body_offset + TmpEntry.StreamOffset;
	  temporalOffset = TmpEntry.TemporalOffset;
	  keyFrameOffset = TmpEntry.KeyFrameOffset;
	  
	  return RESULT_OK;
	}

	//
	void Close()
	{
	  m_File.Close();
	}
      };
      
      //------------------------------------------------------------------------------------------
      //

      //
      template <class ClipT>
	struct TrackSet
	{
	  MXF::Track*    Track;
	  MXF::Sequence* Sequence;
	  ClipT*         Clip;

	TrackSet() : Track(0), Sequence(0), Clip(0) {}
	};

      //
      template <class PackageT, class ClipT>
	TrackSet<ClipT>
	CreateTrackAndSequence(OP1aHeader& Header, PackageT& Package, const std::string TrackName,
			       const MXF::Rational& clip_edit_rate, const UL& Definition, ui32_t TrackID, const Dictionary*& Dict)
	{
	  TrackSet<ClipT> NewTrack;

	  NewTrack.Track = new Track(Dict);
	  Header.AddChildObject(NewTrack.Track);
	  NewTrack.Track->EditRate = clip_edit_rate;
	  Package.Tracks.push_back(NewTrack.Track->InstanceUID);
	  NewTrack.Track->TrackID = TrackID;
	  NewTrack.Track->TrackName = TrackName.c_str();

	  NewTrack.Sequence = new Sequence(Dict);
	  Header.AddChildObject(NewTrack.Sequence);
	  NewTrack.Track->Sequence = NewTrack.Sequence->InstanceUID;
	  NewTrack.Sequence->DataDefinition = Definition;

	  return NewTrack;
	}

      //
      template <class PackageT>
	TrackSet<TimecodeComponent>
	CreateTimecodeTrack(OP1aHeader& Header, PackageT& Package,
			    const MXF::Rational& tc_edit_rate, ui32_t tc_frame_rate, ui64_t TCStart, const Dictionary*& Dict)
	{
	  assert(Dict);
	  UL TCUL(Dict->ul(MDD_TimecodeDataDef));

	  TrackSet<TimecodeComponent> NewTrack =
	    CreateTrackAndSequence<PackageT, TimecodeComponent>(Header, Package, "Timecode Track",
								tc_edit_rate, TCUL, 1, Dict);

	  NewTrack.Clip = new TimecodeComponent(Dict);
	  Header.AddChildObject(NewTrack.Clip);
	  NewTrack.Sequence->StructuralComponents.push_back(NewTrack.Clip->InstanceUID);
	  NewTrack.Clip->RoundedTimecodeBase = tc_frame_rate;
	  NewTrack.Clip->StartTimecode = TCStart;
	  NewTrack.Clip->DataDefinition = TCUL;

	  return NewTrack;
	}


      // state machine for mxf writer
      enum WriterState_t {
	ST_BEGIN,   // waiting for Open()
	ST_INIT,    // waiting for SetSourceStream()
	ST_READY,   // ready to write frames
	ST_RUNNING, // one or more frames written
	ST_FINAL,   // index written, file closed
	ST_MAX
      };

      // implementation of h__WriterState class Goto_* methods
#define Goto_body(s1,s2)			\
      if ( m_State != (s1) ) {			\
	return RESULT_STATE;			\
      }						\
      m_State = (s2);				\
      return RESULT_OK
      //
      class h__WriterState
      {
	ASDCP_NO_COPY_CONSTRUCT(h__WriterState);

      public:
	WriterState_t m_State;
      h__WriterState() : m_State(ST_BEGIN) {}
	~h__WriterState() {}

	inline bool     Test_BEGIN()   { return m_State == ST_BEGIN; }
	inline bool     Test_INIT()    { return m_State == ST_INIT; }
	inline bool     Test_READY()   { return m_State == ST_READY;}
	inline bool     Test_RUNNING() { return m_State == ST_RUNNING; }
	inline bool     Test_FINAL()   { return m_State == ST_FINAL; }
	inline Result_t Goto_INIT()    { Goto_body(ST_BEGIN,   ST_INIT); }
	inline Result_t Goto_READY()   { Goto_body(ST_INIT,    ST_READY); }
	inline Result_t Goto_RUNNING() { Goto_body(ST_READY,   ST_RUNNING); }
	inline Result_t Goto_FINAL()   { Goto_body(ST_RUNNING, ST_FINAL); }
      };

      //------------------------------------------------------------------------------------------
      //

      //
      template <class HeaderType>
	class TrackFileWriter
      {
	KM_NO_COPY_CONSTRUCT(TrackFileWriter);
	TrackFileWriter();

      public:
	const Dictionary*  m_Dict;
	Kumu::FileWriter   m_File;
	ui32_t             m_HeaderSize;
	HeaderType         m_HeaderPart;
	RIP                m_RIP;

	MaterialPackage*   m_MaterialPackage;
	SourcePackage*     m_FilePackage;
	ContentStorage*    m_ContentStorage;

	FileDescriptor*    m_EssenceDescriptor;
	std::list<InterchangeObject*> m_EssenceSubDescriptorList;

	ui32_t             m_FramesWritten;
	ui64_t             m_StreamOffset;
	ASDCP::FrameBuffer m_CtFrameBuf;
	h__WriterState     m_State;
	WriterInfo         m_Info;

	typedef std::list<ui64_t*> DurationElementList_t;
	DurationElementList_t m_DurationUpdateList;

      TrackFileWriter(const Dictionary& d) :
	m_Dict(&d), m_HeaderSize(0), m_HeaderPart(m_Dict), m_RIP(m_Dict),
	  m_MaterialPackage(0), m_FilePackage(0), m_ContentStorage(0),
	  m_EssenceDescriptor(0), m_FramesWritten(0), m_StreamOffset(0)
	  {
	    default_md_object_init();
	  }

	virtual ~TrackFileWriter() {
	  Close();
	}

	const MXF::RIP& GetRIP() const { return m_RIP; }

	void InitHeader()
	{
	  assert(m_Dict);
	  assert(m_EssenceDescriptor);

	  m_HeaderPart.m_Primer.ClearTagList();
	  m_HeaderPart.m_Preface = new Preface(m_Dict);
	  m_HeaderPart.AddChildObject(m_HeaderPart.m_Preface);

	  // Set the Operational Pattern label -- we're just starting and have no RIP or index,
	  // so we tell the world by using OP1a
	  m_HeaderPart.m_Preface->OperationalPattern = UL(m_Dict->ul(MDD_OP1a));
	  m_HeaderPart.OperationalPattern = m_HeaderPart.m_Preface->OperationalPattern;

	  // Identification
	  Identification* Ident = new Identification(m_Dict);
	  m_HeaderPart.AddChildObject(Ident);
	  m_HeaderPart.m_Preface->Identifications.push_back(Ident->InstanceUID);

	  Kumu::GenRandomValue(Ident->ThisGenerationUID);
	  Ident->CompanyName = m_Info.CompanyName.c_str();
	  Ident->ProductName = m_Info.ProductName.c_str();
	  Ident->VersionString = m_Info.ProductVersion.c_str();
	  Ident->ProductUID.Set(m_Info.ProductUUID);
	  Ident->Platform = ASDCP_PLATFORM;

	  std::vector<int> version = version_split(Version());

	  Ident->ToolkitVersion.Major = version[0];
	  Ident->ToolkitVersion.Minor = version[1];
	  Ident->ToolkitVersion.Patch = version[2];
	  Ident->ToolkitVersion.Build = ASDCP_BUILD_NUMBER;
	  Ident->ToolkitVersion.Release = VersionType::RL_RELEASE;
	}

	//
	void AddSourceClip(const MXF::Rational& clip_edit_rate,
			   const MXF::Rational& tc_edit_rate, ui32_t TCFrameRate,
			   const std::string& TrackName, const UL& EssenceUL,
			   const UL& DataDefinition, const std::string& PackageLabel)
	{
	  if ( m_ContentStorage == 0 )
	    {
	      m_ContentStorage = new ContentStorage(m_Dict);
	      m_HeaderPart.AddChildObject(m_ContentStorage);
	      m_HeaderPart.m_Preface->ContentStorage = m_ContentStorage->InstanceUID;
	    }

	  EssenceContainerData* ECD = new EssenceContainerData(m_Dict);
	  m_HeaderPart.AddChildObject(ECD);
	  m_ContentStorage->EssenceContainerData.push_back(ECD->InstanceUID);
	  ECD->IndexSID = 129;
	  ECD->BodySID = 1;

	  UUID assetUUID(m_Info.AssetUUID);
	  UMID SourcePackageUMID, MaterialPackageUMID;
	  SourcePackageUMID.MakeUMID(0x0f, assetUUID);
	  MaterialPackageUMID.MakeUMID(0x0f); // unidentified essence

	  //
	  // Material Package
	  //
	  m_MaterialPackage = new MaterialPackage(m_Dict);
	  m_MaterialPackage->Name = "AS-DCP Material Package";
	  m_MaterialPackage->PackageUID = MaterialPackageUMID;
	  m_HeaderPart.AddChildObject(m_MaterialPackage);
	  m_ContentStorage->Packages.push_back(m_MaterialPackage->InstanceUID);

	  TrackSet<TimecodeComponent> MPTCTrack =
	    CreateTimecodeTrack<MaterialPackage>(m_HeaderPart, *m_MaterialPackage,
						 tc_edit_rate, TCFrameRate, 0, m_Dict);

	  MPTCTrack.Sequence->Duration.set_has_value();
	  m_DurationUpdateList.push_back(&(MPTCTrack.Sequence->Duration.get()));
	  MPTCTrack.Clip->Duration.set_has_value();
	  m_DurationUpdateList.push_back(&(MPTCTrack.Clip->Duration.get()));

	  TrackSet<SourceClip> MPTrack =
	    CreateTrackAndSequence<MaterialPackage, SourceClip>(m_HeaderPart, *m_MaterialPackage,
								TrackName, clip_edit_rate, DataDefinition,
								2, m_Dict);
	  MPTrack.Sequence->Duration.set_has_value();
	  m_DurationUpdateList.push_back(&(MPTrack.Sequence->Duration.get()));

	  MPTrack.Clip = new SourceClip(m_Dict);
	  m_HeaderPart.AddChildObject(MPTrack.Clip);
	  MPTrack.Sequence->StructuralComponents.push_back(MPTrack.Clip->InstanceUID);
	  MPTrack.Clip->DataDefinition = DataDefinition;
	  MPTrack.Clip->SourcePackageID = SourcePackageUMID;
	  MPTrack.Clip->SourceTrackID = 2;

	  MPTrack.Clip->Duration.set_has_value();
	  m_DurationUpdateList.push_back(&(MPTrack.Clip->Duration.get()));

  
	  //
	  // File (Source) Package
	  //
	  m_FilePackage = new SourcePackage(m_Dict);
	  m_FilePackage->Name = PackageLabel.c_str();
	  m_FilePackage->PackageUID = SourcePackageUMID;
	  ECD->LinkedPackageUID = SourcePackageUMID;

	  m_HeaderPart.AddChildObject(m_FilePackage);
	  m_ContentStorage->Packages.push_back(m_FilePackage->InstanceUID);

	  TrackSet<TimecodeComponent> FPTCTrack =
	    CreateTimecodeTrack<SourcePackage>(m_HeaderPart, *m_FilePackage,
					       tc_edit_rate, TCFrameRate,
					       ui64_C(3600) * TCFrameRate, m_Dict);

	  FPTCTrack.Sequence->Duration.set_has_value();
	  m_DurationUpdateList.push_back(&(FPTCTrack.Sequence->Duration.get()));
	  FPTCTrack.Clip->Duration.set_has_value();
	  m_DurationUpdateList.push_back(&(FPTCTrack.Clip->Duration.get()));

	  TrackSet<SourceClip> FPTrack =
	    CreateTrackAndSequence<SourcePackage, SourceClip>(m_HeaderPart, *m_FilePackage,
							      TrackName, clip_edit_rate, DataDefinition,
							      2, m_Dict);

	  FPTrack.Sequence->Duration.set_has_value();
	  m_DurationUpdateList.push_back(&(FPTrack.Sequence->Duration.get()));

	  // Consult ST 379:2004 Sec. 6.3, "Element to track relationship" to see where "12" comes from.
	  FPTrack.Track->TrackNumber = KM_i32_BE(Kumu::cp2i<ui32_t>((EssenceUL.Value() + 12)));

	  FPTrack.Clip = new SourceClip(m_Dict);
	  m_HeaderPart.AddChildObject(FPTrack.Clip);
	  FPTrack.Sequence->StructuralComponents.push_back(FPTrack.Clip->InstanceUID);
	  FPTrack.Clip->DataDefinition = DataDefinition;

	  // for now we do not allow setting this value, so all files will be 'original'
	  FPTrack.Clip->SourceTrackID = 0;
	  FPTrack.Clip->SourcePackageID = NilUMID;

	  FPTrack.Clip->Duration.set_has_value();
	  m_DurationUpdateList.push_back(&(FPTrack.Clip->Duration.get()));

	  m_EssenceDescriptor->LinkedTrackID = FPTrack.Track->TrackID;
	}

	//
	void AddDMSegment(const MXF::Rational& clip_edit_rate,
			  const MXF::Rational& tc_edit_rate, ui32_t tc_frame_rate,
			  const std::string& TrackName, const UL& DataDefinition,
			  const std::string& PackageLabel)
	{
	  if ( m_ContentStorage == 0 )
	    {
	      m_ContentStorage = new ContentStorage(m_Dict);
	      m_HeaderPart.AddChildObject(m_ContentStorage);
	      m_HeaderPart.m_Preface->ContentStorage = m_ContentStorage->InstanceUID;
	    }

	  EssenceContainerData* ECD = new EssenceContainerData(m_Dict);
	  m_HeaderPart.AddChildObject(ECD);
	  m_ContentStorage->EssenceContainerData.push_back(ECD->InstanceUID);
	  ECD->IndexSID = 129;
	  ECD->BodySID = 1;

	  UUID assetUUID(m_Info.AssetUUID);
	  UMID SourcePackageUMID, MaterialPackageUMID;
	  SourcePackageUMID.MakeUMID(0x0f, assetUUID);
	  MaterialPackageUMID.MakeUMID(0x0f); // unidentified essence

	  //
	  // Material Package
	  //
	  m_MaterialPackage = new MaterialPackage(m_Dict);
	  m_MaterialPackage->Name = "AS-DCP Material Package";
	  m_MaterialPackage->PackageUID = MaterialPackageUMID;
	  m_HeaderPart.AddChildObject(m_MaterialPackage);
	  m_ContentStorage->Packages.push_back(m_MaterialPackage->InstanceUID);

	  TrackSet<TimecodeComponent> MPTCTrack =
	    CreateTimecodeTrack<MaterialPackage>(m_HeaderPart, *m_MaterialPackage,
						 tc_edit_rate, tc_frame_rate, 0, m_Dict);

	  MPTCTrack.Sequence->Duration.set_has_value();
	  m_DurationUpdateList.push_back(&(MPTCTrack.Sequence->Duration.get()));
	  MPTCTrack.Clip->Duration.set_has_value();
	  m_DurationUpdateList.push_back(&(MPTCTrack.Clip->Duration.get()));

	  TrackSet<DMSegment> MPTrack =
	    CreateTrackAndSequence<MaterialPackage, DMSegment>(m_HeaderPart, *m_MaterialPackage,
							       TrackName, clip_edit_rate, DataDefinition,
							       2, m_Dict);
	  MPTrack.Sequence->Duration.set_has_value();
	  m_DurationUpdateList.push_back(&(MPTrack.Sequence->Duration.get()));

	  MPTrack.Clip = new DMSegment(m_Dict);
	  m_HeaderPart.AddChildObject(MPTrack.Clip);
	  MPTrack.Sequence->StructuralComponents.push_back(MPTrack.Clip->InstanceUID);
	  MPTrack.Clip->DataDefinition = DataDefinition;
	  //  MPTrack.Clip->SourcePackageID = SourcePackageUMID;
	  //  MPTrack.Clip->SourceTrackID = 2;

	  m_DurationUpdateList.push_back(&(MPTrack.Clip->Duration));

  
	  //
	  // File (Source) Package
	  //
	  m_FilePackage = new SourcePackage(m_Dict);
	  m_FilePackage->Name = PackageLabel.c_str();
	  m_FilePackage->PackageUID = SourcePackageUMID;
	  ECD->LinkedPackageUID = SourcePackageUMID;

	  m_HeaderPart.AddChildObject(m_FilePackage);
	  m_ContentStorage->Packages.push_back(m_FilePackage->InstanceUID);

	  TrackSet<TimecodeComponent> FPTCTrack =
	    CreateTimecodeTrack<SourcePackage>(m_HeaderPart, *m_FilePackage,
					       clip_edit_rate, tc_frame_rate,
					       ui64_C(3600) * tc_frame_rate, m_Dict);

	  FPTCTrack.Sequence->Duration.set_has_value();
	  m_DurationUpdateList.push_back(&(FPTCTrack.Sequence->Duration.get()));
	  FPTCTrack.Clip->Duration.set_has_value();
	  m_DurationUpdateList.push_back(&(FPTCTrack.Clip->Duration.get()));

	  TrackSet<DMSegment> FPTrack =
	    CreateTrackAndSequence<SourcePackage, DMSegment>(m_HeaderPart, *m_FilePackage,
							     TrackName, clip_edit_rate, DataDefinition,
							     2, m_Dict);

	  FPTrack.Sequence->Duration.set_has_value();
	  m_DurationUpdateList.push_back(&(FPTrack.Sequence->Duration.get()));

	  FPTrack.Clip = new DMSegment(m_Dict);
	  m_HeaderPart.AddChildObject(FPTrack.Clip);
	  FPTrack.Sequence->StructuralComponents.push_back(FPTrack.Clip->InstanceUID);
	  FPTrack.Clip->DataDefinition = DataDefinition;
	  FPTrack.Clip->EventComment = "ST 429-5 Timed Text";

	  m_DurationUpdateList.push_back(&(FPTrack.Clip->Duration));

	  m_EssenceDescriptor->LinkedTrackID = FPTrack.Track->TrackID;
	}

	//
	void AddEssenceDescriptor(const UL& WrappingUL)
	{
	  //
	  // Essence Descriptor
	  //
	  m_EssenceDescriptor->EssenceContainer = WrappingUL;
	  m_HeaderPart.m_Preface->PrimaryPackage = m_FilePackage->InstanceUID;

	  //
	  // Essence Descriptors
	  //
	  assert(m_Dict);
	  UL GenericContainerUL(m_Dict->ul(MDD_GCMulti));
	  m_HeaderPart.EssenceContainers.push_back(GenericContainerUL);

	  if ( m_Info.EncryptedEssence )
	    {
	      UL CryptEssenceUL(m_Dict->ul(MDD_EncryptedContainerLabel));
	      m_HeaderPart.EssenceContainers.push_back(CryptEssenceUL);
	      m_HeaderPart.m_Preface->DMSchemes.push_back(UL(m_Dict->ul(MDD_CryptographicFrameworkLabel)));
	      AddDMScrypt(m_HeaderPart, *m_FilePackage, m_Info, WrappingUL, m_Dict);
	      //// TODO: fix DMSegment Duration value
	    }
	  else
	    {
	      m_HeaderPart.EssenceContainers.push_back(WrappingUL);
	    }

	  m_HeaderPart.m_Preface->EssenceContainers = m_HeaderPart.EssenceContainers;
	  m_HeaderPart.AddChildObject(m_EssenceDescriptor);

	  std::list<InterchangeObject*>::iterator sdli = m_EssenceSubDescriptorList.begin();
	  for ( ; sdli != m_EssenceSubDescriptorList.end(); sdli++ )
	    m_HeaderPart.AddChildObject(*sdli);

	  m_FilePackage->Descriptor = m_EssenceDescriptor->InstanceUID;
	}

	//
	void Close()
	{
	  m_File.Close();
	}

      };
      
  }/// namespace MXF

  //------------------------------------------------------------------------------------------
  //

  //
  class h__ASDCPReader : public MXF::TrackFileReader<OP1aHeader, OPAtomIndexFooter>
    {
      ASDCP_NO_COPY_CONSTRUCT(h__ASDCPReader);
      h__ASDCPReader();

    public:
      Partition m_BodyPart;

      h__ASDCPReader(const Dictionary&);
      virtual ~h__ASDCPReader();

      Result_t OpenMXFRead(const std::string& filename);
      Result_t ReadEKLVFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
			     const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC);
      Result_t LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset,
                           i8_t& temporalOffset, i8_t& keyFrameOffset);
    };

  //
  class h__ASDCPWriter : public MXF::TrackFileWriter<OP1aHeader>
    {
      ASDCP_NO_COPY_CONSTRUCT(h__ASDCPWriter);
      h__ASDCPWriter();

    public:
      Partition          m_BodyPart;
      OPAtomIndexFooter  m_FooterPart;

      h__ASDCPWriter(const Dictionary&);
      virtual ~h__ASDCPWriter();

      // all the above for a single source clip
      Result_t WriteASDCPHeader(const std::string& PackageLabel, const UL& WrappingUL,
				const std::string& TrackName, const UL& EssenceUL,
				const UL& DataDefinition, const MXF::Rational& EditRate,
				ui32_t TCFrameRate, ui32_t BytesPerEditUnit = 0);

      Result_t CreateBodyPart(const MXF::Rational& EditRate, ui32_t BytesPerEditUnit = 0);
      Result_t WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf,const byte_t* EssenceUL,
			       AESEncContext* Ctx, HMACContext* HMAC);
      Result_t WriteASDCPFooter();
    };


  // helper class for calculating Integrity Packs, used by WriteEKLVPacket() below.
  //
  class IntegrityPack
    {
    public:
      byte_t Data[klv_intpack_size];

      IntegrityPack() {
	memset(Data, 0, klv_intpack_size);
      }

      ~IntegrityPack() {}

      Result_t CalcValues(const ASDCP::FrameBuffer&, const byte_t* AssetID, ui32_t sequence, HMACContext* HMAC);
      Result_t TestValues(const ASDCP::FrameBuffer&, const byte_t* AssetID, ui32_t sequence, HMACContext* HMAC);
    };


} // namespace ASDCP

#endif // _AS_DCP_INTERNAL_H_


//
// end AS_DCP_internal.h
//
