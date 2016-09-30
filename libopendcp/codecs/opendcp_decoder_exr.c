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
    EXR_COMPRESSION_NO       = 0,          /* no compression                  */
    EXR_COMPRESSION_RLE      = 1,          /* 8-bit run-length-encoded (not supported) */
    EXR_COMPRESSION_ZIPS     = 2,          /* zip single line (not supported) */
    EXR_COMPRESSION_ZIP      = 3,          /* zip 16 lines                    */
    EXR_COMPRESSION_PIZ      = 4,          /* piz (not supported)             */
    EXR_COMPRESSION_PXR24    = 5           /* pixar 24 bit (not supported)    */
    EXR_COMPRESSION_B44      = 6           /* b44 (not supported)             */
    EXR_COMPRESSION_B44A     = 7           /* b44a (not supported)            */
    EXR_COMPRESSION_DWAA     = 8            /* dwaa 32 lines (not supported)  */
    EXR_COMPRESSION_DWAB     = 9            /* dwab 256 lines (not supported) */
} exr_compression_enum;

typedef enum {
    EXR_UINT      = 0,          /* unsigned int 32 bit, never use for color data */
    EXR_HALF      = 1,          /* half, 16 bit floating point */
    EXR_FLOAT     = 2,          /* float, 32 bit floating point */
} exr_dataType_enum;

typedef struct {
    char name[255];            /* channel name                                      */
    unsigned char dataType;    /* channel data type, int, half, float               */
    unsigned char non_linear;  /* non linear, only use for B44 and B44A compression */
    unsigned int sample_x;     /* sample x direction, only support == 1             */
    unsigned int sample_y;     /* sample y direction, only support == 1             */
    unsigned int offset;       /* position in image data                            */
} exr_channel;

typedef struct {
    unsigned short num_channels; /* number channels                                   */
    exr_channel channel[3];      /* channel array, only read data for B, G, R channel */
    unsigned short data_width;   /* channel data width for all channels, for uncompress buffer size */
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
//    unsigned int  width;              /* width of image - calculate */
//    unsigned int  height;             /* height of image - calculate */
} exr_attributes;

/* exr chunk data */
typedef struct {
   unsigned int num_chunks;
   unsigned long *chunk_table;
} exr_chunk_data;

typedef struct {
   float *channel_b;
   float *channel_g;
   float *channel_r;
} exr_image_data;

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
   channel_list.data_width = 0;
   unsigned short channel_index = 0;
   unsigned short offset = 0;
      
   do {
      // ---- find channels 'B', 'G', 'R'
      exr_channel channel;
      unsigned char stringLength = readString255FromFile( exr_fp, channel.name );

      if( stringLength ) {
         unsigned char find_channel = 0x00;

         if( channel.name[0] == 'B' ) {
            find_channel = 0x01;
         }
         else if( channel.name[0] == 'G' ) {
            find_channel = 0x01;
         }
         else if( channel.name[0] == 'R' ) {
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
            if( channel.data_type == EXR_HALF) {
               offset += 2;
               channel_list.data_width += 2;
            }
            else {
               offset += 4;
               channel_list.data_width += 4;
            }
            // ---- save channel
            channel_list.channel[channel_index] = channel;
            channel_list.num_channels++;
            channel_index++;
         }
         // ---- not need this channel skip channel; need read all channel data for next attribute
         else {
            // ---- data offset
            unsigned char data_type = fgetc( exr_fp );
            if( data_type == EXR_HALF) {
               offset += 2;
               channel_list.data_width += 2;
            }
            else {
               offset += 4;
               channel_list.data_width += 4;
            }
            fseek( exr_fp, ftell( exr_fp ) + 15, SEEK_SET );
         }
   }
   else
       finish = 0x01;

   } while( !finish );
 
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
           fread( &(attributes.dataWindow.left), 4, 1, exr_fp );
           fread( &(attributes.dataWindow.bottom), 4, 1, exr_fp );
           fread( &(attributes.dataWindow.right), 4, 1, exr_fp );
           fread( &(attributes.dataWindow.top), 4, 1, exr_fp );
        }
        else if( !strcmp( "displayWindow", attribute_name ) ) {
           fread( &(attributes.displayWindow.left), 4, 1, exr_fp );
           fread( &(attributes.displayWindow.bottom), 4, 1, exr_fp );
           fread( &(attributes.displayWindow.right), 4, 1, exr_fp );
           fread( &(attributes.displayWindow.top), 4, 1, exr_fp );
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

exr_chunk_data read_chunk_data( FILE *exr_fp, exr_attributes *attributes ) {

   exr_chunk_data chunk_data;
   chunk_data.num_chunks = (attributes->dataWindow.top - attributes->dataWindow.bottom) + 1;
    
   // ---- if EXR_COMPRESSION_ZIP, 16 rows per chunk
   if( attributes->compression == EXR_COMPRESSION_ZIP )  // 
       chunk_data.num_chunks = ((attributes->dataWindow.top - attributes->dataWindow.bottom) >> 4) + 1;
   
   // ---- get memory for chunk table
   chunk_data.chunk_table = malloc( chunk_data.num_chunks << 3 );  // 8 bytes
    
   // ----- read address for chunks
   unsigned int chunk_index = 0;
   while( chunk_index < chunk_data.num_chunks ) {
      fread( &(chunk_data.chunk_table[chunk_index]), 8, 1, exr_fp );
      chunk_index++;
   }
    
   return chunk_data;
}


#pragma mark ---- Compression
/* unfilter buffer - from OpenEXR library */
void unfilter_buffer( unsigned char *buffer, unsigned char *unfilteredBuffer, unsigned int length ) {

   {
      unsigned char *t = (unsigned char *)buffer + 1;           // bỏ byte đầu tiên
      unsigned char *stop = (unsigned char *)buffer + length;   // cuối buffer
      
      while (t < stop) {
         // ---- unfilter byte
         int d = (int) (t[-1]) + (int)(t[0]) - 128;  // add difference byte before subtract 128
         //      NSLog( @"ImfFilter: unfilter t[-1] %d + t[0] %d - 128 = d %d", t[-1], t[0], d);
         // ---- save unfiltered byte
         t[0] = d;
         ++t;
      }
   }
 
   // Data organization || 0; (length+1)/2 || 1; (length+1)/2 + 1 || 2; (length+1)/2 + 2 || etc.
   char *t1 = (char *)buffer;          // begin buffer
   char *t2 = (char *)buffer + (length + 1) / 2;  // half buffer
   char *s = (char *)unfilteredBuffer;  // start of unfiltered buffer
   char *stop = s + length;             // end unfiltered buffer
   unsigned char finished = 0x0;
   
   while( !finished ) {
      // ---- if not at unfilteredBuffer end
      if (s < stop)
         *(s++) = *(t1++);  // copy from buffer
      else
         finished = 0x1;
      // ---- if not at unfilteredBuffer end
      if (s < stop)
         *(s++) = *(t2++);  // copy next from half buffer
      else
         finished = 0x1;
   }
}

/* uncompress zip with zlib */
void uncompress_zip( unsigned char *compressed_buffer, unsigned int compressed_buffer_length, unsigned char *uncompressed_buffer, unsigned int uncompressed_buffer_length ) {

   int err;
   z_stream d_stream; // decompression stream data struct
   
   d_stream.zalloc = Z_NULL;
   d_stream.zfree = Z_NULL;
   d_stream.opaque = Z_NULL;
   d_stream.data_type = Z_BINARY;
   
   d_stream.next_in  = compressed_buffer;
   d_stream.avail_in = compressed_buffer_length;

   // ---- check if initialization has no error
   err = inflateInit(&d_stream);

   if( err != Z_OK ) {
      OPENDCP_LOG(LOG_ERROR,"uncompress_zip: error inflateInit %d (%x) d_stream.avail_in %d", err, err, d_stream.avail_in );
   }
   
   // ---- give data to decompress
   d_stream.next_out = uncompressed_buffer;
   d_stream.avail_out = uncompressed_buffer_length;
   
   err = inflate(&d_stream, Z_STREAM_END);
 
   if( err != Z_STREAM_END ) {
      if( err == Z_OK) {
         OPENDCP_LOG(LOG_ERROR,"uncompress_zip: Z_OK d_stream.avail_out %d d_stream.total_out %lu",
               d_stream.avail_out, d_stream.total_out );
      }
      else
         OPENDCP_LOG(LOG_ERROR,"ImfCompressorZIP: uncompress: error inflate %d (%x) d_stream.avail_out %d d_stream.total_out %lu",
            err, err, d_stream.avail_out, d_stream.total_out );
      
   }

   err = inflateEnd( &d_stream );
   if( err != Z_OK )
      OPENDCP_LOG(LOG_ERROR,"ExrZIP: uncompress: error inflateEnd %d (%x) c_stream.avail_in %d", err, err, d_stream.avail_in );
}

/* compression no */
void read_data_compression_no( FILE *exr_fp, exr_chunk_data *chunk_data, exr_attributes *attributes ) {

   unsigned short chunk_number = 0;
   while( chunk_number < chunk_data->num_chunks ) {
      // ---- read line number
      unsigned int line_number = 0;
      fread( &line_number, 4, 1, exr_fp );
      // ---- read data length
      unsigned int data_length = 0; 
      fread( &data_length, 4, 1, exr_fp );
      printf( "%d - %d  %d\n", chunk_number, line_number, data_length );
      chunk_number++;
   }
}

/* compression RLE */
void read_data_compression_rle( FILE *exr_fp, exr_chunk_data *chunk_data, exr_attributes *attributes ) {

   unsigned short chunk_number = 0;
   while( chunk_number < chunk_data->num_chunks ) {
      // ---- read line number
      unsigned int line_number = 0;
      fread( &line_number, 4, 1, exr_fp );
      // ---- read data length
      unsigned int data_length = 0; 
      fread( &data_length, 4, 1, exr_fp );
      printf( "%d - %d  %d\n", chunk_number, line_number, data_length );
      chunk_number++;
   }
}

/* compression ZIPS an ZIP */
void read_data_compression_zip( FILE *exr_fp, exr_chunk_data *chunk_data, exr_attributes *attributes ) {

   unsigned int num_columns = (attributes->dataWindow.right - attributes->dataWindow.left) + 1;
   unsigned char chunk_num_rows = 1;          // number row for each chunk
   unsigned char *compressed_buffer = NULL;   // buffer for compressed data
   unsigned char *uncompressed_buffer = NULL; // buffer for uncompressed data
   unsigned char *unfiltered_buffer = NULL;   // buffer for unfiltered data
  
   unsigned short channel_data_width = attributes->channel_list.data_width;

   unsigned int uncompressed_data_length = num_columns*channel_data_width;

   // ---- check if use ZIP
   if( attributes->compression == EXR_COMPRESSION_ZIP ) {
       uncompressed_data_length <<= 4;  // multiply 16
       chunk_num_rows = 16;
   }

   compressed_buffer = malloc( uncompressed_data_length << 1 );
   uncompressed_buffer = malloc( uncompressed_data_length );
   unfiltered_buffer = malloc( uncompressed_data_length ); 

   unsigned short chunk_number = 0;
   while( chunk_number < chunk_data->num_chunks ) {
      // ---- read line number
      unsigned int line_number = 0;
      fread( &line_number, 4, 1, exr_fp );

      // ---- read data length
      unsigned int data_length = 0; 
      fread( &data_length, 4, 1, exr_fp );

      // ---- read compressed data
      fread( compressed_buffer, 1, data_length, exr_fp );

      // ---- uncompress zip
      uncompress_zip( compressed_buffer, data_length, uncompressed_buffer, uncompressed_data_length );
      
      // ---- unfilter data
      unfilter_buffer( uncompressed_buffer, unfiltered_buffer, uncompressed_data_length );

      // ---- copy data from buffer

      chunk_number++;
   }
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
   
   // ---- check compression
   if( attributes.compression > EXR_COMPRESSION_ZIP ) {
      OPENDCP_LOG(LOG_ERROR,"Only support NO, RLE, ZIPS, ZIP compression in exr file");
      return OPENDCP_FATAL;
   }
    
   // ---- check number channels, need all B, G, R channels
    if( attributes.channel_list.num_channels != 3 ) {
      OPENDCP_LOG(LOG_ERROR,"Exr file not have all B, G, R channels");
      return OPENDCP_FATAL;
   }
   
   // ---- read offset table
   exr_chunk_data chunk_data = read_chunk_data( exr_fp, &attributes );

   // ---- read file data
   if( attributes.compression == EXR_COMPRESSION_NO )
      read_data_compression_no( exr_fp, &chunk_data, &attributes );
   else if( attributes.compression == EXR_COMPRESSION_RLE )
      read_data_compression_rle( exr_fp, &chunk_data, &attributes );
   else if( (attributes.compression == EXR_COMPRESSION_ZIPS) || (attributes.compression == EXR_COMPRESSION_ZIP) )
      read_data_compression_zip( exr_fp , &chunk_data, &attributes );

   // ---- free memory
   free( chunk_table );
}
