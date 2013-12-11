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

#ifndef _OPENDCP_IMAGE_H_
#define _OPENDCP_IMAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

enum SAMPLE_METHOD {
    SAMPLE_NONE = 0,
    NEAREST_PIXEL,
    BICUBIC
};

typedef struct {
    float r;
    float g;
    float b;
} rgb_pixel_float_t;

typedef struct {
    float x;
    float y;
    float z;
} xyz_pixel_float_t;

typedef struct {
    int component_number;
    int *data;
} opendcp_image_component_t;

typedef struct {
    int x0;
    int y0;
    int x1;
    int y1;
    int dx;
    int dy;
    int w;
    int h;
    int bpp;
    int precision;
    int signed_bit;
    opendcp_image_component_t *component;
    int n_components;
} opendcp_image_t;

int  read_image(opendcp_image_t **image, char *file);
void opendcp_image_free(opendcp_image_t *image);
int opendcp_image_size(opendcp_image_t *opendcp_image);
int  opendcp_image_readline(opendcp_image_t *image, int y, unsigned char *data);
int  rgb_to_xyz(opendcp_image_t *image, int gamma, int method);
int  resize(opendcp_image_t **image, int profile, int method);
rgb_pixel_float_t yuv444toRGB888(int y, int cb, int cr);
opendcp_image_t *opendcp_image_create(int n_components, int w, int h);

#ifdef __cplusplus
}
#endif

#endif  //_OPENDCP_H_
