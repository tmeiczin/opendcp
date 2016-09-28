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

typedef enum {
    EXR_UINT      = 0,          /* unsigned int 32 bit, never use for color data */
    EXR_HALF      = 1,          /* half, 16 bit floating point */
    EXR_FLOAT     = 2,          /* float, 32 bit floating point */
} exr_dataType_enum;

typedef struct {
    char channelName[255];
    unsigned char dataType;    /* channel data type, int, half, float               */
    unsigned char non_linear;  /* non linear, only use for B44 and B44A compression */
    unsigned int sample_x;     /* sample x direction, only support == 1             */
    unsigned int sample_y;     /* sample y direction, only support == 1             */
    unsigned int offset;       /* position in image data                            */
} exr_channel;

typedef struct {
    unsigned short num_channels; /* number channels       */
    exr_channel channel[3];      /* channel array, only read data for B, G, R channel */
} exr_channel_list;

typedef struct {
    int left;
    int right;
    int bottom;
    int top;
} exr_window;

/* exr attributes, only save data for create dip */
typedef struct {
    exr_channel_list channel_list;    /* channel list */
    unsigned char compression;        /* compression */
    exr_window dataWindow;            /* data window */
    unsigned char lineOrder;          /* line order */
    unsigned int  width;              /* width of image - calculate */
    unsigned int  height;             /* height of image - calculate */
} exr_attributes;


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


#pragma mark ---- Read Attributes
unsigned char readString255FromFile( FILE *fp, char *string ) {

   unsigned char index = 0;
   do {
     string[index] = fgetc( fp );
     index++;
   } while( string[index-1] != 0x00 );

   return index-1;
}

exr_channel_list readChannelData( FILE *exr_fp) {
   
   exr_channel_list channel_list;
   unsigned char finish = 0x00;
   channel_list.num_channels = 0;
   unsigned short channel_index = 0;
      // ---- read channel name
      
   do {
      // ---- find channels 'B', 'G', 'R'
      exr_channel channel;
      unsigned char stringLength = readString255FromFile( exr_fp, channel.channel_name );

      if( stringLength ) {
         unsigned char find_channel = 0x00;

         if( channel.channel_name[0] == 'B' ) {
            find_channel = 0x01;
         }
         else if( channel_list.channel[channel_index].channel_name[0] == 'G' ) {
            find_channel = 0x01;
         }
         else if( channel_list.channel[channel_index].channel_name[0] == 'R' ) {
            find_channel = 0x01;
         }

         if( find_channel ) {
            // ---- read channel data type
            channel.data_type = fgetc( exr_fp );
            fgetc( exr_fp );
            fgetc( exr_fp );
            fgetc( exr_fp );
            // ---- read channel non linear
            channel.non_linear = fgetc( exr_fp );
            fgetc( exr_fp );
            fgetc( exr_fp );
            fgetc( exr_fp );
            // ---- read sample x
            channel.sample_x = fgetc( exr_fp );
            fgetc( exr_fp );
            fgetc( exr_fp );
            fgetc( exr_fp );
            // ---- read sample y
            channel.sample_y = fgetc( exr_fp );
            fgetc( exr_fp );
            fgetc( exr_fp );
            fgetc( exr_fp );
            // ---- offset
            channel.offset = offset;
            if( channel.data_type == EXR_HALF)
               offset += 2;
            else
               offset += 4;
            // ---- save channel
            channel_list.channel[channel_index] = channel;
            channel_list.num_channels++;
            channel_index++;
         }
         // ---- skip channel
         else {
            // ---- data offset
            unsigned char data_type = fgetc( exr_fp );
            if( data_type == EXR_HALF)
               offset += 2;
            else
               offset += 4; 
            fseek( exr_fp, ftell( exr_fp ) + 15, SEEK_SET );
         }
   }
   else
       finish = 0x01;

   } while( !finish && (channel_index < 3) );
 
   return channel_list;
}


exr_attributes readAttributes( FILE *exr_fp ) {

   exr_attributes attributes;
   unsigned char finish = 0;

   // ---- read attributes, after last attribute have byte == 0x00 
   do {
      // ---- read attribute name
      char attribute_name[255];
      unsigned char string_length = readString255FromFile( exr_fp, attribute_name );

      if( string_length ) {
         // ---- read attribute data type
         char attribute_data_type[255];
         readString255FromFile( exr_fp, attribute_data_type );

         // ---- read attribute length
         unsigned int attribute_length = 0;
         fread( &attribute_length, 4, 1, exr_fp );

         if( !strcmp( "channels", attribute_name ) ) {
            // ---- skip attribute, chưa làm hàm này
            attributes.channel_list = readChannelData( exr_fp );
 //           fseek( exr_fp, ftell( exr_fp ) + attribute_length, SEEK_SET );
         }
         else if( !strcmp( "compression", attribute_name ) )
             attributes.compression = fgetc( exr_fp );
        else if( !strcmp( "dataWindow", attribute_name ) ) {
           fread( &(attributes.dataWindow.bottom), 4, 1, exr_fp );
           fread( &(attributes.dataWindow.left), 4, 1, exr_fp );
           fread( &(attributes.dataWindow.top), 4, 1, exr_fp );
           fread( &(attributes.dataWindow.right), 4, 1, exr_fp );
        }
        else if( !strcmp( "displayWindow", attribute_name ) ) {
           fread( &(attributes.displayWindow.bottom), 4, 1, exr_fp );
           fread( &(attributes.displayWindow.left), 4, 1, exr_fp );
           fread( &(attributes.displayWindow.top), 4, 1, exr_fp );
           fread( &(attributes.displayWindow.right), 4, 1, exr_fp );
        }
        else if( !strcmp( "lineOrder", attribute_name ) ) {
           attributes.lineOrder = fgetc( exr_fp );
        }
        else
         // ---- skip attribute
         fseek( exr_fp, ftell( exr_fp ) + attribute_length, SEEK_SET );

 //        printf( "%s %s %d\n", attribute_name, attribute_data_type, attribute_length );
      }
      else 
         finish = 0x01;
   } while( !finish );

   return attributes;
}


#pragma mark ---- Read EXR File
/* decode exr file */
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
   if( type & 0x1a != 0x00 ) {
      OPENDCP_LOG(LOG_ERROR,"Only support normal scanline exr file, no tile, deep pixel, multipart file");
     return OPENDCP_FATAL;
   }
   
   // ---- skip two bytes, not used
   fgetc(exr_fp);
   fgetc(exr_fp);

   // ---- read EXR attritubes need for dcp
   exr_attributes attritbute = readAttributes( exr_fp );
       
   // ---- read offset table

   // ---- read file data
}
