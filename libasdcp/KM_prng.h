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
  /*! \file    KM_prng.h
    \version $Id: KM_prng.h,v 1.4 2009/04/09 19:24:14 msheby Exp $
    \brief   Fortuna pseudo-random number generator
  */

#ifndef _KM_PRNG_H_
#define _KM_PRNG_H_

#include <KM_util.h>

namespace Kumu
{
  class FortunaRNG
    {
      KM_NO_COPY_CONSTRUCT(FortunaRNG);

    public:
      FortunaRNG();
      ~FortunaRNG();
      const byte_t* FillRandom(byte_t* buf, ui32_t len);
      const byte_t* FillRandom(ByteString&);
    };


  // key_len must be <= 64 (larger values will be truncated)
  void Gen_FIPS_186_Value(const byte_t* key_in, ui32_t key_len, byte_t* buf, ui32_t buf_len);

} // namespace Kumu



#endif // _KM_PRNG_H_

//
// end KM_prng.h
//
