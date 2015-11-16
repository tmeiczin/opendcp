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
  /*! \file    KM_platform.h
    \version $Id: KM_platform.h,v 1.9 2015/10/07 16:41:23 jhurst Exp $
    \brief   platform portability
  */

#ifndef _KM_PLATFORM_H_
# define _KM_PLATFORM_H_

#if defined(__APPLE__) && defined(__MACH__)
#  define KM_MACOSX
#  ifdef __BIG_ENDIAN__
#   define KM_BIG_ENDIAN
#  endif
# endif

# ifdef KM_WIN32
#  define WIN32_LEAN_AND_MEAN
#  define VC_EXTRALEAN
#  include <windows.h>
#  include <stdlib.h>
#  include <stdio.h>
#  include <stdarg.h>
#  pragma warning(disable:4786)			// Ignore "identifer > 255 characters" warning

typedef unsigned __int64   ui64_t;
typedef __int64            i64_t;
#  define i64_C(c)  (i64_t)(c)
#  define ui64_C(c) (ui64_t)(c)
#  define snprintf _snprintf
#  define vsnprintf _vsnprintf

# else // KM_WIN32
typedef unsigned long long ui64_t;
typedef long long          i64_t;
#  define i64_C(c)  c##LL
#  define ui64_C(c) c##ULL

# endif // KM_WIN32

# include <stdio.h>
# include <assert.h>
# include <stdlib.h>
# include <limits.h>

typedef unsigned char  byte_t;
typedef char           i8_t;
typedef unsigned char  ui8_t;
typedef short          i16_t;
typedef unsigned short ui16_t;
typedef int            i32_t;
typedef unsigned int   ui32_t;


namespace Kumu
{
  inline ui16_t Swap2(ui16_t i)
    {
      return ( (i << 8) | (( i & 0xff00) >> 8) );
    }

  inline ui32_t Swap4(ui32_t i)
    {
      return
	( (i & 0x000000ffUL) << 24 ) |
	( (i & 0xff000000UL) >> 24 ) |
	( (i & 0x0000ff00UL) << 8  ) |
	( (i & 0x00ff0000UL) >> 8  );
    }

  inline ui64_t Swap8(ui64_t i)
    {
      return
	( (i & ui64_C(0x00000000000000FF)) << 56 ) |
	( (i & ui64_C(0xFF00000000000000)) >> 56 ) |
	( (i & ui64_C(0x000000000000FF00)) << 40 ) |
	( (i & ui64_C(0x00FF000000000000)) >> 40 ) |
	( (i & ui64_C(0x0000000000FF0000)) << 24 ) |
	( (i & ui64_C(0x0000FF0000000000)) >> 24 ) |
	( (i & ui64_C(0x00000000FF000000)) << 8  ) |
	( (i & ui64_C(0x000000FF00000000)) >> 8  );
    }

  //
  template<class T>
    inline T xmin(T lhs, T rhs) {
    return (lhs < rhs) ? lhs : rhs;
  }

  //
  template<class T>
    inline T xmax(T lhs, T rhs) {
    return (lhs > rhs) ? lhs : rhs;
  }

  //
  template<class T>
    inline T xclamp(T v, T l, T h) {
    if ( v < l ) { return l; }
    if ( v > h ) { return h; }
    return v;
  }

  //
  template<class T>
    inline T xabs(T n) {
    if ( n < 0 ) { return -n; }
    return n;
  }

  // read an integer from byte-structured storage
  template<class T>
  inline T    cp2i(const byte_t* p) { return *(T*)p; }

  // write an integer to byte-structured storage
  template<class T>
  inline void i2p(T i, byte_t* p) { *(T*)p = i; }


# ifdef KM_BIG_ENDIAN
#  define KM_i16_LE(i)        Kumu::Swap2(i)
#  define KM_i32_LE(i)        Kumu::Swap4(i)
#  define KM_i64_LE(i)        Kumu::Swap8(i)
#  define KM_i16_BE(i)        (i)
#  define KM_i32_BE(i)        (i)
#  define KM_i64_BE(i)        (i)
# else
#  define KM_i16_LE(i)        (i)
#  define KM_i32_LE(i)        (i)
#  define KM_i64_LE(i)        (i)
#  define KM_i16_BE(i)        Kumu::Swap2(i)
#  define KM_i32_BE(i)        Kumu::Swap4(i)
#  define KM_i64_BE(i)        Kumu::Swap8(i)
# endif // KM_BIG_ENDIAN

  // A non-reference counting, auto-delete container for internal
  // member object pointers.
  template <class T>
    class mem_ptr
    {
      mem_ptr(T&);

    protected:
      T* m_p; // the thing we point to

    public:
      mem_ptr() : m_p(0) {}
      mem_ptr(T* p) : m_p(p) {}
      ~mem_ptr() { delete m_p; }

      inline T&   operator*()  const { return *m_p; }
      inline T*   operator->() const { assert(m_p!=0); return m_p; }
      inline      operator T*()const { return m_p; }
      inline const mem_ptr<T>& operator=(T* p) { this->set(p); return *this; }
      inline T*   set(T* p)          { delete m_p; m_p = p; return m_p; }
      inline T*   get()        const { return m_p; }
      inline void release()          { m_p = 0; }
      inline bool empty()      const { return m_p == 0; }
    };

} // namespace Kumu

// Produces copy constructor boilerplate. Allows convenient private
// declatarion of copy constructors to prevent the compiler from
// silently manufacturing default methods.
# define KM_NO_COPY_CONSTRUCT(T)   \
          T(const T&); \
          T& operator=(const T&)

/*
// Example
  class foo
    {
      KM_NO_COPY_CONSTRUCT(foo); // accessing private mthods will cause compile time error
    public:
      // ...
    };
*/

// Produces copy constructor boilerplate. Implements
// copy and assignment, see example below
# define KM_EXPLICIT_COPY_CONSTRUCT(T)	\
  T(const T&);				\
  const T& operator=(const T&)

# define KM_EXPLICIT_COPY_CONSTRUCT_IMPL_START(N, T)	\
  void T##_copy_impl(N::T& lhs, const N::T& rhs)	\
  {

#define KM_COPY_ITEM(I) lhs.I = rhs.I;

# define KM_EXPLICIT_COPY_CONSTRUCT_IMPL_END(N, T)	\
  }							\
  N::T::T(const N::T& rhs) { T##_copy_impl(*this, rhs); }		\
  const N::T& N::T::operator=(const N::T& rhs) { T##_copy_impl(*this, rhs); return *this; }

/*
// Example
namespace bar {
  class foo
    {
    public:
      std::string param_a;
      int param_b;

      KM_EXPLICIT_COPY_CONSTRUCT(foo);
      // ...
    };
}

//
KM_EXPLICIT_COPY_CONSTRUCT_IMPL_START(bar, foo)
KM_COPY_ITEM(param_a)
KM_COPY_ITEM(param_b)
KM_EXPLICIT_COPY_CONSTRUCT_IMPL_END(bar, foo)
*/

#endif // _KM_PLATFORM_H_

//
// KM_platform.h
//
