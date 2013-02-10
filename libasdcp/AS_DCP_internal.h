/*
Copyright (c) 2004-2012, John Hurst
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
    \version $Id: AS_DCP_internal.h,v 1.27 2012/02/07 18:54:25 jhurst Exp $       
    \brief   AS-DCP library, non-public common elements
*/

#ifndef _AS_DCP_INTERNAL_H_
#define _AS_DCP_INTERNAL_H_

#include <KM_platform.h>
#include <KM_util.h>
#include <KM_log.h>
#include "Metadata.h"

using Kumu::DefaultLogSink;
// using namespace std;
using namespace ASDCP;
using namespace ASDCP::MXF;

#ifdef DEFAULT_MD_DECL
ASDCP::MXF::OPAtomHeader *g_OPAtomHeader;
ASDCP::MXF::OPAtomIndexFooter *g_OPAtomIndexFooter;
#else
extern MXF::OPAtomHeader *g_OPAtomHeader;
extern MXF::OPAtomIndexFooter *g_OPAtomIndexFooter;
#endif


namespace ASDCP
{
  void default_md_object_init();


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

  Result_t MD_to_WriterInfo(MXF::Identification*, WriterInfo&);
  Result_t MD_to_CryptoInfo(MXF::CryptographicContext*, WriterInfo&, const Dictionary&);
  Result_t EncryptFrameBuffer(const ASDCP::FrameBuffer&, ASDCP::FrameBuffer&, AESEncContext*);
  Result_t DecryptFrameBuffer(const ASDCP::FrameBuffer&, ASDCP::FrameBuffer&, AESDecContext*);

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

  //
  class h__Reader
    {
      ASDCP_NO_COPY_CONSTRUCT(h__Reader);
      h__Reader();

    public:
      const Dictionary*  m_Dict;
      Kumu::FileReader   m_File;
      OPAtomHeader       m_HeaderPart;
      Partition          m_BodyPart;
      OPAtomIndexFooter  m_FooterPart;
      ui64_t             m_EssenceStart;
      WriterInfo         m_Info;
      ASDCP::FrameBuffer m_CtFrameBuf;
      Kumu::fpos_t       m_LastPosition;

      h__Reader(const Dictionary&);
      virtual ~h__Reader();

      Result_t InitInfo();
      Result_t OpenMXFRead(const char* filename);
      Result_t InitMXFIndex();

      // positions file before reading
      Result_t ReadEKLVFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
			     const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC);

      // reads from current position
      Result_t ReadEKLVPacket(ui32_t FrameNum, ui32_t SequenceNum, ASDCP::FrameBuffer& FrameBuf,
			      const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC);
      void     Close();
    };


  // state machine for mxf writer
  enum WriterState_t {
    ST_BEGIN,   // waiting for Open()
    ST_INIT,    // waiting for SetSourceStream()
    ST_READY,   // ready to write frames
    ST_RUNNING, // one or more frames written
    ST_FINAL,   // index written, file closed
  };

  // implementation of h__WriterState class Goto_* methods
#define Goto_body(s1,s2) if ( m_State != (s1) ) \
                           return RESULT_STATE; \
                         m_State = (s2); \
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

  typedef std::list<ui64_t*> DurationElementList_t;

  //
  class h__Writer
    {
      ASDCP_NO_COPY_CONSTRUCT(h__Writer);
      h__Writer();

    public:
      const Dictionary*  m_Dict;
      Kumu::FileWriter   m_File;
      ui32_t             m_HeaderSize;
      OPAtomHeader       m_HeaderPart;
      Partition          m_BodyPart;
      OPAtomIndexFooter  m_FooterPart;
      ui64_t             m_EssenceStart;

      MaterialPackage*   m_MaterialPackage;
      SourcePackage*     m_FilePackage;

      FileDescriptor*    m_EssenceDescriptor;
      std::list<InterchangeObject*> m_EssenceSubDescriptorList;

      ui32_t             m_FramesWritten;
      ui64_t             m_StreamOffset;
      ASDCP::FrameBuffer m_CtFrameBuf;
      h__WriterState     m_State;
      WriterInfo         m_Info;
      DurationElementList_t m_DurationUpdateList;

      h__Writer(const Dictionary&);
      virtual ~h__Writer();

      void InitHeader();
      void AddSourceClip(const MXF::Rational& EditRate, ui32_t TCFrameRate,
			 const std::string& TrackName, const UL& EssenceUL,
			 const UL& DataDefinition, const std::string& PackageLabel);
      void AddDMSegment(const MXF::Rational& EditRate, ui32_t TCFrameRate,
			const std::string& TrackName, const UL& DataDefinition,
			const std::string& PackageLabel);
      void AddEssenceDescriptor(const UL& WrappingUL);
      Result_t CreateBodyPart(const MXF::Rational& EditRate, ui32_t BytesPerEditUnit = 0);

      // all the above for a single source clip
      Result_t WriteMXFHeader(const std::string& PackageLabel, const UL& WrappingUL,
			      const std::string& TrackName, const UL& EssenceUL,
			      const UL& DataDefinition, const MXF::Rational& EditRate,
			      ui32_t TCFrameRate, ui32_t BytesPerEditUnit = 0);

      Result_t WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf,
			       const byte_t* EssenceUL, AESEncContext* Ctx, HMACContext* HMAC);

      Result_t WriteMXFFooter();

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
  
      Result_t CalcValues(const ASDCP::FrameBuffer&, byte_t* AssetID, ui32_t sequence, HMACContext* HMAC);
      Result_t TestValues(const ASDCP::FrameBuffer&, byte_t* AssetID, ui32_t sequence, HMACContext* HMAC);
    };


} // namespace ASDCP

#endif // _AS_DCP_INTERNAL_H_


//
// end AS_DCP_internal.h
//
