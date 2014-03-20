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
#include <stdio.h>
#include "opendcp.h"
#include "opendcp_decoder.h"

int opendcp_decode_none(opendcp_image_t **image_ptr, const char *infile) {
    UNUSED(image_ptr);
    UNUSED(infile);

    return OPENDCP_NO_ERROR;
}

opendcp_decoder_t opendcp_decoders[] = {
    FOREACH_OPENDCP_DECODER(GENERATE_DECODER_STRUCT)
};

/*!
 @function opendcp_decoder_find
 @abstract Find an decoder structure
 @discussion This function will find and return a specified decoder structure.
     If the name is not NULL it is used, otherwise if it is
     NULL the id will be used.
 @param name The name of the decoder to find. If set to NULL, then ext or id will be used
 @param ext  The file extension to locate the correct decoder. If set to NULL, name or id will be used.
 @param id The id of the decoder to find, used only when name and ext are NULL

 @return Returns an opendcp_decoder_t structure on success, opendcp_decoder_none if not found
*/
opendcp_decoder_t *opendcp_decoder_find(char *name, char *ext, int id) {
    int x;

    for (x = 0; x < OPENDCP_DECODER_NONE; x++) {
        if (!opendcp_decoders[x].enabled) {
           continue;
        }
        if (name != NULL) {
            if (!strncasecmp(opendcp_decoders[x].name, name, 3)) {
                return &opendcp_decoders[x];
            }
        } else if (ext != NULL) {
            if (strcasefind(opendcp_decoders[x].extensions, ext)) {
                return &opendcp_decoders[x];
            }
        } else {
            if (opendcp_decoders[x].id == id) {
                return &opendcp_decoders[x];
            }
        }
    }

    return &opendcp_decoders[OPENDCP_DECODER_NONE];
}

char *opendcp_decoder_extensions() {
    int x;
    char *extensions;

    extensions = malloc(sizeof(char) * 256);

    for (x = 0; x < OPENDCP_DECODER_NONE; x++) {
        sprintf(extensions, "%s;%s", extensions, opendcp_decoders[x].extensions);
    }

    return extensions;
}
