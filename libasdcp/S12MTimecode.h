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
/*! \file    S12MTimecode.cpp
    \version $Id: S12MTimecode.h,v 1.5 2009/04/09 19:24:14 msheby Exp $       
    \brief   AS-DCP library, Timecode PCM essence reader and writer implementation
*/

/*

  DROP-FRAME NOT SUPPORTED!

*/

#ifndef _S12MTIMECODE_H_
#define _S12MTIMECODE_H_

#include "KM_util.h"
#include "KM_memio.h"

namespace ASDCP {

 class S12MTimecode : public Kumu::IArchive
{
 protected:
  ui32_t m_FrameCount;
  ui32_t m_FPS;

public:
  S12MTimecode() : m_FrameCount(0), m_FPS(0) {}

  S12MTimecode(ui32_t frame_count, ui32_t fps) : m_FrameCount(frame_count), m_FPS(fps) {}

  S12MTimecode(const std::string& tc, ui32_t fps) : m_FrameCount(0), m_FPS(fps)
  {
    DecodeString(tc);
  }

  S12MTimecode(const S12MTimecode& rhs) : IArchive(), m_FrameCount(0), m_FPS(0)
  {
    m_FPS = rhs.m_FPS;
    m_FrameCount = rhs.m_FrameCount;
  }

  ~S12MTimecode() {}

  const S12MTimecode& operator=(const S12MTimecode& rhs)
  {
    assert(m_FPS != 0);
    m_FrameCount = rhs.m_FrameCount;
    return *this;
  }

  inline void   SetFPS(ui32_t fps) { m_FPS = fps; }
  inline ui32_t GetFPS() const { return m_FPS; }

  inline void   SetFrames(ui32_t frame_count) { m_FrameCount = frame_count; }
  inline ui32_t GetFrames() const { return m_FrameCount; }

  inline bool operator==(const S12MTimecode& rhs) const { return m_FrameCount == rhs.m_FrameCount; }
  inline bool operator<(const S12MTimecode& rhs) const { return m_FrameCount < rhs.m_FrameCount; }

  inline const S12MTimecode operator+(const S12MTimecode& rhs){
    assert(m_FPS > 0);
    assert(m_FPS == rhs.m_FPS);
    return S12MTimecode(m_FrameCount + rhs.m_FrameCount, m_FPS);
  }

  inline const S12MTimecode operator-(const S12MTimecode& rhs){
    assert(m_FPS > 0);
    assert(m_FPS == rhs.m_FPS);
    return S12MTimecode(m_FrameCount + rhs.m_FrameCount, m_FPS);
  }


  void DecodeString(const std::string& tc)
  {
    assert(m_FPS > 0);
    const char* p = tc.c_str();

    while ( *p != 0 && ! isdigit(*p) )
      p++;

    if ( *p != 0 )
      {
	ui32_t hours = atoi(p);
	ui32_t minutes = atoi(p+3);
	ui32_t seconds = atoi(p+6);
	ui32_t frames = atoi(p+9);

	m_FrameCount = (((((hours * 60) + minutes) * 60) + seconds) * m_FPS)+ frames;
      }
  }

  const char* EncodeString(char* buf, ui32_t buf_len)
  {
    assert(m_FPS > 0);
    ui32_t frames = m_FrameCount % m_FPS;
    m_FrameCount /= m_FPS;
    ui32_t seconds = m_FrameCount % 60;
    m_FrameCount /= 60;
    ui32_t minutes = m_FrameCount % 60;
    ui32_t hours = m_FrameCount / 60;

    snprintf(buf, buf_len, "%02d:%02d:%02d:%02d", hours, minutes, seconds, frames);
    return buf;
  }

  // IArchive
  bool HasValue() const { return (m_FPS > 0); }
  ui32_t ArchiveLength() const { return sizeof(ui32_t)*2; }

  bool Archive(Kumu::MemIOWriter* Writer) const
  {
    if ( ! Writer->WriteUi32BE(m_FPS) ) return false;
    if ( ! Writer->WriteUi32BE(m_FrameCount) ) return false;
    return true;
  }

  bool Unarchive(Kumu::MemIOReader* Reader)
  {
    if ( ! Reader->ReadUi32BE(&m_FPS) ) return false;
    if ( ! Reader->ReadUi32BE(&m_FrameCount) ) return false;
    return true;
  }
};


} // namespace ASDCP

#endif // _S12MTIMECODE_H_

//
// end S12MTimecode.h
//
