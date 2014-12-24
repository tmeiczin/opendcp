/*
    OpenDCP: Builds Digital Cinema Packages
    Copyright (c) 2010-2014 Terrence Meiczinger, All Rights Reserved

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
#include <sys/stat.h>
#include "opendcp.h"
#include "opendcp_image.h"
#include "opendcp_writer.h"
#include "codecs/opendcp_encoder.h"

enum {
    WRITER_FILELIST = 0,
    WRITER_MXF, 
} writers;

typedef struct {
    filelist_t *filelist;
} filelist_writer_t;

int filelist_write_frame(opendcp_writer_t *self, int frame, opendcp_t *opendcp, opendcp_image_t *image);
int filelist_write_next(opendcp_writer_t *self, opendcp_image_t *image);

int is_a_dir(char *path) {
    struct stat st_in;

    if (stat(path, &st_in) != 0 ) {
        return 0;
    }

    if (S_ISDIR(st_in.st_mode)) {
        return 1;
    }

    return 0;
}

char *basename_no_ext(const char *str) {
    if (str == 0 || strlen(str) == 0) {
        return NULL;
    }

    char *base = strrchr(str, '/') + 1;
    char *ext  = strrchr(str, '.');

    return strndup(base, ext - base);
}

void build_filename(const char *in, char *path, char *out) {
    OPENDCP_LOG(LOG_DEBUG, "Building filename from %s", in);

    if (!is_a_dir(path)) {
        snprintf(out, MAX_FILENAME_LENGTH, "%s", path);
    }
    else {
        printf("path: %s\n", path);
        char *base = basename_no_ext(in);
        printf("base: %s path: %s\n", base, path);
        snprintf(out, MAX_FILENAME_LENGTH, "%s/%s.j2c", path, base);
        printf("filename %s\n", out);

        if (base) {
            free(base);
        }
    }
}

opendcp_writer_t *writer_new(filelist_t *filelist) {
    char *extension;

    opendcp_writer_t *writer = (opendcp_writer_t *)malloc(sizeof(opendcp_writer_t));

    char out[MAX_FILENAME_LENGTH];
    build_filename(filelist->files[0], "/tmp/foo.j2c", out);

    extension = strrchr(out, '.');
    extension++; 

    writer->encoder = opendcp_encoder_find(NULL, extension, 0);

    if (writer->encoder->id == OPENDCP_ENCODER_NONE) {
        OPENDCP_LOG(LOG_ERROR, "no encoder found for image extension %s", extension);
        free(writer);
        return NULL;
    }

    OPENDCP_LOG(LOG_DEBUG, "encoder %s found for image extension %s", writer->encoder->name, extension);
    filelist_writer_t *context = malloc(sizeof(filelist_writer_t));
    context->filelist = filelist; 

    writer->context = context;
    writer->write_frame = filelist_write_frame;
    writer->write_next = filelist_write_frame;
    writer->current = 0;
    writer->total = filelist->nfiles;

    return writer;
}

void writer_free(opendcp_writer_t *self) {
    if (self) {
        if (self->context) {
            free(self->context);
        }
        free(self);
    }
}

int filelist_write_next(opendcp_writer_t *self, opendcp_image_t *image) {
    return filelist_write_frame(self, self->current++, NULL, image);
}

int filelist_write_frame(opendcp_writer_t *self, int frame, opendcp_t *opendcp, opendcp_image_t *image) {
    char *filename;
    int   result;

    if (frame < 0 || frame > self->total) {
        return 0;
    }


    filelist_writer_t *context = (filelist_writer_t *)self->context;
    filename = context->filelist->files[frame];
    char out[MAX_FILENAME_LENGTH];
    build_filename(filename, "/tmp/test", out);

    result = self->encoder->encode(opendcp, image, out);

    if (result != OPENDCP_NO_ERROR) {
        return OPENDCP_ERROR;
    }

    return OPENDCP_NO_ERROR;
}
