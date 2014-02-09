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
#include <libgen.h>
#include "opendcp.h"
#include "opendcp_encoder.h"

/*!
 @function opendcp_encoder_kakadu
 @abstract Encode image to file.
 @discussion This function will take the opendcp_image_t struct and encode it.
 @param opendcp An opendcp_t context struct
 @param simage The source image memory buffer to encoder
 @param dfile The output file
 @return An OPENDCP_ERROR value
*/
int opendcp_encode_kakadu(opendcp_t *opendcp, opendcp_image_t *simage, char *dfile) {
    int result;
    int max_cs_len;
    int max_comp_size;
    char k_lengths[128];
    char temp_file[255];
    char temp_path[255];
    char cmd[512];
    FILE *cmdfp = NULL;
    int bw;

    if (opendcp->j2k.bw) {
        bw = opendcp->j2k.bw;
    } else {
        bw = MAX_DCP_JPEG_BITRATE;
    }

    if (opendcp->tmp_path == NULL) {
        sprintf(temp_path,"./");
    } else {
        sprintf(temp_path,"%s", opendcp->tmp_path);
    }

    sprintf(temp_file, "%s/tmp_%s.tif", temp_path, basename(dfile));
    OPENDCP_LOG(LOG_DEBUG, "writing temporary tif %s", temp_file);
    result = opendcp_encode_tif(opendcp, simage, temp_file);

    if (result != OPENDCP_NO_ERROR) {
        OPENDCP_LOG(LOG_ERROR,"writing temporary tif failed");
        return OPENDCP_ERROR;
    }

    /* set the max image and component sizes based on frame_rate */
    max_cs_len = ((float)bw)/8/opendcp->frame_rate;

    /* adjust cs for 3D */
    if (opendcp->stereoscopic) {
        max_cs_len = max_cs_len/2;
    }

    max_comp_size = ((float)max_cs_len)/1.25;

    sprintf(k_lengths,"Creslengths=%d Creslengths:C0=%d,%d Creslengths:C1=%d,%d Creslengths:C2=%d,%d",max_cs_len,max_cs_len,max_comp_size,max_cs_len,max_comp_size,max_cs_len,max_comp_size);

    if (opendcp->cinema_profile == DCP_CINEMA2K) {
        sprintf(cmd,"kdu_compress -i \"%s\" -o \"%s\" Sprofile=CINEMA2K %s -quiet",temp_file, dfile, k_lengths);
    } else {
        sprintf(cmd,"kdu_compress -i \"%s\" -o \"%s\" Sprofile=CINEMA4K %s -quiet",temp_file, dfile, k_lengths);
    }

    OPENDCP_LOG(LOG_DEBUG, cmd);

    cmdfp = popen(cmd,"r");
    result = pclose(cmdfp);

    remove(temp_file);

    if (result) {
        return OPENDCP_ERROR;
    }

    return OPENDCP_NO_ERROR;
}
