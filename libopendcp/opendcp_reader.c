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
#include "opendcp.h"
#include "opendcp_image.h"
#include "opendcp_reader.h"
#include "codecs/opendcp_decoder.h"

enum {
    READER_FILELIST = 0,
    READER_VIDEO
} readers;

typedef struct {
    filelist_t *filelist;
} filelist_reader_t;

int filelist_read_frame(opendcp_reader_t *self, int frame, opendcp_image_t **image);
int filelist_read_next(opendcp_reader_t *self, opendcp_image_t **image);
int filelist_reader_create_context(opendcp_reader_t *reader, filelist_t *filelist);

opendcp_reader_t *reader_new(filelist_t *filelist) {
    char *extension;

    opendcp_reader_t *reader = (opendcp_reader_t *)malloc(sizeof(opendcp_reader_t));

    extension = strrchr(filelist->files[0], '.');
    extension++; 

    reader->decoder = opendcp_decoder_find(NULL, extension, 0);

    /* image */
    if (reader->decoder->id != OPENDCP_DECODER_NONE) {
        OPENDCP_LOG(LOG_DEBUG, "decoder %s found for image extension %s", reader->decoder->name, extension);
        filelist_reader_create_context(reader, filelist);

        return reader;
    }

    /* video */
    //if (video_decoder_find(file)) {
    //    OPENDCP_LOG(LOG_DEBUG, "decoder %s found for image extension %s", reader->decoder->name, extension);
    //}

    OPENDCP_LOG(LOG_ERROR, "no Decoder found for image extension %s", extension);
    free(reader);

    return NULL;
}

void reader_free(opendcp_reader_t *self) {
    if (self) {
        if (self->context) {
            free(self->context);
        }
        free(self);
    }
}

int filelist_reader_create_context(opendcp_reader_t *reader, filelist_t *filelist) {
    filelist_reader_t *context = malloc(sizeof(filelist_reader_t));
    context->filelist = filelist;

    reader->context = context;
    reader->read_frame = filelist_read_frame;
    reader->read_next = filelist_read_frame;
    reader->current = 0;
    reader->total = filelist->nfiles;

    return 0;
} 

int filelist_read_next(opendcp_reader_t *self, opendcp_image_t **image) {
    return filelist_read_frame(self, self->current++, image);
}

int filelist_read_frame(opendcp_reader_t *self, int frame, opendcp_image_t **image) {
    char *filename;
    int   result;

    if (frame < 0 || frame > self->total) {
        return 0;
    }

    filelist_reader_t *context = (filelist_reader_t *)self->context;
    filename = context->filelist->files[frame];

    result  = self->decoder->decode(image, filename);

    if (result != OPENDCP_NO_ERROR) {
        return OPENDCP_ERROR;
    }

    return OPENDCP_NO_ERROR;
}
