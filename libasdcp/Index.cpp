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
/*! \file    Index.cpp
    \version $Id: Index.cpp,v 1.21 2012/02/03 19:49:56 jhurst Exp $
    \brief   MXF index segment objects
*/

#include "MXF.h"
const ui32_t kl_length = ASDCP::SMPTE_UL_LENGTH + ASDCP::MXF_BER_LENGTH;


//
ASDCP::MXF::IndexTableSegment::IndexTableSegment(const Dictionary*& d) :
  InterchangeObject(d), m_Dict(d),
  IndexStartPosition(0), IndexDuration(0), EditUnitByteCount(0),
  IndexSID(129), BodySID(1), SliceCount(0), PosTableCount(0)
{
  assert(m_Dict);
  m_UL = m_Dict->ul(MDD_IndexTableSegment);
}

//
ASDCP::MXF::IndexTableSegment::~IndexTableSegment()
{
}

//
void
ASDCP::MXF::IndexTableSegment::Copy(const IndexTableSegment& rhs)
{
  InterchangeObject::Copy(rhs);
  IndexEditRate = rhs.IndexEditRate;
  IndexStartPosition = rhs.IndexStartPosition;
  IndexDuration = rhs.IndexDuration;
  EditUnitByteCount = rhs.EditUnitByteCount;
  IndexSID = rhs.IndexSID;
  BodySID = rhs.BodySID;
  SliceCount = rhs.SliceCount;
  PosTableCount = rhs.PosTableCount;
  DeltaEntryArray = rhs.DeltaEntryArray;
  IndexEntryArray = rhs.IndexEntryArray;
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(IndexTableSegmentBase, IndexEditRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(IndexTableSegmentBase, IndexStartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(IndexTableSegmentBase, IndexDuration));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(IndexTableSegmentBase, EditUnitByteCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(IndexTableSegmentBase, IndexSID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(EssenceContainerData, BodySID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(IndexTableSegmentBase, SliceCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(IndexTableSegmentBase, PosTableCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(IndexTableSegment, DeltaEntryArray));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(IndexTableSegment, IndexEntryArray));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(IndexTableSegmentBase, IndexEditRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(IndexTableSegmentBase, IndexStartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(IndexTableSegmentBase, IndexDuration));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(IndexTableSegmentBase, EditUnitByteCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(IndexTableSegmentBase, IndexSID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(EssenceContainerData, BodySID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(IndexTableSegmentBase, SliceCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(IndexTableSegmentBase, PosTableCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(IndexTableSegment, DeltaEntryArray));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(IndexTableSegment, IndexEntryArray));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::InitFromBuffer(const byte_t* p, ui32_t l)
{
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return InterchangeObject::WriteToBuffer(Buffer);
}

//
void
ASDCP::MXF::IndexTableSegment::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  IndexEditRate      = %s\n",  IndexEditRate.EncodeString(identbuf, IdentBufferLen));
  fprintf(stream, "  IndexStartPosition = %s\n",  i64sz(IndexStartPosition, identbuf));
  fprintf(stream, "  IndexDuration      = %s\n",  i64sz(IndexDuration, identbuf));
  fprintf(stream, "  EditUnitByteCount  = %u\n",  EditUnitByteCount);
  fprintf(stream, "  IndexSID           = %u\n",  IndexSID);
  fprintf(stream, "  BodySID            = %u\n",  BodySID);
  fprintf(stream, "  SliceCount         = %hu\n", SliceCount);
  fprintf(stream, "  PosTableCount      = %hu\n", PosTableCount);

  fprintf(stream, "  DeltaEntryArray:\n");  DeltaEntryArray.Dump(stream);

  if ( IndexEntryArray.size() < 100 )
    {
      fprintf(stream, "  IndexEntryArray:\n");
      IndexEntryArray.Dump(stream);
    }
  else
    {
      fprintf(stream, "  IndexEntryArray: %zu entries\n", IndexEntryArray.size());
    }
}

//------------------------------------------------------------------------------------------
//

//
const char*
ASDCP::MXF::IndexTableSegment::DeltaEntry::EncodeString(char* str_buf, ui32_t buf_len) const
{
  snprintf(str_buf, buf_len, "%3d %-3hu %-3u", PosTableIndex, Slice, ElementData);
  return str_buf;
}

//
bool
ASDCP::MXF::IndexTableSegment::DeltaEntry::Unarchive(Kumu::MemIOReader* Reader)
{
  if ( ! Reader->ReadUi8((ui8_t*)&PosTableIndex) ) return false;
  if ( ! Reader->ReadUi8(&Slice) ) return false;
  if ( ! Reader->ReadUi32BE(&ElementData) ) return false;
  return true;
}

//
bool
ASDCP::MXF::IndexTableSegment::DeltaEntry::Archive(Kumu::MemIOWriter* Writer) const
{
  if ( ! Writer->WriteUi8((ui8_t)PosTableIndex) ) return false;
  if ( ! Writer->WriteUi8(Slice) ) return false;
  if ( ! Writer->WriteUi32BE(ElementData) ) return false;
  return true;
}

//------------------------------------------------------------------------------------------
//

// Flags:
// Bit 7: Random Access
// Bit 6: Sequence Header
// Bit 5: forward prediction flag
// Bit 4: backward prediction flag 
//   e.g.
//   00== I frame (no prediction)
//   10== P frame(forward prediction from previous  frame)
//   01== B frame (backward prediction from future  frame)
//   11== B frame (forward & backward prediction)
// Bits 0-3: reserved  [RP210 Flags to indicate coding of elements in this  edit unit]

//
const char*
ASDCP::MXF::IndexTableSegment::IndexEntry::EncodeString(char* str_buf, ui32_t buf_len) const
{
  char intbuf[IntBufferLen];
  char txt_flags[6];

  txt_flags[0] = ( (Flags & 0x80) != 0 ) ? 'r' : ' ';
  txt_flags[1] = ( (Flags & 0x40) != 0 ) ? 's' : ' ';
  txt_flags[2] = ( (Flags & 0x20) != 0 ) ? 'f' : ' ';
  txt_flags[3] = ( (Flags & 0x10) != 0 ) ? 'b' : ' ';
  txt_flags[4] = ( (Flags & 0x0f) == 3 ) ? 'B' : ( (Flags & 0x0f) == 2 ) ? 'P' : 'I';
  txt_flags[5] = 0;

  snprintf(str_buf, buf_len, "%3i %-3hu %s %s",
	   TemporalOffset, KeyFrameOffset, txt_flags,
	   i64sz(StreamOffset, intbuf));

  return str_buf;
}

//
bool
ASDCP::MXF::IndexTableSegment::IndexEntry::Unarchive(Kumu::MemIOReader* Reader)
{
  if ( ! Reader->ReadUi8((ui8_t*)&TemporalOffset) ) return false;
  if ( ! Reader->ReadUi8((ui8_t*)&KeyFrameOffset) ) return false;
  if ( ! Reader->ReadUi8(&Flags) ) return false;
  if ( ! Reader->ReadUi64BE(&StreamOffset) ) return false;
  return true;
}

//
bool
ASDCP::MXF::IndexTableSegment::IndexEntry::Archive(Kumu::MemIOWriter* Writer) const
{
  if ( ! Writer->WriteUi8((ui8_t)TemporalOffset) ) return false;
  if ( ! Writer->WriteUi8((ui8_t)KeyFrameOffset) ) return false;
  if ( ! Writer->WriteUi8(Flags) ) return false;
  if ( ! Writer->WriteUi64BE(StreamOffset) ) return false;
  return true;
}


//
// end Index.cpp
//

