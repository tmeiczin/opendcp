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
        return CODEC_JP2;
    } else if (j2k.magic_num == J2K_MAGIC_NUMBER) {
        return CODEC_J2K;
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
    FILE              *fp;
    opj_image_t       *opj_image = NULL;
    opendcp_image_t      *image = 00;
    opj_dinfo_t       *dinfo = NULL;
    opj_cio_t         *cio = NULL;
    opj_dparameters_t parameters;
    j2k_image_t       j2k;
    unsigned char     *buffer = NULL;
    int               file_length, index, nread;

    int format = detect_format(sfile);

    if (format < 0) {
        OPENDCP_LOG(LOG_DEBUG,"unkown j2k format %d", format);
        return OPENDCP_ERROR;
    }

    OPENDCP_LOG(LOG_DEBUG,"opening j2k file %s", sfile);
    fp = fopen(sfile, "r");

    if (!fp) {
        OPENDCP_LOG(LOG_ERROR,"failed to open %s for reading", sfile);
        return OPENDCP_ERROR;
    }

    fseek(fp, 0, SEEK_END);
    file_length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buffer = malloc(file_length + 1);

    if (!buffer) {
        OPENDCP_LOG(LOG_ERROR,"failed to allocate memory for file");
        fclose(fp);
        return OPENDCP_ERROR;;
    }

    nread = fread(buffer, 1, file_length, fp);

    if (nread != file_length) {
        OPENDCP_LOG(LOG_ERROR,"could not read entire file, read %d but expected %d", nread, file_length);
        free(buffer);
        fclose(fp);
        return OPENDCP_ERROR;;
    }

    fclose(fp);

    opj_set_default_decoder_parameters(&parameters);

    dinfo = opj_create_decompress(format);

    if (!dinfo) {
        OPENDCP_LOG(LOG_ERROR,"failed to create decoder");
        free(buffer);
    }

    opj_setup_decoder(dinfo, &parameters);
    cio = opj_cio_open((opj_common_ptr)dinfo, buffer, file_length);
    opj_image = opj_decode(dinfo, cio);

    if (!opj_image) {
        OPENDCP_LOG(LOG_ERROR,"failed to decode image");
        opj_destroy_decompress(dinfo);
        opj_cio_close(cio);
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
    opj_destroy_decompress(dinfo);
    opj_cio_close(cio);
    opj_image_destroy(opj_image);
    free(buffer);

    *image_ptr = image;

    return OPENDCP_NO_ERROR;
}
