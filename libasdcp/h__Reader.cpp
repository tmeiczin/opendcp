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
/*! \file    h__Reader.cpp
    \version $Id: h__Reader.cpp,v 1.31 2012/02/07 18:54:25 jhurst Exp $
    \brief   MXF file reader base class
*/

#define DEFAULT_MD_DECL
#include "AS_DCP_internal.h"
#include "KLV.h"

using namespace ASDCP;
using namespace ASDCP::MXF;

static Kumu::Mutex sg_DefaultMDInitLock;
static bool        sg_DefaultMDTypesInit = false;
static const ASDCP::Dictionary *sg_dict;

//
void
ASDCP::default_md_object_init()
{
  if ( ! sg_DefaultMDTypesInit )
    {
      Kumu::AutoMutex BlockLock(sg_DefaultMDInitLock);

      if ( ! sg_DefaultMDTypesInit )
	{
	  sg_dict = &DefaultSMPTEDict();
	  g_OPAtomHeader = new ASDCP::MXF::OPAtomHeader(sg_dict);
	  g_OPAtomIndexFooter = new ASDCP::MXF::OPAtomIndexFooter(sg_dict);
	  sg_DefaultMDTypesInit = true;
	}
    }
}


//
ASDCP::h__Reader::h__Reader(const Dictionary& d) :
  m_HeaderPart(m_Dict), m_BodyPart(m_Dict), m_FooterPart(m_Dict), m_Dict(&d), m_EssenceStart(0)
{
  default_md_object_init();
}

ASDCP::h__Reader::~h__Reader()
{
  Close();
}

void
ASDCP::h__Reader::Close()
{
  m_File.Close();
}

//------------------------------------------------------------------------------------------
//

//
Result_t
ASDCP::h__Reader::InitInfo()
{
  assert(m_Dict);
  InterchangeObject* Object;

  m_Info.LabelSetType = LS_MXF_UNKNOWN;

  if ( m_HeaderPart.OperationalPattern.ExactMatch(MXFInterop_OPAtom_Entry().ul) )
    m_Info.LabelSetType = LS_MXF_INTEROP;
  else if ( m_HeaderPart.OperationalPattern.ExactMatch(SMPTE_390_OPAtom_Entry().ul) )
    m_Info.LabelSetType = LS_MXF_SMPTE;

  // Identification
  Result_t result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(Identification), &Object);

  if( ASDCP_SUCCESS(result) )
    MD_to_WriterInfo((Identification*)Object, m_Info);

  // SourcePackage
  if( ASDCP_SUCCESS(result) )
    result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(SourcePackage), &Object);

  if( ASDCP_SUCCESS(result) )
    {
      SourcePackage* SP = (SourcePackage*)Object;
      memcpy(m_Info.AssetUUID, SP->PackageUID.Value() + 16, UUIDlen);
    }

  // optional CryptographicContext
  if( ASDCP_SUCCESS(result) )
    {
      Result_t cr_result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(CryptographicContext), &Object);

      if( ASDCP_SUCCESS(cr_result) )
	MD_to_CryptoInfo((CryptographicContext*)Object, m_Info, *m_Dict);
    }

  return result;
}


// standard method of opening an MXF file for read
Result_t
ASDCP::h__Reader::OpenMXFRead(const char* filename)
{
  m_LastPosition = 0;
  Result_t result = m_File.OpenRead(filename);

  if ( ASDCP_SUCCESS(result) )
    result = m_HeaderPart.InitFromFile(m_File);

  if ( ASDCP_SUCCESS(result) )
    {
      // if this is a three partition file, go to the body
      // partition and read the partition pack
      if ( m_HeaderPart.m_RIP.PairArray.size() > 2 )
	{
	  Array<RIP::Pair>::iterator r_i = m_HeaderPart.m_RIP.PairArray.begin();
	  r_i++;
	  m_File.Seek((*r_i).ByteOffset);

	  result = m_BodyPart.InitFromFile(m_File);
	}

      m_EssenceStart = m_File.Tell();
    }

  return result;
}


// standard method of populating the in-memory index
Result_t
ASDCP::h__Reader::InitMXFIndex()
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  Result_t result = m_File.Seek(m_HeaderPart.FooterPartition);

  if ( ASDCP_SUCCESS(result) )
    {
      m_FooterPart.m_Lookup = &m_HeaderPart.m_Primer;
      result = m_FooterPart.InitFromFile(m_File);
    }

  if ( ASDCP_SUCCESS(result) )
    m_File.Seek(m_EssenceStart);

  return result;
}

//
Result_t
ASDCP::KLReader::ReadKLFromFile(Kumu::FileReader& Reader)
{
  ui32_t read_count;
  ui32_t header_length = SMPTE_UL_LENGTH + MXF_BER_LENGTH;
  Result_t result = Reader.Read(m_KeyBuf, header_length, &read_count);

  if ( ASDCP_FAILURE(result) )
    return result;

  if ( read_count != header_length )
    return RESULT_READFAIL;

  const byte_t* ber_start = m_KeyBuf + SMPTE_UL_LENGTH;

  if ( ( *ber_start & 0x80 ) == 0 )
    {
      DefaultLogSink().Error("BER encoding error.\n");
      return RESULT_FORMAT;
    }

  ui8_t ber_size = ( *ber_start & 0x0f ) + 1;

  if ( ber_size > 9 )
    {
      DefaultLogSink().Error("BER size encoding error.\n");
      return RESULT_FORMAT;
    }

  if ( ber_size < MXF_BER_LENGTH )
    {
      DefaultLogSink().Error("BER size %d shorter than AS-DCP minimum %d.\n",
			     ber_size, MXF_BER_LENGTH);
      return RESULT_FORMAT;
    }

  if ( ber_size > MXF_BER_LENGTH )
    {
      ui32_t diff = ber_size - MXF_BER_LENGTH;
      assert((SMPTE_UL_LENGTH + MXF_BER_LENGTH + diff) <= (SMPTE_UL_LENGTH * 2));
      result = Reader.Read(m_KeyBuf + SMPTE_UL_LENGTH + MXF_BER_LENGTH, diff, &read_count);

      if ( ASDCP_FAILURE(result) )
	return result;

      if ( read_count != diff )
	return RESULT_READFAIL;

      header_length += diff;
    }

  return InitFromBuffer(m_KeyBuf, header_length);
}

// standard method of reading a plaintext or encrypted frame
Result_t
ASDCP::h__Reader::ReadEKLVFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
				const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC)
{
  // look up frame index node
  IndexTableSegment::IndexEntry TmpEntry;

  if ( ASDCP_FAILURE(m_FooterPart.Lookup(FrameNum, TmpEntry)) )
    {
      DefaultLogSink().Error("Frame value out of range: %u\n", FrameNum);
      return RESULT_RANGE;
    }

  // get frame position and go read the frame's key and length
  Kumu::fpos_t FilePosition = m_EssenceStart + TmpEntry.StreamOffset;
  Result_t result = RESULT_OK;

  if ( FilePosition != m_LastPosition )
    {
      m_LastPosition = FilePosition;
      result = m_File.Seek(FilePosition);
    }

  if( ASDCP_SUCCESS(result) )
    result = ReadEKLVPacket(FrameNum, FrameNum + 1, FrameBuf, EssenceUL, Ctx, HMAC);

  return result;
}


Result_t
ASDCP::h__Reader::ReadEKLVPacket(ui32_t FrameNum, ui32_t SequenceNum, ASDCP::FrameBuffer& FrameBuf,
				 const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC)
{
  KLReader Reader;
  Result_t result = Reader.ReadKLFromFile(m_File);

  if ( ASDCP_FAILURE(result) )
    return result;

  UL Key(Reader.Key());
  ui64_t PacketLength = Reader.Length();
  m_LastPosition = m_LastPosition + Reader.KLLength() + PacketLength;
  assert(m_Dict);

  if ( Key.MatchIgnoreStream(m_Dict->ul(MDD_CryptEssence)) )  // ignore the stream numbers
    {
      if ( ! m_Info.EncryptedEssence )
	{
	  DefaultLogSink().Error("EKLV packet found, no Cryptographic Context in header.\n");
	  return RESULT_FORMAT;
	}

      // read encrypted triplet value into internal buffer
      assert(PacketLength <= 0xFFFFFFFFL);
      m_CtFrameBuf.Capacity((ui32_t) PacketLength);
      ui32_t read_count;
      result = m_File.Read(m_CtFrameBuf.Data(), (ui32_t) PacketLength,
			   &read_count);

      if ( ASDCP_FAILURE(result) )
	return result;

      if ( read_count != PacketLength )
	{
	  DefaultLogSink().Error("read length is smaller than EKLV packet length.\n");
          return RESULT_FORMAT;
        }

      m_CtFrameBuf.Size((ui32_t) PacketLength);

      // should be const but mxflib::ReadBER is not
      byte_t* ess_p = m_CtFrameBuf.Data();

      // read context ID length
      if ( ! Kumu::read_test_BER(&ess_p, UUIDlen) )
	return RESULT_FORMAT;

      // test the context ID
      if ( memcmp(ess_p, m_Info.ContextID, UUIDlen) != 0 )
	{
	  DefaultLogSink().Error("Packet's Cryptographic Context ID does not match the header.\n");
	  return RESULT_FORMAT;
	}
      ess_p += UUIDlen;

      // read PlaintextOffset length
      if ( ! Kumu::read_test_BER(&ess_p, sizeof(ui64_t)) )
	return RESULT_FORMAT;

      ui32_t PlaintextOffset = (ui32_t)KM_i64_BE(Kumu::cp2i<ui64_t>(ess_p));
      ess_p += sizeof(ui64_t);

      // read essence UL length
      if ( ! Kumu::read_test_BER(&ess_p, SMPTE_UL_LENGTH) )
	return RESULT_FORMAT;

      // test essence UL
      if ( ! UL(ess_p).MatchIgnoreStream(EssenceUL) ) // ignore the stream number
	{
	  char strbuf[IntBufferLen];
	  const MDDEntry* Entry = m_Dict->FindUL(Key.Value());
	  if ( Entry == 0 )
	    DefaultLogSink().Warn("Unexpected Encrypted Essence UL found: %s.\n", Key.EncodeString(strbuf, IntBufferLen));
	  else
	    DefaultLogSink().Warn("Unexpected Encrypted Essence UL found: %s.\n", Entry->name);
	  return RESULT_FORMAT;
	}
      ess_p += SMPTE_UL_LENGTH;

      // read SourceLength length
      if ( ! Kumu::read_test_BER(&ess_p, sizeof(ui64_t)) )
	return RESULT_FORMAT;

      ui32_t SourceLength = (ui32_t)KM_i64_BE(Kumu::cp2i<ui64_t>(ess_p));
      ess_p += sizeof(ui64_t);
      assert(SourceLength);
	  
      if ( FrameBuf.Capacity() < SourceLength )
	{
	  DefaultLogSink().Error("FrameBuf.Capacity: %u SourceLength: %u\n", FrameBuf.Capacity(), SourceLength);
	  return RESULT_SMALLBUF;
	}

      ui32_t esv_length = calc_esv_length(SourceLength, PlaintextOffset);

      // read ESV length
      if ( ! Kumu::read_test_BER(&ess_p, esv_length) )
	{
	  DefaultLogSink().Error("read_test_BER did not return %u\n", esv_length);
	  return RESULT_FORMAT;
	}

      ui32_t tmp_len = esv_length + (m_Info.UsesHMAC ? klv_intpack_size : 0);

      if ( PacketLength < tmp_len )
	{
	  DefaultLogSink().Error("Frame length is larger than EKLV packet length.\n");
	  return RESULT_FORMAT;
	}

      if ( Ctx )
	{
	  // wrap the pointer and length as a FrameBuffer for use by
	  // DecryptFrameBuffer() and TestValues()
	  FrameBuffer TmpWrapper;
	  TmpWrapper.SetData(ess_p, tmp_len);
	  TmpWrapper.Size(tmp_len);
	  TmpWrapper.SourceLength(SourceLength);
	  TmpWrapper.PlaintextOffset(PlaintextOffset);

	  result = DecryptFrameBuffer(TmpWrapper, FrameBuf, Ctx);
	  FrameBuf.FrameNumber(FrameNum);
  
	  // detect and test integrity pack
	  if ( ASDCP_SUCCESS(result) && m_Info.UsesHMAC && HMAC )
	    {
	      IntegrityPack IntPack;
	      result = IntPack.TestValues(TmpWrapper, m_Info.AssetUUID, SequenceNum, HMAC);
	    }
	}
      else // return ciphertext to caller
	{
	  if ( FrameBuf.Capacity() < tmp_len )
	    {
	      char intbuf[IntBufferLen];
	      DefaultLogSink().Error("FrameBuf.Capacity: %u FrameLength: %s\n",
				     FrameBuf.Capacity(), ui64sz(PacketLength, intbuf));
	      return RESULT_SMALLBUF;
	    }

	  memcpy(FrameBuf.Data(), ess_p, tmp_len);
	  FrameBuf.Size(tmp_len);
	  FrameBuf.FrameNumber(FrameNum);
	  FrameBuf.SourceLength(SourceLength);
	  FrameBuf.PlaintextOffset(PlaintextOffset);
	}
    }
  else if ( Key.MatchIgnoreStream(EssenceUL) ) // ignore the stream number
    { // read plaintext frame
       if ( FrameBuf.Capacity() < PacketLength )
	{
	  char intbuf[IntBufferLen];
	  DefaultLogSink().Error("FrameBuf.Capacity: %u FrameLength: %s\n",
				 FrameBuf.Capacity(), ui64sz(PacketLength, intbuf));
	  return RESULT_SMALLBUF;
	}

      // read the data into the supplied buffer
      ui32_t read_count;
      assert(PacketLength <= 0xFFFFFFFFL);
      result = m_File.Read(FrameBuf.Data(), (ui32_t) PacketLength, &read_count);
	  
      if ( ASDCP_FAILURE(result) )
	return result;

      if ( read_count != PacketLength )
	{
	  char intbuf1[IntBufferLen];
	  char intbuf2[IntBufferLen];
	  DefaultLogSink().Error("read_count: %s != FrameLength: %s\n",
				 ui64sz(read_count, intbuf1),
				 ui64sz(PacketLength, intbuf2) );
	  
	  return RESULT_READFAIL;
	}

      FrameBuf.FrameNumber(FrameNum);
      FrameBuf.Size(read_count);
    }
  else
    {
      char strbuf[IntBufferLen];
      const MDDEntry* Entry = m_Dict->FindUL(Key.Value());
      if ( Entry == 0 )
        DefaultLogSink().Warn("Unexpected Essence UL found: %s.\n", Key.EncodeString(strbuf, IntBufferLen));
      else
        DefaultLogSink().Warn("Unexpected Essence UL found: %s.\n", Entry->name);
      return RESULT_FORMAT;
    }

  return result;
}


//
// end h__Reader.cpp
//
