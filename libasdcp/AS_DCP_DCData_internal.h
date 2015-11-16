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
/*! \file    AS_DCP_DCData_internal.h
    \version $Id: AS_DCP_DCData_internal.h,v 1.3 2014/01/02 23:29:22 jhurst Exp $
    \brief   AS-DCP library, non-public common DCData reader and writer implementation
*/

#ifndef _AS_DCP_DCDATA_INTERNAL_H_
#define _AS_DCP_DCDATA_INTERNAL_H_

#include <list>

#include "AS_DCP_internal.h"

namespace ASDCP
{

namespace MXF
{
  class InterchangeObject;
}

namespace DCData
{
  typedef std::list<MXF::InterchangeObject*> SubDescriptorList_t;

  class h__Reader : public ASDCP::h__ASDCPReader
  {
    MXF::DCDataDescriptor* m_EssenceDescriptor;
    ASDCP_NO_COPY_CONSTRUCT(h__Reader);
    h__Reader();

   public:
    DCDataDescriptor m_DDesc;

    h__Reader(const Dictionary& d) : ASDCP::h__ASDCPReader(d), m_EssenceDescriptor(0),
                                     m_DDesc() {}
    ~h__Reader() {}
    Result_t    OpenRead(const std::string&);
    Result_t    ReadFrame(ui32_t, FrameBuffer&, AESDecContext*, HMACContext*);
    Result_t    MD_to_DCData_DDesc(DCData::DCDataDescriptor& DDesc);
  };

  class h__Writer : public ASDCP::h__ASDCPWriter
  {
    ASDCP_NO_COPY_CONSTRUCT(h__Writer);
    h__Writer();

   public:
    DCDataDescriptor m_DDesc;
    byte_t           m_EssenceUL[SMPTE_UL_LENGTH];

    h__Writer(const Dictionary& d) : ASDCP::h__ASDCPWriter(d) {
      memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
    }

    ~h__Writer(){}

    Result_t OpenWrite(const std::string&, ui32_t HeaderSize, const SubDescriptorList_t& subDescriptors);
    Result_t SetSourceStream(const DCDataDescriptor&, const byte_t*, const std::string&, const std::string&);
    Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);
    Result_t Finalize();
    Result_t DCData_DDesc_to_MD(DCData::DCDataDescriptor& DDesc);
};


} // namespace DCData
} // namespace ASDCP

#endif // _AS_DCP_DCDATA_INTERNAL_H_
