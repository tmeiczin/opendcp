/*
Copyright (c) 2005-2011, John Hurst
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
/*! \file    KM_xml.h
    \version $Id: KM_xml.h,v 1.8 2011/08/15 23:03:26 jhurst Exp $
    \brief   XML writer
*/


#ifndef _KM_XML_H_
#define _KM_XML_H_

#include <KM_util.h>
#include <list>
#include <string>

namespace Kumu
{
  class XMLElement;

  // Return true if the given string contains an XML document (or the start of one).
  bool StringIsXML(const char* document, ui32_t len = 0);

  //
  struct NVPair
  {
    std::string name;
    std::string value;
  };

  //
  typedef std::list<NVPair> AttributeList;
  typedef AttributeList::const_iterator Attr_i;
  typedef std::list<XMLElement*> ElementList;
  typedef ElementList::const_iterator Elem_i;

  //
  class XMLNamespace
  {
    std::string   m_Prefix;
    std::string   m_Name;

    KM_NO_COPY_CONSTRUCT(XMLNamespace);
    XMLNamespace();

    public:
  XMLNamespace(const char* prefix, const char* name) : m_Prefix(prefix), m_Name(name) {}
    ~XMLNamespace() {}

    inline const std::string& Prefix() const { return m_Prefix; }
    inline const std::string& Name() const { return m_Name; }
  };

  //
  class XMLElement
    {
      KM_NO_COPY_CONSTRUCT(XMLElement);
      XMLElement();

    protected:
      AttributeList       m_AttrList;
      ElementList         m_ChildList;
      const XMLNamespace* m_Namespace;
      void*               m_NamespaceOwner;

      std::string   m_Name;
      std::string   m_Body;

    public:
      XMLElement(const char* name);
      ~XMLElement();

      inline const XMLNamespace* Namespace() const { return m_Namespace; }
      inline void                SetNamespace(const XMLNamespace* ns) { assert(ns); m_Namespace = ns; }

      bool        ParseString(const char* document, ui32_t doc_len);
      bool        ParseString(const ByteString& document);
      bool        ParseString(const std::string& document);

      // building
      void        SetName(const char* name);
      void        SetBody(const std::string& value);
      void        AppendBody(const std::string& value);
      void        SetAttr(const char* name, const char* value);
      void        SetAttr(const char* name, const std::string& value) { SetAttr(name, value.c_str()); }
      XMLElement* AddChild(XMLElement* element);
      XMLElement* AddChild(const char* name);
      XMLElement* AddChildWithContent(const char* name, const char* value);
      XMLElement* AddChildWithContent(const char* name, const std::string& value);
      XMLElement* AddChildWithPrefixedContent(const char* name, const char* prefix, const char* value);
      void        AddComment(const char* value);
      void        Render(std::string&) const;
      void        RenderElement(std::string& outbuf, ui32_t depth) const;

      // querying
      inline const std::string&   GetBody() const { return m_Body; }
      inline const ElementList&   GetChildren() const { return m_ChildList; }
      inline const std::string&   GetName() const { return m_Name; }
      inline const AttributeList& GetAttributes() const { return m_AttrList; }
      const char*        GetAttrWithName(const char* name) const;
      XMLElement*        GetChildWithName(const char* name) const;
      const ElementList& GetChildrenWithName(const char* name, ElementList& outList) const;
      bool               HasName(const char* name) const;

      // altering
      void        DeleteAttributes();
      void        DeleteAttrWithName(const char* name);
      void        DeleteChildren();
      void        DeleteChild(const XMLElement* element);
      void        ForgetChild(const XMLElement* element);
    };
} // namespace Kumu

#endif // _KM_XML_H_

//
// end KM_xml.h
//
