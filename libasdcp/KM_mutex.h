/*
Copyright (c) 2004-2009, John Hurst
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
  /*! \file    KM_mutex.h
    \version $Id: KM_mutex.h,v 1.2 2009/04/09 19:16:49 msheby Exp $
    \brief   platform portability
  */

#ifndef _KM_MUTEX_H_
#define _KM_MUTEX_H_

#include <KM_platform.h>

#ifndef KM_WIN32
# include <pthread.h>
#endif

namespace Kumu
{
#ifdef KM_WIN32
  class Mutex
    {
      CRITICAL_SECTION m_Mutex;
      KM_NO_COPY_CONSTRUCT(Mutex);

    public:
      inline Mutex()       { ::InitializeCriticalSection(&m_Mutex); }
      inline ~Mutex()      { ::DeleteCriticalSection(&m_Mutex); }
      inline void Lock()   { ::EnterCriticalSection(&m_Mutex); }
      inline void Unlock() { ::LeaveCriticalSection(&m_Mutex); }
    };
#else // KM_WIN32
  class Mutex
    {
      pthread_mutex_t m_Mutex;
      KM_NO_COPY_CONSTRUCT(Mutex);
      
    public:
      inline Mutex()       { pthread_mutex_init(&m_Mutex, 0); }
      inline ~Mutex()      { pthread_mutex_destroy(&m_Mutex); }
      inline void Lock()   { pthread_mutex_lock(&m_Mutex); }
      inline void Unlock() { pthread_mutex_unlock(&m_Mutex); }
    };
#endif // KM_WIN32

  // automatic Mutex management within a block - 
  // the mutex is created by the constructor and
  // released by the destructor
  class AutoMutex
    {
      Mutex& m_Mutex;
      AutoMutex();
      KM_NO_COPY_CONSTRUCT(AutoMutex);

    public:
      AutoMutex(Mutex& Mtx) : m_Mutex(Mtx) { m_Mutex.Lock(); }
      ~AutoMutex() { m_Mutex.Unlock(); }
    };

} // namespace Kumu

#endif // _KM_MUTEX_H_

//
// end KM_mutex.h
//
