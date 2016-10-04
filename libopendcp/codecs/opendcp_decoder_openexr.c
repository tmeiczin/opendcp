/* opendcp_decoder_exr.c */

// FOR TWO FUNCTION: uncompress_rle() and unfilter_buffer() 
///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

// Primary authors:
//     Florian Kainz <kainz@ilm.com>
//     Rod Bogart <rgb@ilm.com>

// OTHER FUNCTIONS: Public domain


#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "opendcp.h"
#include "opendcp_image.h"

#define MAGIC_NUMBER_EXR 0x762f3101

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

/* exr window */
typedef struct {
    int left;          /* left column    */
    int bottom;        /* bottom row     */
    int right;         /* right column   */
    int top;           /* top row        */
} exr_window;

/* exr attributes, only save data for create dcp */
typedef struct {
    exr_channel_list channel_list;    /* channel list */
    unsigned char compression;        /* compression */
    exr_window dataWindow;            /* data window */
} exr_attributes;

/* exr chunk data */
typedef struct {
   unsigned int num_chunks;        /* number chunks */
   unsigned long *chunk_table;     /* chunk table - address for chunks in file (from begin file) */
} exr_chunk_data;

/* exr image data */
typedef struct {
   unsigned short width;    /* width      */
   unsigned short height;   /* height     */
   float *channel_b;        /* channel b  */ 
   float *channel_g;        /* channel g  */
   float *channel_r;        /* channel r  */
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
unsigned char read_string_255_from_file( FILE *fp, char *string ) {

   unsigned char index = 0;
   do {
     string[index] = fgetc( fp );
     index++;
   } while( string[index-1] != 0x00 );

   return index-1;
}

exr_channel_list read_channel_data( FILE *exr_fp) {
   
   exr_channel_list channel_list;
   unsigned char finish = 0x00;
   channel_list.num_channels = 0;
   channel_list.data_width = 0;
   unsigned short channel_index = 0;
   unsigned short offset = 0;
      
   do {
      // ---- find channels 'B', 'G', 'R'
      exr_channel channel;
      unsigned char stringLength = read_string_255_from_file( exr_fp, channel.name );

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


exr_attributes read_attributes( FILE *exr_fp ) {

   exr_attributes attributes;
   unsigned char finish = 0;

   // ---- read attributes, after last attribute have byte == 0x00 
   do {
      // ---- read attribute name
      char attribute_name[255];
      unsigned char string_length = read_string_255_from_file( exr_fp, attribute_name );

      if( string_length ) {
         // ---- read attribute data type
         char attribute_data_type[255];
         read_string_255_from_file( exr_fp, attribute_data_type );

         // ---- read attribute length
         unsigned int attribute_length = 0;
         fread( &attribute_length, 4, 1, exr_fp );

         if( !strcmp( "channels", attribute_name ) ) {
            // ---- skip attribute, chưa làm hàm này
            attributes.channel_list = read_channel_data( exr_fp );
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
   unsigned short num_rows = (attributes->dataWindow.top - attributes->dataWindow.bottom) + 1;

   // ---- if EXR_COMPRESSION_ZIP, 16 rows per chunk
   if( attributes->compression == EXR_COMPRESSION_ZIP ) { //
      chunk_data.num_chunks = num_rows >> 4;
      if( num_rows & 0xf )
         chunk_data.num_chunks++;
   }
   else
      chunk_data.num_chunks = num_rows;
   
   // ---- get memory for chunk table
   chunk_data.chunk_table = malloc( chunk_data.num_chunks << 3 );  // 8 bytes for each table element
    
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

/* uncompress rle - from OpenEXR library */
void uncompress_rle( unsigned char *compressed_buffer, unsigned int compressed_buffer_length, unsigned char *uncompressed_buffer, unsigned int uncompressed_buffer_length) {

   unsigned char *outStart = compressed_buffer;
   
   // ---- while not finish all in buffer
   while (compressed_buffer_length > 0) {
      // ---- if signed byte value less than zero
      if (*compressed_buffer > 127) {
         // ---- count for not same byte value
         int count = -((char)*compressed_buffer);
         compressed_buffer++;
         // ---- reduce amount count of bytes remaining for in buffer
         compressed_buffer_length -= count + 1;
         // ---- if count larger than out buffer length still available
         if (0 > (uncompressed_buffer_length -= count)) {
            printf( "uncompress_rle: Error buffer not length not enough\n" );
            return;
          }
         // ---- copy not same byte and move to next byte
         while (count-- > 0)
            *uncompressed_buffer++ = *(compressed_buffer++);
      }
      else {
         // ---- count number bytes same
         int count = *compressed_buffer++;
         // ---- reduce amount count of remaining bytes for in buffer
         compressed_buffer_length -= 2;
         // ---- if count larger than out buffer length still available
         if (0 > (uncompressed_buffer_length -= count + 1)) {
            printf( "uncompress_rle: Error buffer not length not enough\n" );
            return;
         }
         // ---- copy same byte
         while (count-- >= 0)
            *uncompressed_buffer++ = *compressed_buffer;
         // ---- move to next byte
         compressed_buffer++;
      }
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

/* copy half data */
void copy_half_data( unsigned char *buffer, float *channel_data, unsigned short num_rows, unsigned short num_columns, unsigned short start_row_number, exr_channel *channel ) {

   // ---- calculate offset for channel data,
   unsigned int channel_data_offset = num_columns*start_row_number;

   unsigned short row_index = 0;
   while( row_index < num_rows ) {
      // ---- calculate offset for channels
      unsigned int buffer_offset = num_columns*channel->offset*row_index;

      unsigned short column_index = 0;
      while( column_index < num_columns ) {
         // ---- combine data for half number
         unsigned short half = buffer[buffer_offset] | buffer[buffer_offset+1] << 8;

         // ---- convert half and save float data in channel
         channel_data[channel_data_offset] = half2float( half );

         channel_data_offset++;
         buffer_offset += 2;
         column_index++;
      }
      row_index++;
   }
}

/* copy float data */
void copy_float_data( unsigned char *buffer, float *channel_data, unsigned short num_rows, unsigned short num_columns, unsigned short start_row_number, exr_channel *channel ) {

   // ---- calculate offset for channel data,
   unsigned int channel_data_offset = num_columns*start_row_number;

   u_intfloat int_fl;

   unsigned short row_index = 0;
   while( row_index < num_rows ) {
      // ---- calculate offset for channels
      unsigned int buffer_offset = num_columns*channel->offset*row_index;

      unsigned short column_index = 0;
      while( column_index < num_columns ) {
         // ---- combine data for float number
         int_fl.i = buffer[buffer_offset] | buffer[buffer_offset+1] << 8 | buffer[buffer_offset+2] << 16 | buffer[buffer_offset+3] << 24;

         // ---- save float data in channel
         channel_data[channel_data_offset] = int_fl.f;

         channel_data_offset++;
         buffer_offset += 4;
         column_index++;
      }
      row_index++;
   }
}

/* compression no */
void read_data_compression_no( FILE *exr_fp, exr_chunk_data *chunk_data, exr_attributes *attributes, exr_image_data *image_data ) {

   unsigned int num_columns = (attributes->dataWindow.right - attributes->dataWindow.left) + 1;
  
   unsigned short channel_data_width = attributes->channel_list.data_width;

   unsigned char *data_buffer = malloc( num_columns*channel_data_width );

   unsigned short chunk_number = 0;
   while( chunk_number < chunk_data->num_chunks ) {
      // ---- begin chunk
      fseek( exr_fp, chunk_data->chunk_table[chunk_number], SEEK_SET );

      // ---- read row number
      unsigned int row_number = 0;
      fread( &row_number, 4, 1, exr_fp );

      // ---- read data length
      unsigned int data_length = 0; 
      fread( &data_length, 4, 1, exr_fp );

      // ---- read compressed data
      fread( data_buffer, 1, data_length, exr_fp );

      // ---- copy data from buffer
      if( attributes->channel_list.channel[0].data_type == EXR_HALF )
         copy_half_data( data_buffer, image_data->channel_b, 1, num_columns, row_number, &(attributes->channel_list.channel[0]) );
      else
         copy_float_data( data_buffer, image_data->channel_b, 1, num_columns, row_number, &(attributes->channel_list.channel[0]) );

      if( attributes->channel_list.channel[1].data_type == EXR_HALF )
         copy_half_data( data_buffer, image_data->channel_g, 1, num_columns, row_number, &(attributes->channel_list.channel[1]) );
      else
         copy_float_data( data_buffer, image_data->channel_g, 1, num_columns, row_number, &(attributes->channel_list.channel[1]) );

      if( attributes->channel_list.channel[2].data_type == EXR_HALF )
         copy_half_data( data_buffer, image_data->channel_r, 1, num_columns, row_number, &(attributes->channel_list.channel[2]) );
      else
         copy_float_data( data_buffer, image_data->channel_r, 1, num_columns, row_number, &(attributes->channel_list.channel[2]) );

      // ---- next chunk
      chunk_number++;
   }
}

/* compression RLE */
void read_data_compression_rle( FILE *exr_fp, exr_chunk_data *chunk_data, exr_attributes *attributes, exr_image_data *image_data ) {

   unsigned int num_columns = (attributes->dataWindow.right - attributes->dataWindow.left) + 1;
  
   unsigned short channel_data_width = attributes->channel_list.data_width;

   unsigned int uncompressed_data_length = num_columns*channel_data_width;

   unsigned char *compressed_buffer = malloc( uncompressed_data_length << 1 );
   unsigned char *uncompressed_buffer = malloc( uncompressed_data_length );
   unsigned char *unfiltered_buffer = malloc( uncompressed_data_length ); 

   unsigned short chunk_number = 0;
   while( chunk_number < chunk_data->num_chunks ) {
      // ---- begin chunk
      fseek( exr_fp, chunk_data->chunk_table[chunk_number], SEEK_SET );

      // ---- read row number
      unsigned int row_number = 0;
      fread( &row_number, 4, 1, exr_fp );

      // ---- read data length
      unsigned int data_length = 0; 
      fread( &data_length, 4, 1, exr_fp );

      // ---- read compressed data
      fread( compressed_buffer, 1, data_length, exr_fp );

      // ---- uncompress rle
      uncompress_rle( compressed_buffer, data_length, uncompressed_buffer, uncompressed_data_length );
      
      // ---- unfilter data
      unfilter_buffer( uncompressed_buffer, unfiltered_buffer, uncompressed_data_length );

      // ---- copy data from buffer
      if( attributes->channel_list.channel[0].data_type == EXR_HALF )
         copy_half_data( unfiltered_buffer, image_data->channel_b, 1, num_columns, row_number, &(attributes->channel_list.channel[0]) );
      else
         copy_float_data( unfiltered_buffer, image_data->channel_b, 1, num_columns, row_number, &(attributes->channel_list.channel[0]) );

      if( attributes->channel_list.channel[1].data_type == EXR_HALF )
         copy_half_data( unfiltered_buffer, image_data->channel_g, 1, num_columns, row_number, &(attributes->channel_list.channel[1]) );
      else
         copy_float_data( unfiltered_buffer, image_data->channel_g, 1, num_columns, row_number, &(attributes->channel_list.channel[1]) );

      if( attributes->channel_list.channel[2].data_type == EXR_HALF )
         copy_half_data( unfiltered_buffer, image_data->channel_r, 1, num_columns, row_number, &(attributes->channel_list.channel[2]) );
      else
         copy_float_data( unfiltered_buffer, image_data->channel_r, 1, num_columns, row_number, &(attributes->channel_list.channel[2]) );

      // ---- next chunk
      chunk_number++;
   }

   // ---- free memory
   free( compressed_buffer );
   free( uncompressed_buffer );
   free( unfiltered_buffer );
}

/* compression ZIPS an ZIP */
void read_data_compression_zip( FILE *exr_fp, exr_chunk_data *chunk_data, exr_attributes *attributes, exr_image_data *image_data ) {

   unsigned int num_columns = (attributes->dataWindow.right - attributes->dataWindow.left) + 1;
   unsigned char num_rows = 1;
   unsigned char *compressed_buffer = NULL;
   unsigned char *uncompressed_buffer = NULL;
   unsigned char *unfiltered_buffer = NULL;
  
   unsigned short channel_data_width = attributes->channel_list.data_width;

   unsigned int uncompressed_data_length = num_columns*channel_data_width;

   // ---- check if use ZIP
   if( attributes->compression == EXR_COMPRESSION_ZIP ) {
       uncompressed_data_length <<= 4;  // multiply 16
       num_rows = 16;
   }

   compressed_buffer = malloc( uncompressed_data_length << 1 );
   uncompressed_buffer = malloc( uncompressed_data_length );
   unfiltered_buffer = malloc( uncompressed_data_length ); 

   unsigned short chunk_number = 0;
   while( chunk_number < chunk_data->num_chunks ) {
      // ---- begin chunk
      fseek( exr_fp, chunk_data->chunk_table[chunk_number], SEEK_SET );

      // ---- read row number
      unsigned int row_number = 0;
      fread( &row_number, 4, 1, exr_fp );

      // ---- read data length
      unsigned int data_length = 0; 
      fread( &data_length, 4, 1, exr_fp );

      // ---- check if use ZIP and number row remain, only for last chunk
      if( attributes->compression == EXR_COMPRESSION_ZIP ) {
         // ---- calculate number row reamin in image, check if have 16 rows for this chunk
         unsigned short num_rows_remain = image_data->height - row_number;
         if( num_rows_remain < 16 ) {
            uncompressed_data_length = (uncompressed_data_length >> 4)*num_rows_remain;
            num_rows = num_rows_remain;
         }
      }

      // ---- read compressed data
      fread( compressed_buffer, 1, data_length, exr_fp );

      // ---- uncompress zip
      uncompress_zip( compressed_buffer, data_length, uncompressed_buffer, uncompressed_data_length );
      
      // ---- unfilter data
      unfilter_buffer( uncompressed_buffer, unfiltered_buffer, uncompressed_data_length );

      // ---- copy data from buffer
      if( attributes->channel_list.channel[0].data_type == EXR_HALF )
         copy_half_data( unfiltered_buffer, image_data->channel_b, num_rows, num_columns, row_number, &(attributes->channel_list.channel[0]) );
      else
         copy_float_data( unfiltered_buffer, image_data->channel_b, num_rows, num_columns, row_number, &(attributes->channel_list.channel[0]) );

      if( attributes->channel_list.channel[1].data_type == EXR_HALF )
         copy_half_data( unfiltered_buffer, image_data->channel_g, num_rows, num_columns, row_number, &(attributes->channel_list.channel[1]) );
      else
         copy_float_data( unfiltered_buffer, image_data->channel_g, num_rows, num_columns, row_number, &(attributes->channel_list.channel[1]) );

      if( attributes->channel_list.channel[2].data_type == EXR_HALF )
         copy_half_data( unfiltered_buffer, image_data->channel_r, num_rows, num_columns, row_number, &(attributes->channel_list.channel[2]) );
      else
         copy_float_data( unfiltered_buffer, image_data->channel_r, num_rows, num_columns, row_number, &(attributes->channel_list.channel[2]) );

      // ---- next chunk
      chunk_number++;
   }

   // ---- free memory
   free( compressed_buffer );
   free( uncompressed_buffer );
   free( unfiltered_buffer );
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
   exr_attributes attritbute = read_attributes( exr_fp );
   
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

    // ---- for store image data
   exr_image_data image_data;
   image_data.width = attributes.dataWindow.right - attributes.dataWindow.left + 1;
   image_data.height = attributes.dataWindow.top - attributes.dataWindow.bottom + 1;
   // ---- create buffers for image data
   image_data.channel_b = malloc( image_data.width*image_data.height * sizeof( float ) );
   image_data.channel_g = malloc( image_data.width*image_data.height * sizeof( float ) );
   image_data.channel_r = malloc( image_data.width*image_data.height * sizeof( float ) );
 
   // ---- read file data
   if( attributes.compression == EXR_COMPRESSION_NO )
      read_data_compression_no( exr_fp, &chunk_data, &attributes, &image_data );
   else if( attributes.compression == EXR_COMPRESSION_RLE )
      read_data_compression_rle( exr_fp, &chunk_data, &attributes, &image_data );
   else if( (attributes.compression == EXR_COMPRESSION_ZIPS) || (attributes.compression == EXR_COMPRESSION_ZIP) )
      read_data_compression_zip( exr_fp , &chunk_data, &attributes, &image_data );

   // ---- close file
   fclose( exr_fp );
 
    /* create the image (float data) */
   image = opendcp_image_create_float(3, image_data.width, mage_data.height);
  
   unsigned int image_size = image_data.width * mage_data.height;
   for (index = 0; index < image_size; index++) {
    // ---- need copy float data from exr image data to float opendcp image data
      // ----- correct channel order?
      image->component[0].float_data[index] = image_data.channel_b[index];
      image->component[1].float_data[index] = image_data.channel_g[index];
      image->component[2].float_data[index] = image_data.channel_r[index];
   }
 
   // ---- free chunk table
   free( chunk_data.chunk_table );
 
   // ---- free exr image data
   free( image_data.channel_b );
   free( image_data.channel_g );
   free( image_data.channel_r );

   OPENDCP_LOG(LOG_DEBUG,"done reading exr image");
   *image_ptr = image;

   return OPENDCP_NO_ERROR;
}
