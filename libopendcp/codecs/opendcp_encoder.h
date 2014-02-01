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

#include "opendcp.h"
#include "opendcp_image.h"

#define FOREACH_OPENDCP_ENCODER(OPENDCP_ENCODER) \
            OPENDCP_ENCODER(OPENDCP_ENCODER_KAKADU,   kakadu,   "j2c;j2k",  0)  \
            OPENDCP_ENCODER(OPENDCP_ENCODER_OPENJPEG, openjpeg, "j2c;j2k",  1)  \
            OPENDCP_ENCODER(OPENDCP_ENCODER_RAGNAROK, ragnarok, "j2c;j2k",  0)  \
            OPENDCP_ENCODER(OPENDCP_ENCODER_REMOTE,   remote,   "j2c;j2k",  0)  \
            OPENDCP_ENCODER(OPENDCP_ENCODER_TIFF,     tif,      "tif;tiff", 1)  \
            OPENDCP_ENCODER(OPENDCP_ENCODER_NONE,     none,     "none",     1)

#define GENERATE_ENCODER_ENUM(ENCODER, NAME, EXT, ENABLED) ENCODER,
#define GENERATE_ENCODER_STRING(ENCODER, NAME, EXT, ENABLED) #NAME,
#define GENERATE_ENCODER_NAME(ENCODER, NAME, EXT, ENABLED) #ENCODER,
#define GENERATE_ENCODER_STRUCT(ENCODER, NAME, EXT, ENABLED) { ENCODER, ENABLED, #NAME, EXT, opendcp_encode_ ## NAME },
#define GENERATE_ENCODER_EXTERN(ENCODER, NAME, EXT, ENABLED) extern int opendcp_encode_ ## NAME(opendcp_t *opendcp, opendcp_image_t *opendcp_image, char *output_file);

/*!
 *  @enum OPENDCP_ENCODERS
 *  @abstract An enum of encoders used as the id in an opendcp_encoder_t struct.
*/
enum OPENDCP_ENCODERS {
     FOREACH_OPENDCP_ENCODER(GENERATE_ENCODER_ENUM)
};

FOREACH_OPENDCP_ENCODER(GENERATE_ENCODER_EXTERN)

/*!
 @typedef opendcp_encoder_t
 @abstract opendcp encoder structure
 @discussion This struct contains information about a encoder.
 @field id The id of this encoder a by the OPENDCP_ENCODERS enum.
 @field enabled Indicates whether the encoder is enabled
 @field name The string name of this encoder.
 @field extensions A semicolon separated string of file extensions this encoder can service.
 @field encode The encode function that will be invoked by this encoder
*/
typedef struct {
    int  id;
    int  enabled;
    char *name;
    char *extensions;
    int (*encode) (opendcp_t *opendcp, opendcp_image_t *opendcp_image, char *output_file);
} opendcp_encoder_t;

int opendcp_encoder_enable(char *ext, char *name, int id);
opendcp_encoder_t *opendcp_encoder_find(char *name, char *ext, int id);
