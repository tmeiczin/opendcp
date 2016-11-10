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
#include <sys/stat.h>
#include "opendcp.h"
#include "opendcp_image.h"
#include "opendcp_xyz.h"
#include "codecs/opendcp_decoder.h"

#define CLIP(m,max)                                 \
  (m)<0?0:((m)>max?max:(m))

extern int rgb_to_xyz_calculate(opendcp_image_t *image, int index);
extern int rgb_to_xyz_lut(opendcp_image_t *image, int index);

/* create opendcp image structure for int */
opendcp_image_t *opendcp_image_create(int n_components, int w, int h) {
    int x;
    opendcp_image_t *image = 00;

    image = (opendcp_image_t*) malloc(sizeof(opendcp_image_t));

    if (image) {
        memset(image, 0, sizeof(opendcp_image_t));
        image->component = (opendcp_image_component_t*) malloc(n_components * sizeof(opendcp_image_component_t));

        if (!image->component) {
            OPENDCP_LOG(LOG_ERROR, "unable to allocate memory for image components");
            opendcp_image_free(image);
            return 00;
        }

        memset(image->component, 0, n_components * sizeof(opendcp_image_component_t));

        for (x = 0; x < n_components; x++) {
            image->component[x].component_number = x;
            image->component[x].data = (int *)malloc((w * h) * sizeof(int));

            if (!image->component[x].data) {
                OPENDCP_LOG(LOG_ERROR, "unable to allocate memory for image components");
                opendcp_image_free(image);
                return NULL;
            }
        }
    }

    /* set default image parameters 12-bit RGB integer */
    image->bpp          = 12;
    image->precision    = 12;
    image->n_components = 3;
    image->signed_bit   = 0;  /* use unsigned integer */
    image->dx           = 1;
    image->dy           = 1;
    image->w            = w;  /* image width  (pixel) */
    image->h            = h;  /* image height (pixel) */
    image->x0           = 0;
    image->y0           = 0;
    image->x1 = !image->x0 ? (w - 1) * image->dx + 1 : image->x0 + (w - 1) * image->dx + 1;
    image->y1 = !image->y0 ? (h - 1) * image->dy + 1 : image->y0 + (h - 1) * image->dy + 1;
    image->use_float    = 0;   /* use integer data */

    return image;
}

/* create opendcp image structure for float */
opendcp_image_t *opendcp_image_float_create(int n_components, int w, int h) {
    int x;
    opendcp_image_t *image = 00;

    image = (opendcp_image_t*) malloc(sizeof(opendcp_image_t));

    if (image) {
        memset(image, 0, sizeof(opendcp_image_t));
        image->component = (opendcp_image_component_t*) malloc(n_components * sizeof(opendcp_image_component_t));

        if (!image->component) {
            OPENDCP_LOG(LOG_ERROR, "unable to allocate memory for image components");
            opendcp_image_free(image);
            return 00;
        }

        memset(image->component, 0, n_components * sizeof(opendcp_image_component_t));

        for (x = 0; x < n_components; x++) {
            image->component[x].component_number = x;
            image->component[x].float_data = (int *)malloc((w * h) * sizeof(int));

            if (!image->component[x].float_data) {
                OPENDCP_LOG(LOG_ERROR, "unable to allocate memory for image float components");
                opendcp_image_free(image);
                return NULL;
            }
        }
    }

    /* set default image parameters 12-bit RGB integer */
    image->bpp          = 12; /* bit per pixel (output after transforms) */
    image->precision    = 12;
    image->n_components = 3;
    image->signed_bit   = 0;  /* use unsigned integer */
    image->dx           = 1;
    image->dy           = 1;
    image->w            = w;  /* image width  (pixel) */
    image->h            = h;  /* image height (pixel) */
    image->x0           = 0;
    image->y0           = 0;
    image->x1 = !image->x0 ? (w - 1) * image->dx + 1 : image->x0 + (w - 1) * image->dx + 1;
    image->y1 = !image->y0 ? (h - 1) * image->dy + 1 : image->y0 + (h - 1) * image->dy + 1;
    image->use_float    = 1;  /* use float data */

    return image;
}

/* free open DCP image structure (int data) */
void opendcp_image_free(opendcp_image_t *opendcp_image) {
    int i;

    if (opendcp_image) {
        if (opendcp_image->component) {
            for (i = 0; i < opendcp_image->n_components; i++) {
                opendcp_image_component_t *component = &opendcp_image->component[i];

                if (component->data) {
                    free(component->data);
                }
            }

            free(opendcp_image->component);
        }

        free(opendcp_image);
    }
}

/* free open DCP image structure (float data) */
void opendcp_image_free_float(opendcp_image_t *opendcp_image) {
    int i;

    if (opendcp_image) {
        if (opendcp_image->component) {
            for (i = 0; i < opendcp_image->n_components; i++) {
                opendcp_image_component_t *component = &opendcp_image->component[i];

                if (component->float_data) {
                    free(component->float_data);
                }
            }

            free(opendcp_image->component);
        }

        free(opendcp_image);
    }
}

/* open DCP image size */
int opendcp_image_size(opendcp_image_t *opendcp_image) {
    int i, size;

    size = sizeof(opendcp_image_t);

    for (i = 0; i < opendcp_image->n_components; i++) {
        size += opendcp_image->w * opendcp_image->h * sizeof(int);
    }

    return size;
}

int read_image(opendcp_image_t **dimage, char *sfile) {
    char *extension;
    int  result;
    opendcp_decoder_t *decoder;;

    extension = strrchr(sfile, '.');
    extension++;

    decoder = opendcp_decoder_find(NULL, extension, 0);

    if (decoder->id == OPENDCP_DECODER_NONE) {
        OPENDCP_LOG(LOG_ERROR, "no Decoder found for image extension %s", extension);
        return OPENDCP_ERROR;
    }

    OPENDCP_LOG(LOG_DEBUG, "decoder %s found for image extension %s", decoder->name, extension);

    /* int or float data changes with decoder */
    result  = decoder->decode(dimage, sfile);

    if (result != OPENDCP_NO_ERROR) {
        return OPENDCP_ERROR;
    }

    return OPENDCP_NO_ERROR;
}

/* open DCP read line (int data) */
int opendcp_image_readline(opendcp_image_t *image, int y, unsigned char *dbuffer) {
    int x, i;
    int d = 0;

    for (x = 0; x < image->w; x += 2) {
        i = (x + y + 0) + ((image->w - 1) * y);
        /* dbuffer use big endian order, 12 bit * 3 = 36 bit pixel, 2 pixel 72 bit = 9 byte */
        /* prepare for jpeg 2000 convsion ? */
        dbuffer[d + 0] = (image->component[0].data[i] >> 4);
        dbuffer[d + 1] = ((image->component[0].data[i] & 0x0f) << 4 ) | ((image->component[1].data[i] >> 8) & 0x0f);
        dbuffer[d + 2] = image->component[1].data[i];
        dbuffer[d + 3] = (image->component[2].data[i] >> 4);
        dbuffer[d + 4] = ((image->component[2].data[i] & 0x0f) << 4 ) | ((image->component[0].data[i + 1] >> 8) & 0x0f);
        dbuffer[d + 5] = image->component[0].data[i + 1];
        dbuffer[d + 6] = (image->component[1].data[i + 1] >> 4);
        dbuffer[d + 7] = ((image->component[1].data[i + 1] & 0x0f) << 4 ) | ((image->component[2].data[i + 1] >> 8) & 0x0f);
        dbuffer[d + 8] = image->component[2].data[i + 1];
        d += 9;
    }

    return OPENDCP_NO_ERROR;
}

/* open DCP read line (float data) Need understand dbuffer before can fix this function */
int opendcp_image_readline_float(opendcp_image_t *image, int y, unsigned char *dbuffer) {
    int x, i;
    int d = 0;

    for (x = 0; x < image->w; x += 2) {
        i = (x + y + 0) + ((image->w - 1) * y);
         /* get componets for two pixels, convert to 12 bit int */
        /* pixel 0 */
        int pixel0_b = (int)4095*image->component[0].float_data[i];
        int pixel0_g = (int)4095*image->component[1].float_data[i];
        int pixel0_r = (int)4095*image->component[2].float_data[i];
        /* pixel 1 */
        int pixel1_b = (int)4095*image->component[0].float_data[i+1];
        int pixel1_g = (int)4095*image->component[1].float_data[i+1];
        int pixel1_r = (int)4095*image->component[2].float_data[i+1];
        /* put pixel data in dbuffer */
        dbuffer[d + 0] = pixel0_b >> 4;
        dbuffer[d + 1] = (pixel0_b & 0x0f) << 4 ) | ((pixel0_g >> 8) & 0x0f);
        dbuffer[d + 2] = pixel0_g;
        dbuffer[d + 3] = pixel0_r >> 4;
        dbuffer[d + 4] = (pixel0_r & 0x0f) << 4 ) | ((pixel1_b >> 8) & 0x0f);
        dbuffer[d + 5] = pixel1_b;
        dbuffer[d + 6] = (pixel1_g >> 4);
        dbuffer[d + 7] = (pixel1_g << 4 ) | (pixel1_r >> 8) & 0x0f);
        dbuffer[d + 8] = pixel1_r;
        d += 9;
    }

    return OPENDCP_NO_ERROR;
}

int check_image_compliance(int profile, opendcp_image_t *image, char *file) {
    int w, h;
    int dci_w = MAX_WIDTH_2K;
    int dci_h = MAX_HEIGHT_2K;
    opendcp_image_t *tmp;

    if (image == NULL) {
        OPENDCP_LOG(LOG_DEBUG, "reading file %s", file);

        if (read_image(&tmp, file) == OPENDCP_NO_ERROR) {
            h = tmp->h;
            w = tmp->w;
            opendcp_image_free(tmp);
        }
        else {
            opendcp_image_free(tmp);
            return OPENDCP_ERROR;
        }
    }
    else {
        h = image->h;
        w = image->w;
    }

    if (profile == DCP_CINEMA4K) {
        dci_w = dci_w *2;
        dci_h = dci_h *2;
    }

    if ((w != dci_w) && (h != dci_h)) {
        OPENDCP_LOG(LOG_WARN, "image does not match at least one dimension of the DCI container");
        return OPENDCP_ERROR;
    }

    if ((w > dci_w) || (h > dci_h)) {
        OPENDCP_LOG(LOG_WARN, "image dimension exceeds DCI container");
        return OPENDCP_ERROR;
    }

    if ((w % 2) || (h % 2)) {
        OPENDCP_LOG(LOG_WARN, "image dimensions are not an even value");
        return OPENDCP_ERROR;
    }

    return OPENDCP_NO_ERROR;
}

/* yuv444 to rgb 8888 (int data) */
rgb_pixel_float_t yuv444toRGB888(int y, int cb, int cr) {
    rgb_pixel_float_t p;

    p.r = CLIP(y + 1.402 * (cr - 128), 255);
    p.g = CLIP(y - 0.344 * (cb - 128) - 0.714 * (cr - 128), 255);
    p.b = CLIP(y + 1.772 * (cb - 128), 255);

    return(p);
}

/* yuv444 to rgb 8888 (float data) */
rgb_pixel_float_t yuv444toRGB888_float(float y, float cb, float cr) {
    rgb_pixel_float_t p;

    p.r = CLIP(y + 1.402f * (cr - 0.5f), 1.0f);
    p.g = CLIP(y - 0.344f * (cb - 0.5f) - 0.714 * (cr - 0.5f), 1.0f);
    p.b = CLIP(y + 1.772f * (cb - 0.5f), 1.0f);

    return(p);
}

/* complex gamma function */
float complex_gamma(float p, float gamma, int index) {
    float v;

    p = p / COLOR_DEPTH;

    if (index) {
        if (p > 0.081) {
            v = pow((p + 0.099) / 1.099, gamma);
        }
        else {
            v = p / 4.5;
            v = pow((p + 0.099) / 1.099, gamma);
        }
    }
    else {
        if (p > 0.04045) {
            v = pow((p + 0.055) / 1.055, gamma);
        }
        else {
            v = p / 12.92;
        }
    }

    return v;
}

int adjust_headroom(int p) {
    if (p < HEADROOM * 3)  {
        return  p - HEADROOM;
    }

    return p;
}

/* dci transfer (int data) */
int dci_transfer(float p) {
    int v;

    v = (pow((p * DCI_COEFFICENT), DCI_DEGAMMA) * COLOR_DEPTH);

    v -= HEADROOM;

    return v;
}

/* dci transfer (floatt data) */
float dci_transfer(float p) {
    float v;

    v = pow((p * DCI_COEFFICENT), DCI_DEGAMMA);
    // adjust value below full color pixel?
    v -= HEADROOM/4096.0f;  //<---- assume int data function give 12 bit data

    return v;
}

/* dci transfer inverse (int data) */
int dci_transfer_inverse(float p) {
    p = p / COLOR_DEPTH;

    return (pow(p, 1 / DCI_GAMMA));
}

/* dci transfer inverse (float data) */
float dci_transfer_inverse(float p) {

    return (pow(p, 1 / DCI_GAMMA));
}

/* rbg to xyz (int data) */
int rgb_to_xyz(opendcp_image_t *image, int index, int method) {
    int result;

    if (method) {
        OPENDCP_LOG(LOG_DEBUG, "rgb_to_xyz_calculate, index: %d", index);
        result = rgb_to_xyz_calculate(image, index);
    }
    else {
        OPENDCP_LOG(LOG_DEBUG, "rgb_to_xyz_lut, index: %d", index);
        result = rgb_to_xyz_lut(image, index);
    }

    return result;
}

/* rbg to xyz (float data) */
float rgb_to_xyz_float(opendcp_image_t *image, int index) {
    int result;

    OPENDCP_LOG(LOG_DEBUG, "rgb_to_xyz_calculate_float, index: %d", index);
    result = rgb_to_xyz_calculate_float(image, index);

    return result;
}

/* rgb to xyz color conversion 12-bit LUT (only for int data) */
int rgb_to_xyz_lut(opendcp_image_t *image, int index) {
    int i;
    int size;
    rgb_pixel_float_t s;
    xyz_pixel_float_t d;

    size = image->w * image->h;

    for (i = 0; i < size; i++) {
        /* in gamma lut */
        s.r = lut_in[index][image->component[0].data[i]];
        s.g = lut_in[index][image->component[1].data[i]];
        s.b = lut_in[index][image->component[2].data[i]];

        /* RGB to XYZ Matrix */
        d.x = ((s.r * color_matrix[index][0][0]) + (s.g * color_matrix[index][0][1]) + (s.b * color_matrix[index][0][2]));
        d.y = ((s.r * color_matrix[index][1][0]) + (s.g * color_matrix[index][1][1]) + (s.b * color_matrix[index][1][2]));
        d.z = ((s.r * color_matrix[index][2][0]) + (s.g * color_matrix[index][2][1]) + (s.b * color_matrix[index][2][2]));

        /* DCI Companding */
        d.x = d.x * DCI_COEFFICENT * (DCI_LUT_SIZE - 1);
        d.y = d.y * DCI_COEFFICENT * (DCI_LUT_SIZE - 1);
        d.z = d.z * DCI_COEFFICENT * (DCI_LUT_SIZE - 1);

        /* out gamma lut */
        image->component[0].data[i] = lut_out[LO_DCI][(int)d.x];
        image->component[1].data[i] = lut_out[LO_DCI][(int)d.y];
        image->component[2].data[i] = lut_out[LO_DCI][(int)d.z];
    }

    return OPENDCP_NO_ERROR;
}

/* rgb to xyz color conversion hard calculations (int data) */
int rgb_to_xyz_calculate(opendcp_image_t *image, int index) {
    int i;
    int size;
    rgb_pixel_float_t s;
    xyz_pixel_float_t d;

    size = image->w * image->h;
    OPENDCP_LOG(LOG_DEBUG, "gamma: %f", GAMMA[index]);

    for (i = 0; i < size; i++) {
        s.r = complex_gamma(image->component[0].data[i], GAMMA[index], index);
        s.g = complex_gamma(image->component[1].data[i], GAMMA[index], index);
        s.b = complex_gamma(image->component[2].data[i], GAMMA[index], index);

        d.x = ((s.r * color_matrix[index][0][0]) + (s.g * color_matrix[index][0][1]) + (s.b * color_matrix[index][0][2]));
        d.y = ((s.r * color_matrix[index][1][0]) + (s.g * color_matrix[index][1][1]) + (s.b * color_matrix[index][1][2]));
        d.z = ((s.r * color_matrix[index][2][0]) + (s.g * color_matrix[index][2][1]) + (s.b * color_matrix[index][2][2]));

        image->component[0].data[i] = dci_transfer(d.x);
        image->component[1].data[i] = dci_transfer(d.y);
        image->component[2].data[i] = dci_transfer(d.z);
    }

    return OPENDCP_NO_ERROR;
}

/* rgb to xyz color conversion hard calculations (float data) */
int rgb_to_xyz_calculate_float(opendcp_image_t *image, int index) {
    int i;
    int size;
    rgb_pixel_float_t s;
    xyz_pixel_float_t d;

    size = image->w * image->h;
    OPENDCP_LOG(LOG_DEBUG, "gamma: %f", GAMMA[index]);

    for (i = 0; i < size; i++) {
        s.r = complex_gamma(image->component[0].float_data[i], GAMMA[index], index);
        s.g = complex_gamma(image->component[1].float_data[i], GAMMA[index], index);
        s.b = complex_gamma(image->component[2].float_data[i], GAMMA[index], index);

        d.x = ((s.r * color_matrix[index][0][0]) + (s.g * color_matrix[index][0][1]) + (s.b * color_matrix[index][0][2]));
        d.y = ((s.r * color_matrix[index][1][0]) + (s.g * color_matrix[index][1][1]) + (s.b * color_matrix[index][1][2]));
        d.z = ((s.r * color_matrix[index][2][0]) + (s.g * color_matrix[index][2][1]) + (s.b * color_matrix[index][2][2]));

        image->component[0].float_data[i] = dci_transfer_float(d.x);
        image->component[1].float_data[i] = dci_transfer_float(d.y);
        image->component[2].float_data[i] = dci_transfer_float(d.z);
    }

    return OPENDCP_NO_ERROR;
}

float b_spline(float x) {
    float c = (float)1 / (float)6;

    if (x > 2.0 || x < -2.0) {
        return 0.0f;
    }

    if (x < -1.0) {
        return(((x + 2.0f) * (x + 2.0f) * (x + 2.0f)) * c);
    }

    if (x < 0.0) {
        return(((x + 4.0f) * (x) * (-6.0f - 3.0f * x)) * c);
    }

    if (x < 1.0) {
        return(((x + 4.0f) * (x) * (-6.0f + 3.0f * x)) * c);
    }

    if (x < 2.0) {
        return(((2.0f - x) * (2.0f - x) * (2.0f - x)) * c);
    }

    return 0;
}

/* get the pixel index based on x,y (int data) */
static inline rgb_pixel_float_t get_pixel(opendcp_image_t *image, int x, int y) {
    rgb_pixel_float_t p;
    int i;

    i = x + (image->w * y);
    //i = (x+y) + ((image->w-1)*y);

    p.r = image->component[0].data[i];
    p.g = image->component[1].data[i];
    p.b = image->component[2].data[i];

    return p;
}

/* get the pixel index based on x,y (float data) */
static inline rgb_pixel_float_t get_pixel_float(opendcp_image_t *image, int x, int y) {
    rgb_pixel_float_t p;
    int i;

    i = x + (image->w * y);
    //i = (x+y) + ((image->w-1)*y);

    p.r = image->component[0].float_data[i];
    p.g = image->component[1].float_data[i];
    p.b = image->component[2].float_data[i];

    return p;
}

int letterbox(opendcp_image_t **image, int w, int h) {
    int num_components = 3;
    int x, y;
    opendcp_image_t *ptr = *image;
    rgb_pixel_float_t p;

    /* create the image */
    opendcp_image_t *d_image = opendcp_image_create(num_components, w, h);

    if (!d_image) {
        return -1;
    }

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            p = get_pixel(ptr, x, y);
            d_image->component[0].data[0] = (int)p.r;
            d_image->component[1].data[0] = (int)p.g;
            d_image->component[2].data[0] = (int)p.b;
        }
    }

    opendcp_image_free(*image);
    *image = d_image;

    return OPENDCP_NO_ERROR;
}

/* letter box (float data) */
int letterbox_float(opendcp_image_t **image, int w, int h) {
    int num_components = 3;
    int x, y;
    opendcp_image_t *ptr = *image;
    rgb_pixel_float_t p;

    /* create the image */
    opendcp_image_t *d_image = opendcp_image_create(num_components, w, h);

    if (!d_image) {
        return -1;
    }

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            p = get_pixel(ptr, x, y);
            d_image->component[0].float_data[0] = p.r;
            d_image->component[1].float_data[0] = p.g;
            d_image->component[2].float_data[0] = p.b;
        }
    }

    opendcp_image_free_float(*image);
    *image = d_image;

    return OPENDCP_NO_ERROR;
}
/* resize image (int data) */
int resize(opendcp_image_t **image, int profile, int method) {
    int num_components = 3;
    opendcp_image_t *ptr = *image;
    rgb_pixel_float_t p;
    int w, h;
    float aspect;

    /* aspect ratio */
    aspect = (float)ptr->w / (float)ptr->h;

    /* depending on the aspect ratio, set height or weight priority */
    if (aspect <= 2.10) {
        w = (ptr->w * MAX_HEIGHT_2K / ptr->h);
        h = MAX_HEIGHT_2K;
    }
    else {
        w = MAX_WIDTH_2K;
        h = (ptr->h * MAX_WIDTH_2K / ptr->w);
    }

    /* if we overshot width, scale back */
    if (w > MAX_WIDTH_2K) {
        w = MAX_WIDTH_2K;
        h = w / aspect;
    }

    /* if we overshot height, scale back */
    if (h > MAX_HEIGHT_2K) {
        h = MAX_HEIGHT_2K;
        w = h * aspect;
    }

    /* the image dimensions must be even values */
    if (w % 2) {
        w = w - 1;
    }

    if (h % 2) {
        h = h - 1;
    }

    /* adjust for 4K */
    if (profile == DCP_CINEMA4K) {
        w *= 2;
        h *= 2;
    }

    OPENDCP_LOG(LOG_INFO, "resizing from %dx%d to %dx%d (%f) (int data)", ptr->w, ptr->h, w, h, aspect);

    /* create the image */
    opendcp_image_t *d_image = opendcp_image_create(num_components, w, h);

    if (!d_image) {
        return -1;
    }

    /* simple resize - pixel double */
    if (method == NEAREST_PIXEL) {
        int x, y, i, dx, dy;
        float tx, ty;

        tx = (float)ptr->w / w;
        ty = (float)ptr->h / h;

        for (y = 0; y < h; y++) {
            dy = y * ty;

            for (x = 0; x < w; x++) {
                dx = x * tx;
                p = get_pixel(ptr, dx, dy);
                i = x + (w * y);
                d_image->component[0].data[i] = (int)p.r;
                d_image->component[1].data[i] = (int)p.g;
                d_image->component[2].data[i] = (int)p.b;
            }
        }
    }

    opendcp_image_free(*image);
    *image = d_image;

    return OPENDCP_NO_ERROR;
}

/* resize image (float data) */
int resize_float(opendcp_image_t **image, int profile, int method) {
    int num_components = 3;
    opendcp_image_t *ptr = *image;
    rgb_pixel_float_t p;
    int w, h;
    float aspect;

    /* aspect ratio */
    aspect = (float)ptr->w / (float)ptr->h;

    /* depending on the aspect ratio, set height or weight priority */
    if (aspect <= 2.10) {
        w = (ptr->w * MAX_HEIGHT_2K / ptr->h);
        h = MAX_HEIGHT_2K;
    }
    else {
        w = MAX_WIDTH_2K;
        h = (ptr->h * MAX_WIDTH_2K / ptr->w);
    }

    /* if we overshot width, scale back */
    if (w > MAX_WIDTH_2K) {
        w = MAX_WIDTH_2K;
        h = w / aspect;
    }

    /* if we overshot height, scale back */
    if (h > MAX_HEIGHT_2K) {
        h = MAX_HEIGHT_2K;
        w = h * aspect;
    }

    /* the image dimensions must be even values */
    if (w % 2) {
        w = w - 1;
    }

    if (h % 2) {
        h = h - 1;
    }

    /* adjust for 4K */
    if (profile == DCP_CINEMA4K) {
        w *= 2;
        h *= 2;
    }

    OPENDCP_LOG(LOG_INFO, "resizing from %dx%d to %dx%d (%f) (float data)", ptr->w, ptr->h, w, h, aspect);

    /* create the image */
    opendcp_image_t *d_image = opendcp_image_create_float(num_components, w, h);

    if (!d_image) {
        return -1;
    }

    /* simple resize - pixel double */
    if (method == NEAREST_PIXEL) {
        int x, y, i, dx, dy;
        float tx, ty;

        tx = (float)ptr->w / w;
        ty = (float)ptr->h / h;

        for (y = 0; y < h; y++) {
            dy = y * ty;

            for (x = 0; x < w; x++) {
                dx = x * tx;
                p = get_pixel(ptr, dx, dy);
                i = x + (w * y);
                d_image->component[0].float_data[i] = p.r;
                d_image->component[1].float_data[i] = p.g;
                d_image->component[2].float_data[i] = p.b;
            }
        }
    }

    opendcp_image_free_float(*image);
    *image = d_image;

    return OPENDCP_NO_ERROR;
}
