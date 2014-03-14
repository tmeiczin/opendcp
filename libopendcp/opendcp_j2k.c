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
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include "opendcp.h"
#include "opendcp_encoder.h"

int convert_to_j2k(opendcp_t *opendcp, char *sfile, char *dfile) {
    opendcp_image_t *opendcp_image;
    opendcp_encoder_t *encoder;
    char *extension;
    int result = 0;

    extension = strrchr(dfile, '.');
    extension++;

    if (opendcp_encoder_enable(extension, NULL, opendcp->j2k.encoder)) {
        OPENDCP_LOG(LOG_ERROR, "could not enabled encoder");
    }

    encoder = opendcp_encoder_find(NULL, extension, 0);
    OPENDCP_LOG(LOG_INFO, "using %s encoder (%s) to convert file %s to %s", encoder->name, extension, basename(sfile), basename(dfile));

    OPENDCP_LOG(LOG_DEBUG, "reading input file %s", basename(sfile));
    result = read_image(&opendcp_image, sfile);

    if (result != OPENDCP_NO_ERROR) {
        OPENDCP_LOG(LOG_ERROR, "unable to read file %s", basename(sfile));
        return OPENDCP_ERROR;
    }

    if (!opendcp_image) {
        OPENDCP_LOG(LOG_ERROR, "could not load image");
        return OPENDCP_ERROR;
    }

    /* verify image is dci compliant */
    if (check_image_compliance(opendcp->cinema_profile, opendcp_image, NULL) != OPENDCP_NO_ERROR) {

        /* resize image */
        if (opendcp->j2k.resize) {
            if (resize(&opendcp_image, opendcp->cinema_profile, opendcp->j2k.resize) != OPENDCP_NO_ERROR) {
                opendcp_image_free(opendcp_image);
                return OPENDCP_ERROR;
            }
        } else {
            OPENDCP_LOG(LOG_WARN, "the image resolution of %s is not DCI compliant", sfile);
            opendcp_image_free(opendcp_image);
            return OPENDCP_ERROR;
        }
    }

    if (opendcp->j2k.xyz) {
        OPENDCP_LOG(LOG_INFO, "RGB->XYZ color conversion %s", basename(sfile));

        if (rgb_to_xyz(opendcp_image, opendcp->j2k.lut, opendcp->j2k.xyz_method)) {
            OPENDCP_LOG(LOG_ERROR, "color conversion failed %s", basename(sfile));
            opendcp_image_free(opendcp_image);
            return OPENDCP_ERROR;
        }
    }

    result = encoder->encode(opendcp, opendcp_image, dfile);

    opendcp_image_free(opendcp_image);

    if ( result != OPENDCP_NO_ERROR) {
        OPENDCP_LOG(LOG_ERROR, "JPEG2000 conversion failed %s", basename(sfile));
        return OPENDCP_ERROR;
    }

    return OPENDCP_NO_ERROR;
}
