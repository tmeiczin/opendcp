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

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <openjpeg.h>
#include "opendcp.h"
#include "opendcp_image.h"

#define JP2_MAGIC_NUMBER 0x0A870A0D
#define J2K_MAGIC_NUMBER 0x51FF4FFF
#define JPF_MAGIC_NUMBER 0xC000000

enum JPEG2000 {
    JPEG2000_JP2,
    JPEG2000_J2K,
    JPEG2000_UNKNOWN
};

typedef struct {
    uint32_t magic_num;        /* magic number 0x53445058 (J2K) or 0x58504453 (JP2) */
} j2k_header_t;

typedef struct {
    int bps;
    int h;
    int w;
    int size;
    int n_components;
} j2k_image_t;

int fill_j2k_image(opj_image_t *opj_image, j2k_image_t *j2k_image) {
    j2k_image->bps = opj_image->comps[0].prec;
    j2k_image->w   = opj_image->comps[0].w;
    j2k_image->h   = opj_image->comps[0].h;
    j2k_image->n_components = opj_image->numcomps;
    j2k_image->size = j2k_image->h * j2k_image->w;

    return OPENDCP_NO_ERROR;
}

int detect_format(const char *sfile) {
    FILE *fp;
    j2k_header_t     j2k;

    fp = fopen(sfile, "rb");

    if (!fp) {
        OPENDCP_LOG(LOG_DEBUG,"failed to open %s for reading", sfile);
        return OPENDCP_ERROR;
    }

    fread(&j2k, sizeof(j2k_header_t), 1, fp);
    OPENDCP_LOG(LOG_DEBUG,"reading file header %s", sfile);
    fclose(fp);

    OPENDCP_LOG(LOG_DEBUG,"magic_number %d (0x%x)", j2k.magic_num, j2k.magic_num);

    if (j2k.magic_num == JP2_MAGIC_NUMBER || j2k.magic_num == JPF_MAGIC_NUMBER) {
        return OPJ_CODEC_JP2;
    } else if (j2k.magic_num == J2K_MAGIC_NUMBER) {
        return OPJ_CODEC_J2K;
    }

    return -1;
}

/*!
 @function opendcp_decode_openjpeg
 @abstract Read an image file and populates an opendcp_image_t structure.
 @discussion This function will read and decode a file and place the
     decoded image in an opendcp_image_t struct.
 @param image_ptr Pointer to the destination opendcp_image_t struct.
 @param sfile The name if the source image file.
 @return OPENDCP_ERROR value
*/
int opendcp_decode_openjpeg(opendcp_image_t **image_ptr, const char *sfile) {
    opj_stream_t      *l_stream = NULL; 
    opj_codec_t       *l_codec = NULL;
    opj_image_t       *opj_image = NULL;
    opendcp_image_t   *image = 00;
    opj_dparameters_t parameters;
    j2k_image_t       j2k;
    int               index, result;

    int format = detect_format(sfile);

    if (format < 0) {
        OPENDCP_LOG(LOG_DEBUG,"unkown j2k format %d", format);
        return OPENDCP_ERROR;
    }

    opj_set_default_decoder_parameters(&parameters);

    l_stream = opj_stream_create_default_file_stream(sfile,1);
    if (!l_stream) {
        OPENDCP_LOG(LOG_ERROR,"could not create input file stream %s", sfile);
        return OPENDCP_ERROR;
    }


    l_codec = opj_create_decompress(format);
    if (!l_codec) {
        OPENDCP_LOG(LOG_ERROR,"failed to create decoder");
        opj_stream_destroy(l_stream);
        return OPENDCP_ERROR;
    }

    result = opj_setup_decoder(l_codec, &parameters);
    if (!result) {
        OPENDCP_LOG(LOG_ERROR,"could setup decoder %s", sfile);
        opj_stream_destroy(l_stream);
        opj_destroy_codec(l_codec);
        return OPENDCP_ERROR;
    }
  
    result = opj_read_header(l_stream, l_codec, &opj_image);
    if (!result) {
        OPENDCP_LOG(LOG_ERROR,"failed to read header %s", sfile);
        opj_stream_destroy(l_stream);
        opj_destroy_codec(l_codec);
        opj_image_destroy(opj_image);
        return OPENDCP_ERROR;
    }

    result = opj_decode(l_codec, l_stream, opj_image);
    if (!result) {
        OPENDCP_LOG(LOG_ERROR,"failed decode %s", sfile);
        opj_stream_destroy(l_stream);
        opj_destroy_codec(l_codec);
        opj_image_destroy(opj_image);
        return OPENDCP_ERROR;
    }

    result = opj_end_decompress(l_codec, l_stream);
    if (!result) {
        OPENDCP_LOG(LOG_ERROR,"failed close decompressor  %s", sfile);
        opj_stream_destroy(l_stream);
        opj_destroy_codec(l_codec);
        opj_image_destroy(opj_image);
        return OPENDCP_ERROR;
    }

    fill_j2k_image(opj_image, &j2k);

    /* create the image */
    OPENDCP_LOG(LOG_DEBUG,"allocating opendcp image %d bits", j2k.bps);
    image = opendcp_image_create(3, j2k.w, j2k.h);

    for (index = 0; index < j2k.size; index++) {
        switch (j2k.bps) {
            case 8:
                image->component[0].data[index] = opj_image->comps[0].data[index] << 4;
                image->component[1].data[index] = opj_image->comps[1].data[index] << 4;
                image->component[2].data[index] = opj_image->comps[2].data[index] << 4;
                break;
            case 12:
                image->component[0].data[index] = opj_image->comps[0].data[index];
                image->component[1].data[index] = opj_image->comps[1].data[index];
                image->component[2].data[index] = opj_image->comps[2].data[index];
                break;
           case 16:
                image->component[0].data[index] = opj_image->comps[0].data[index] >> 4;
                image->component[1].data[index] = opj_image->comps[1].data[index] >> 4;
                image->component[2].data[index] = opj_image->comps[2].data[index] >> 4;
                break;
        }
    }

    OPENDCP_LOG(LOG_DEBUG,"done reading image");
    opj_stream_destroy(l_stream);
    opj_destroy_codec(l_codec);
    opj_image_destroy(opj_image);

    *image_ptr = image;

    return OPENDCP_NO_ERROR;
}
