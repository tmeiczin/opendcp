/*
Copyright (c) 2004-2015, John Hurst
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
/*! \file    h__Writer.cpp
    \version $Id: h__Writer.cpp,v 1.59 2015/10/09 23:41:11 jhurst Exp $
    \brief   MXF file writer base class
*/

#include "AS_DCP_internal.h"
#include "KLV.h"

using namespace ASDCP;
using namespace ASDCP::MXF;

//
ui32_t
ASDCP::derive_timecode_rate_from_edit_rate(const ASDCP::Rational& edit_rate)
{
  return floor(0.5 + edit_rate.Quotient());
}

//
// add DMS CryptographicFramework entry to source package
void
ASDCP::AddDMScrypt(Partition& HeaderPart, SourcePackage& Package,
		   WriterInfo& Descr, const UL& WrappingUL, const Dictionary*& Dict)
{
  assert(Dict);
  // Essence Track
  StaticTrack* NewTrack = new StaticTrack(Dict);
  HeaderPart.AddChildObject(NewTrack);
  Package.Tracks.push_back(NewTrack->InstanceUID);
  NewTrack->TrackName = "Descriptive Track";
  NewTrack->TrackID = 3;

  Sequence* Seq = new Sequence(Dict);
  HeaderPart.AddChildObject(Seq);
  NewTrack->Sequence = Seq->InstanceUID;
  Seq->DataDefinition = UL(Dict->ul(MDD_DescriptiveMetaDataDef));

  DMSegment* Segment = new DMSegment(Dict);
  HeaderPart.AddChildObject(Segment);
  Seq->StructuralComponents.push_back(Segment->InstanceUID);
  Segment->EventComment = "AS-DCP KLV Encryption";

  CryptographicFramework* CFW = new CryptographicFramework(Dict);
  HeaderPart.AddChildObject(CFW);
  Segment->DMFramework = CFW->InstanceUID;

  CryptographicContext* Context = new CryptographicContext(Dict);
  HeaderPart.AddChildObject(Context);
  CFW->ContextSR = Context->InstanceUID;

  Context->ContextID.Set(Descr.ContextID);
  Context->SourceEssenceContainer = WrappingUL; // ??????
  Context->CipherAlgorithm.Set(Dict->ul(MDD_CipherAlgorithm_AES));
  Context->MICAlgorithm.Set( Descr.UsesHMAC ? Dict->ul(MDD_MICAlgorithm_HMAC_SHA1) : Dict->ul(MDD_MICAlgorithm_NONE) );
  Context->CryptographicKeyID.Set(Descr.CryptographicKeyID);
}



//
ASDCP::h__ASDCPWriter::h__ASDCPWriter(const Dictionary& d) :
  MXF::TrackFileWriter<OP1aHeader>(d), m_BodyPart(m_Dict), m_FooterPart(m_Dict) {}

ASDCP::h__ASDCPWriter::~h__ASDCPWriter() {}


//
Result_t
ASDCP::h__ASDCPWriter::CreateBodyPart(const MXF::Rational& EditRate, ui32_t BytesPerEditUnit)
{
  assert(m_Dict);
  Result_t result = RESULT_OK;

  // create a body partition if we're writing proper 429-3/OP-Atom
  if ( m_Info.LabelSetType == LS_MXF_SMPTE )
    {
      // Body Partition
      m_BodyPart.EssenceContainers = m_HeaderPart.EssenceContainers;
      m_BodyPart.ThisPartition = m_File.Tell();
      m_BodyPart.BodySID = 1;
      UL OPAtomUL(m_Dict->ul(MDD_OPAtom));
      m_BodyPart.OperationalPattern = OPAtomUL;
      m_RIP.PairArray.push_back(RIP::PartitionPair(1, m_BodyPart.ThisPartition)); // Second RIP Entry
      
      UL BodyUL(m_Dict->ul(MDD_ClosedCompleteBodyPartition));
      result = m_BodyPart.WriteToFile(m_File, BodyUL);
    }
  else
    {
      m_HeaderPart.BodySID = 1;
    }

  if ( ASDCP_SUCCESS(result) )
    {
      // Index setup
      Kumu::fpos_t ECoffset = m_File.Tell();
      m_FooterPart.IndexSID = 129;

      if ( BytesPerEditUnit == 0 )
	{
	  m_FooterPart.SetIndexParamsVBR(&m_HeaderPart.m_Primer, EditRate, ECoffset);
	}
      else
	{
	  m_FooterPart.SetIndexParamsCBR(&m_HeaderPart.m_Primer, BytesPerEditUnit, EditRate);
	}
    }

  return result;
}

//
Result_t
ASDCP::h__ASDCPWriter::WriteASDCPHeader(const std::string& PackageLabel, const UL& WrappingUL,
					const std::string& TrackName, const UL& EssenceUL, const UL& DataDefinition,
					const MXF::Rational& EditRate, ui32_t TCFrameRate, ui32_t BytesPerEditUnit)
{
  InitHeader();

  // First RIP Entry
  if ( m_Info.LabelSetType == LS_MXF_SMPTE )  // ERK
    {
      m_RIP.PairArray.push_back(RIP::PartitionPair(0, 0)); // 3-part, no essence in header
    }
  else
    {
      m_RIP.PairArray.push_back(RIP::PartitionPair(1, 0)); // 2-part, essence in header
    }

  // timecode rate and essence rate are the same
  AddSourceClip(EditRate, EditRate, TCFrameRate, TrackName, EssenceUL, DataDefinition, PackageLabel);
  AddEssenceDescriptor(WrappingUL);

  Result_t result = m_HeaderPart.WriteToFile(m_File, m_HeaderSize);

  if ( KM_SUCCESS(result) )
    result = CreateBodyPart(EditRate, BytesPerEditUnit);

  return result;
}

//
Result_t
ASDCP::h__ASDCPWriter::WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf,const byte_t* EssenceUL,
				       AESEncContext* Ctx, HMACContext* HMAC)
{
  return Write_EKLV_Packet(m_File, *m_Dict, m_HeaderPart, m_Info, m_CtFrameBuf, m_FramesWritten,
			   m_StreamOffset, FrameBuf, EssenceUL, Ctx, HMAC);
}

// standard method of writing the header and footer of a completed MXF file
//
Result_t
ASDCP::h__ASDCPWriter::WriteASDCPFooter()
{
  // update all Duration properties
  DurationElementList_t::iterator dli = m_DurationUpdateList.begin();

  for (; dli != m_DurationUpdateList.end(); ++dli )
    {
      **dli = m_FramesWritten;
    }

  m_EssenceDescriptor->ContainerDuration = m_FramesWritten;
  m_FooterPart.PreviousPartition = m_RIP.PairArray.back().ByteOffset;

  Kumu::fpos_t here = m_File.Tell();
  m_RIP.PairArray.push_back(RIP::PartitionPair(0, here)); // Last RIP Entry
  m_HeaderPart.FooterPartition = here;

  assert(m_Dict);
  // re-label the header partition, set the footer
  UL OPAtomUL(m_Dict->ul(MDD_OPAtom));
  m_HeaderPart.OperationalPattern = OPAtomUL;
  m_HeaderPart.m_Preface->OperationalPattern = OPAtomUL;
  m_FooterPart.OperationalPattern = OPAtomUL;

  m_FooterPart.EssenceContainers = m_HeaderPart.EssenceContainers;
  m_FooterPart.FooterPartition = here;
  m_FooterPart.ThisPartition = here;

  Result_t result = m_FooterPart.WriteToFile(m_File, m_FramesWritten);

  if ( ASDCP_SUCCESS(result) )
    result = m_RIP.WriteToFile(m_File);

  if ( ASDCP_SUCCESS(result) )
    result = m_File.Seek(0);

  if ( ASDCP_SUCCESS(result) )
    result = m_HeaderPart.WriteToFile(m_File, m_HeaderSize);

  m_File.Close();
  return result;
}


//------------------------------------------------------------------------------------------
//


// standard method of writing a plaintext or encrypted frame
Result_t
ASDCP::Write_EKLV_Packet(Kumu::FileWriter& File, const ASDCP::Dictionary& Dict, const MXF::OP1aHeader& HeaderPart,
			 const ASDCP::WriterInfo& Info, ASDCP::FrameBuffer& CtFrameBuf, ui32_t& FramesWritten,
			 ui64_t & StreamOffset, const ASDCP::FrameBuffer& FrameBuf, const byte_t* EssenceUL,
			 AESEncContext* Ctx, HMACContext* HMAC)
{
  Result_t result = RESULT_OK;
  IntegrityPack IntPack;

  byte_t overhead[128];
  Kumu::MemIOWriter Overhead(overhead, 128);

  if ( FrameBuf.Size() == 0 )
    {
      DefaultLogSink().Error("Cannot write empty frame buffer\n");
      return RESULT_EMPTY_FB;
    }

  if ( Info.EncryptedEssence )
    {
      if ( ! Ctx )
	return RESULT_CRYPT_CTX;

      if ( Info.UsesHMAC && ! HMAC )
	return RESULT_HMAC_CTX;

      if ( FrameBuf.PlaintextOffset() > FrameBuf.Size() )
	return RESULT_LARGE_PTO;

      // encrypt the essence data (create encrypted source value)
      result = EncryptFrameBuffer(FrameBuf, CtFrameBuf, Ctx);

      // create HMAC
      if ( ASDCP_SUCCESS(result) && Info.UsesHMAC )
      	result = IntPack.CalcValues(CtFrameBuf, Info.AssetUUID, FramesWritten + 1, HMAC);

      if ( ASDCP_SUCCESS(result) )
	{ // write UL
	  Overhead.WriteRaw(Dict.ul(MDD_CryptEssence), SMPTE_UL_LENGTH);

	  // construct encrypted triplet header
	  ui32_t ETLength = klv_cryptinfo_size + CtFrameBuf.Size();
	  ui32_t BER_length = MXF_BER_LENGTH;

	  if ( Info.UsesHMAC )
	    ETLength += klv_intpack_size;
	  else
	    ETLength += (MXF_BER_LENGTH * 3); // for empty intpack

	  if ( ETLength > 0x00ffffff ) // Need BER integer longer than MXF_BER_LENGTH bytes
	    {
	      BER_length = Kumu::get_BER_length_for_value(ETLength);

	      // the packet is longer by the difference in expected vs. actual BER length
	      ETLength += BER_length - MXF_BER_LENGTH;

	      if ( BER_length == 0 )
		result = RESULT_KLV_CODING;
	    }

	  if ( ASDCP_SUCCESS(result) )
	    {
	      if ( ! ( Overhead.WriteBER(ETLength, BER_length)                      // write encrypted triplet length
		       && Overhead.WriteBER(UUIDlen, MXF_BER_LENGTH)                // write ContextID length
		       && Overhead.WriteRaw(Info.ContextID, UUIDlen)              // write ContextID
		       && Overhead.WriteBER(sizeof(ui64_t), MXF_BER_LENGTH)         // write PlaintextOffset length
		       && Overhead.WriteUi64BE(FrameBuf.PlaintextOffset())          // write PlaintextOffset
		       && Overhead.WriteBER(SMPTE_UL_LENGTH, MXF_BER_LENGTH)        // write essence UL length
		       && Overhead.WriteRaw((byte_t*)EssenceUL, SMPTE_UL_LENGTH)    // write the essence UL
		       && Overhead.WriteBER(sizeof(ui64_t), MXF_BER_LENGTH)         // write SourceLength length
		       && Overhead.WriteUi64BE(FrameBuf.Size())                     // write SourceLength
		       && Overhead.WriteBER(CtFrameBuf.Size(), BER_length) ) )    // write ESV length
		{
		  result = RESULT_KLV_CODING;
		}
	    }

	  if ( ASDCP_SUCCESS(result) )
	    result = File.Writev(Overhead.Data(), Overhead.Length());
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  StreamOffset += Overhead.Length();
	  // write encrypted source value
	  result = File.Writev((byte_t*)CtFrameBuf.RoData(), CtFrameBuf.Size());
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  StreamOffset += CtFrameBuf.Size();

	  byte_t hmoverhead[512];
	  Kumu::MemIOWriter HMACOverhead(hmoverhead, 512);

	  // write the HMAC
	  if ( Info.UsesHMAC )
	    {
	      HMACOverhead.WriteRaw(IntPack.Data, klv_intpack_size);
	    }
	  else
	    { // we still need the var-pack length values if the intpack is empty
	      for ( ui32_t i = 0; i < 3 ; i++ )
		HMACOverhead.WriteBER(0, MXF_BER_LENGTH);
	    }

	  // write HMAC
	  result = File.Writev(HMACOverhead.Data(), HMACOverhead.Length());
	  StreamOffset += HMACOverhead.Length();
	}
    }
  else
    {
      ui32_t BER_length = MXF_BER_LENGTH;

      if ( FrameBuf.Size() > 0x00ffffff ) // Need BER integer longer than MXF_BER_LENGTH bytes
	{
	  BER_length = Kumu::get_BER_length_for_value(FrameBuf.Size());

	  if ( BER_length == 0 )
	    result = RESULT_KLV_CODING;
	}

      Overhead.WriteRaw((byte_t*)EssenceUL, SMPTE_UL_LENGTH);
      Overhead.WriteBER(FrameBuf.Size(), BER_length);

      if ( ASDCP_SUCCESS(result) )
	result = File.Writev(Overhead.Data(), Overhead.Length());
 
      if ( ASDCP_SUCCESS(result) )
	result = File.Writev((byte_t*)FrameBuf.RoData(), FrameBuf.Size());

      if ( ASDCP_SUCCESS(result) )
	StreamOffset += Overhead.Length() + FrameBuf.Size();
    }

  if ( ASDCP_SUCCESS(result) )
    result = File.Writev();

  return result;
}

//
// end h__Writer.cpp
//
