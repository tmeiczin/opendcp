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

#include <string.h>
#include <stdlib.h>
#include "opendcp.h"
#include "opendcp_image.h"
#include "opendcp_encoder.h"

int opendcp_encode_none(opendcp_t *opendcp, opendcp_image_t *opendcp_image, char *output_file) {
    UNUSED(opendcp);
    UNUSED(opendcp_image);
    UNUSED(output_file);

    return OPENDCP_NO_ERROR;
}

static opendcp_encoder_t opendcp_encoders[] = {
    FOREACH_OPENDCP_ENCODER(GENERATE_ENCODER_STRUCT)
};

/*!
 @function opendcp_encoder_find
 @abstract Find an encoder structure
 @discussion This function will find and return a specified encoder structure.
     If the name is not NULL it is used, otherwise if it is
     NULL the id will be used.
 @param name The name of the encoder to find. If set to NULL, then ext or id will be used
 @param ext  The file extension to locate the correct encoder. If set to NULL, name or id will be used.
 @param id The id of the encoder to find, used only when name and ext are NULL
 @return Returns an opendcp_encoder_t structure on success, opendcp_encoder_none if not found
*/
opendcp_encoder_t *opendcp_encoder_find(char *name, char *ext, int id) {
    int x;

    for (x = 0; x < OPENDCP_ENCODER_NONE; x++) {
        if (!opendcp_encoders[x].enabled) {
           continue;
        }
        if (name != NULL) {
            if (!strncasecmp(opendcp_encoders[x].name, name, 3)) {
                return &opendcp_encoders[x];
            }
        } else if (ext != NULL) {
            if (strcasefind(opendcp_encoders[x].extensions, ext)) {
                return &opendcp_encoders[x];
            }
        } else {
            if (opendcp_encoders[x].id == id) {
                return &opendcp_encoders[x];
            }
        }
    }

    return &opendcp_encoders[OPENDCP_ENCODER_NONE];
}

/*!
 @function opendcp_encoder_enable
 @abstract Changes which encoders will be enabled for a given file extension.
 @discussion This function enables an encoder for a file extension. Used when multiple
     encoders can support a file type.
 @param ext  The file extension of desired file type
 @param name The name of the encoder to enable, used only if non NULL
 @param id The id of the encoder to enable, used only when name is NULL
 @return Returns OPENDCP_NO_ERROR on success, OPENDCP_ERROR on failure
*/
int opendcp_encoder_enable(char *ext, char *name, int id) {
    int x;

    for (x = 0; x < OPENDCP_ENCODER_NONE; x++) {
        if (strcasefind(opendcp_encoders[x].extensions, ext)) {
            if (name !=NULL) {
                if (!strncasecmp(opendcp_encoders[x].name, name, 3)) {
                    opendcp_encoders[x].enabled = 1;
                } else {
                    opendcp_encoders[x].enabled = 0;
                }
            } else {
                if (opendcp_encoders[x].id == id) {
                    opendcp_encoders[x].enabled = 1;
                } else {
                    opendcp_encoders[x].enabled = 0;
                }
            }
        }
    }

    return OPENDCP_NO_ERROR;
}
