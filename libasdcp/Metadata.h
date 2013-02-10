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
/*! \file    Metadata.h
    \version $Id: Metadata.h,v 1.29 2012/02/21 02:09:31 jhurst Exp $
    \brief   MXF metadata objects
*/

#ifndef _Metadata_H_
#define _Metadata_H_

#include "MXF.h"

namespace ASDCP
{
  namespace MXF
    {
      void Metadata_InitTypes(const Dictionary*& Dict);

      //

      //
      class Identification : public InterchangeObject
	{
	  Identification();

	public:
	  const Dictionary*& m_Dict;
          UUID ThisGenerationUID;
          UTF16String CompanyName;
          UTF16String ProductName;
          VersionType ProductVersion;
          UTF16String VersionString;
          UUID ProductUID;
          Kumu::Timestamp ModificationDate;
          VersionType ToolkitVersion;
          UTF16String Platform;

      Identification(const Dictionary*& d);
      Identification(const Identification& rhs);
      virtual ~Identification() {}

      const Identification& operator=(const Identification& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const Identification& rhs);
      virtual const char* HasName() { return "Identification"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class ContentStorage : public InterchangeObject
	{
	  ContentStorage();

	public:
	  const Dictionary*& m_Dict;
          Batch<UUID> Packages;
          Batch<UUID> EssenceContainerData;

      ContentStorage(const Dictionary*& d);
      ContentStorage(const ContentStorage& rhs);
      virtual ~ContentStorage() {}

      const ContentStorage& operator=(const ContentStorage& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const ContentStorage& rhs);
      virtual const char* HasName() { return "ContentStorage"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class EssenceContainerData : public InterchangeObject
	{
	  EssenceContainerData();

	public:
	  const Dictionary*& m_Dict;
          UMID LinkedPackageUID;
          ui32_t IndexSID;
          ui32_t BodySID;

      EssenceContainerData(const Dictionary*& d);
      EssenceContainerData(const EssenceContainerData& rhs);
      virtual ~EssenceContainerData() {}

      const EssenceContainerData& operator=(const EssenceContainerData& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const EssenceContainerData& rhs);
      virtual const char* HasName() { return "EssenceContainerData"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericPackage : public InterchangeObject
	{
	  GenericPackage();

	public:
	  const Dictionary*& m_Dict;
          UMID PackageUID;
          UTF16String Name;
          Kumu::Timestamp PackageCreationDate;
          Kumu::Timestamp PackageModifiedDate;
          Batch<UUID> Tracks;

      GenericPackage(const Dictionary*& d);
      GenericPackage(const GenericPackage& rhs);
      virtual ~GenericPackage() {}

      const GenericPackage& operator=(const GenericPackage& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const GenericPackage& rhs);
      virtual const char* HasName() { return "GenericPackage"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
	};

      //
      class MaterialPackage : public GenericPackage
	{
	  MaterialPackage();

	public:
	  const Dictionary*& m_Dict;

      MaterialPackage(const Dictionary*& d);
      MaterialPackage(const MaterialPackage& rhs);
      virtual ~MaterialPackage() {}

      const MaterialPackage& operator=(const MaterialPackage& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const MaterialPackage& rhs);
      virtual const char* HasName() { return "MaterialPackage"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class SourcePackage : public GenericPackage
	{
	  SourcePackage();

	public:
	  const Dictionary*& m_Dict;
          UUID Descriptor;

      SourcePackage(const Dictionary*& d);
      SourcePackage(const SourcePackage& rhs);
      virtual ~SourcePackage() {}

      const SourcePackage& operator=(const SourcePackage& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const SourcePackage& rhs);
      virtual const char* HasName() { return "SourcePackage"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericTrack : public InterchangeObject
	{
	  GenericTrack();

	public:
	  const Dictionary*& m_Dict;
          ui32_t TrackID;
          ui32_t TrackNumber;
          UTF16String TrackName;
          UUID Sequence;

      GenericTrack(const Dictionary*& d);
      GenericTrack(const GenericTrack& rhs);
      virtual ~GenericTrack() {}

      const GenericTrack& operator=(const GenericTrack& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const GenericTrack& rhs);
      virtual const char* HasName() { return "GenericTrack"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
	};

      //
      class StaticTrack : public GenericTrack
	{
	  StaticTrack();

	public:
	  const Dictionary*& m_Dict;

      StaticTrack(const Dictionary*& d);
      StaticTrack(const StaticTrack& rhs);
      virtual ~StaticTrack() {}

      const StaticTrack& operator=(const StaticTrack& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const StaticTrack& rhs);
      virtual const char* HasName() { return "StaticTrack"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class Track : public GenericTrack
	{
	  Track();

	public:
	  const Dictionary*& m_Dict;
          Rational EditRate;
          ui64_t Origin;

      Track(const Dictionary*& d);
      Track(const Track& rhs);
      virtual ~Track() {}

      const Track& operator=(const Track& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const Track& rhs);
      virtual const char* HasName() { return "Track"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class StructuralComponent : public InterchangeObject
	{
	  StructuralComponent();

	public:
	  const Dictionary*& m_Dict;
          UL DataDefinition;
          ui64_t Duration;

      StructuralComponent(const Dictionary*& d);
      StructuralComponent(const StructuralComponent& rhs);
      virtual ~StructuralComponent() {}

      const StructuralComponent& operator=(const StructuralComponent& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const StructuralComponent& rhs);
      virtual const char* HasName() { return "StructuralComponent"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
	};

      //
      class Sequence : public StructuralComponent
	{
	  Sequence();

	public:
	  const Dictionary*& m_Dict;
          Batch<UUID> StructuralComponents;

      Sequence(const Dictionary*& d);
      Sequence(const Sequence& rhs);
      virtual ~Sequence() {}

      const Sequence& operator=(const Sequence& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const Sequence& rhs);
      virtual const char* HasName() { return "Sequence"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class SourceClip : public StructuralComponent
	{
	  SourceClip();

	public:
	  const Dictionary*& m_Dict;
          ui64_t StartPosition;
          UMID SourcePackageID;
          ui32_t SourceTrackID;

      SourceClip(const Dictionary*& d);
      SourceClip(const SourceClip& rhs);
      virtual ~SourceClip() {}

      const SourceClip& operator=(const SourceClip& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const SourceClip& rhs);
      virtual const char* HasName() { return "SourceClip"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class TimecodeComponent : public StructuralComponent
	{
	  TimecodeComponent();

	public:
	  const Dictionary*& m_Dict;
          ui16_t RoundedTimecodeBase;
          ui64_t StartTimecode;
          ui8_t DropFrame;

      TimecodeComponent(const Dictionary*& d);
      TimecodeComponent(const TimecodeComponent& rhs);
      virtual ~TimecodeComponent() {}

      const TimecodeComponent& operator=(const TimecodeComponent& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const TimecodeComponent& rhs);
      virtual const char* HasName() { return "TimecodeComponent"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericDescriptor : public InterchangeObject
	{
	  GenericDescriptor();

	public:
	  const Dictionary*& m_Dict;
          Batch<UUID> Locators;
          Batch<UUID> SubDescriptors;

      GenericDescriptor(const Dictionary*& d);
      GenericDescriptor(const GenericDescriptor& rhs);
      virtual ~GenericDescriptor() {}

      const GenericDescriptor& operator=(const GenericDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const GenericDescriptor& rhs);
      virtual const char* HasName() { return "GenericDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
	};

      //
      class FileDescriptor : public GenericDescriptor
	{
	  FileDescriptor();

	public:
	  const Dictionary*& m_Dict;
          ui32_t LinkedTrackID;
          Rational SampleRate;
          ui64_t ContainerDuration;
          UL EssenceContainer;
          UL Codec;

      FileDescriptor(const Dictionary*& d);
      FileDescriptor(const FileDescriptor& rhs);
      virtual ~FileDescriptor() {}

      const FileDescriptor& operator=(const FileDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const FileDescriptor& rhs);
      virtual const char* HasName() { return "FileDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericSoundEssenceDescriptor : public FileDescriptor
	{
	  GenericSoundEssenceDescriptor();

	public:
	  const Dictionary*& m_Dict;
          Rational AudioSamplingRate;
          ui8_t Locked;
          ui8_t AudioRefLevel;
          ui32_t ChannelCount;
          ui32_t QuantizationBits;
          ui8_t DialNorm;

      GenericSoundEssenceDescriptor(const Dictionary*& d);
      GenericSoundEssenceDescriptor(const GenericSoundEssenceDescriptor& rhs);
      virtual ~GenericSoundEssenceDescriptor() {}

      const GenericSoundEssenceDescriptor& operator=(const GenericSoundEssenceDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const GenericSoundEssenceDescriptor& rhs);
      virtual const char* HasName() { return "GenericSoundEssenceDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class WaveAudioDescriptor : public GenericSoundEssenceDescriptor
	{
	  WaveAudioDescriptor();

	public:
	  const Dictionary*& m_Dict;
          ui16_t BlockAlign;
          ui8_t SequenceOffset;
          ui32_t AvgBps;
          UL ChannelAssignment;

      WaveAudioDescriptor(const Dictionary*& d);
      WaveAudioDescriptor(const WaveAudioDescriptor& rhs);
      virtual ~WaveAudioDescriptor() {}

      const WaveAudioDescriptor& operator=(const WaveAudioDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const WaveAudioDescriptor& rhs);
      virtual const char* HasName() { return "WaveAudioDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericPictureEssenceDescriptor : public FileDescriptor
	{
	  GenericPictureEssenceDescriptor();

	public:
	  const Dictionary*& m_Dict;
          ui8_t FrameLayout;
          ui32_t StoredWidth;
          ui32_t StoredHeight;
          Rational AspectRatio;
          UL PictureEssenceCoding;

      GenericPictureEssenceDescriptor(const Dictionary*& d);
      GenericPictureEssenceDescriptor(const GenericPictureEssenceDescriptor& rhs);
      virtual ~GenericPictureEssenceDescriptor() {}

      const GenericPictureEssenceDescriptor& operator=(const GenericPictureEssenceDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const GenericPictureEssenceDescriptor& rhs);
      virtual const char* HasName() { return "GenericPictureEssenceDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class RGBAEssenceDescriptor : public GenericPictureEssenceDescriptor
	{
	  RGBAEssenceDescriptor();

	public:
	  const Dictionary*& m_Dict;
          ui32_t ComponentMaxRef;
          ui32_t ComponentMinRef;

      RGBAEssenceDescriptor(const Dictionary*& d);
      RGBAEssenceDescriptor(const RGBAEssenceDescriptor& rhs);
      virtual ~RGBAEssenceDescriptor() {}

      const RGBAEssenceDescriptor& operator=(const RGBAEssenceDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const RGBAEssenceDescriptor& rhs);
      virtual const char* HasName() { return "RGBAEssenceDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class JPEG2000PictureSubDescriptor : public InterchangeObject
	{
	  JPEG2000PictureSubDescriptor();

	public:
	  const Dictionary*& m_Dict;
          ui16_t Rsize;
          ui32_t Xsize;
          ui32_t Ysize;
          ui32_t XOsize;
          ui32_t YOsize;
          ui32_t XTsize;
          ui32_t YTsize;
          ui32_t XTOsize;
          ui32_t YTOsize;
          ui16_t Csize;
          Raw PictureComponentSizing;
          Raw CodingStyleDefault;
          Raw QuantizationDefault;

      JPEG2000PictureSubDescriptor(const Dictionary*& d);
      JPEG2000PictureSubDescriptor(const JPEG2000PictureSubDescriptor& rhs);
      virtual ~JPEG2000PictureSubDescriptor() {}

      const JPEG2000PictureSubDescriptor& operator=(const JPEG2000PictureSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const JPEG2000PictureSubDescriptor& rhs);
      virtual const char* HasName() { return "JPEG2000PictureSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class CDCIEssenceDescriptor : public GenericPictureEssenceDescriptor
	{
	  CDCIEssenceDescriptor();

	public:
	  const Dictionary*& m_Dict;
          ui32_t ComponentDepth;
          ui32_t HorizontalSubsampling;
          ui32_t VerticalSubsampling;
          ui8_t ColorSiting;

      CDCIEssenceDescriptor(const Dictionary*& d);
      CDCIEssenceDescriptor(const CDCIEssenceDescriptor& rhs);
      virtual ~CDCIEssenceDescriptor() {}

      const CDCIEssenceDescriptor& operator=(const CDCIEssenceDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const CDCIEssenceDescriptor& rhs);
      virtual const char* HasName() { return "CDCIEssenceDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class MPEG2VideoDescriptor : public CDCIEssenceDescriptor
	{
	  MPEG2VideoDescriptor();

	public:
	  const Dictionary*& m_Dict;
          ui8_t CodedContentType;
          ui8_t LowDelay;
          ui32_t BitRate;
          ui8_t ProfileAndLevel;

      MPEG2VideoDescriptor(const Dictionary*& d);
      MPEG2VideoDescriptor(const MPEG2VideoDescriptor& rhs);
      virtual ~MPEG2VideoDescriptor() {}

      const MPEG2VideoDescriptor& operator=(const MPEG2VideoDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const MPEG2VideoDescriptor& rhs);
      virtual const char* HasName() { return "MPEG2VideoDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class DMSegment : public InterchangeObject
	{
	  DMSegment();

	public:
	  const Dictionary*& m_Dict;
          UL DataDefinition;
          ui64_t EventStartPosition;
          ui64_t Duration;
          UTF16String EventComment;
          UUID DMFramework;

      DMSegment(const Dictionary*& d);
      DMSegment(const DMSegment& rhs);
      virtual ~DMSegment() {}

      const DMSegment& operator=(const DMSegment& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const DMSegment& rhs);
      virtual const char* HasName() { return "DMSegment"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class CryptographicFramework : public InterchangeObject
	{
	  CryptographicFramework();

	public:
	  const Dictionary*& m_Dict;
          UUID ContextSR;

      CryptographicFramework(const Dictionary*& d);
      CryptographicFramework(const CryptographicFramework& rhs);
      virtual ~CryptographicFramework() {}

      const CryptographicFramework& operator=(const CryptographicFramework& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const CryptographicFramework& rhs);
      virtual const char* HasName() { return "CryptographicFramework"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class CryptographicContext : public InterchangeObject
	{
	  CryptographicContext();

	public:
	  const Dictionary*& m_Dict;
          UUID ContextID;
          UL SourceEssenceContainer;
          UL CipherAlgorithm;
          UL MICAlgorithm;
          UUID CryptographicKeyID;

      CryptographicContext(const Dictionary*& d);
      CryptographicContext(const CryptographicContext& rhs);
      virtual ~CryptographicContext() {}

      const CryptographicContext& operator=(const CryptographicContext& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const CryptographicContext& rhs);
      virtual const char* HasName() { return "CryptographicContext"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericDataEssenceDescriptor : public FileDescriptor
	{
	  GenericDataEssenceDescriptor();

	public:
	  const Dictionary*& m_Dict;
          UL DataEssenceCoding;

      GenericDataEssenceDescriptor(const Dictionary*& d);
      GenericDataEssenceDescriptor(const GenericDataEssenceDescriptor& rhs);
      virtual ~GenericDataEssenceDescriptor() {}

      const GenericDataEssenceDescriptor& operator=(const GenericDataEssenceDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const GenericDataEssenceDescriptor& rhs);
      virtual const char* HasName() { return "GenericDataEssenceDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class TimedTextDescriptor : public GenericDataEssenceDescriptor
	{
	  TimedTextDescriptor();

	public:
	  const Dictionary*& m_Dict;
          UUID ResourceID;
          UTF16String UCSEncoding;
          UTF16String NamespaceURI;

      TimedTextDescriptor(const Dictionary*& d);
      TimedTextDescriptor(const TimedTextDescriptor& rhs);
      virtual ~TimedTextDescriptor() {}

      const TimedTextDescriptor& operator=(const TimedTextDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const TimedTextDescriptor& rhs);
      virtual const char* HasName() { return "TimedTextDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class TimedTextResourceSubDescriptor : public InterchangeObject
	{
	  TimedTextResourceSubDescriptor();

	public:
	  const Dictionary*& m_Dict;
          UUID AncillaryResourceID;
          UTF16String MIMEMediaType;
          ui32_t EssenceStreamID;

      TimedTextResourceSubDescriptor(const Dictionary*& d);
      TimedTextResourceSubDescriptor(const TimedTextResourceSubDescriptor& rhs);
      virtual ~TimedTextResourceSubDescriptor() {}

      const TimedTextResourceSubDescriptor& operator=(const TimedTextResourceSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const TimedTextResourceSubDescriptor& rhs);
      virtual const char* HasName() { return "TimedTextResourceSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class StereoscopicPictureSubDescriptor : public InterchangeObject
	{
	  StereoscopicPictureSubDescriptor();

	public:
	  const Dictionary*& m_Dict;

      StereoscopicPictureSubDescriptor(const Dictionary*& d);
      StereoscopicPictureSubDescriptor(const StereoscopicPictureSubDescriptor& rhs);
      virtual ~StereoscopicPictureSubDescriptor() {}

      const StereoscopicPictureSubDescriptor& operator=(const StereoscopicPictureSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const StereoscopicPictureSubDescriptor& rhs);
      virtual const char* HasName() { return "StereoscopicPictureSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class NetworkLocator : public InterchangeObject
	{
	  NetworkLocator();

	public:
	  const Dictionary*& m_Dict;
          UTF16String URLString;

      NetworkLocator(const Dictionary*& d);
      NetworkLocator(const NetworkLocator& rhs);
      virtual ~NetworkLocator() {}

      const NetworkLocator& operator=(const NetworkLocator& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const NetworkLocator& rhs);
      virtual const char* HasName() { return "NetworkLocator"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class MCALabelSubDescriptor : public InterchangeObject
	{
	  MCALabelSubDescriptor();

	public:
	  const Dictionary*& m_Dict;
          UL MCALabelDictionaryID;
          UUID MCALinkID;
          UTF16String MCATagSymbol;
          UTF16String MCATagName;
          ui32_t MCAChannelID;
          ISO8String RFC5646SpokenLanguage;

      MCALabelSubDescriptor(const Dictionary*& d);
      MCALabelSubDescriptor(const MCALabelSubDescriptor& rhs);
      virtual ~MCALabelSubDescriptor() {}

      const MCALabelSubDescriptor& operator=(const MCALabelSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const MCALabelSubDescriptor& rhs);
      virtual const char* HasName() { return "MCALabelSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class AudioChannelLabelSubDescriptor : public MCALabelSubDescriptor
	{
	  AudioChannelLabelSubDescriptor();

	public:
	  const Dictionary*& m_Dict;
          UUID SoundfieldGroupLinkID;

      AudioChannelLabelSubDescriptor(const Dictionary*& d);
      AudioChannelLabelSubDescriptor(const AudioChannelLabelSubDescriptor& rhs);
      virtual ~AudioChannelLabelSubDescriptor() {}

      const AudioChannelLabelSubDescriptor& operator=(const AudioChannelLabelSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const AudioChannelLabelSubDescriptor& rhs);
      virtual const char* HasName() { return "AudioChannelLabelSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class SoundfieldGroupLabelSubDescriptor : public MCALabelSubDescriptor
	{
	  SoundfieldGroupLabelSubDescriptor();

	public:
	  const Dictionary*& m_Dict;
          Array<UUID> GroupOfSoundfieldGroupsLinkID;

      SoundfieldGroupLabelSubDescriptor(const Dictionary*& d);
      SoundfieldGroupLabelSubDescriptor(const SoundfieldGroupLabelSubDescriptor& rhs);
      virtual ~SoundfieldGroupLabelSubDescriptor() {}

      const SoundfieldGroupLabelSubDescriptor& operator=(const SoundfieldGroupLabelSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const SoundfieldGroupLabelSubDescriptor& rhs);
      virtual const char* HasName() { return "SoundfieldGroupLabelSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GroupOfSoundfieldGroupsLabelSubDescriptor : public MCALabelSubDescriptor
	{
	  GroupOfSoundfieldGroupsLabelSubDescriptor();

	public:
	  const Dictionary*& m_Dict;

      GroupOfSoundfieldGroupsLabelSubDescriptor(const Dictionary*& d);
      GroupOfSoundfieldGroupsLabelSubDescriptor(const GroupOfSoundfieldGroupsLabelSubDescriptor& rhs);
      virtual ~GroupOfSoundfieldGroupsLabelSubDescriptor() {}

      const GroupOfSoundfieldGroupsLabelSubDescriptor& operator=(const GroupOfSoundfieldGroupsLabelSubDescriptor& rhs) { Copy(rhs); return *this; }
      virtual void Copy(const GroupOfSoundfieldGroupsLabelSubDescriptor& rhs);
      virtual const char* HasName() { return "GroupOfSoundfieldGroupsLabelSubDescriptor"; }
      virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
      virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
      virtual void     Dump(FILE* = 0);
      virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
      virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

    } // namespace MXF
} // namespace ASDCP


#endif // _Metadata_H_

//
// end Metadata.h
//
