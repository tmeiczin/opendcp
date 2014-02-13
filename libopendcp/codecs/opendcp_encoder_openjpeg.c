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
#include <openjpeg.h>
#include <stdbool.h>
#ifdef OPENMP
#include <omp.h>
#endif
#include "opendcp.h"

#define OPJ_CPRL CPRL
#define OPJ_CLRSPC_SRGB CLRSPC_SRGB

void set_cinema_encoder_parameters(opendcp_t *opendcp, opj_cparameters_t *parameters);
static int initialize_4K_poc(opj_poc_t *POC, int numres);
int opendcp_to_opj(opendcp_image_t *opendcp, opj_image_t **opj_ptr);

static int initialize_4K_poc(opj_poc_t *POC, int numres) {
    POC[0].tile    = 1;
    POC[0].resno0  = 0;
    POC[0].compno0 = 0;
    POC[0].layno1  = 1;
    POC[0].resno1  = numres-1;
    POC[0].compno1 = 3;
    POC[0].prg1    = OPJ_CPRL;
    POC[1].tile    = 1;
    POC[1].resno0  = numres-1;
    POC[1].compno0 = 0;
    POC[1].layno1  = 1;
    POC[1].resno1  = numres;
    POC[1].compno1 = 3;
    POC[1].prg1    = OPJ_CPRL;
    return 2;
}

void set_cinema_encoder_parameters(opendcp_t *opendcp, opj_cparameters_t *parameters) {
    parameters->tile_size_on = false;
    parameters->cp_tdx=1;
    parameters->cp_tdy=1;

    /* Tile part */
    parameters->tp_flag = 'C';
    parameters->tp_on = 1;

    /* Tile and Image shall be at (0,0) */
    parameters->cp_tx0 = 0;
    parameters->cp_ty0 = 0;
    parameters->image_offset_x0 = 0;
    parameters->image_offset_y0 = 0;

    /*Codeblock size = 32*32*/
    parameters->cblockw_init = 32;
    parameters->cblockh_init = 32;
    parameters->csty |= 0x01;

    /* The progression order shall be CPRL */
    parameters->prog_order = OPJ_CPRL;

    /* No ROI */
    parameters->roi_compno = -1;

    parameters->subsampling_dx = 1;
    parameters->subsampling_dy = 1;

    /* 9-7 transform */
    parameters->irreversible = 1;

    parameters->tcp_rates[0] = 0;
    parameters->tcp_numlayers++;
    parameters->cp_disto_alloc = 1;

    parameters->cp_rsiz = opendcp->cinema_profile;
    if ( opendcp->cinema_profile == DCP_CINEMA4K ) {
            parameters->numpocs = initialize_4K_poc(parameters->POC,parameters->numresolution);
    }
}

/* convert opendcp to openjpeg image format */
int opendcp_to_opj(opendcp_image_t *opendcp, opj_image_t **opj_ptr) {
    OPJ_COLOR_SPACE color_space;
    opj_image_cmptparm_t cmptparm[3];
    opj_image_t *opj = NULL;
    int j,size;

    color_space = OPJ_CLRSPC_SRGB;

    /* initialize image components */
    memset(&cmptparm[0], 0, opendcp->n_components * sizeof(opj_image_cmptparm_t));
    for (j = 0;j <  opendcp->n_components;j++) {
            cmptparm[j].w = opendcp->w;
            cmptparm[j].h = opendcp->h;
            cmptparm[j].prec = opendcp->precision;
            cmptparm[j].bpp = opendcp->bpp;
            cmptparm[j].sgnd = opendcp->signed_bit;
            cmptparm[j].dx = opendcp->dx;
            cmptparm[j].dy = opendcp->dy;
    }

    /* create the image */
    opj = opj_image_create(opendcp->n_components, &cmptparm[0], color_space);

    if(!opj) {
        OPENDCP_LOG(LOG_ERROR,"Failed to create image");
        return OPENDCP_ERROR;
    }

    /* set image offset and reference grid */
    opj->x0 = opendcp->x0;
    opj->y0 = opendcp->y0;
    opj->x1 = opendcp->x1;
    opj->y1 = opendcp->y1;

    size = opendcp->w * opendcp->h;

    memcpy(opj->comps[0].data,opendcp->component[0].data,size*sizeof(int));
    memcpy(opj->comps[1].data,opendcp->component[1].data,size*sizeof(int));
    memcpy(opj->comps[2].data,opendcp->component[2].data,size*sizeof(int));

    *opj_ptr = opj;
    return OPENDCP_NO_ERROR;
}

/*!
 @function opendcp_encoder_openjpeg
 @abstract Encode image to file.
 @discussion This function will take the opendcp_image_t struct and encode it.
 @param opendcp An opendcp_t context struct
 @param simage The source image memory buffer to encoder
 @param dfile The output file
 @return An OPENDCP_ERROR value
*/
int opendcp_encode_openjpeg(opendcp_t *opendcp, opendcp_image_t *opendcp_image, char *dfile) {
    bool result;
    int codestream_length;
    int max_comp_size;
    int max_cs_len;
    opj_cparameters_t parameters;
    opj_image_t *opj_image = NULL;
    opj_cio_t *cio = NULL;
    opj_cinfo_t *cinfo = NULL;
    FILE *f = NULL;
    int bw;

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

    max_comp_size = ((float)max_cs_len)/1.25;

    opendcp_to_opj(opendcp_image, &opj_image);

    /* set encoding parameters to default values */
    opj_set_default_encoder_parameters(&parameters);

    /* set default cinema parameters */
    set_cinema_encoder_parameters(opendcp, &parameters);

    parameters.cp_comment = (char*)malloc(strlen(OPENDCP_NAME)+1);
    sprintf(parameters.cp_comment,"%s", OPENDCP_NAME);

    /* adjust cinema enum type */
    if (opendcp->cinema_profile == DCP_CINEMA4K) {
        parameters.cp_cinema = CINEMA4K_24;
    } else {
        parameters.cp_cinema = CINEMA2K_24;
    }

    /* Decide if MCT should be used */
    parameters.tcp_mct = opj_image->numcomps == 3 ? 1 : 0;

    /* set max image */
    parameters.max_comp_size = max_comp_size;
    parameters.tcp_rates[0]= ((float) (opj_image->numcomps * opj_image->comps[0].w * opj_image->comps[0].h * opj_image->comps[0].prec))/
                              (max_cs_len * 8 * opj_image->comps[0].dx * opj_image->comps[0].dy);

    /* get a J2K compressor handle */
    OPENDCP_LOG(LOG_DEBUG, "creating compressor %s", dfile);
    cinfo = opj_create_compress(CODEC_J2K);

    /* set event manager to null (openjpeg 1.3 bug) */
    cinfo->event_mgr = NULL;

    /* setup the encoder parameters using the current image and user parameters */
    OPENDCP_LOG(LOG_DEBUG, "setting up j2k encoder");
    opj_setup_encoder(cinfo, &parameters, opj_image);

    /* open a byte stream for writing */
    /* allocate memory for all tiles */
    OPENDCP_LOG(LOG_DEBUG, "opening J2k output stream");
    cio = opj_cio_open((opj_common_ptr)cinfo, NULL, 0);

    OPENDCP_LOG(LOG_INFO,"encoding file %s", dfile);
    result = opj_encode(cinfo, cio, opj_image, NULL);
    OPENDCP_LOG(LOG_DEBUG, "encoding file %s complete", dfile);

    if (!result) {
        OPENDCP_LOG(LOG_ERROR,"unable to encode jpeg2000 file %s", dfile);
        opj_cio_close(cio);
        opj_destroy_compress(cinfo);
        opj_image_destroy(opj_image);
        return OPENDCP_ERROR;
    }

    codestream_length = cio_tell(cio);

    f = fopen(dfile, "wb");

    if (!f) {
        OPENDCP_LOG(LOG_ERROR,"unable to write jpeg2000 file %s", dfile);
        opj_cio_close(cio);
        opj_destroy_compress(cinfo);
        opj_image_destroy(opj_image);
        return OPENDCP_ERROR;
    }

    fwrite(cio->buffer, 1, codestream_length, f);
    fclose(f);

    /* free openjpeg structure */
    opj_cio_close(cio);
    opj_destroy_compress(cinfo);
    opj_image_destroy(opj_image);

    /* free user parameters structure */
    if(parameters.cp_comment) free(parameters.cp_comment);
    if(parameters.cp_matrice) free(parameters.cp_matrice);

    return OPENDCP_NO_ERROR;
}
