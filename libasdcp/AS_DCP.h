/*
Copyright (c) 2003-2014, John Hurst
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
/*! \file    AS_DCP.h
    \version $Id: AS_DCP.h,v 1.52 2015/04/21 03:55:31 jhurst Exp $
    \brief   AS-DCP library, public interface

The asdcplib library is a set of file access objects that offer simplified
access to files conforming to the standards published by the SMPTE
D-Cinema Technology Committee 21DC. The file format, labeled AS-DCP,
is described in a series of documents which includes but may not
be be limited to:

 o SMPTE ST 429-2:2011 DCP Operational Constraints
 o SMPTE ST 429-3:2006 Sound and Picture Track File
 o SMPTE ST 429-4:2006 MXF JPEG 2000 Application
 o SMPTE ST 429-5:2009 Timed Text Track File
 o SMPTE ST 429-6:2006 MXF Track File Essence Encryption
 o SMPTE ST 429-10:2008 Stereoscopic Picture Track File
 o SMPTE ST 429-14:2008 Aux Data Track File
 o SMPTE ST 330:2004 - UMID
 o SMPTE ST 336:2001 - KLV
 o SMPTE ST 377:2004 - MXF (old version, required)
 o SMPTE ST 377-1:2011 - MXF
 o SMPTE ST 377-4:2012 - MXF Multichannel Audio Labeling Framework
 o SMPTE ST 390:2011 - MXF OP-Atom
 o SMPTE ST 379-1:2009 - MXF Generic Container (GC)
 o SMPTE ST 381-1:2005 - MPEG2 picture in GC
 o SMPTE ST 422:2006 - JPEG 2000 picture in GC
 o SMPTE ST 382:2007 - WAV/PCM sound in GC
 o IETF RFC 2104 - HMAC/SHA1
 o NIST FIPS 197 - AES (Rijndael) (via OpenSSL)

 o MXF Interop Track File Specification
 o MXF Interop Track File Essence Encryption Specification
 o MXF Interop Operational Constraints Specification
 - Note: the MXF Interop documents are not formally published.
   Contact the asdcplib support address to get copies.

The following use cases are supported by the library:

 o Write essence to a plaintext or ciphertext AS-DCP file:
 o Read essence from a plaintext or ciphertext AS-DCP file:
     MPEG2 Video Elementary Stream
     JPEG 2000 codestreams
     JPEG 2000 stereoscopic codestream pairs
     PCM audio streams
     SMPTE 429-7 Timed Text XML with font and image resources
     Aux Data (frame-wrapped synchronous blob)
     Proposed Dolby (TM) Atmos track file

 o Read header metadata from an AS-DCP file

This project depends upon the following libraries:
 - OpenSSL http://www.openssl.org/
 - Expat http://expat.sourceforge.net/  or
     Xerces-C http://xerces.apache.org/xerces-c/
   An XML library is not needed if you don't need support for SMPTE ST 429-5:2009.
*/

#ifndef _AS_DCP_H_
#define _AS_DCP_H_

#include <KM_error.h>
#include <KM_fileio.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <iosfwd>
#include <string>
#include <cstring>
#include <list>

//--------------------------------------------------------------------------------
// common integer types
// supply your own by defining ASDCP_NO_BASE_TYPES

#ifndef ASDCP_NO_BASE_TYPES
typedef unsigned char  byte_t;
typedef char           i8_t;
typedef unsigned char  ui8_t;
typedef short          i16_t;
typedef unsigned short ui16_t;
typedef int            i32_t;
typedef unsigned int   ui32_t;
#endif


//--------------------------------------------------------------------------------
// convenience macros

// Convenience macros for managing return values in predicates
#define ASDCP_SUCCESS(v) (((v) < 0) ? 0 : 1)
#define ASDCP_FAILURE(v) (((v) < 0) ? 1 : 0)


// Returns RESULT_PTR if the given argument is NULL.
// See Result_t below for an explanation of RESULT_* symbols.
#define ASDCP_TEST_NULL(p) \
  if ( (p) == 0  ) { \
    return ASDCP::RESULT_PTR; \
  }

// Returns RESULT_PTR if the given argument is NULL. See Result_t
// below for an explanation of RESULT_* symbols. It then assumes
// that the argument is a pointer to a string and returns
// RESULT_NULL_STR if the first character is '\0'.
//
#define ASDCP_TEST_NULL_STR(p) \
  ASDCP_TEST_NULL(p); \
  if ( (p)[0] == '\0' ) { \
    return ASDCP::RESULT_NULL_STR; \
  }

// Produces copy constructor boilerplate. Allows convenient private
// declatarion of copy constructors to prevent the compiler from
// silently manufacturing default methods.
#define ASDCP_NO_COPY_CONSTRUCT(T)   \
          T(const T&); \
          T& operator=(const T&)

//--------------------------------------------------------------------------------
// All library components are defined in the namespace ASDCP
//
namespace ASDCP {
  //
  // The version number declaration and explanation have moved to ../configure.ac
  const char* Version();

  // UUIDs are passed around as strings of UUIDlen bytes
  const ui32_t UUIDlen = 16;

  // Encryption keys are passed around as strings of KeyLen bytes
  const ui32_t KeyLen = 16;

  //---------------------------------------------------------------------------------
  // return values

  using Kumu::Result_t;

  using Kumu::RESULT_FALSE;
  using Kumu::RESULT_OK;
  using Kumu::RESULT_FAIL;
  using Kumu::RESULT_PTR;
  using Kumu::RESULT_NULL_STR;
  using Kumu::RESULT_ALLOC;
  using Kumu::RESULT_PARAM;
  using Kumu::RESULT_SMALLBUF;
  using Kumu::RESULT_INIT;
  using Kumu::RESULT_NOT_FOUND;
  using Kumu::RESULT_NO_PERM;
  using Kumu::RESULT_FILEOPEN;
  using Kumu::RESULT_BADSEEK;
  using Kumu::RESULT_READFAIL;
  using Kumu::RESULT_WRITEFAIL;
  using Kumu::RESULT_STATE;
  using Kumu::RESULT_ENDOFFILE;
  using Kumu::RESULT_CONFIG;

  KM_DECLARE_RESULT(FORMAT,     -101, "The file format is not proper OP-Atom/AS-DCP.");
  KM_DECLARE_RESULT(RAW_ESS,    -102, "Unknown raw essence file type.");
  KM_DECLARE_RESULT(RAW_FORMAT, -103, "Raw essence format invalid.");
  KM_DECLARE_RESULT(RANGE,      -104, "Frame number out of range.");
  KM_DECLARE_RESULT(CRYPT_CTX,  -105, "AESEncContext required when writing to encrypted file.");
  KM_DECLARE_RESULT(LARGE_PTO,  -106, "Plaintext offset exceeds frame buffer size.");
  KM_DECLARE_RESULT(CAPEXTMEM,  -107, "Cannot resize externally allocated memory.");
  KM_DECLARE_RESULT(CHECKFAIL,  -108, "The check value did not decrypt correctly.");
  KM_DECLARE_RESULT(HMACFAIL,   -109, "HMAC authentication failure.");
  KM_DECLARE_RESULT(HMAC_CTX,   -110, "HMAC context required.");
  KM_DECLARE_RESULT(CRYPT_INIT, -111, "Error initializing block cipher context.");
  KM_DECLARE_RESULT(EMPTY_FB,   -112, "Empty frame buffer.");
  KM_DECLARE_RESULT(KLV_CODING, -113, "KLV coding error.");
  KM_DECLARE_RESULT(SPHASE,     -114, "Stereoscopic phase mismatch.");
  KM_DECLARE_RESULT(SFORMAT,    -115, "Rate mismatch, file may contain stereoscopic essence.");

  //---------------------------------------------------------------------------------
  // file identification

  // The file accessors in this library implement a bounded set of essence types.
  // This list will be expanded when support for new types is added to the library.
  enum EssenceType_t {
    ESS_UNKNOWN,              // the file is not a supported AS-DCP of AS-02 essence container

    // 
    ESS_MPEG2_VES,            // the file contains an MPEG-2 video elementary stream

    // d-cinema essence types
    ESS_JPEG_2000,            // the file contains one or more JPEG 2000 codestreams
    ESS_PCM_24b_48k,          // the file contains one or more PCM audio pairs
    ESS_PCM_24b_96k,          // the file contains one or more PCM audio pairs
    ESS_TIMED_TEXT,           // the file contains an XML timed text document and one or more resources
    ESS_JPEG_2000_S,          // the file contains one or more JPEG 2000 codestream pairs (stereoscopic)
    ESS_DCDATA_UNKNOWN,       // the file contains one or more D-Cinema Data bytestreams
    ESS_DCDATA_DOLBY_ATMOS,   // the file contains one or more DolbyATMOS bytestreams

    // IMF essence types
    ESS_AS02_JPEG_2000,       // the file contains one or more JPEG 2000 codestreams
    ESS_AS02_PCM_24b_48k,     // the file contains one or more PCM audio pairs, clip wrapped
    ESS_AS02_PCM_24b_96k,     // the file contains one or more PCM audio pairs, clip wrapped
    ESS_AS02_TIMED_TEXT,      // the file contains a TTML document and zero or more resources

    ESS_MAX
  };

  // Determine the type of essence contained in the given MXF file. RESULT_OK
  // is returned if the file is successfully opened and contains a valid MXF
  // stream. If there is an error, the result code will indicate the reason.
  Result_t EssenceType(const std::string& filename, EssenceType_t& type);

  // Determine the type of essence contained in the given raw file. RESULT_OK
  // is returned if the file is successfully opened and contains a known
  // stream type. If there is an error, the result code will indicate the reason.
  Result_t RawEssenceType(const std::string& filename, EssenceType_t& type);


  //---------------------------------------------------------------------------------
  // base types

  // A simple container for rational numbers.
  class Rational
  {
  public:
    i32_t Numerator;
    i32_t Denominator;

    Rational() : Numerator(0), Denominator(0) {}
    Rational(i32_t n, i32_t d) : Numerator(n), Denominator(d) {}

    inline double Quotient() const {
      return (double)Numerator / (double)Denominator;
    }

    inline bool operator==(const Rational& rhs) const {
      return ( rhs.Numerator == Numerator && rhs.Denominator == Denominator );
    }

    inline bool operator!=(const Rational& rhs) const {
      return ( rhs.Numerator != Numerator || rhs.Denominator != Denominator );
    }

    inline bool operator<(const Rational& rhs) {
      if ( Numerator < rhs.Numerator )     return true;
      if ( Numerator == rhs.Numerator && Denominator < rhs.Denominator )    return true;
      return false;
    }

    inline bool operator>(const Rational& rhs) {
      if ( Numerator > rhs.Numerator )     return true;
      if ( Numerator == rhs.Numerator && Denominator > rhs.Denominator )     return true;
      return false;
    }
  };

  // Encodes a rational number as a string having a single delimiter character between
  // numerator and denominator.  Retuns the buffer pointer to allow convenient in-line use.
  const char* EncodeRational(const Rational&, char* str_buf, ui32_t buf_len, char delimiter = ' ');

  // Decodes a rational number havng a single non-digit delimiter character between
  // the numerator and denominator.  Returns false if the string does not contain
  // the expected syntax.
  bool DecodeRational(const char* str_rat, Rational&);


  // common edit rates, use these instead of hard coded constants
  const Rational EditRate_24 = Rational(24,1);
  const Rational EditRate_23_98 = Rational(24000,1001); // Not a DCI-compliant value!
  const Rational EditRate_48 = Rational(48,1);
  const Rational SampleRate_48k = Rational(48000,1);
  const Rational SampleRate_96k = Rational(96000,1);

  // Additional frame rates, see ST 428-11, ST 429-13
  // These rates are new and not supported by all systems. Do not assume that
  // a package made using one of these rates will work just anywhere!
  const Rational EditRate_25 = Rational(25,1);
  const Rational EditRate_30 = Rational(30,1);
  const Rational EditRate_50 = Rational(50,1);
  const Rational EditRate_60 = Rational(60,1);
  const Rational EditRate_96 = Rational(96,1);
  const Rational EditRate_100 = Rational(100,1);
  const Rational EditRate_120 = Rational(120,1);

  // Archival frame rates, see ST 428-21
  // These rates are new and not supported by all systems. Do not assume that
  // a package made using one of these rates will work just anywhere!
  const Rational EditRate_16 = Rational(16,1);
  const Rational EditRate_18 = Rational(200,11); // 18.182
  const Rational EditRate_20 = Rational(20,1);
  const Rational EditRate_22 = Rational(240,11); // 21.818


  // Non-reference counting container for internal member objects.
  // Please do not use this class for any other purpose.
  template <class T>
    class mem_ptr
    {
      T* m_p; // the thing we point to
      mem_ptr(T&);

    public:
      mem_ptr() : m_p(0) {}
      mem_ptr(T* p) : m_p(p) {}
      ~mem_ptr() { delete m_p; }

      inline T&   operator*()  const { return *m_p; }
      inline T*   operator->() const { return m_p; }
      inline      operator T*()const { return m_p; }
      inline const mem_ptr<T>& operator=(T* p) { set(p); return *this; }
      inline T*   set(T* p)          { delete m_p; m_p = p; return m_p; }
      inline T*   get()        const { return m_p; }
      inline void release()          { m_p = 0; }
      inline bool empty()      const { return m_p == 0; }
    };


  //---------------------------------------------------------------------------------
  // WriterInfo class - encapsulates writer identification details used for
  // OpenWrite() calls.  Replace these values at runtime to identify your product.
  //
  // MXF files use SMPTE Universal Labels to identify data items. The set of Labels
  // in a file is determined by the MXF Operational Pattern and any constraining
  // documentation. There are currently two flavors of AS-DCP file in use: MXF Interop
  // (AKA JPEG Interop) and SMPTE. The two differ in the values of three labels:
  //
  //   OPAtom
  //      Interop : 06 0e 2b 34 04 01 01 01  0d 01 02 01 10 00 00 00
  //      SMPTE   : 06 0e 2b 34 04 01 01 02  0d 01 02 01 10 00 00 00
  //
  //   EKLV Packet:
  //      Interop : 06 0e 2b 34 02 04 01 07  0d 01 03 01 02 7e 01 00
  //      SMPTE   : 06 0e 2b 34 02 04 01 01  0d 01 03 01 02 7e 01 00
  //
  //   GenericDescriptor/SubDescriptors:
  //      Interop : 06 0e 2b 34 01 01 01 02  06 01 01 04 06 10 00 00
  //      SMPTE   : 06 0e 2b 34 01 01 01 09  06 01 01 04 06 10 00 00
  //
  // asdcplib will read any (otherwise valid) file which has any combination of the
  // above values. When writing files, MXF Interop labels are used by default. To
  // write a file containing SMPTE labels, replace the default label set value in
  // the WriterInfo before calling OpenWrite()
  //
  //
  enum LabelSet_t
  {
    LS_MXF_UNKNOWN,
    LS_MXF_INTEROP,
    LS_MXF_SMPTE,
    LS_MAX
  };

  //
  struct WriterInfo
  {
    byte_t      ProductUUID[UUIDlen];
    byte_t      AssetUUID[UUIDlen];
    byte_t      ContextID[UUIDlen];
    byte_t      CryptographicKeyID[UUIDlen];
    bool        EncryptedEssence; // true if essence data is (or is going to be) encrypted
    bool        UsesHMAC;         // true if HMAC exists or is to be calculated
    std::string ProductVersion;
    std::string CompanyName;
    std::string ProductName;
    LabelSet_t  LabelSetType;

    WriterInfo() : EncryptedEssence(false), UsesHMAC(false), LabelSetType(LS_MXF_INTEROP)
    {
      static byte_t default_ProductUUID_Data[UUIDlen] = {
	0x43, 0x05, 0x9a, 0x1d, 0x04, 0x32, 0x41, 0x01,
	0xb8, 0x3f, 0x73, 0x68, 0x15, 0xac, 0xf3, 0x1d };

      memcpy(ProductUUID, default_ProductUUID_Data, UUIDlen);
      memset(AssetUUID, 0, UUIDlen);
      memset(ContextID, 0, UUIDlen);
      memset(CryptographicKeyID, 0, UUIDlen);

      ProductVersion = "Unreleased ";
      ProductVersion += Version();
      CompanyName = "DCI";
      ProductName = "asdcplib";
    }
  };

  // Print WriterInfo to std::ostream
  std::ostream& operator << (std::ostream& strm, const WriterInfo& winfo);
  // Print WriterInfo to stream, stderr by default.
  void WriterInfoDump(const WriterInfo&, FILE* = 0);

  //---------------------------------------------------------------------------------
  // cryptographic support

  // The following classes define interfaces to Rijndael contexts having the following properties:
  //  o 16 byte key
  //  o CBC mode with 16 byte block size
  const ui32_t CBC_KEY_SIZE = 16;
  const ui32_t CBC_BLOCK_SIZE = 16;
  const ui32_t HMAC_SIZE = 20;

  //
  class AESEncContext
    {
      class h__AESContext;
      mem_ptr<h__AESContext> m_Context;
      ASDCP_NO_COPY_CONSTRUCT(AESEncContext);

    public:
      AESEncContext();
      ~AESEncContext();

      // Initializes Rijndael CBC encryption context.
      // Returns error if the key argument is NULL.
      Result_t InitKey(const byte_t* key);

      // Initializes 16 byte CBC Initialization Vector. This operation may be performed
      // any number of times for a given key.
      // Returns error if the i_vec argument is NULL.
      Result_t SetIVec(const byte_t* i_vec);
      Result_t GetIVec(byte_t* i_vec) const;

      // Encrypt a block of data. The block size must be a multiple of CBC_BLOCK_SIZE.
      // Returns error if either argument is NULL.
      Result_t EncryptBlock(const byte_t* pt_buf, byte_t* ct_buf, ui32_t block_size);
    };

  //
  class AESDecContext
    {
      class h__AESContext;
      mem_ptr<h__AESContext> m_Context;
      ASDCP_NO_COPY_CONSTRUCT(AESDecContext);

    public:
      AESDecContext();
      ~AESDecContext();

      // Initializes Rijndael CBC decryption context.
      // Returns error if the key argument is NULL.
      Result_t InitKey(const byte_t* key);

      // Initializes 16 byte CBC Initialization Vector. This operation may be performed
      // any number of times for a given key.
      // Returns error if the i_vec argument is NULL.
      Result_t SetIVec(const byte_t* i_vec);

      // Decrypt a block of data. The block size must be a multiple of CBC_BLOCK_SIZE.
      // Returns error if either argument is NULL.
      Result_t DecryptBlock(const byte_t* ct_buf, byte_t* pt_buf, ui32_t block_size);
    };

  //
  class HMACContext
    {
      class h__HMACContext;
      mem_ptr<h__HMACContext> m_Context;
      ASDCP_NO_COPY_CONSTRUCT(HMACContext);

    public:
      HMACContext();
      ~HMACContext();

      // Initializes HMAC context. The key argument must point to a binary
      // key that is CBC_KEY_SIZE bytes in length. Returns error if the key
      // argument is NULL.
      Result_t InitKey(const byte_t* key, LabelSet_t);

      // Reset internal state, allows repeated cycles of Update -> Finalize
      void Reset();

      // Add data to the digest. Returns error if the key argument is NULL or
      // if the digest has been finalized.
      Result_t Update(const byte_t* buf, ui32_t buf_len);

      // Finalize digest.  Returns error if the digest has already been finalized.
      Result_t Finalize();

      // Writes HMAC value to given buffer. buf must point to a writable area of
      // memory that is at least HMAC_SIZE bytes in length. Returns error if the
      // buf argument is NULL or if the digest has not been finalized.
      Result_t GetHMACValue(byte_t* buf) const;

      // Tests the given value against the finalized value in the object. buf must
      // point to a readable area of memory that is at least HMAC_SIZE bytes in length.
      // Returns error if the buf argument is NULL or if the values do ot match.
      Result_t TestHMACValue(const byte_t* buf) const;
    };

  //---------------------------------------------------------------------------------
  // frame buffer base class
  //
  // The supported essence types are stored using per-frame KLV packetization. The
  // following class implements essence-neutral functionality for managing a buffer
  // containing a frame of essence.

  class FrameBuffer
    {
      ASDCP_NO_COPY_CONSTRUCT(FrameBuffer);

    protected:
      byte_t* m_Data;          // pointer to memory area containing frame data
      ui32_t  m_Capacity;      // size of memory area pointed to by m_Data
      bool    m_OwnMem;        // if false, m_Data points to externally allocated memory
      ui32_t  m_Size;          // size of frame data in memory area pointed to by m_Data
      ui32_t  m_FrameNumber;   // delivery-order frame number

      // It is possible to read raw ciphertext from an encrypted AS-DCP file.
      // After reading an encrypted AS-DCP file in raw mode, the frame buffer will
      // contain the encrypted source value portion of the Encrypted Triplet, followed
      // by the integrity pack, if it exists.
      // The buffer will begin with the IV and CheckValue, followed by encrypted essence
      // and optional integrity pack
      // The SourceLength and PlaintextOffset values from the packet will be held in the
      // following variables:
      ui32_t  m_SourceLength;       // plaintext length (delivered plaintext+decrypted ciphertext)
      ui32_t  m_PlaintextOffset;    // offset to first byte of ciphertext

     public:
      FrameBuffer();
      virtual ~FrameBuffer();

      // Instructs the object to use an externally allocated buffer. The external
      // buffer will not be cleaned up by the frame buffer when it exits.
      // Call with (0,0) to revert to internally allocated buffer.
      // Returns error if the buf_addr argument is NULL and buf_size is non-zero.
      Result_t SetData(byte_t* buf_addr, ui32_t buf_size);

      // Sets the size of the internally allocate buffer. Returns RESULT_CAPEXTMEM
      // if the object is using an externally allocated buffer via SetData();
      // Resets content size to zero.
      Result_t Capacity(ui32_t cap);

      // returns the size of the buffer
      inline ui32_t  Capacity() const { return m_Capacity; }

      // returns a const pointer to the essence data
      inline const byte_t* RoData() const { return m_Data; }

      // returns a non-const pointer to the essence data
      inline byte_t* Data() { return m_Data; }

      // set the size of the buffer's contents
      inline ui32_t  Size(ui32_t size) { return m_Size = size; }

      // returns the size of the buffer's contents
      inline ui32_t  Size() const { return m_Size; }

      // Sets the absolute frame number of this frame in the file in delivery order.
      inline void    FrameNumber(ui32_t num) { m_FrameNumber = num; }

      // Returns the absolute frame number of this frame in the file in delivery order.
      inline ui32_t  FrameNumber() const { return m_FrameNumber; }

      // Sets the length of the plaintext essence data
      inline void    SourceLength(ui32_t len) { m_SourceLength = len; }

      // When this value is 0 (zero), the buffer contains only plaintext. When it is
      // non-zero, the buffer contains raw ciphertext and the return value is the length
      // of the original plaintext.
      inline ui32_t  SourceLength() const { return m_SourceLength; }

      // Sets the offset into the buffer at which encrypted data begins
      inline void    PlaintextOffset(ui32_t ofst) { m_PlaintextOffset = ofst; }

      // Returns offset into buffer of first byte of ciphertext.
      inline ui32_t  PlaintextOffset() const { return m_PlaintextOffset; }
    };

  //---------------------------------------------------------------------------------
  // Accessors in the MXFReader and MXFWriter classes below return these types to
  // provide direct access to MXF metadata structures declared in MXF.h and Metadata.h

  namespace MXF {
    // #include<Metadata.h> to use these
    class OP1aHeader;
    class OPAtomIndexFooter;
    class RIP;
  };

  //---------------------------------------------------------------------------------
  // MPEG2 video elementary stream support

  //
  namespace MPEG2
    {
      // MPEG picture coding type
      enum FrameType_t {
	FRAME_U = 0x00, // Unknown
	FRAME_I = 0x01, // I-Frame
	FRAME_P = 0x02, // P-Frame
	FRAME_B = 0x03  // B-Frame
      };

      // convert FrameType_t to char
      inline char FrameTypeChar(FrameType_t type)
	{
	  switch ( type )
	    {
	    case FRAME_I: return 'I';
	    case FRAME_B: return 'B';
	    case FRAME_P: return 'P';
	    default: return 'U';
	    }
	}

      // Structure represents the metadata elements in the file header's
      // MPEG2VideoDescriptor object.
      struct VideoDescriptor
	{
	  Rational EditRate;                //
	  ui32_t   FrameRate;               //
	  Rational SampleRate;              //
	  ui8_t    FrameLayout;             //
	  ui32_t   StoredWidth;             //
	  ui32_t   StoredHeight;            //
	  Rational AspectRatio;             //
	  ui32_t   ComponentDepth;          //
	  ui32_t   HorizontalSubsampling;   //
	  ui32_t   VerticalSubsampling;     //
	  ui8_t    ColorSiting;             //
	  ui8_t    CodedContentType;        //
	  bool     LowDelay;                //
	  ui32_t   BitRate;                 //
	  ui8_t    ProfileAndLevel;         //
	  ui32_t   ContainerDuration;       //
      };

      // Print VideoDescriptor to std::ostream
      std::ostream& operator << (std::ostream& strm, const VideoDescriptor& vdesc);
      // Print VideoDescriptor to stream, stderr by default.
      void VideoDescriptorDump(const VideoDescriptor&, FILE* = 0);

      // A container for MPEG frame data.
      class FrameBuffer : public ASDCP::FrameBuffer
	{
	  ASDCP_NO_COPY_CONSTRUCT(FrameBuffer); // TODO: should have copy construct

	protected:
	  FrameType_t m_FrameType;
	  ui8_t       m_TemporalOffset;
	  bool        m_ClosedGOP;
	  bool        m_GOPStart;

	public:
	  FrameBuffer() :
	    m_FrameType(FRAME_U), m_TemporalOffset(0),
	    m_ClosedGOP(false), m_GOPStart(false) {}

	  FrameBuffer(ui32_t size) :
	    m_FrameType(FRAME_U), m_TemporalOffset(0),
	    m_ClosedGOP(false), m_GOPStart(false)
	    {
	      Capacity(size);
	    }

	  virtual ~FrameBuffer() {}

	  // Sets the MPEG frame type of the picture data in the frame buffer.
	  inline void FrameType(FrameType_t type) { m_FrameType = type; }

	  // Returns the MPEG frame type of the picture data in the frame buffer.
	  inline FrameType_t FrameType() const { return m_FrameType; }

	  // Sets the MPEG temporal offset of the picture data in the frame buffer.
	  inline void TemporalOffset(ui8_t offset) { m_TemporalOffset = offset; }

	  // Returns the MPEG temporal offset of the picture data in the frame buffer.
	  inline ui8_t TemporalOffset() const { return m_TemporalOffset; }

	  // Sets the MPEG GOP 'start' attribute for the frame buffer.
	  inline void GOPStart(bool start) { m_GOPStart = start; }

	  // True if the frame in the buffer is the first in the GOP (in transport order)
	  inline bool GOPStart() const { return m_GOPStart; }

	  // Sets the MPEG GOP 'closed' attribute for the frame buffer.
	  inline void ClosedGOP(bool closed) { m_ClosedGOP = closed; }

	  // Returns true if the frame in the buffer is from a closed GOP, false if
	  // the frame is from an open GOP.  Always returns false unless GOPStart()
	  // returns true.
	  inline bool ClosedGOP() const { return m_ClosedGOP; }

	  // Print object state to stream, include n bytes of frame data if indicated.
	  // Default stream is stderr.
	  void    Dump(FILE* = 0, ui32_t dump_len = 0) const;
	};


      // An object which opens and reads an MPEG2 Video Elementary Stream file.  The call to
      // OpenRead() reads metadata from the file and populates an internal VideoDescriptor object.
      // Each subsequent call to ReadFrame() reads exactly one frame from the stream into the
      // given FrameBuffer object.
      class Parser
	{
	  class h__Parser;
	  mem_ptr<h__Parser> m_Parser;
	  ASDCP_NO_COPY_CONSTRUCT(Parser);

	public:
	  Parser();
	  virtual ~Parser();

	  // Opens the stream for reading, parses enough data to provide a complete
	  // set of stream metadata for the MXFWriter below.
	  Result_t OpenRead(const std::string& filename) const;

	  // Fill a VideoDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillVideoDescriptor(VideoDescriptor&) const;

	  // Rewind the stream to the beginning.
	  Result_t Reset() const;

	  // Reads the next sequential frame in the input file and places it in the
	  // frame buffer. Fails if the buffer is too small or the stream is empty.
	  // The frame buffer's PlaintextOffset parameter will be set to the first
	  // data byte of the first slice. Set this value to zero if you want
	  // encrypted headers.
	  Result_t ReadFrame(FrameBuffer&) const;
	};

      // A class which creates and writes MPEG frame data to an AS-DCP format MXF file.
      // Not yet implemented
      class MXFWriter
	{
	  class h__Writer;
	  mem_ptr<h__Writer> m_Writer;
	  ASDCP_NO_COPY_CONSTRUCT(MXFWriter);

	public:
	  MXFWriter();
	  virtual ~MXFWriter();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual MXF::OP1aHeader& OP1aHeader();
	  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
	  virtual MXF::RIP& RIP();

	  // Open the file for writing. The file must not exist. Returns error if
	  // the operation cannot be completed or if nonsensical data is discovered
	  // in the essence descriptor.
	  Result_t OpenWrite(const std::string& filename, const WriterInfo&,
			     const VideoDescriptor&, ui32_t HeaderSize = 16384);

	  // Writes a frame of essence to the MXF file. If the optional AESEncContext
	  // argument is present, the essence is encrypted prior to writing.
	  // Fails if the file is not open, is finalized, or an operating system
	  // error occurs.
	  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);

	  // Closes the MXF file, writing the index and revised header.
	  Result_t Finalize();
	};

      // A class which reads MPEG frame data from an AS-DCP format MXF file.
      class MXFReader
	{
	  class h__Reader;
	  mem_ptr<h__Reader> m_Reader;
	  ASDCP_NO_COPY_CONSTRUCT(MXFReader);

	public:
	  MXFReader();
	  virtual ~MXFReader();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual MXF::OP1aHeader& OP1aHeader();
	  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
	  virtual MXF::RIP& RIP();

	  // Open the file for reading. The file must exist. Returns error if the
	  // operation cannot be completed.
	  Result_t OpenRead(const std::string& filename) const;

	  // Returns RESULT_INIT if the file is not open.
	  Result_t Close() const;

	  // Fill a VideoDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillVideoDescriptor(VideoDescriptor&) const;

	  // Fill a WriterInfo struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillWriterInfo(WriterInfo&) const;

	  // Reads a frame of essence from the MXF file. If the optional AESEncContext
	  // argument is present, the essence is decrypted after reading. If the MXF
	  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
	  // will contain the ciphertext frame data. If the HMACContext argument is
	  // not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadFrame(ui32_t frame_number, FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Using the index table read from the footer partition, lookup the frame number
	  // and return the offset into the file at which to read that frame of essence.
	  // Returns RESULT_INIT if the file is not open, and RESULT_RANGE if the frame number is
	  // out of range.
	  Result_t LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset, i8_t& temporalOffset, i8_t& keyFrameOffset) const;

	  // Calculates the first frame in transport order of the GOP in which the requested
	  // frame is located.  Calls ReadFrame() to fetch the frame at the calculated position.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t ReadFrameGOPStart(ui32_t frame_number, FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Calculates the first frame in transport order of the GOP in which the requested
	  // frame is located.  Sets key_frame_number to the number of the frame at the calculated position.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FindFrameGOPStart(ui32_t frame_number, ui32_t& key_frame_number) const;

	  // Returns the type of the frame at the given position.
	  // Returns RESULT_INIT if the file is not open or RESULT_RANGE if the index is out of range.
	  Result_t FrameType(ui32_t frame_number, FrameType_t&) const;

	  // Print debugging information to stream
	  void     DumpHeaderMetadata(FILE* = 0) const;
	  void     DumpIndex(FILE* = 0) const;
	};
    } // namespace MPEG2

  //---------------------------------------------------------------------------------
  //



  namespace PCM
    {
      // The default value of the ChannelFormat element of the AudioDescriptor struct
      // is CF_NONE. If the value is set to one of the other ChannelFormat_t enum
      // values, the file's Wave Audio Descriptor will contain a Channel Assignment
      // element.
      //
      // The channel format should be one of (CF_CFG_1, CF_CFG_2, or CF_CFG_3) for
      // SMPTE 429-2 files. It should normally be CF_NONE for JPEG Interop files, but
      // the 429-2 may also be used.
      //
      enum ChannelFormat_t {
	CF_NONE,
	CF_CFG_1, // 5.1 with optional HI/VI
	CF_CFG_2, // 6.1 (5.1 + center surround) with optional HI/VI
	CF_CFG_3, // 7.1 (SDDS) with optional HI/VI
	CF_CFG_4, // Wild Track Format
	CF_CFG_5, // 7.1 DS with optional HI/VI
	CF_CFG_6, // ST 377-4 (MCA) labels (see also ASDCP::MXF::decode_mca_string)
	CF_MAXIMUM
      };

      struct AudioDescriptor
	{
	  Rational EditRate;         // rate of frame wrapping
	  Rational AudioSamplingRate;  // rate of audio sample
	  ui32_t   Locked;             //
	  ui32_t   ChannelCount;       // number of channels
	  ui32_t   QuantizationBits;   // number of bits per single-channel sample
	  ui32_t   BlockAlign;         // number of bytes ber sample, all channels
	  ui32_t   AvgBps;             //
	  ui32_t   LinkedTrackID;      //
	  ui32_t   ContainerDuration;  // number of frames
	  ChannelFormat_t ChannelFormat; // audio channel arrangement
      };

      // Print AudioDescriptor to std::ostream
      std::ostream& operator << (std::ostream& strm, const AudioDescriptor& adesc);
      // Print debugging information to stream (stderr default)
      void   AudioDescriptorDump(const AudioDescriptor&, FILE* = 0);

      // Returns size in bytes of a single sample of data described by ADesc
      inline ui32_t CalcSampleSize(const AudioDescriptor& ADesc)
	{
	  return (ADesc.QuantizationBits / 8) * ADesc.ChannelCount;
	}

      // Returns number of samples per frame of data described by ADesc
      inline ui32_t CalcSamplesPerFrame(const AudioDescriptor& ADesc)
	{
	  double tmpd = ADesc.AudioSamplingRate.Quotient() / ADesc.EditRate.Quotient();
	  return (ui32_t)ceil(tmpd);
	}

      // Returns the size in bytes of a frame of data described by ADesc
      inline ui32_t CalcFrameBufferSize(const AudioDescriptor& ADesc)
	{
	  return CalcSampleSize(ADesc) * CalcSamplesPerFrame(ADesc);
	}

      //
      class FrameBuffer : public ASDCP::FrameBuffer
	{
	public:
	  FrameBuffer() {}
	  FrameBuffer(ui32_t size) { Capacity(size); }
	  virtual ~FrameBuffer() {}

	  // Print debugging information to stream (stderr default)
	  void Dump(FILE* = 0, ui32_t dump_bytes = 0) const;
	};

      // An object which opens and reads a WAV file.  The call to OpenRead() reads metadata from
      // the file and populates an internal AudioDescriptor object. Each subsequent call to
      // ReadFrame() reads exactly one frame from the stream into the given FrameBuffer object.
      // A "frame" is either 2000 or 2002 samples, depending upon the value of PictureRate.
      class WAVParser
	{
	  class h__WAVParser;
	  mem_ptr<h__WAVParser> m_Parser;
	  ASDCP_NO_COPY_CONSTRUCT(WAVParser);

	public:
	  WAVParser();
	  virtual ~WAVParser();

	  // Opens the stream for reading, parses enough data to provide a complete
	  // set of stream metadata for the MXFWriter below. PictureRate controls
	  // ther frame rate for the MXF frame wrapping option.
	  Result_t OpenRead(const std::string& filename, const Rational& PictureRate) const;

	  // Fill an AudioDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillAudioDescriptor(AudioDescriptor&) const;

	  // Rewind the stream to the beginning.
	  Result_t Reset() const;

	  // Reads the next sequential frame in the input file and places it in the
	  // frame buffer. Fails if the buffer is too small or the stream is empty.
	  Result_t ReadFrame(FrameBuffer&) const;
	};


      //
      class MXFWriter
	{
	  class h__Writer;
	  mem_ptr<h__Writer> m_Writer;
	  ASDCP_NO_COPY_CONSTRUCT(MXFWriter);

	public:
	  MXFWriter();
	  virtual ~MXFWriter();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual MXF::OP1aHeader& OP1aHeader();
	  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
	  virtual MXF::RIP& RIP();

	  // Open the file for writing. The file must not exist. Returns error if
	  // the operation cannot be completed or if nonsensical data is discovered
	  // in the essence descriptor.
	  Result_t OpenWrite(const std::string& filename, const WriterInfo&,
			     const AudioDescriptor&, ui32_t HeaderSize = 16384);

	  // Writes a frame of essence to the MXF file. If the optional AESEncContext
	  // argument is present, the essence is encrypted prior to writing.
	  // Fails if the file is not open, is finalized, or an operating system
	  // error occurs.
	  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);

	  // Closes the MXF file, writing the index and revised header.
	  Result_t Finalize();
	};

      //
      class MXFReader
	{
	  class h__Reader;
	  mem_ptr<h__Reader> m_Reader;
	  ASDCP_NO_COPY_CONSTRUCT(MXFReader);

	public:
	  MXFReader();
	  virtual ~MXFReader();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual MXF::OP1aHeader& OP1aHeader();
	  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
	  virtual MXF::RIP& RIP();

	  // Open the file for reading. The file must exist. Returns error if the
	  // operation cannot be completed.
	  Result_t OpenRead(const std::string& filename) const;

	  // Returns RESULT_INIT if the file is not open.
	  Result_t Close() const;

	  // Fill an AudioDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillAudioDescriptor(AudioDescriptor&) const;

	  // Fill a WriterInfo struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillWriterInfo(WriterInfo&) const;

	  // Reads a frame of essence from the MXF file. If the optional AESEncContext
	  // argument is present, the essence is decrypted after reading. If the MXF
	  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
	  // will contain the ciphertext frame data. If the HMACContext argument is
	  // not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadFrame(ui32_t frame_number, FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Using the index table read from the footer partition, lookup the frame number
	  // and return the offset into the file at which to read that frame of essence.
	  // Returns RESULT_INIT if the file is not open, and RESULT_RANGE if the frame number is
	  // out of range.
	  Result_t LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset, i8_t& temporalOffset, i8_t& keyFrameOffset) const;

	  // Print debugging information to stream
	  void     DumpHeaderMetadata(FILE* = 0) const;
	  void     DumpIndex(FILE* = 0) const;
	};
    } // namespace PCM

  //---------------------------------------------------------------------------------
  //
  namespace JP2K
    {
      const ui32_t MaxComponents = 3;
      const ui32_t MaxPrecincts = 32; // ISO 15444-1 Annex A.6.1
      const ui32_t MaxDefaults = 256; // made up

#pragma pack(1)
      struct ImageComponent_t  // ISO 15444-1 Annex A.5.1
      {
	ui8_t Ssize;
	ui8_t XRsize;
	ui8_t YRsize;
      };

      struct CodingStyleDefault_t // ISO 15444-1 Annex A.6.1
      {
	ui8_t   Scod;

	struct
	{
	  ui8_t  ProgressionOrder;
	  ui8_t  NumberOfLayers[sizeof(ui16_t)];
	  ui8_t  MultiCompTransform;
	} SGcod;

	struct
	{
	  ui8_t  DecompositionLevels;
	  ui8_t  CodeblockWidth;
	  ui8_t  CodeblockHeight;
	  ui8_t  CodeblockStyle;
	  ui8_t  Transformation;
	  ui8_t  PrecinctSize[MaxPrecincts];
	} SPcod;
      };

      struct QuantizationDefault_t // ISO 15444-1 Annex A.6.4
      {
	ui8_t  Sqcd;
	ui8_t  SPqcd[MaxDefaults];
	ui8_t  SPqcdLength;
      };
#pragma pack()

      struct PictureDescriptor
      {
	Rational       EditRate;
	ui32_t         ContainerDuration;
	Rational       SampleRate;
	ui32_t         StoredWidth;
	ui32_t         StoredHeight;
	Rational       AspectRatio;
	ui16_t         Rsize;
	ui32_t         Xsize;
	ui32_t         Ysize;
	ui32_t         XOsize;
	ui32_t         YOsize;
	ui32_t         XTsize;
	ui32_t         YTsize;
	ui32_t         XTOsize;
	ui32_t         YTOsize;
	ui16_t         Csize;
	ImageComponent_t      ImageComponents[MaxComponents];
	CodingStyleDefault_t  CodingStyleDefault;
	QuantizationDefault_t QuantizationDefault;
      };

      // Print debugging information to std::ostream
      std::ostream& operator << (std::ostream& strm, const PictureDescriptor& pdesc);
      // Print debugging information to stream (stderr default)
      void   PictureDescriptorDump(const PictureDescriptor&, FILE* = 0);

      //
      class FrameBuffer : public ASDCP::FrameBuffer
	{
	public:
	  FrameBuffer() {}
	  FrameBuffer(ui32_t size) { Capacity(size); }
	  virtual ~FrameBuffer() {}

	  // Print debugging information to stream (stderr default)
	  void Dump(FILE* = 0, ui32_t dump_bytes = 0) const;
	};


      // An object which opens and reads a JPEG 2000 codestream file.  The file is expected
      // to contain exactly one complete frame of picture essence as an unwrapped (raw)
      // ISO/IEC 15444 codestream.
      class CodestreamParser
	{
	  class h__CodestreamParser;
	  mem_ptr<h__CodestreamParser> m_Parser;
	  ASDCP_NO_COPY_CONSTRUCT(CodestreamParser);

	public:
	  CodestreamParser();
	  virtual ~CodestreamParser();

	  // Opens a file for reading, parses enough data to provide a complete
          // set of stream metadata for the MXFWriter below.
	  // The frame buffer's PlaintextOffset parameter will be set to the first
	  // byte of the data segment. Set this value to zero if you want
	  // encrypted headers.
	  Result_t OpenReadFrame(const std::string& filename, FrameBuffer&) const;

	  // Fill a PictureDescriptor struct with the values from the file's codestream.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillPictureDescriptor(PictureDescriptor&) const;
	};

      // Parses the data in the frame buffer to fill in the picture descriptor. Copies
      // the offset of the image data into start_of_data. Returns error if the parser fails.
      Result_t ParseMetadataIntoDesc(const FrameBuffer&, PictureDescriptor&, byte_t* start_of_data = 0);

      // An object which reads a sequence of files containing JPEG 2000 pictures.
      class SequenceParser
	{
	  class h__SequenceParser;
	  mem_ptr<h__SequenceParser> m_Parser;
	  ASDCP_NO_COPY_CONSTRUCT(SequenceParser);

	public:
	  SequenceParser();
	  virtual ~SequenceParser();

	  // Opens a directory for reading.  The directory is expected to contain one or
	  // more files, each containing the codestream for exactly one picture. The
	  // files must be named such that the frames are in temporal order when sorted
	  // alphabetically by filename. The parser will automatically parse enough data
	  // from the first file to provide a complete set of stream metadata for the
	  // MXFWriter below.  If the "pedantic" parameter is given and is true, the
	  // parser will check the metadata for each codestream and fail if a
	  // mismatch is detected.
	  Result_t OpenRead(const std::string& filename, bool pedantic = false) const;

	  // Opens a file sequence for reading.  The sequence is expected to contain one or
	  // more filenames, each naming a file containing the codestream for exactly one
	  // picture. The parser will automatically parse enough data
	  // from the first file to provide a complete set of stream metadata for the
	  // MXFWriter below.  If the "pedantic" parameter is given and is true, the
	  // parser will check the metadata for each codestream and fail if a
	  // mismatch is detected.
	  Result_t OpenRead(const std::list<std::string>& file_list, bool pedantic = false) const;

	  // Fill a PictureDescriptor struct with the values from the first file's codestream.
	  // Returns RESULT_INIT if the directory is not open.
	  Result_t FillPictureDescriptor(PictureDescriptor&) const;

	  // Rewind the directory to the beginning.
	  Result_t Reset() const;

	  // Reads the next sequential frame in the directory and places it in the
	  // frame buffer. Fails if the buffer is too small or the direcdtory
	  // contains no more files.
	  // The frame buffer's PlaintextOffset parameter will be set to the first
	  // byte of the data segment. Set this value to zero if you want
	  // encrypted headers.
	  Result_t ReadFrame(FrameBuffer&) const;
	};


      //
      class MXFWriter
	{
	  class h__Writer;
	  mem_ptr<h__Writer> m_Writer;
	  ASDCP_NO_COPY_CONSTRUCT(MXFWriter);

	public:
	  MXFWriter();
	  virtual ~MXFWriter();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual MXF::OP1aHeader& OP1aHeader();
	  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
	  virtual MXF::RIP& RIP();

	  // Open the file for writing. The file must not exist. Returns error if
	  // the operation cannot be completed or if nonsensical data is discovered
	  // in the essence descriptor.
	  Result_t OpenWrite(const std::string& filename, const WriterInfo&,
			     const PictureDescriptor&, ui32_t HeaderSize = 16384);

	  // Writes a frame of essence to the MXF file. If the optional AESEncContext
	  // argument is present, the essence is encrypted prior to writing.
	  // Fails if the file is not open, is finalized, or an operating system
	  // error occurs.
	  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);

	  // Closes the MXF file, writing the index and revised header.
	  Result_t Finalize();
	};

      //
      class MXFReader
	{
	  class h__Reader;
	  mem_ptr<h__Reader> m_Reader;
	  ASDCP_NO_COPY_CONSTRUCT(MXFReader);

	public:
	  MXFReader();
	  virtual ~MXFReader();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual MXF::OP1aHeader& OP1aHeader();
	  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
	  virtual MXF::RIP& RIP();

	  // Open the file for reading. The file must exist. Returns error if the
	  // operation cannot be completed.
	  Result_t OpenRead(const std::string& filename) const;

	  // Returns RESULT_INIT if the file is not open.
	  Result_t Close() const;

	  // Fill an AudioDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillPictureDescriptor(PictureDescriptor&) const;

	  // Fill a WriterInfo struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillWriterInfo(WriterInfo&) const;

	  // Reads a frame of essence from the MXF file. If the optional AESEncContext
	  // argument is present, the essence is decrypted after reading. If the MXF
	  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
	  // will contain the ciphertext frame data. If the HMACContext argument is
	  // not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadFrame(ui32_t frame_number, FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Using the index table read from the footer partition, lookup the frame number
	  // and return the offset into the file at which to read that frame of essence.
	  // Returns RESULT_INIT if the file is not open, and RESULT_FRAME if the frame number is
	  // out of range.
	  Result_t LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset, i8_t& temporalOffset, i8_t& keyFrameOffset) const;

	  // Print debugging information to stream
	  void     DumpHeaderMetadata(FILE* = 0) const;
	  void     DumpIndex(FILE* = 0) const;
	};


      // Stereoscopic Image support
      //

      enum StereoscopicPhase_t
      {
	SP_LEFT,
	SP_RIGHT
      };

      struct SFrameBuffer
      {
	JP2K::FrameBuffer Left;
	JP2K::FrameBuffer Right;

	SFrameBuffer(ui32_t size) {
	  Left.Capacity(size);
	  Right.Capacity(size);
	}
      };

      class MXFSWriter
      {
	  class h__SWriter;
	  mem_ptr<h__SWriter> m_Writer;
	  ASDCP_NO_COPY_CONSTRUCT(MXFSWriter);

	public:
	  MXFSWriter();
	  virtual ~MXFSWriter();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual MXF::OP1aHeader& OP1aHeader();
	  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
	  virtual MXF::RIP& RIP();

	  // Open the file for writing. The file must not exist. Returns error if
	  // the operation cannot be completed or if nonsensical data is discovered
	  // in the essence descriptor.
	  Result_t OpenWrite(const std::string& filename, const WriterInfo&,
			     const PictureDescriptor&, ui32_t HeaderSize = 16384);

	  // Writes a pair of frames of essence to the MXF file. If the optional AESEncContext
	  // argument is present, the essence is encrypted prior to writing.
	  // Fails if the file is not open, is finalized, or an operating system
	  // error occurs.
	  Result_t WriteFrame(const SFrameBuffer&, AESEncContext* = 0, HMACContext* = 0);

	  // Writes a frame of essence to the MXF file. If the optional AESEncContext
	  // argument is present, the essence is encrypted prior to writing.
	  // Fails if the file is not open, is finalized, or an operating system
	  // error occurs. Frames must be written in the proper phase (L-R-L-R),
	  // RESULT_SPHASE will be returned if phase is reversed. The first frame
	  // written must be left eye.
	  Result_t WriteFrame(const FrameBuffer&, StereoscopicPhase_t phase,
			      AESEncContext* = 0, HMACContext* = 0);

	  // Closes the MXF file, writing the index and revised header.  Returns
	  // RESULT_SPHASE if WriteFrame was called an odd number of times.
	  Result_t Finalize();
	};

      //
      class MXFSReader
	{
	  class h__SReader;
	  mem_ptr<h__SReader> m_Reader;
	  ASDCP_NO_COPY_CONSTRUCT(MXFSReader);

	public:
	  MXFSReader();
	  virtual ~MXFSReader();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual MXF::OP1aHeader& OP1aHeader();
	  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
	  virtual MXF::RIP& RIP();

	  // Open the file for reading. The file must exist. Returns error if the
	  // operation cannot be completed.
	  Result_t OpenRead(const std::string& filename) const;

	  // Returns RESULT_INIT if the file is not open.
	  Result_t Close() const;

	  // Fill an AudioDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillPictureDescriptor(PictureDescriptor&) const;

	  // Fill a WriterInfo struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillWriterInfo(WriterInfo&) const;

	  // Reads a pair of frames of essence from the MXF file. If the optional AESEncContext
	  // argument is present, the essence is decrypted after reading. If the MXF
	  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
	  // will contain the ciphertext frame data. If the HMACContext argument is
	  // not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadFrame(ui32_t frame_number, SFrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Reads a frame of essence from the MXF file. If the optional AESEncContext
	  // argument is present, the essence is decrypted after reading. If the MXF
	  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
	  // will contain the ciphertext frame data. If the HMACContext argument is
	  // not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadFrame(ui32_t frame_number, StereoscopicPhase_t phase,
			     FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Using the index table read from the footer partition, lookup the frame number
	  // and return the offset into the file at which to read that frame of essence.
	  // Returns RESULT_INIT if the file is not open, and RESULT_FRAME if the frame number is
	  // out of range.
	  Result_t LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset, i8_t& temporalOffset, i8_t& keyFrameOffset) const;

	  // Print debugging information to stream
	  void     DumpHeaderMetadata(FILE* = 0) const;
	  void     DumpIndex(FILE* = 0) const;
	};
    } // namespace JP2K

  //---------------------------------------------------------------------------------
  //
  namespace TimedText
    {
      enum MIMEType_t { MT_BIN, MT_PNG, MT_OPENTYPE };

      struct TimedTextResourceDescriptor
      {
	byte_t      ResourceID[UUIDlen];
          MIMEType_t  Type;

        TimedTextResourceDescriptor() : Type(MT_BIN) {}
      };

      typedef std::list<TimedTextResourceDescriptor> ResourceList_t;

      struct TimedTextDescriptor
      {
	Rational       EditRate;                //
	ui32_t         ContainerDuration;
	byte_t         AssetID[UUIDlen];
	std::string    NamespaceName;
	std::string    EncodingName;
	ResourceList_t ResourceList;

      TimedTextDescriptor() : ContainerDuration(0), EncodingName("UTF-8") {} // D-Cinema format is always UTF-8
      };

      // Print debugging information to std::ostream
      std::ostream& operator << (std::ostream& strm, const TimedTextDescriptor& tinfo);
      // Print debugging information to stream (stderr default)
      void   DescriptorDump(const TimedTextDescriptor&, FILE* = 0);

      //
      class FrameBuffer : public ASDCP::FrameBuffer
      {
	ASDCP_NO_COPY_CONSTRUCT(FrameBuffer); // TODO: should have copy construct

      protected:
	byte_t      m_AssetID[UUIDlen];
	std::string m_MIMEType;

      public:
	FrameBuffer() { memset(m_AssetID, 0, UUIDlen); }
	FrameBuffer(ui32_t size) { Capacity(size); memset(m_AssetID, 0, UUIDlen); }
	virtual ~FrameBuffer() {}

	inline const byte_t* AssetID() const { return m_AssetID; }
	inline void          AssetID(const byte_t* buf) { memcpy(m_AssetID, buf, UUIDlen); }
	inline const char*   MIMEType() const { return m_MIMEType.c_str(); }
	inline void          MIMEType(const std::string& s) { m_MIMEType = s; }

	// Print debugging information to stream (stderr default)
	void Dump(FILE* = 0, ui32_t dump_bytes = 0) const;
      };

      // An abstract base for a lookup service that returns the resource data
      // identified by the given ancillary resource id.
      //
      class IResourceResolver
      {
      public:
	virtual ~IResourceResolver() {}
	virtual Result_t ResolveRID(const byte_t* uuid, FrameBuffer&) const = 0; // return data for RID
      };

      // Resolves resource references by testing the named directory for file names containing
      // the respective UUID.
      //
      class LocalFilenameResolver : public ASDCP::TimedText::IResourceResolver
	{
	  std::string m_Dirname;
	  ASDCP_NO_COPY_CONSTRUCT(LocalFilenameResolver);

	public:
	  LocalFilenameResolver();
	  virtual ~LocalFilenameResolver();
	  Result_t OpenRead(const std::string& dirname);
	  Result_t ResolveRID(const byte_t* uuid, FrameBuffer& FrameBuf) const;
	};

      //
      class DCSubtitleParser
	{
	  class h__SubtitleParser;
	  mem_ptr<h__SubtitleParser> m_Parser;
	  ASDCP_NO_COPY_CONSTRUCT(DCSubtitleParser);

	public:
	  DCSubtitleParser();
	  virtual ~DCSubtitleParser();

	  // Opens an XML file for reading, parses data to provide a complete
	  // set of stream metadata for the MXFWriter below.
	  Result_t OpenRead(const std::string& filename) const;

	  // Parses an XML document to provide a complete set of stream metadata
	  // for the MXFWriter below. The optional filename argument is used to
	  // initialize the default resource resolver (see ReadAncillaryResource).
	  Result_t OpenRead(const std::string& xml_doc, const std::string& filename) const;

	  // Fill a TimedTextDescriptor struct with the values from the file's contents.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillTimedTextDescriptor(TimedTextDescriptor&) const;

	  // Reads the complete Timed Text Resource into the given string.
	  Result_t ReadTimedTextResource(std::string&) const;

	  // Reads the Ancillary Resource having the given ID. Fails if the buffer
	  // is too small or the resource does not exist. The optional Resolver
	  // argument can be provided which will be used to retrieve the resource
	  // having a particulat UUID. If a Resolver is not supplied, the default
	  // internal resolver will return the contents of the file having the UUID
	  // as the filename. The filename must exist in the same directory as the
	  // XML file opened with OpenRead().
	  Result_t ReadAncillaryResource(const byte_t* uuid, FrameBuffer&,
					 const IResourceResolver* Resolver = 0) const;
	};

      //
      class MXFWriter
	{
	  class h__Writer;
	  mem_ptr<h__Writer> m_Writer;
	  ASDCP_NO_COPY_CONSTRUCT(MXFWriter);

	public:
	  MXFWriter();
	  virtual ~MXFWriter();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual MXF::OP1aHeader& OP1aHeader();
	  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
	  virtual MXF::RIP& RIP();

	  // Open the file for writing. The file must not exist. Returns error if
	  // the operation cannot be completed or if nonsensical data is discovered
	  // in the essence descriptor.
	  Result_t OpenWrite(const std::string& filename, const WriterInfo&,
			     const TimedTextDescriptor&, ui32_t HeaderSize = 16384);

	  // Writes the Timed-Text Resource to the MXF file. The file must be UTF-8
	  // encoded. If the optional AESEncContext argument is present, the essence
	  // is encrypted prior to writing. Fails if the file is not open, is finalized,
	  // or an operating system error occurs.
	  // This method may only be called once, and it must be called before any
	  // call to WriteAncillaryResource(). RESULT_STATE will be returned if these
	  // conditions are not met.
	  Result_t WriteTimedTextResource(const std::string& XMLDoc, AESEncContext* = 0, HMACContext* = 0);

	  // Writes an Ancillary Resource to the MXF file. If the optional AESEncContext
	  // argument is present, the essence is encrypted prior to writing.
	  // Fails if the file is not open, is finalized, or an operating system
	  // error occurs. RESULT_STATE will be returned if the method is called before
	  // WriteTimedTextResource()
	  Result_t WriteAncillaryResource(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);

	  // Closes the MXF file, writing the index and revised header.
	  Result_t Finalize();
	};

      //
      class MXFReader
	{
	  class h__Reader;
	  mem_ptr<h__Reader> m_Reader;
	  ASDCP_NO_COPY_CONSTRUCT(MXFReader);

	public:
	  MXFReader();
	  virtual ~MXFReader();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual MXF::OP1aHeader& OP1aHeader();
	  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
	  virtual MXF::RIP& RIP();

	  // Open the file for reading. The file must exist. Returns error if the
	  // operation cannot be completed.
	  Result_t OpenRead(const std::string& filename) const;

	  // Returns RESULT_INIT if the file is not open.
	  Result_t Close() const;

	  // Fill a TimedTextDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillTimedTextDescriptor(TimedTextDescriptor&) const;

	  // Fill a WriterInfo struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillWriterInfo(WriterInfo&) const;

	  // Reads the complete Timed Text Resource into the given string. Fails if the resource
	  // is encrypted and AESDecContext is NULL (use the following method to retrieve the
	  // raw ciphertet block).
	  Result_t ReadTimedTextResource(std::string&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Reads the complete Timed Text Resource from the MXF file. If the optional AESEncContext
	  // argument is present, the resource is decrypted after reading. If the MXF
	  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
	  // will contain the ciphertext frame data. If the HMACContext argument is
	  // not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadTimedTextResource(FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Reads the timed-text resource having the given UUID from the MXF file. If the
	  // optional AESEncContext argument is present, the resource is decrypted after
	  // reading. If the MXF file is encrypted and the AESDecContext argument is NULL,
	  // the frame buffer will contain the ciphertext frame data. If the HMACContext
	  // argument is not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadAncillaryResource(const byte_t* uuid, FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Print debugging information to stream
	  void     DumpHeaderMetadata(FILE* = 0) const;
	  void     DumpIndex(FILE* = 0) const;
	};
    } // namespace TimedText

  //---------------------------------------------------------------------------------
  //
  namespace DCData
  {
    struct DCDataDescriptor
    {
      Rational EditRate;                 // Sample rate
      ui32_t   ContainerDuration;          // number of frames
      byte_t   AssetID[UUIDlen];           // The UUID for the DCData track
      byte_t   DataEssenceCoding[UUIDlen]; // The coding for the data carried
    };

    // Print DCDataDescriptor to std::ostream
    std::ostream& operator << (std::ostream& strm, const DCDataDescriptor& ddesc);
    // Print debugging information to stream (stderr default)
    void DCDataDescriptorDump(const DCDataDescriptor&, FILE* = 0);

    //
    class FrameBuffer : public ASDCP::FrameBuffer
	{
     public:
	  FrameBuffer() {}
	  FrameBuffer(ui32_t size) { Capacity(size); }
	  virtual ~FrameBuffer() {}

	  // Print debugging information to stream (stderr default)
	  void Dump(FILE* = 0, ui32_t dump_bytes = 0) const;
	};

    // An object which opens and reads a DC Data file.  The file is expected
    // to contain exactly one complete frame of DC data essence as an unwrapped (raw)
    // byte stream.
    class BytestreamParser
	{
	  class h__BytestreamParser;
	  mem_ptr<h__BytestreamParser> m_Parser;
	  ASDCP_NO_COPY_CONSTRUCT(BytestreamParser);

     public:
	  BytestreamParser();
	  virtual ~BytestreamParser();

	  // Opens a file for reading, parses enough data to provide a complete
      // set of stream metadata for the MXFWriter below.
	  // The frame buffer's PlaintextOffset parameter will be set to the first
	  // byte of the data segment. Set this value to zero if you want
	  // encrypted headers.
	  Result_t OpenReadFrame(const std::string& filename, FrameBuffer&) const;

	  // Fill a DCDataDescriptor struct with the values from the file's bytestream.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillDCDataDescriptor(DCDataDescriptor&) const;
	};

    // An object which reads a sequence of files containing DC Data.
    class SequenceParser
	{
	  class h__SequenceParser;
	  mem_ptr<h__SequenceParser> m_Parser;
	  ASDCP_NO_COPY_CONSTRUCT(SequenceParser);

     public:
	  SequenceParser();
	  virtual ~SequenceParser();

	  // Opens a directory for reading.  The directory is expected to contain one or
	  // more files, each containing the bytestream for exactly one frame. The files
      // must be named such that the frames are in temporal order when sorted
	  // alphabetically by filename.
	  Result_t OpenRead(const std::string& filename) const;

	  // Opens a file sequence for reading.  The sequence is expected to contain one or
	  // more filenames, each naming a file containing the bytestream for exactly one
	  // frame.
	  Result_t OpenRead(const std::list<std::string>& file_list) const;

	  // Fill a DCDataDescriptor struct with default values.
	  // Returns RESULT_INIT if the directory is not open.
	  Result_t FillDCDataDescriptor(DCDataDescriptor&) const;

	  // Rewind the directory to the beginning.
	  Result_t Reset() const;

	  // Reads the next sequential frame in the directory and places it in the
	  // frame buffer. Fails if the buffer is too small or the direcdtory
	  // contains no more files.
	  // The frame buffer's PlaintextOffset parameter will be set to the first
	  // byte of the data segment. Set this value to zero if you want
	  // encrypted headers.
	  Result_t ReadFrame(FrameBuffer&) const;
	};

    //
    class MXFWriter
	{
	  class h__Writer;
	  mem_ptr<h__Writer> m_Writer;
	  ASDCP_NO_COPY_CONSTRUCT(MXFWriter);

     public:
	  MXFWriter();
	  virtual ~MXFWriter();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual MXF::OP1aHeader& OP1aHeader();
	  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
	  virtual MXF::RIP& RIP();

	  // Open the file for writing. The file must not exist. Returns error if
	  // the operation cannot be completed or if nonsensical data is discovered
	  // in the essence descriptor.
	  Result_t OpenWrite(const std::string& filename, const WriterInfo&,
			     const DCDataDescriptor&, ui32_t HeaderSize = 16384);

	  // Writes a frame of essence to the MXF file. If the optional AESEncContext
	  // argument is present, the essence is encrypted prior to writing.
	  // Fails if the file is not open, is finalized, or an operating system
	  // error occurs.
	  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);

	  // Closes the MXF file, writing the index and revised header.
	  Result_t Finalize();
	};

    //
    class MXFReader
	{
	  class h__Reader;
	  mem_ptr<h__Reader> m_Reader;
	  ASDCP_NO_COPY_CONSTRUCT(MXFReader);

     public:
	  MXFReader();
	  virtual ~MXFReader();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual MXF::OP1aHeader& OP1aHeader();
	  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
	  virtual MXF::RIP& RIP();

	  // Open the file for reading. The file must exist. Returns error if the
	  // operation cannot be completed.
	  Result_t OpenRead(const std::string& filename) const;

	  // Returns RESULT_INIT if the file is not open.
	  Result_t Close() const;

	  // Fill a DCDataDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillDCDataDescriptor(DCDataDescriptor&) const;

	  // Fill a WriterInfo struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillWriterInfo(WriterInfo&) const;

	  // Reads a frame of essence from the MXF file. If the optional AESEncContext
	  // argument is present, the essence is decrypted after reading. If the MXF
	  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
	  // will contain the ciphertext frame data. If the HMACContext argument is
	  // not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadFrame(ui32_t frame_number, FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Using the index table read from the footer partition, lookup the frame number
	  // and return the offset into the file at which to read that frame of essence.
	  // Returns RESULT_INIT if the file is not open, and RESULT_RANGE if the frame number is
	  // out of range.
	  Result_t LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset, i8_t& temporalOffset, i8_t& keyFrameOffset) const;

	  // Print debugging information to stream
	  void     DumpHeaderMetadata(FILE* = 0) const;
	  void     DumpIndex(FILE* = 0) const;
	};

  } // namespace DCData

  //---------------------------------------------------------------------------------
  //
  namespace ATMOS
  {
    struct AtmosDescriptor : public DCData::DCDataDescriptor
    {
      ui32_t FirstFrame;       // Frame number of the frame to align with the FFOA of the picture track
      ui16_t MaxChannelCount;  // Max number of channels in bitstream
      ui16_t MaxObjectCount;   // Max number of objects in bitstream
      byte_t AtmosID[UUIDlen]; // UUID of Atmos Project
      ui8_t  AtmosVersion;     // ATMOS Coder Version used to create bitstream
    };

    // Print AtmosDescriptor to std::ostream
    std::ostream& operator << (std::ostream& strm, const AtmosDescriptor& adesc);
    // Print debugging information to stream (stderr default)
    void AtmosDescriptorDump(const AtmosDescriptor&, FILE* = 0);
    // Determine if a file is a raw atmos file
    bool IsDolbyAtmos(const std::string& filename);

    //
    class MXFWriter
	{

      class h__Writer;
	  mem_ptr<h__Writer> m_Writer;
	  ASDCP_NO_COPY_CONSTRUCT(MXFWriter);

     public:
	  MXFWriter();
	  virtual ~MXFWriter();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual MXF::OP1aHeader& OP1aHeader();
	  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
	  virtual MXF::RIP& RIP();

	  // Open the file for writing. The file must not exist. Returns error if
	  // the operation cannot be completed or if nonsensical data is discovered
	  // in the essence descriptor.
	  Result_t OpenWrite(const std::string& filename, const WriterInfo&,
			     const AtmosDescriptor&, ui32_t HeaderSize = 16384);

	  // Writes a frame of essence to the MXF file. If the optional AESEncContext
	  // argument is present, the essence is encrypted prior to writing.
	  // Fails if the file is not open, is finalized, or an operating system
	  // error occurs.
      Result_t WriteFrame(const DCData::FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);

	  // Closes the MXF file, writing the index and revised header.
	  Result_t Finalize();
	};

    //
    class MXFReader
	{
      class h__Reader;
	  mem_ptr<h__Reader> m_Reader;
	  ASDCP_NO_COPY_CONSTRUCT(MXFReader);

     public:
	  MXFReader();
	  virtual ~MXFReader();

	  // Warning: direct manipulation of MXF structures can interfere
	  // with the normal operation of the wrapper.  Caveat emptor!
	  virtual MXF::OP1aHeader& OP1aHeader();
	  virtual MXF::OPAtomIndexFooter& OPAtomIndexFooter();
	  virtual MXF::RIP& RIP();

	  // Open the file for reading. The file must exist. Returns error if the
	  // operation cannot be completed.
	  Result_t OpenRead(const std::string& filename) const;

	  // Returns RESULT_INIT if the file is not open.
	  Result_t Close() const;

	  // Fill an AtmosDescriptor struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillAtmosDescriptor(AtmosDescriptor&) const;

	  // Fill a WriterInfo struct with the values from the file's header.
	  // Returns RESULT_INIT if the file is not open.
	  Result_t FillWriterInfo(WriterInfo&) const;

	  // Reads a frame of essence from the MXF file. If the optional AESEncContext
	  // argument is present, the essence is decrypted after reading. If the MXF
	  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
	  // will contain the ciphertext frame data. If the HMACContext argument is
	  // not NULL, the HMAC will be calculated (if the file supports it).
	  // Returns RESULT_INIT if the file is not open, failure if the frame number is
	  // out of range, or if optional decrypt or HAMC operations fail.
	  Result_t ReadFrame(ui32_t frame_number, DCData::FrameBuffer&, AESDecContext* = 0, HMACContext* = 0) const;

	  // Using the index table read from the footer partition, lookup the frame number
	  // and return the offset into the file at which to read that frame of essence.
	  // Returns RESULT_INIT if the file is not open, and RESULT_RANGE if the frame number is
	  // out of range.
	  Result_t LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset, i8_t& temporalOffset, i8_t& keyFrameOffset) const;

	  // Print debugging information to stream
	  void     DumpHeaderMetadata(FILE* = 0) const;
	  void     DumpIndex(FILE* = 0) const;
	};

  } // namespace ATMOS



} // namespace ASDCP


#endif // _AS_DCP_H_

//
// end AS_DCP.h
//
