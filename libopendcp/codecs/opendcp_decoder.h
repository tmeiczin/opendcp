/*
     OpenDCP: Builds Digital Cinema Packages
     Copyright (c) 2010-2013 Terrence Meiczinger, All Rights Reserved

     This program is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define FOREACH_OPENDCP_DECODER(OPENDCP_DECODER) \
            OPENDCP_DECODER(OPENDCP_DECODER_BMP,  bmp, "bmp", 1)  \
            OPENDCP_DECODER(OPENDCP_DECODER_DPX,  dpx, "dpx", 1)  \
            OPENDCP_DECODER(OPENDCP_DECODER_OPENJPEG,  openjpeg, "j2c;j2k;jp2;jpf", 1)  \
            OPENDCP_DECODER(OPENDCP_DECODER_TIFF, tif, "tif;tiff", 1)  \
            OPENDCP_DECODER(OPENDCP_DECODER_NONE, none, "none", 1)

#define GENERATE_DECODER_ENUM(DECODER, NAME, EXT, ENABLED) DECODER,
#define GENERATE_DECODER_STRING(DECODER, NAME, EXT, ENABLED) #NAME,
#define GENERATE_DECODER_NAME(DECODER, NAME, EXT, ENABLED) #DECODER,
#define GENERATE_DECODER_STRUCT(DECODER, NAME, EXT, ENABLED) { DECODER, ENABLED, #NAME, EXT, opendcp_decode_ ## NAME },
#define GENERATE_DECODER_EXTERN(DECODER, NAME, EXT, ENABLED) extern int opendcp_decode_ ## NAME(opendcp_image_t **image_ptr, const char *infile);

/*!
 @enum OPENDCP_DECODERS
 @abstract An enum of decoders used as the id in an opendcp_decoder_t struct.
*/
enum OPENDCP_DECODERS {
     FOREACH_OPENDCP_DECODER(GENERATE_DECODER_ENUM)
};

//static const char *OPENDCP_DECODER_NAME[] = {
//    FOREACH_OPENDCP_DECODER(GENERATE_DECODER_STRING)
//};

FOREACH_OPENDCP_DECODER(GENERATE_DECODER_EXTERN)

/*!
 @typedef opendcp_decoder_t
 @abstract opendcp decoder structure
 @discussion This struct contains information about a decoder.
 @field id The id of this encoder a by the OPENDCP_DECODERS enum.
 @field enabled Indicares whether the decoder is enabled
 @field name The string name of this decoder.
 @field extensions A semicolon separated string of file extensions this decoder can service.
 @field decode The decode function that will be invoked by this decoder
*/
typedef struct {
    int  id;
    int  enabled;
    char *name;
    char *extensions;
    int  (*decode)(opendcp_image_t **image_ptr, const char *infile);
} opendcp_decoder_t;

opendcp_decoder_t *opendcp_decoder_find(char *name, char *ext, int id);
char *opendcp_decoder_extensions();
