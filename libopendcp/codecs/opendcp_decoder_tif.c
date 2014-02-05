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
#include <tiffio.h>
#include "opendcp.h"
#include "opendcp_image.h"

typedef struct {
    TIFF       *fp;
    tstrip_t   strip;
    tdata_t    strip_data;
    tsize_t    strip_size;
    tsize_t    read_size;
    uint32_t   strip_num;
    uint32_t   image_size;
    int        w;
    int        h;
    uint16_t   bps;
    uint16_t   spp;
    uint16_t   photo;
    uint16_t   planar;
    int        supported;
} tiff_image_t;

void opendcp_tif_set_strip(tiff_image_t *tif) {
    tif->strip_num  = TIFFNumberOfStrips(tif->fp);
    tif->strip_size = TIFFStripSize(tif->fp);
    tif->strip_data = _TIFFmalloc(tif->strip_size);
}

/*!
 @function opendcp_decode_tif
 @abstract Read an image file and populates an opendcp_image_t structure.
 @discussion This function will read and decode a file and place the
     decoded image in an opendcp_image_t struct.
 @param image_ptr Pointer to the destination opendcp_image_t struct.
 @param sfile The name if the source image file.
 @return OPENDCP_ERROR value
*/
int opendcp_decode_tif(opendcp_image_t **image_ptr, const char *sfile) {
    tiff_image_t tif;
    unsigned int i,index;
    opendcp_image_t *image = 00;

    TIFFSetWarningHandler(NULL);
    memset(&tif, 0, sizeof(tiff_image_t));

    /* open tiff using filename or file descriptor */
    OPENDCP_LOG(LOG_DEBUG,"opening tiff file %s", sfile);
    tif.fp = TIFFOpen(sfile, "r");

    if (!tif.fp) {
        OPENDCP_LOG(LOG_ERROR,"failed to open %s for reading", sfile);
        return OPENDCP_ERROR;
    }

    TIFFGetField(tif.fp, TIFFTAG_IMAGEWIDTH, &tif.w);
    TIFFGetField(tif.fp, TIFFTAG_IMAGELENGTH, &tif.h);
    TIFFGetField(tif.fp, TIFFTAG_BITSPERSAMPLE, &tif.bps);
    TIFFGetField(tif.fp, TIFFTAG_SAMPLESPERPIXEL, &tif.spp);
    TIFFGetField(tif.fp, TIFFTAG_PHOTOMETRIC, &tif.photo);
    TIFFGetField(tif.fp, TIFFTAG_PLANARCONFIG, &tif.planar);
    tif.image_size = tif.w * tif.h;

    OPENDCP_LOG(LOG_DEBUG,"tif attributes photo: %d bps: %d spp: %d planar: %d",tif.photo,tif.bps,tif.spp,tif.planar);

    /* check if image is supported */
    switch (tif.photo) {
        case PHOTOMETRIC_YCBCR:
            if (tif.bps == 8 || tif.bps == 16 || tif.bps == 24) {
                tif.supported = 1;
            } else {
                OPENDCP_LOG(LOG_ERROR,"YUV/YCbCr tiff conversion failed, bitdepth %d, only 8,16,24 bits are supported", tif.bps);
            }
            break;
        case PHOTOMETRIC_MINISWHITE:
            OPENDCP_LOG(LOG_ERROR,"1-bit BW images are not supported", tif.bps);
            break;
        case PHOTOMETRIC_MINISBLACK:
            if (tif.bps == 1 || tif.bps == 8 || tif.bps == 16 || tif.bps == 24) {
                tif.supported = 1;
            } else {
                OPENDCP_LOG(LOG_ERROR,"grayscale tiff conversion failed, bitdepth %d, only 8,16,24 bits are supported", tif.bps);
            }
            break;
        case PHOTOMETRIC_RGB:
            if (tif.bps == 8 || tif.bps == 12 || tif.bps == 16) {
                tif.supported = 1;
            } else {
                OPENDCP_LOG(LOG_ERROR,"RGB tiff conversion failed, bitdepth %d, only 8,12,16 bits are supported", tif.bps);
            }
            break;
        default:
            tif.supported = 0;
            OPENDCP_LOG(LOG_ERROR,"tif image type not supported");
            break;
    }

    if (tif.supported == 0) {
        TIFFClose(tif.fp);
       return(OPENDCP_ERROR);
    }

    /* create the image */
    OPENDCP_LOG(LOG_DEBUG,"allocating opendcp image");
    image = opendcp_image_create(3,tif.w,tif.h);

    if (!image) {
        TIFFClose(tif.fp);
        OPENDCP_LOG(LOG_ERROR,"failed to create image %s",sfile);
        return OPENDCP_ERROR;
    }

    /* BW */
    if (tif.photo == PHOTOMETRIC_MINISWHITE) {
        opendcp_tif_set_strip(&tif);
        uint8_t *data  = (uint8_t *)tif.strip_data;
        for (tif.strip = 0; tif.strip < tif.strip_num; tif.strip++) {
            tif.read_size = TIFFReadEncodedStrip(tif.fp, tif.strip, tif.strip_data, tif.strip_size);
            for (i=0; i<tif.image_size; i++) {
                image->component[0].data[i] = data[i] << 4; // R
                image->component[1].data[i] = data[i] << 4; // G
                image->component[2].data[i] = data[i] << 4; // B
            }
        }
        _TIFFfree(tif.strip_data);
    }

    /* GRAYSCALE */
    else if (tif.photo == PHOTOMETRIC_MINISBLACK) {
        uint32_t *raster = (uint32_t*) _TIFFmalloc(tif.image_size * sizeof(uint32_t));
        TIFFReadRGBAImageOriented(tif.fp, tif.w, tif.h, raster, ORIENTATION_TOPLEFT,0);
        /* 8/16/24 bits per pixel */
        if (tif.bps==8 || tif.bps==16 || tif.bps==24) {
            for (i=0;i<tif.image_size;i++) {
                image->component[0].data[i] = (raster[i] & 0xFF)       << 4;
                image->component[1].data[i] = (raster[i] >> 8 & 0xFF)  << 4;
                image->component[2].data[i] = (raster[i] >> 16 & 0xFF) << 4;
            }
        }
        _TIFFfree(raster);
    }

    /* YUV */
    else if (tif.photo == PHOTOMETRIC_YCBCR) {
        uint32_t *raster = (uint32_t*) _TIFFmalloc(tif.image_size * sizeof(uint32_t));
        TIFFReadRGBAImageOriented(tif.fp, tif.w, tif.h, raster, ORIENTATION_TOPLEFT,0);
        /* 8/16/24 bits per pixel */
        if (tif.bps==8 || tif.bps==16 || tif.bps==24) {
            for (i=0;i<tif.image_size;i++) {
                image->component[0].data[i] = (raster[i] & 0xFF)       << 4;
                image->component[1].data[i] = (raster[i] >> 8 & 0xFF)  << 4;
                image->component[2].data[i] = (raster[i] >> 16 & 0xFF) << 4;
            }
        }
        _TIFFfree(raster);
    }

    /* RGB(A) and GRAYSCALE */
    else if (tif.photo == PHOTOMETRIC_RGB) {
        opendcp_tif_set_strip(&tif);
        uint8_t *data  = (uint8_t *)tif.strip_data;

        /* 8 bits per pixel */
        if (tif.bps==8) {
            index = 0;
            for (tif.strip = 0; tif.strip < tif.strip_num; tif.strip++) {
                tif.read_size = TIFFReadEncodedStrip(tif.fp, tif.strip, tif.strip_data, tif.strip_size);
                for (i=0; i<tif.read_size && index<tif.image_size; i+=tif.spp) {
                    /* rounded to 12 bits */
                    image->component[0].data[index] = data[i+0] << 4; // R
                    image->component[1].data[index] = data[i+1] << 4; // G
                    image->component[2].data[index] = data[i+2] << 4; // B
                    index++;
                }
            }
        /*12 bits per pixel*/
        } else if (tif.bps==12) {
            index = 0;
            for (tif.strip = 0; tif.strip < tif.strip_num; tif.strip++) {
                tif.read_size = TIFFReadEncodedStrip(tif.fp, tif.strip, tif.strip_data, tif.strip_size);
                for (i=0; i<tif.read_size && (index+1)<tif.image_size; i+=(3*tif.spp)) {
                    image->component[0].data[index]   = ( data[i+0] << 4)         | (data[i+1] >> 4); // R
                    image->component[1].data[index]   = ((data[i+1] & 0x0f) << 8) | (data[i+2]);      // G
                    image->component[2].data[index]   = ( data[i+3] << 4)         | (data[i+4] >> 4); // B
                    if (tif.spp == 4) {
                        /* skip alpha channel */
                        image->component[0].data[index+1] = ( data[i+6] << 4)         | (data[i+7] >> 4);  // R
                        image->component[1].data[index+1] = ((data[i+7] & 0x0f) << 8) | (data[i+8]);       // G
                        image->component[2].data[index+1] = ( data[i+9] << 4)         | (data[i+10] >> 4); // B
                    } else {
                        image->component[0].data[index+1] = ((data[i+4] & 0x0f) << 8) | (data[i+5]);       // R
                        image->component[1].data[index+1] = ( data[i+6] <<4 )         | (data[i+7] >> 4);  // G
                        image->component[2].data[index+1] = ((data[i+7] & 0x0f) << 8) | (data[i+8]);       // B
                    }
                    index+=2;
                }
            }
        /* 16 bits per pixel */
        } else if (tif.bps==16) {
            index = 0;
            for (tif.strip = 0; tif.strip < tif.strip_num; tif.strip++) {
                tif.read_size = TIFFReadEncodedStrip(tif.fp, tif.strip, tif.strip_data, tif.strip_size);
                for (i=0; i<tif.read_size && index<tif.image_size; i+=(2*tif.spp)) {
                    /* rounded to 12 bits */
                    image->component[0].data[index] = ((data[i+1] << 8) | data[i+0]) >> 4; // R
                    image->component[1].data[index] = ((data[i+3] << 8) | data[i+2]) >> 4; // G
                    image->component[2].data[index] = ((data[i+5] << 8) | data[i+4]) >> 4; // B
                    index++;
                }
            }
        }
        _TIFFfree(tif.strip_data);
    }

    TIFFClose(tif.fp);

    OPENDCP_LOG(LOG_DEBUG,"tiff read complete");
    *image_ptr = image;

    return OPENDCP_NO_ERROR;
}
