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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "opendcp.h"

#ifdef HAVE_RAGNAROK
#include <opendcp_encoder_ragnarok.h>
#endif

/*!
 @function opendcp_encoder_openjpeg
 @abstract Encode image to file.
 @discussion This function will take the opendcp_image_t struct and encode it.
 @param opendcp An opendcp_t context struct
 @param simage The source image memory buffer to encoder
 @param dfile The output file
 @return An OPENDCP_ERROR value
*/
int opendcp_encode_ragnarok(opendcp_t *opendcp, opendcp_image_t *opendcp_image, char *dfile) {
    int max_cs_len;
    int bw, i, x;

    if (opendcp->j2k.bw) {
        bw = opendcp->j2k.bw;
    } else {
        bw = MAX_DCP_JPEG_BITRATE;
    }

    /* set the max image and component sizes based on frame_rate */
    max_cs_len = ((float)bw)/8/opendcp->frame_rate;

    /* adjust cs for 3D */
    if (opendcp->stereoscopic) {
        max_cs_len = max_cs_len/2;
    }

#ifdef HAVE_RAGNAROK
    ragnarok_t ragnarok;

    ragnarok.h = opendcp_image->h;
    ragnarok.w = opendcp_image->w;
    ragnarok.n_components = opendcp_image->n_components;
    ragnarok.max_cs_len   = max_cs_len;
    ragnarok.profile = opendcp->cinema_profile;

    int size = ragnarok.h * ragnarok.w;
    int n_samples = size * ragnarok.n_components;
    unsigned char *b = malloc(n_samples * sizeof(int *));

    for (i = 0, x = 0; x < size; x++) {
        b[i++] = opendcp_image->component[0].data[x] >> 4;
        b[i++] = opendcp_image->component[1].data[x] >> 4;
        b[i++] = opendcp_image->component[2].data[x] >> 4;
    }

    ragnarok_encode(&ragnarok, b, dfile); 

    free(b);
#endif

    return OPENDCP_NO_ERROR;
}
