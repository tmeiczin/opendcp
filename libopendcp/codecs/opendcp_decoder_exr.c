/* opendcp_decoder_exr.c */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "opendcp.h"
#include "opendcp_image.h"

#define MAGIC_NUMBER_EXR 0x762f3101

typedef enum {
    EXR_TOP         = 0,
    EXR_BOTTOM      = 1,
} line_order;

typedef enum {
    EXR_NO       = 0,          /* no compression                  */
    EXR_RLE      = 1,          /* 8-bit run-length-encoded (not supported) */
    EXR_ZIPS     = 2,          /* zip single line (not supported) */
    EXR_ZIP      = 3,          /* zip 16 lines                    */
    EXR_PIZ      = 4,          /* piz (not supported)             */
    EXR_PXR24    = 5           /* pixar 24 bit (not supported)    */
    EXR_B44      = 6           /* b44 (not supported)             */
    EXR_B44A     = 7           /* b44a (not supported)            */
    EXR_DWAA     = 8            /* dwaa 32 lines (not supported)  */
    EXR_DWAB     = 9            /* dwab 256 lines (not supported) */
} exr_compression_enum;

typedef struct {
    uint32_t magic_num;         /* magic number 0x762f3101 (file signature) */
} exr_magic_num_t;

typedef struct {
    uint16 num_channel;
} exr_channel_list;

typedef struct {
    int32_t  width;              /* width of image                */
    int32_t  height;             /* height of image               */
} exr_image_header_t;

typedef struct {
    exr_image_header_t       image;
    int                      row_order;
} exr_image_t;

#pragma mark ---- Half --> Float
/* for change half become float */
typedef union {
    unsigned int i;
    float f;
} u_intfloat;

/* shift for not normal numbers */
unsigned char shiftForNumber( unsigned short value ) {
 
    unsigned char shift = 0;
  
   if( value < 1 )   // never see this case
      shift = 0;
   else if( value < 2 )
      shift = 1;
   else if( value < 4 )
      shift = 2;
   else if( value < 8 )
      shift = 3;
   else if( value < 16 )
      shift = 4;
   else if( value < 32 )
      shift = 5;
   else if( value < 64 )
      shift = 6;
   else if( value < 128 )
      shift = 7;
   else if( value < 256 )
      shift = 8;
   else if( value < 512 )
      shift = 9;
   else if( value < 1024 )
      shift = 10;
   // never see case after 1024
    
   return shift;
}

float half2float( unsigned short half ) {
    
   u_intfloat int_fl;

   // normal number +
   if( (half > 0x03ff) && (half < 0x7c00) ) {
      unsigned int exponent = (((half & 0x7c00) >> 10) + 112) << 23;
      unsigned int value = (half & 0x03ff) << 13;
      int_fl.i = exponent | value;
   }
   // normal number -
   else if( (half > 0x83ff) && (half < 0xfc00) ) {
      unsigned int exponent = (((half & 0x7c00) >> 10) + 112) << 23;
      unsigned int value = (half & 0x03ff) << 13;
      int_fl.i = 0x80000000 + exponent + value;
   }
   // not normal number +
   else if( (half > 0x0000) && (half < 0x0400) ) {
      unsigned char shift = shiftForNumber( half );
      unsigned int exponent = (shift + 102) << 23;
      unsigned int value = (half << (24 - shift)) & 0x007fffff;
      int_fl.i = exponent + value;
    }
    // not normal number -
    else if( (half > 0x8000) && (half < 0x8400) ) {
       unsigned char shift = shiftForNumber( half & 0x7fff );
       unsigned int exponent = (shift + 102) << 23;
       unsigned int value = (half << (24 - shift)) & 0x007fffff;
      int_fl.i = 0x80000000 + exponent + value;
   }
   // zero +
   else if( half == 0x0000 )
      int_fl.i = 0x00000000;
 
   // zero -
   else if( half == 0x8000 )
      int_fl.i = 0x80000000;
 
   // infinity +
   else if( half == 0x7c00 ) 
      int_fl.i = 0x7f800000;
  
   // infinity -
   else if( half == 0xfc00 )
      int_fl.i = 0xff800000;

   // NaN +
   else if( (half > 0x7c00) && (half < 0x8000) ) 
      int_fl.i = ((half & 0x3ff) << 13) | 0x7f800000;
 
   // NaN -
   else if( half > 0xfc00 )
      int_fl.i = ((half & 0x3ff) << 13) | 0xff800000;

   return int_fl.f;
}

#pragma mark ---- Read EXR File
int opendcp_decode_exr(opendcp_image_t **image_ptr, const char *sfile) {

   FILE *exr_fp;
    
      /* open exr using filename or file descriptor */
   OPENDCP_LOG(LOG_DEBUG,"%-15.15s: opening exr file %s","read_exr",sfile);

   exr_fp = fopen(sfile, "rb");
    
   if (!exr_fp) {
      OPENDCP_LOG(LOG_ERROR,"%-15.15s: opening exr file %s","read_bmp",sfile);
      return OPENDCP_FATAL;
   }
    
   // ---- check magic number/file signature
   unsigned int magicNumber = 0x0;
   // ---- careful about endian 
   magicNumber = fgetc(exr_fp) << 24;
   magicNumber |= fgetc(exr_fp) << 16;
   magicNumber |= fgetc(exr_fp) << 8;
   magicNumber |= fgetc(exr_fp);
    
   if (readsize != MAGIC_NUMBER_EXR ) {
      OPENDCP_LOG(LOG_ERROR,"%-15.15s: failed to read magic number expected 0x%08x read 0x%08x","read_exr", MAGIC_NUMBER_EXR, magicNumber );
      OPENDCP_LOG(LOG_ERROR,"%s is not a valid EXR file", sfile);
      return OPENDCP_FATAL;
   }
   
   // ---- version
   unsigned char version = fgetc(exr_fp);
   if( version != 2 ) {
     OPENDCP_LOG(LOG_ERROR,"Only support exr file version 2, this file version %d", version);
     return OPENDCP_FATAL;
   }

   // ---- file type: normal, deep pixel, multipart (only support normal)
   unsigned char type = fgetc(exr_fp);
   if( type & 0x1b != 0x00 ) {
      OPENDCP_LOG(LOG_ERROR,"Only support normal scanline exr file, no tile, deep pixel, multipart file");
     return OPENDCP_FATAL;
   }
   
   // ---- skip two bytes, not used
   fgetc(exr_fp);
   fgetc(exr_fp);
  
   // ---- read standard EXR attritubes
   
       
   // ---- 
}
