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
/*! \file    KM_xml.cpp
    \version $Id: KM_xml.cpp,v 1.28 2015/08/25 19:03:38 jhurst Exp $
    \brief   XML writer
*/

#include <KM_xml.h>
#include <KM_log.h>
#include <KM_mutex.h>
#include <stack>
#include <map>

#ifdef HAVE_EXPAT
# ifdef HAVE_XERCES_C
#  error "Both HAVE_EXPAT and HAVE_XERCES_C defined"
# endif
#include <expat.h>
#endif

#ifdef HAVE_XERCES_C
# ifdef HAVE_EXPAT
#  error "Both HAVE_EXPAT and HAVE_XERCES_C defined"
# endif

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/TransService.hpp>
#include <xercesc/sax/AttributeList.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/XMLPScanToken.hpp>


XERCES_CPP_NAMESPACE_USE 

extern "C"
{
  void kumu_init_xml_dom();
  bool kumu_UTF_8_to_XercesString(const std::string& in_str, std::basic_string<XMLCh>& out_str);
  bool kumu_UTF_8_to_XercesString_p(const char* in_str, std::basic_string<XMLCh>& out_str);
  bool kumu_XercesString_to_UTF_8(const std::basic_string<XMLCh>& in_str, std::string& out_str);
  bool kumu_XercesString_to_UTF_8_p(const XMLCh* in_str, std::string& out_str);
}

#endif

using namespace Kumu;


class ns_map : public std::map<std::string, XMLNamespace*>
{
public:
  ~ns_map()
  {
    while ( ! empty() )
      {
	ns_map::iterator ni = begin();
	delete ni->second;
	erase(ni);
      }
  }
};


Kumu::XMLElement::XMLElement(const char* name) : m_Namespace(0), m_NamespaceOwner(0)
{
  m_Name = name;
}

Kumu::XMLElement::~XMLElement()
{
  for ( Elem_i i = m_ChildList.begin(); i != m_ChildList.end(); i++ )
    delete *i;

  delete (ns_map*)m_NamespaceOwner;
}

//
void
Kumu::XMLElement::SetAttr(const char* name, const char* value)
{
  NVPair TmpVal;
  TmpVal.name = name;
  TmpVal.value = value;

  m_AttrList.push_back(TmpVal);
}

//
Kumu::XMLElement*
Kumu::XMLElement::AddChild(Kumu::XMLElement* element)
{
  m_ChildList.push_back(element); // takes posession!
  return element;
}

//
Kumu::XMLElement*
Kumu::XMLElement::AddChild(const char* name)
{
  XMLElement* tmpE = new XMLElement(name);
  m_ChildList.push_back(tmpE);
  return tmpE;
}

//
Kumu::XMLElement*
Kumu::XMLElement::AddChildWithContent(const char* name, const std::string& value)
{
  return AddChildWithContent(name, value.c_str());
}

//
void
Kumu::XMLElement::AppendBody(const std::string& value)
{
  m_Body += value;
}

//
void
Kumu::XMLElement::SetBody(const std::string& value)
{
  m_Body = value;
}

//
Kumu::XMLElement*
Kumu::XMLElement::AddChildWithContent(const char* name, const char* value)
{
  assert(name);
  assert(value);
  XMLElement* tmpE = new XMLElement(name);
  tmpE->m_Body = value;
  m_ChildList.push_back(tmpE);
  return tmpE;
}

//
Kumu::XMLElement*
Kumu::XMLElement::AddChildWithPrefixedContent(const char* name, const char* prefix, const char* value)
{
  XMLElement* tmpE = new XMLElement(name);
  tmpE->m_Body = prefix;
  tmpE->m_Body += value;
  m_ChildList.push_back(tmpE);
  return tmpE;
}

//
void
Kumu::XMLElement::AddComment(const char* value)
{
  m_Body += "  <!-- ";
  m_Body += value;
  m_Body += " -->\n";
}

//
void
Kumu::XMLElement::Render(std::string& outbuf, const bool& pretty) const
{
  outbuf = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  RenderElement(outbuf, 0, pretty);
}

//
inline void
add_spacer(std::string& outbuf, i32_t depth)
{
  while ( depth-- )
    outbuf+= "  ";
}

//
void
Kumu::XMLElement::RenderElement(std::string& outbuf, const ui32_t& depth, const bool& pretty) const
{
  if ( pretty )
    {
      add_spacer(outbuf, depth);
    }

  outbuf += "<";
  outbuf += m_Name;

  // render attributes
  for ( Attr_i i = m_AttrList.begin(); i != m_AttrList.end(); ++i )
    {
      outbuf += " ";
      outbuf += (*i).name;
      outbuf += "=\"";
      outbuf += (*i).value;
      outbuf += "\"";
    }

  outbuf += ">";

  // body contents and children
  if ( ! m_ChildList.empty() )
    {
      outbuf += "\n";

      // render body
      if ( m_Body.length() > 0 )
	{
	  outbuf += m_Body;
	}

      for ( Elem_i i = m_ChildList.begin(); i != m_ChildList.end(); ++i )
	{
	  (*i)->RenderElement(outbuf, depth + 1, pretty);
	}

      if ( pretty )
	{
	  add_spacer(outbuf, depth);
	}
    }
  else if ( m_Body.length() > 0 )
    {
      outbuf += m_Body;
    }

  outbuf += "</";
  outbuf += m_Name;
  outbuf += ">\n";
}

//
bool
Kumu::XMLElement::HasName(const char* name) const
{
  if ( name == 0 || *name == 0 )
    return false;

  return (m_Name == name);
}


void
Kumu::XMLElement::SetName(const char* name)
{
  if ( name != 0)
    m_Name = name;
}

//
const char*
Kumu::XMLElement::GetAttrWithName(const char* name) const
{
  for ( Attr_i i = m_AttrList.begin(); i != m_AttrList.end(); i++ )
    {
      if ( (*i).name == name )
	return (*i).value.c_str();
    }

  return 0;
}

//
Kumu::XMLElement*
Kumu::XMLElement::GetChildWithName(const char* name) const
{
  for ( Elem_i i = m_ChildList.begin(); i != m_ChildList.end(); i++ )
    {
      if ( (*i)->HasName(name) )
	return *i;
    }

  return 0;
}

//
const Kumu::ElementList&
Kumu::XMLElement::GetChildrenWithName(const char* name, ElementList& outList) const
{
  assert(name);
  for ( Elem_i i = m_ChildList.begin(); i != m_ChildList.end(); i++ )
    {
      if ( (*i)->HasName(name) )
	outList.push_back(*i);

      if ( ! (*i)->m_ChildList.empty() )
	(*i)->GetChildrenWithName(name, outList);
    }

  return outList;
}

//
void
Kumu::XMLElement::DeleteAttributes()
{
  m_AttrList.clear();
}

//
void
Kumu::XMLElement::DeleteAttrWithName(const char* name)
{
  assert(name);
  AttributeList::iterator i = m_AttrList.begin();

  while ( i != m_AttrList.end() )
    {
      if ( i->name == std::string(name) )
	m_AttrList.erase(i++);
      else
	++i;
    }
}

//
void
Kumu::XMLElement::DeleteChildren()
{
  while ( ! m_ChildList.empty() )
    {
      delete m_ChildList.back();
      m_ChildList.pop_back();
    }
}

//
void
Kumu::XMLElement::DeleteChild(const XMLElement* element)
{
  if ( element != 0 )
    {
      for ( ElementList::iterator i = m_ChildList.begin(); i != m_ChildList.end(); i++ )
	{
	  if ( *i == element )
	    {
	      delete *i;
	      m_ChildList.erase(i);
	      return;
	    }
	}
    }
}

//
void
Kumu::XMLElement::ForgetChild(const XMLElement* element)
{
  if ( element != 0 )
    {
      for ( ElementList::iterator i = m_ChildList.begin(); i != m_ChildList.end(); i++ )
	{
	  if ( *i == element )
	    {
	      m_ChildList.erase(i);
	      return;
	    }
	}
    }
}

//
bool
Kumu::XMLElement::ParseString(const ByteString& document)
{
  return ParseString((const char*)document.RoData(), document.Length());
}

//
bool
Kumu::XMLElement::ParseString(const std::string& document)
{
  return ParseString(document.c_str(), document.size());
}

//
bool
Kumu::XMLElement::ParseFirstFromString(const ByteString& document)
{
  return ParseFirstFromString((const char*)document.RoData(), document.Length());
}

//
bool
Kumu::XMLElement::ParseFirstFromString(const std::string& document)
{
  return ParseFirstFromString(document.c_str(), document.size());
}


//----------------------------------------------------------------------------------------------------

#ifdef HAVE_EXPAT


class ExpatParseContext
{
  KM_NO_COPY_CONSTRUCT(ExpatParseContext);
  ExpatParseContext();
public:
  ns_map*                  Namespaces;
  std::stack<XMLElement*>  Scope;
  XMLElement*              Root;

  ExpatParseContext(XMLElement* root) : Root(root) {
    Namespaces = new ns_map;
    assert(Root);
  }

  ~ExpatParseContext() {}
};

// expat wrapper functions
// 
static void
xph_start(void* p, const XML_Char* name, const XML_Char** attrs)
{
  assert(p);  assert(name);  assert(attrs);
  ExpatParseContext* Ctx = (ExpatParseContext*)p;
  XMLElement* Element;

  const char* ns_root = name;
  const char* local_name = strchr(name, '|');
  if ( local_name != 0 )
    name = local_name + 1;

  if ( Ctx->Scope.empty() )
    {
      Ctx->Scope.push(Ctx->Root);
    }
  else
    {
      Element = Ctx->Scope.top();
      Ctx->Scope.push(Element->AddChild(name));
    }

  Element = Ctx->Scope.top();
  Element->SetName(name);

  // map the namespace
  std::string key;
  if ( ns_root != name )
    key.assign(ns_root, name - ns_root - 1);
  
  ns_map::iterator ni = Ctx->Namespaces->find(key);
  if ( ni != Ctx->Namespaces->end() )
    Element->SetNamespace(ni->second);

  // set attributes
  for ( int i = 0; attrs[i] != 0; i += 2 )
    {
      if ( ( local_name = strchr(attrs[i], '|') ) == 0 )
	local_name = attrs[i];
      else
	local_name++;

      Element->SetAttr(local_name, attrs[i+1]);
    }
}

//
static void
xph_end(void* p, const XML_Char* name)
{
  assert(p);  assert(name);
  ExpatParseContext* Ctx = (ExpatParseContext*)p;
  Ctx->Scope.pop();
}

//
static void
xph_char(void* p, const XML_Char* data, int len)
{
  assert(p);  assert(data);
  ExpatParseContext* Ctx = (ExpatParseContext*)p;

  if ( len > 0 )
    {
      std::string tmp_str;
      tmp_str.assign(data, len);
      Ctx->Scope.top()->AppendBody(tmp_str);
    }
}

//
void
xph_namespace_start(void* p, const XML_Char* ns_prefix, const XML_Char* ns_name)
{
  assert(p);  assert(ns_name);
  ExpatParseContext* Ctx = (ExpatParseContext*)p;
  
  if ( ns_prefix == 0 )
    ns_prefix = "";

  ns_map::iterator ni = Ctx->Namespaces->find(ns_name);

  if  ( ni != Ctx->Namespaces->end() )
    {
      if ( ni->second->Name() != std::string(ns_name) )
	{
	  DefaultLogSink().Error("Duplicate prefix: %s\n", ns_prefix);
	  return;
	}
    }
  else
    {
      XMLNamespace* Namespace = new XMLNamespace(ns_prefix, ns_name);
      Ctx->Namespaces->insert(ns_map::value_type(ns_name, Namespace));
    }
}

//
bool
Kumu::XMLElement::ParseString(const char* document, ui32_t doc_len)
{
  if ( doc_len == 0 )
    {
      return false;
    }

  XML_Parser Parser = XML_ParserCreateNS("UTF-8", '|');

  if ( Parser == 0 )
    {
      DefaultLogSink().Error("Error allocating memory for XML parser.\n");
      return false;
    }

  ExpatParseContext Ctx(this);
  XML_SetUserData(Parser, (void*)&Ctx);
  XML_SetElementHandler(Parser, xph_start, xph_end);
  XML_SetCharacterDataHandler(Parser, xph_char);
  XML_SetStartNamespaceDeclHandler(Parser, xph_namespace_start);

  if ( ! XML_Parse(Parser, document, doc_len, 1) )
    {
      DefaultLogSink().Error("XML Parse error on line %d: %s\n",
			     XML_GetCurrentLineNumber(Parser),
			     XML_ErrorString(XML_GetErrorCode(Parser)));
      XML_ParserFree(Parser);
      return false;
    }

  XML_ParserFree(Parser);

  if ( ! Ctx.Namespaces->empty() )
    {
      m_NamespaceOwner = (void*)Ctx.Namespaces;
    }

  return true;
}

// expat wrapper functions
// 
static void
xph_start_one_shot(void* p, const XML_Char* name, const XML_Char** attrs)
{
  xph_start(p, name, attrs);
  XML_Parser parser = (XML_Parser)p;
  XML_StopParser(parser, false);
}

//
bool
Kumu::XMLElement::ParseFirstFromString(const char* document, ui32_t doc_len)
{
  if ( doc_len == 0 )
    {
      return false;
    }

  XML_Parser Parser = XML_ParserCreateNS("UTF-8", '|');

  if ( Parser == 0 )
    {
      DefaultLogSink().Error("Error allocating memory for XML parser.\n");
      return false;
    }

  ExpatParseContext Ctx(this);
  XML_SetUserData(Parser, (void*)&Ctx);
  XML_SetElementHandler(Parser, xph_start_one_shot, xph_end);
  XML_SetCharacterDataHandler(Parser, xph_char);
  XML_SetStartNamespaceDeclHandler(Parser, xph_namespace_start);

  if ( ! XML_Parse(Parser, document, doc_len, 1) )
    {
      DefaultLogSink().Error("XML Parse error on line %d: %s\n",
			     XML_GetCurrentLineNumber(Parser),
			     XML_ErrorString(XML_GetErrorCode(Parser)));
      XML_ParserFree(Parser);
      return false;
    }

  XML_ParserFree(Parser);

  if ( ! Ctx.Namespaces->empty() )
    {
      m_NamespaceOwner = (void*)Ctx.Namespaces;
    }

  return true;
}



#endif

//----------------------------------------------------------------------------------------------------

#ifdef HAVE_XERCES_C

static Mutex sg_xerces_init_lock; // protect the xerces initialized
static bool  sg_xml_init = false; // signal initialization
static Mutex sg_coder_lock;       // protect the transcoder context 
static XMLTranscoder*   sg_coder = 0;
static const int sg_coder_buf_len = 128 * 1024;
static char sg_coder_buf[sg_coder_buf_len + 8];
static unsigned char sg_coder_counts[sg_coder_buf_len / sizeof(XMLCh)]; // see XMLTranscoder::transcodeFrom

static const XMLCh sg_LS[] = { chLatin_L, chLatin_S, chNull };
static const XMLCh sg_label_UTF_8[] = { chLatin_U, chLatin_T, chLatin_F,
					chDash, chDigit_8, chNull}; 

//
void
kumu_init_xml_dom()
{
  if ( ! sg_xml_init )
    {
      AutoMutex AL(sg_xerces_init_lock);

      if ( ! sg_xml_init )
	{
	  try
	    {
	      XMLPlatformUtils::Initialize();
	      sg_xml_init = true;

	      XMLTransService::Codes ret;
	      sg_coder = XMLPlatformUtils::fgTransService->makeNewTranscoderFor(sg_label_UTF_8, ret, sg_coder_buf_len);

	      if ( ret != XMLTransService::Ok )
		{
		  const char* message = "Undefined Error";

		  switch ( ret )
		    {
		    case XMLTransService::UnsupportedEncoding:  message = "Unsupported encoding";  break;
		    case XMLTransService::InternalFailure: 	message = "Internal failure";  break;
		    case XMLTransService::SupportFilesNotFound: message = "Support files not found";  break;
		    }

		  DefaultLogSink().Error("Xerces transform initialization error: %s\n", message);
		}
	    }
	  catch (const XMLException &e)
	    {
	      DefaultLogSink().Error("Xerces initialization error: %s\n", e.getMessage());
	    }
  	}
    }
}

//
bool
kumu_XercesString_to_UTF_8(const std::basic_string<XMLCh>& in_str, std::string& out_str) {
  return kumu_XercesString_to_UTF_8_p(in_str.c_str(), out_str);
}

//
bool
kumu_XercesString_to_UTF_8_p(const XMLCh* in_str, std::string& out_str)
{
  assert(in_str);
  assert(sg_xml_init);
  AutoMutex AL(sg_coder_lock);
  ui32_t str_len = XMLString::stringLen(in_str);
  ui32_t read_total = 0;

  try
    {
      while ( str_len > 0 )
	{
#if XERCES_VERSION_MAJOR < 3
 	  ui32_t read_count = 0;
#else
	  XMLSize_t read_count = 0;
#endif
	  ui32_t write_count = sg_coder->transcodeTo(in_str + read_total, str_len,
						     (XMLByte*)sg_coder_buf, sg_coder_buf_len,
						     read_count, XMLTranscoder::UnRep_Throw);

	  out_str.append(sg_coder_buf, write_count);
	  str_len -= read_count;
	  read_total += read_count;
	  assert(str_len >= 0);
	}
    }
  catch (...)
    {
      return false;
    }

  return true;
}

//
bool
kumu_UTF_8_to_XercesString(const std::string& in_str, std::basic_string<XMLCh>& out_str) {
  return kumu_UTF_8_to_XercesString_p(in_str.c_str(), out_str);
}

//
bool
kumu_UTF_8_to_XercesString_p(const char* in_str, std::basic_string<XMLCh>& out_str)
{
  assert(in_str);
  assert(sg_xml_init);
  AutoMutex AL(sg_coder_lock);
  ui32_t str_len = strlen(in_str);
  ui32_t read_total = 0;

  try
    {
      while ( str_len > 0 )
	{
#if XERCES_VERSION_MAJOR < 3
 	  ui32_t read_count = 0;
#else
	  XMLSize_t read_count = 0;
#endif
	  ui32_t write_count = sg_coder->transcodeFrom((const XMLByte*)(in_str + read_total), str_len,
						       (XMLCh*)sg_coder_buf, sg_coder_buf_len / sizeof(XMLCh),
						       read_count, sg_coder_counts);

	  out_str.append((XMLCh*)sg_coder_buf, write_count * sizeof(XMLCh));
	  str_len -= read_count;
	  read_total += read_count;
	  assert(str_len >= 0);
	}
    }
  catch (...)
    {
      return false;
    }

  return true;
}

//
class MyTreeHandler : public HandlerBase
{
  ns_map*                  m_Namespaces;
  std::stack<XMLElement*>  m_Scope;
  XMLElement*              m_Root;
  bool                     m_HasEncodeErrors;

public:
  MyTreeHandler(XMLElement* root) : m_Namespaces(0), m_Root(root), m_HasEncodeErrors(false)
  {
    assert(m_Root);
    m_Namespaces = new ns_map;
  }

  ~MyTreeHandler() {
    delete m_Namespaces;
  }

  bool HasEncodeErrors() const { return m_HasEncodeErrors; }

  ns_map* TakeNamespaceMap()
  {
    if ( m_Namespaces == 0 || m_Namespaces->empty() )
      return 0;

    ns_map* ret = m_Namespaces;
    m_Namespaces = 0;
    return ret;
  }

  //
  void AddNamespace(const char* ns_prefix, const char* ns_name)
  {
    assert(ns_prefix);
    assert(ns_name);

    if ( ns_prefix[0] == ':' )
      {
	ns_prefix++;
      }
    else
      {
	assert(ns_prefix[0] == 0);
	ns_prefix = "";
      }

    ns_map::iterator ni = m_Namespaces->find(ns_prefix);

    if  ( ni != m_Namespaces->end() )
      {
	if ( ni->second->Name() != std::string(ns_name) )
	  {
	    DefaultLogSink().Error("Duplicate prefix: %s\n", ns_prefix);
	    return;
	  }
      }
    else
      {
	XMLNamespace* Namespace = new XMLNamespace(ns_prefix, ns_name);
	m_Namespaces->insert(ns_map::value_type(ns_prefix, Namespace));
      }

    assert(!m_Namespaces->empty());
  }

  //
  void startElement(const XMLCh* const x_name,
		    XERCES_CPP_NAMESPACE::AttributeList& attributes)
  {
    assert(x_name);
    std::string tx_name;

    if ( ! kumu_XercesString_to_UTF_8(x_name, tx_name) )
      m_HasEncodeErrors = true;

    const char* name = tx_name.c_str();
    XMLElement* Element;
    const char* ns_root = name;
    const char* local_name = strchr(name, ':');

    if ( local_name != 0 )
      name = local_name + 1;

    if ( m_Scope.empty() )
      {
	m_Scope.push(m_Root);
      }
    else
      {
	Element = m_Scope.top();
	m_Scope.push(Element->AddChild(name));
      }

    Element = m_Scope.top();
    Element->SetName(name);

    // set attributes
    ui32_t a_len = attributes.getLength();

    for ( ui32_t i = 0; i < a_len; i++)
      {
	std::string aname, value;
	if ( ! kumu_XercesString_to_UTF_8(attributes.getName(i), aname) )
	  m_HasEncodeErrors = true;

	if ( ! kumu_XercesString_to_UTF_8(attributes.getValue(i), value) )
	  m_HasEncodeErrors = true;

	const char* x_aname = aname.c_str();
	const char* x_value = value.c_str();

	if ( strncmp(x_aname, "xmlns", 5) == 0 )
	  AddNamespace(x_aname+5, x_value);

	if ( ( local_name = strchr(x_aname, ':') ) == 0 )
	  local_name = x_aname;
	else
	  local_name++;

	Element->SetAttr(local_name, x_value);
      }

    // map the namespace
    std::string key;
    if ( ns_root != name )
      key.assign(ns_root, name - ns_root - 1);
  
    ns_map::iterator ni = m_Namespaces->find(key);
    if ( ni != m_Namespaces->end() )
      Element->SetNamespace(ni->second);
  }

  void endElement(const XMLCh *const name) {
    m_Scope.pop();
  }

  void characters(const XMLCh *const chars, const unsigned int length)
  {
    if ( length > 0 )
      {
	std::string tmp;
	if ( ! kumu_XercesString_to_UTF_8(chars, tmp) )
	  m_HasEncodeErrors = true;

	m_Scope.top()->AppendBody(tmp);
      }
  }
};

//
bool
Kumu::XMLElement::ParseString(const char* document, ui32_t doc_len)
{
  if ( doc_len == 0 )
    {
      return false;
    }

  kumu_init_xml_dom();

  int errorCount = 0;
  SAXParser* parser = new SAXParser();

  parser->setValidationScheme(SAXParser::Val_Always);
  parser->setDoNamespaces(true);    // optional

  MyTreeHandler* docHandler = new MyTreeHandler(this);
  parser->setDocumentHandler(docHandler);
  parser->setErrorHandler(docHandler);

  try
    {
      MemBufInputSource xmlSource(reinterpret_cast<const XMLByte*>(document),
				  static_cast<const unsigned int>(doc_len),
				  "pidc_rules_file");

      parser->parse(xmlSource);
    }
  catch (const XMLException& e)
    {
      char* message = XMLString::transcode(e.getMessage());
      DefaultLogSink().Error("Parser error: %s\n", message);
      XMLString::release(&message);
      errorCount++;
    }
  catch (const SAXParseException& e)
    {
      char* message = XMLString::transcode(e.getMessage());
      DefaultLogSink().Error("Parser error: %s at line %d\n", message, e.getLineNumber());
      XMLString::release(&message);
      errorCount++;
    }
  catch (...)
    {
      DefaultLogSink().Error("Unexpected XML parser error\n");
      errorCount++;
    }
  
  if ( errorCount == 0 )
    m_NamespaceOwner = (void*)docHandler->TakeNamespaceMap();

  delete parser;
  delete docHandler;

  return errorCount > 0 ? false : true;
}

//
bool
Kumu::XMLElement::ParseFirstFromString(const char* document, ui32_t doc_len)
{
  if ( doc_len == 0 )
    {
      return false;
    }

  kumu_init_xml_dom();
  
  int errorCount = 0;
  SAXParser* parser = new SAXParser();

  parser->setValidationScheme(SAXParser::Val_Always);
  parser->setDoNamespaces(true);    // optional

  MyTreeHandler* docHandler = new MyTreeHandler(this);
  parser->setDocumentHandler(docHandler);
  parser->setErrorHandler(docHandler);
  XMLPScanToken token;

  try
    {
      MemBufInputSource xmlSource(reinterpret_cast<const XMLByte*>(document),
				  static_cast<const unsigned int>(doc_len),
				  "pidc_rules_file");

      if ( ! parser->parseFirst(xmlSource, token) )
	{
	  ++errorCount;
	}

      if ( ! parser->parseNext(token) )
	{
	  ++errorCount;
	}
    }
  catch (const XMLException& e)
    {
      char* message = XMLString::transcode(e.getMessage());
      DefaultLogSink().Error("Parser error: %s\n", message);
      XMLString::release(&message);
      errorCount++;
    }
  catch (const SAXParseException& e)
    {
      char* message = XMLString::transcode(e.getMessage());
      DefaultLogSink().Error("Parser error: %s at line %d\n", message, e.getLineNumber());
      XMLString::release(&message);
      errorCount++;
    }
  catch (...)
    {
      DefaultLogSink().Error("Unexpected XML parser error\n");
      errorCount++;
    }
  
  if ( errorCount == 0 )
    m_NamespaceOwner = (void*)docHandler->TakeNamespaceMap();

  delete parser;
  delete docHandler;

  return errorCount > 0 ? false : true;
}


#endif

//----------------------------------------------------------------------------------------------------

#if ! defined(HAVE_EXPAT) && ! defined(HAVE_XERCES_C)

//
bool
Kumu::XMLElement::ParseString(const char* document, ui32_t doc_len)
{
  DefaultLogSink().Error("Kumu compiled without XML parser support.\n");
  return false;
}

bool
Kumu::XMLElement::ParseFirstFromString(const char* document, ui32_t doc_len)
{
  DefaultLogSink().Error("Kumu compiled without XML parser support.\n");
  return false;
}

#endif


//----------------------------------------------------------------------------------------------------

//
bool
Kumu::GetXMLDocType(const ByteString& buf, std::string& ns_prefix, std::string& type_name, std::string& namespace_name,
		    AttributeList& doc_attr_list)
{
  return GetXMLDocType(buf.RoData(), buf.Length(), ns_prefix, type_name, namespace_name, doc_attr_list);
}

//
bool
Kumu::GetXMLDocType(const std::string& buf, std::string& ns_prefix, std::string& type_name, std::string& namespace_name,
		    AttributeList& doc_attr_list)
{
  return GetXMLDocType((const byte_t*)buf.c_str(), buf.size(), ns_prefix, type_name, namespace_name, doc_attr_list);
}

//
bool
Kumu::GetXMLDocType(const byte_t* buf, ui32_t buf_len, std::string& ns_prefix, std::string& type_name, std::string& namespace_name,
		    AttributeList& doc_attr_list)
{
  XMLElement tmp_element("tmp");

  if ( ! tmp_element.ParseFirstFromString((const char*)buf, buf_len) )
    {
      return false;
    }

  const XMLNamespace* ns = tmp_element.Namespace();

  if ( ns != 0 )
    {
      ns_prefix = ns->Prefix();
      namespace_name = ns->Name();
    }

  type_name = tmp_element.GetName();
  doc_attr_list = tmp_element.GetAttributes();
  return true;
}


//
// end KM_xml.cpp
//
