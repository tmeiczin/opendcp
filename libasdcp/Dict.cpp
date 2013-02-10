/*
Copyright (c) 2006-2009, John Hurst
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
/*! \file    Dict.cpp
  \version $Id: Dict.cpp,v 1.15 2012/02/08 02:59:21 jhurst Exp $
  \brief   MXF dictionary
*/


#include "KM_mutex.h"
#include "KM_log.h"
#include "KLV.h"
#include "MDD.cpp"

//------------------------------------------------------------------------------------------

// The composite dict is the union of the SMPTE and Interop dicts
//
static ASDCP::Dictionary s_CompositeDict;
static Kumu::Mutex s_CompositeDictLock;
static bool s_CompositeDictInit = false;

//
const ASDCP::Dictionary&
ASDCP::DefaultCompositeDict()
{
  if ( ! s_CompositeDictInit )
    {
      Kumu::AutoMutex AL(s_CompositeDictLock);

      if ( ! s_CompositeDictInit )
	{
	  s_CompositeDict.Init();
	  s_CompositeDictInit = true;
	}
    }

  return s_CompositeDict;
}

//
//
static ASDCP::Dictionary s_InteropDict;
static Kumu::Mutex s_InteropDictLock;
static bool s_InteropDictInit = false;

//
const ASDCP::Dictionary&
ASDCP::DefaultInteropDict()
{
  if ( ! s_InteropDictInit )
    {
      Kumu::AutoMutex AL(s_InteropDictLock);

      if ( ! s_InteropDictInit )
	{
	  s_InteropDict.Init();

	  s_InteropDict.DeleteEntry(MDD_MXFInterop_OPAtom);
	  s_InteropDict.DeleteEntry(MDD_MXFInterop_CryptEssence);
	  s_InteropDict.DeleteEntry(MDD_MXFInterop_GenericDescriptor_SubDescriptors);

	  s_InteropDict.AddEntry(s_MDD_Table[MDD_MXFInterop_OPAtom], MDD_OPAtom);
	  s_InteropDict.AddEntry(s_MDD_Table[MDD_MXFInterop_CryptEssence], MDD_CryptEssence);
	  s_InteropDict.AddEntry(s_MDD_Table[MDD_MXFInterop_GenericDescriptor_SubDescriptors],
				     MDD_GenericDescriptor_SubDescriptors);

	  s_InteropDictInit = true;
	}
    }

  return s_InteropDict;
}

//
//
static ASDCP::Dictionary s_SMPTEDict;
static Kumu::Mutex s_SMPTEDictLock;
static bool s_SMPTEDictInit = false;

//
const ASDCP::Dictionary&
ASDCP::DefaultSMPTEDict()
{
  if ( ! s_SMPTEDictInit )
    {
      Kumu::AutoMutex AL(s_SMPTEDictLock);

      if ( ! s_SMPTEDictInit )
	{
	  s_SMPTEDict.Init();

	  s_SMPTEDict.DeleteEntry(MDD_MXFInterop_OPAtom);
	  s_SMPTEDict.DeleteEntry(MDD_MXFInterop_CryptEssence);
	  s_SMPTEDict.DeleteEntry(MDD_MXFInterop_GenericDescriptor_SubDescriptors);

	  s_SMPTEDictInit = true;
	}
    }

  return s_SMPTEDict;
}

//
const ASDCP::MDDEntry&
ASDCP::MXFInterop_OPAtom_Entry() {
  return s_MDD_Table[MDD_MXFInterop_OPAtom];
}

//
const ASDCP::MDDEntry&
ASDCP::SMPTE_390_OPAtom_Entry() {
  return s_MDD_Table[MDD_OPAtom];
}


//------------------------------------------------------------------------------------------
//

ASDCP::Dictionary::Dictionary() {}
ASDCP::Dictionary::~Dictionary() {}

//
void
ASDCP::Dictionary::Init()
{
  m_md_lookup.clear();
  memset(m_MDD_Table, 0, sizeof(m_MDD_Table));

  for ( ui32_t x = 0; x < (ui32_t)ASDCP::MDD_Max; x++ )
    {
      if ( x == MDD_PartitionMetadata_IndexSID_DEPRECATED  // 30
	   || x == MDD_PartitionMetadata_BodySID_DEPRECATED  // 32
	   || x == MDD_PartitionMetadata_EssenceContainers_DEPRECATED  // 34
	   || x == MDD_IndexTableSegmentBase_IndexSID_DEPRECATED  // 56
	   || x == MDD_IndexTableSegmentBase_BodySID_DEPRECATED  // 57
	   || x == MDD_PartitionArray_RandomIndexMetadata_BodySID_DEPRECATED  // 73
	   || x == MDD_Preface_EssenceContainers_DEPRECATED  // 85
	   || x == MDD_EssenceContainerData_IndexSID_DEPRECATED  // 103
	   || x == MDD_EssenceContainerData_BodySID_DEPRECATED  // 104
	   || x == MDD_DMSegment_DataDefinition_DEPRECATED // 266
	   || x == MDD_DMSegment_Duration_DEPRECATED // 267
	   || x == MDD_PartitionMetadata_OperationalPattern_DEPRECATED  // 33
	   || x == MDD_Preface_OperationalPattern_DEPRECATED  // 84
	   || x == MDD_TimedTextResourceSubDescriptor_EssenceStreamID_DEPRECATED // 264
	   )
	continue;

      AddEntry(s_MDD_Table[x], x);
    }
}

//
bool
ASDCP::Dictionary::AddEntry(const MDDEntry& Entry, ui32_t index)
{
  if ( index >= (ui32_t)MDD_Max )
    {
      Kumu::DefaultLogSink().Warn("UL Dictionary: index exceeds maximum: %d\n", index);
      return false;
    }

  bool result = true;
  // is this index already there?
  std::map<ui32_t, ASDCP::UL>::iterator rii = m_md_rev_lookup.find(index);

  if ( rii != m_md_rev_lookup.end() )
    {
      DeleteEntry(index);
      result = false;
    }

  UL TmpUL(Entry.ul);

#ifdef MDD_AUTHORING_MODE
  char buf[64];
  std::map<ASDCP::UL, ui32_t>::iterator ii = m_md_lookup.find(TmpUL);
  if ( ii != m_md_lookup.end() )
    {
      fprintf(stderr, "DUPE! %s (%02x, %02x) %s | (%02x, %02x) %s\n",
	      TmpUL.EncodeString(buf, 64),
	      m_MDD_Table[ii->second].tag.a, m_MDD_Table[ii->second].tag.b,
	      m_MDD_Table[ii->second].name,
	      Entry.tag.a, Entry.tag.b, Entry.name);
    }
#endif

  m_md_lookup.insert(std::map<UL, ui32_t>::value_type(TmpUL, index));
  m_md_rev_lookup.insert(std::map<ui32_t, UL>::value_type(index, TmpUL));
  m_md_sym_lookup.insert(std::map<std::string, ui32_t>::value_type(Entry.name, index));
  m_MDD_Table[index] = Entry;

  return result;
}

//
bool
ASDCP::Dictionary::DeleteEntry(ui32_t index)
{
  std::map<ui32_t, ASDCP::UL>::iterator rii = m_md_rev_lookup.find(index);
  if ( rii != m_md_rev_lookup.end() )
    {
      std::map<ASDCP::UL, ui32_t>::iterator ii = m_md_lookup.find(rii->second);
      assert(ii != m_md_lookup.end());

      MDDEntry NilEntry;
      memset(&NilEntry, 0, sizeof(NilEntry));

      m_md_lookup.erase(ii);
      m_md_rev_lookup.erase(rii);
      m_MDD_Table[index] = NilEntry;
      return true;
    }

  return false;
}

//
const ASDCP::MDDEntry&
ASDCP::Dictionary::Type(MDD_t type_id) const
{
  assert(m_MDD_Table[0].name[0]);
  std::map<ui32_t, ASDCP::UL>::const_iterator rii = m_md_rev_lookup.find(type_id);

  if ( rii == m_md_rev_lookup.end() )
    Kumu::DefaultLogSink().Warn("UL Dictionary: unknown UL type_id: %d\n", type_id);

  return m_MDD_Table[type_id];
}

//
const ASDCP::MDDEntry*
ASDCP::Dictionary::FindUL(const byte_t* ul_buf) const
{
  assert(m_MDD_Table[0].name[0]);
  std::map<UL, ui32_t>::const_iterator i = m_md_lookup.find(UL(ul_buf));
  
  if ( i == m_md_lookup.end() )
    {
      byte_t tmp_ul[SMPTE_UL_LENGTH];
      memcpy(tmp_ul, ul_buf, SMPTE_UL_LENGTH);
      tmp_ul[SMPTE_UL_LENGTH-1] = 0;

      i = m_md_lookup.find(UL(tmp_ul));

      if ( i == m_md_lookup.end() )
	{
	  char buf[64];
	  UL TmpUL(ul_buf);
	  Kumu::DefaultLogSink().Warn("UL Dictionary: unknown UL: %s\n", TmpUL.EncodeString(buf, 64));
	  return 0;
	}
    }

  return &m_MDD_Table[i->second];
}

//
const ASDCP::MDDEntry*
ASDCP::Dictionary::FindSymbol(const std::string& str) const
{
  assert(m_MDD_Table[0].name[0]);
  std::map<std::string, ui32_t>::const_iterator i = m_md_sym_lookup.find(str);
  
  if ( i == m_md_sym_lookup.end() )
    {
      Kumu::DefaultLogSink().Warn("UL Dictionary: unknown symbol: %s\n", str.c_str());
      return 0;
    }

  return &m_MDD_Table[i->second];
}

//
void
ASDCP::Dictionary::Dump(FILE* stream) const
{
  if ( stream == 0 )
    stream = stderr;

  MDD_t di = (MDD_t)0;
  char     str_buf[64];

  while ( di < MDD_Max )
    {
      if ( m_MDD_Table[di].name != 0 )
	{
	  UL TmpUL(m_MDD_Table[di].ul);
	  fprintf(stream, "%s: %s\n", TmpUL.EncodeString(str_buf, 64), m_MDD_Table[di].name);
	}

      di = (MDD_t)(di + 1);
    }
}

//
// end Dict.cpp
//
