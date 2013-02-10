/*
Copyright (c) 2007-2009, John Hurst
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
/*! \file    AS_DCP_TimedText.cpp
    \version $Id: TimedText_Parser.cpp,v 1.15 2010/11/15 17:04:13 jhurst Exp $       
    \brief   AS-DCP library, PCM essence reader and writer implementation
*/


#include "AS_DCP_internal.h"
#include "S12MTimecode.h"
#include "KM_xml.h"

using namespace Kumu;
using namespace ASDCP;

using Kumu::DefaultLogSink;

const char* c_dcst_namespace_name = "http://www.smpte-ra.org/schemas/428-7/2007/DCST";

//------------------------------------------------------------------------------------------



class FilenameResolver : public ASDCP::TimedText::IResourceResolver
{
  std::string m_Dirname;

  FilenameResolver();
  bool operator==(const FilenameResolver&);

public:
  FilenameResolver(const std::string& dirname)
  {
    if ( PathIsDirectory(dirname) )
      {
	m_Dirname = dirname;
	return;
      }

    DefaultLogSink().Error("Path '%s' is not a directory, defaulting to '.'\n", dirname.c_str());
    m_Dirname = ".";
  }

  //
  Result_t ResolveRID(const byte_t* uuid, TimedText::FrameBuffer& FrameBuf) const
  {
    FileReader Reader;
    char buf[64];
    UUID RID(uuid);
    std::string filename = m_Dirname + "/" + RID.EncodeHex(buf, 64);
    DefaultLogSink().Debug("retrieving resource %s from file %s\n", buf, filename.c_str());

    Result_t result = Reader.OpenRead(filename.c_str());

    if ( KM_SUCCESS(result) )
      {
	ui32_t read_count, read_size = Reader.Size();

	result = FrameBuf.Capacity(read_size);

	if ( KM_SUCCESS(result) )
	  result = Reader.Read(FrameBuf.Data(), read_size, &read_count);

	if ( KM_SUCCESS(result) )
	  FrameBuf.Size(read_count);
      }

    return result;
  }
};

//------------------------------------------------------------------------------------------

typedef std::map<Kumu::UUID, TimedText::MIMEType_t> ResourceTypeMap_t;

class ASDCP::TimedText::DCSubtitleParser::h__SubtitleParser
{
  XMLElement  m_Root;
  ResourceTypeMap_t m_ResourceTypes;
  Result_t OpenRead();

  ASDCP_NO_COPY_CONSTRUCT(h__SubtitleParser);

public:
  std::string m_Filename;
  std::string m_XMLDoc;
  TimedTextDescriptor  m_TDesc;
  mem_ptr<FilenameResolver> m_DefaultResolver;

  h__SubtitleParser() : m_Root("**ParserRoot**")
  {
    memset(&m_TDesc.AssetID, 0, UUIDlen);
  }

  ~h__SubtitleParser() {}

  TimedText::IResourceResolver* GetDefaultResolver()
  {
    if ( m_DefaultResolver.empty() )
      m_DefaultResolver = new FilenameResolver(PathDirname(m_Filename));
    
    return m_DefaultResolver;
  }

  Result_t OpenRead(const char* filename);
  Result_t OpenRead(const std::string& xml_doc, const char* filename);
  Result_t ReadAncillaryResource(const byte_t* uuid, FrameBuffer& FrameBuf, const IResourceResolver& Resolver) const;
};

//
bool
get_UUID_from_element(XMLElement* Element, UUID& ID)
{
  assert(Element);
  const char* p = Element->GetBody().c_str();
  if ( strncmp(p, "urn:uuid:", 9) == 0 )    p += 9;
  return ID.DecodeHex(p);
}

//
bool
get_UUID_from_child_element(const char* name, XMLElement* Parent, UUID& outID)
{
  assert(name); assert(Parent);
  XMLElement* Child = Parent->GetChildWithName(name);
  if ( Child == 0 )    return false;
  return get_UUID_from_element(Child, outID);
}

//
static ASDCP::Rational
decode_rational(const char* str_rat)
{
  assert(str_rat);
  ui32_t Num = atoi(str_rat);
  ui32_t Den = 0;

  const char* den_str = strrchr(str_rat, ' ');
  if ( den_str != 0 )
    Den = atoi(den_str+1);

  return ASDCP::Rational(Num, Den);
}

//
Result_t
ASDCP::TimedText::DCSubtitleParser::h__SubtitleParser::OpenRead(const char* filename)
{
  Result_t result = ReadFileIntoString(filename, m_XMLDoc);

  if ( KM_SUCCESS(result) )
    result = OpenRead();

  m_Filename = filename;
  return result;
}

//
Result_t
ASDCP::TimedText::DCSubtitleParser::h__SubtitleParser::OpenRead(const std::string& xml_doc, const char* filename)
{
  m_XMLDoc = xml_doc;

  if ( filename != 0 )
    m_Filename = filename;
  else
    m_Filename = "<string>";

  return OpenRead();
}

//
Result_t
ASDCP::TimedText::DCSubtitleParser::h__SubtitleParser::OpenRead()
{
  if ( ! m_Root.ParseString(m_XMLDoc.c_str()) )
    return RESULT_FORMAT;

  m_TDesc.EncodingName = "UTF-8"; // the XML parser demands UTF-8
  m_TDesc.ResourceList.clear();
  m_TDesc.ContainerDuration = 0;
  const XMLNamespace* ns = m_Root.Namespace();

  if ( ns == 0 )
    {
      DefaultLogSink(). Warn("Document has no namespace name, assuming %s\n", c_dcst_namespace_name);
      m_TDesc.NamespaceName = c_dcst_namespace_name;
    }
  else
    {
      m_TDesc.NamespaceName = ns->Name();
    }

  UUID DocID;
  if ( ! get_UUID_from_child_element("Id", &m_Root, DocID) )
    {
      DefaultLogSink(). Error("Id element missing from input document\n");
      return RESULT_FORMAT;
    }

  memcpy(m_TDesc.AssetID, DocID.Value(), DocID.Size());
  XMLElement* EditRate = m_Root.GetChildWithName("EditRate");

  if ( EditRate == 0 )
    {
      DefaultLogSink(). Error("EditRate element missing from input document\n");
      return RESULT_FORMAT;
    }

  m_TDesc.EditRate = decode_rational(EditRate->GetBody().c_str());

  if ( m_TDesc.EditRate != EditRate_23_98
       && m_TDesc.EditRate != EditRate_24
       && m_TDesc.EditRate != EditRate_25
       && m_TDesc.EditRate != EditRate_30
       && m_TDesc.EditRate != EditRate_48
       && m_TDesc.EditRate != EditRate_50
       && m_TDesc.EditRate != EditRate_60 )
    {
      DefaultLogSink(). Error("Unexpected EditRate: %d/%d\n",
			      m_TDesc.EditRate.Numerator, m_TDesc.EditRate.Denominator);
      return RESULT_FORMAT;
    }

  // list of fonts
  ElementList FontList;
  m_Root.GetChildrenWithName("LoadFont", FontList);

  for ( Elem_i i = FontList.begin(); i != FontList.end(); i++ )
    {
      UUID AssetID;
      if ( ! get_UUID_from_element(*i, AssetID) )
	{
	  DefaultLogSink(). Error("LoadFont element does not contain a urn:uuid value as expected.\n");
	  return RESULT_FORMAT;
	}

      TimedTextResourceDescriptor TmpResource;
      memcpy(TmpResource.ResourceID, AssetID.Value(), UUIDlen);
      TmpResource.Type = MT_OPENTYPE;
      m_TDesc.ResourceList.push_back(TmpResource);
      m_ResourceTypes.insert(ResourceTypeMap_t::value_type(UUID(TmpResource.ResourceID), MT_OPENTYPE));
    }

  // list of images
  ElementList ImageList;
  m_Root.GetChildrenWithName("Image", ImageList);

  for ( Elem_i i = ImageList.begin(); i != ImageList.end(); i++ )
    {
      UUID AssetID;
      if ( ! get_UUID_from_element(*i, AssetID) )
	{
	  DefaultLogSink(). Error("Image element does not contain a urn:uuid value as expected.\n");
	  return RESULT_FORMAT;
	}

      TimedTextResourceDescriptor TmpResource;
      memcpy(TmpResource.ResourceID, AssetID.Value(), UUIDlen);
      TmpResource.Type = MT_PNG;
      m_TDesc.ResourceList.push_back(TmpResource);
      m_ResourceTypes.insert(ResourceTypeMap_t::value_type(UUID(TmpResource.ResourceID), MT_PNG));
    }

  // Calculate the timeline duration.
  // This is a little ugly because the last element in the file is not necessarily
  // the last instance to be displayed, e.g., element n and element n-1 may have the
  // same start time but n-1 may have a greater duration making it the last to be seen.
  // We must scan the list to accumulate the latest TimeOut value.
  ElementList InstanceList;
  ElementList::const_iterator ei;
  ui32_t end_count = 0;
  
  m_Root.GetChildrenWithName("Subtitle", InstanceList);

  if ( InstanceList.empty() )
    {
      DefaultLogSink(). Error("XML document contains no Subtitle elements.\n");
      return RESULT_FORMAT;
    }

  // assumes edit rate is constrained above
  ui32_t TCFrameRate = ( m_TDesc.EditRate == EditRate_23_98  ) ? 24 : m_TDesc.EditRate.Numerator;

  S12MTimecode beginTC;
  beginTC.SetFPS(TCFrameRate);
  XMLElement* StartTime = m_Root.GetChildWithName("StartTime");

  if ( StartTime != 0 )
    beginTC.DecodeString(StartTime->GetBody());

  for ( ei = InstanceList.begin(); ei != InstanceList.end(); ei++ )
    {
      S12MTimecode tmpTC((*ei)->GetAttrWithName("TimeOut"), TCFrameRate);
      if ( end_count < tmpTC.GetFrames() )
	end_count = tmpTC.GetFrames();
    }

  if ( end_count <= beginTC.GetFrames() )
    {
      DefaultLogSink(). Error("Timed Text file has zero-length timeline.\n");
      return RESULT_FORMAT;
    }

  m_TDesc.ContainerDuration = end_count - beginTC.GetFrames();

  return RESULT_OK;
}


//
Result_t
ASDCP::TimedText::DCSubtitleParser::h__SubtitleParser::ReadAncillaryResource(const byte_t* uuid, FrameBuffer& FrameBuf,
									     const IResourceResolver& Resolver) const
{
  FrameBuf.AssetID(uuid);
  UUID TmpID(uuid);
  char buf[64];

  ResourceTypeMap_t::const_iterator rmi = m_ResourceTypes.find(TmpID);

  if ( rmi == m_ResourceTypes.end() )
    {
      DefaultLogSink().Error("Unknown ancillary resource id: %s\n", TmpID.EncodeHex(buf, 64));
      return RESULT_RANGE;
    }

  Result_t result = Resolver.ResolveRID(uuid, FrameBuf);

  if ( KM_SUCCESS(result) )
    {
      if ( (*rmi).second == MT_PNG )
	FrameBuf.MIMEType("image/png");
	      
      else if ( (*rmi).second == MT_OPENTYPE )
	FrameBuf.MIMEType("application/x-font-opentype");

      else
	FrameBuf.MIMEType("application/octet-stream");
    }

  return result;
}

//------------------------------------------------------------------------------------------

ASDCP::TimedText::DCSubtitleParser::DCSubtitleParser()
{
}

ASDCP::TimedText::DCSubtitleParser::~DCSubtitleParser()
{
}

// Opens the stream for reading, parses enough data to provide a complete
// set of stream metadata for the MXFWriter below.
ASDCP::Result_t
ASDCP::TimedText::DCSubtitleParser::OpenRead(const char* filename) const
{
  const_cast<ASDCP::TimedText::DCSubtitleParser*>(this)->m_Parser = new h__SubtitleParser;

  Result_t result = m_Parser->OpenRead(filename);

  if ( ASDCP_FAILURE(result) )
    const_cast<ASDCP::TimedText::DCSubtitleParser*>(this)->m_Parser = 0;

  return result;
}

// Parses an XML document to provide a complete set of stream metadata for the MXFWriter below.
Result_t
ASDCP::TimedText::DCSubtitleParser::OpenRead(const std::string& xml_doc, const char* filename) const
{
  const_cast<ASDCP::TimedText::DCSubtitleParser*>(this)->m_Parser = new h__SubtitleParser;

  Result_t result = m_Parser->OpenRead(xml_doc, filename);

  if ( ASDCP_FAILURE(result) )
    const_cast<ASDCP::TimedText::DCSubtitleParser*>(this)->m_Parser = 0;

  return result;
}

//
ASDCP::Result_t
ASDCP::TimedText::DCSubtitleParser::FillTimedTextDescriptor(TimedTextDescriptor& TDesc) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  TDesc = m_Parser->m_TDesc;
  return RESULT_OK;
}

// Reads the complete Timed Text Resource into the given string.
ASDCP::Result_t
ASDCP::TimedText::DCSubtitleParser::ReadTimedTextResource(std::string& s) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  s = m_Parser->m_XMLDoc;
  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::TimedText::DCSubtitleParser::ReadAncillaryResource(const byte_t* uuid, FrameBuffer& FrameBuf,
							  const IResourceResolver* Resolver) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  if ( Resolver == 0 )
    Resolver = m_Parser->GetDefaultResolver();

  return m_Parser->ReadAncillaryResource(uuid, FrameBuf, *Resolver);
}


//
// end AS_DCP_timedText.cpp
//
