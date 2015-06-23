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

#include <getopt.h>
#include <signal.h>
#ifdef OPENMP
#include <omp.h>
#endif
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <opendcp.h>
#include <opendcp_image.h>
#include <opendcp_reader.h>
#include <opendcp_writer.h>
#include <opendcp_encoder.h>
#include <opendcp_decoder.h>
#include "opendcp_cli.h"
#include "opendcp_cli_encode.h"

#ifndef _WIN32
sig_atomic_t SIGINT_received = 0;

void sig_handler(int signum) {
    UNUSED(signum);
    SIGINT_received = 1;
    #pragma omp flush(SIGINT_received)
}
#else
int SIGINT_received = 0;
#endif

/* prototypes */
void  build_j2k_filename(const char *in, char *path, char *out);
void  progress_bar(int val, int total);
int   frame_count;
int   frame_index;

enum SOURCE_TYPE {
    IMAGE = 0,
    VIDEO,
    UNKNOWN
};

typedef struct {
    filelist_t *filelist;
    int         current;
    int         count;
    int         (*read)();
} frame_reader_t;

int filelist_read(frame_reader_t *reader, opendcp_image_t **dimage) {
    char *extension;
    int  result;
    opendcp_decoder_t *decoder;

    char *file = reader->filelist->files[reader->current++];

    extension = strrchr(file, '.');
    extension++;

    decoder = opendcp_decoder_find(NULL, extension, 0);
    result  = decoder->decode(dimage, file);

    return result;
}

int video_read(frame_reader_t *reader, opendcp_image_t **dimage) {
    char *extension;
    int  result;
    opendcp_decoder_t *decoder;

    char *file = reader->filelist->files[reader->current++];

    extension = strrchr(file, '.');
    extension++;

    decoder = opendcp_decoder_find(NULL, extension, 0);
    result  = decoder->decode(dimage, file);

    return result;
}

frame_reader_t filelist_reader = {NULL, 0, 0, filelist_read};

int get_input_type(const char *file) {
    char *extension;
    opendcp_decoder_t *decoder;

    extension = strrchr(file, '.');
    extension++;

    decoder = opendcp_decoder_find(NULL, extension, 0);
    if (decoder->id != OPENDCP_DECODER_NONE) {
        return IMAGE;
    }

    if (video_decoder_find(file)) {
        return VIDEO;
    }

    return UNKNOWN;
}

void build_j2k_filename(const char *in, char *path, char *out) {
    OPENDCP_LOG(LOG_DEBUG, "Building filename from %s", in);

    if (!is_dir(path)) {
        snprintf(out, MAX_FILENAME_LENGTH, "%s", path);
    }
    else {
        char *base = basename_noext(in);
        snprintf(out, MAX_FILENAME_LENGTH, "%s/%s.j2c", path, base);

        if (base) {
            free(base);
        }
    }
}

int frame_done(void *p) {
    UNUSED(p);
    frame_index++;
    progress_bar(frame_index, frame_count);

    return OPENDCP_NO_ERROR;
}

void progress_bar(int val, int total) {
    int x;
    int step = 20;
    float c = (float)step / total * (float)val;
#ifdef OPENMP
    int nthreads = omp_get_num_threads();
#else
    int nthreads = 1;
#endif
    printf("  JPEG2000 Conversion (%d thread", nthreads);

    if (nthreads > 1) {
        printf("s");
    }

    printf(") [");

    for (x = 0; x < step; x++) {
        if (c > x) {
            printf("=");
        }
        else {
            printf(" ");
        }
    }

    printf("] 100%% [%d/%d]\r", val, total);
    fflush(stdout);
}

int opendcp_command_j2k(opendcp_t *opendcp, opendcp_args_t *args) {
    int rc, c, result, count = 0, input_type;
    int openmp_flag = 0;
    char *in_path  = args->input;
    char *out_path = args->output;
    filelist_t *filelist;

#ifndef _WIN32
    struct sigaction sig_action;
    sig_action.sa_handler = sig_handler;
    sig_action.sa_flags = 0;
    sigemptyset(&sig_action.sa_mask);
    sigaction(SIGINT,  &sig_action, NULL);
#endif

    if (opendcp->log_level > 0 && opendcp->log_level < 3) {
        opendcp->j2k.frame_done.callback  = frame_done;
    }

    if (opendcp_encoder_enable("j2c", NULL, opendcp->j2k.encoder)) {
        dcp_fatal(opendcp, "Could not enabled encoder");
    }

    if (opendcp->log_level > 0) {
        printf("\nOpenDCP J2K %s %s\n", OPENDCP_VERSION, OPENDCP_COPYRIGHT);

        if (opendcp->j2k.encoder == OPENDCP_ENCODER_KAKADU) {
            printf("  Encoder: Kakadu\n");
        }
        else if (opendcp->j2k.encoder == OPENDCP_ENCODER_REMOTE)  {
            printf("  Encoder: Remote\n");
        }
        else if (opendcp->j2k.encoder == OPENDCP_ENCODER_RAGNAROK)  {
            printf("  Encoder: Ragnarok\n");
        }
        else {
            printf("  Encoder: OpenJPEG\n");
        }
    }

    /* cinema profile check */
    if (opendcp->cinema_profile != DCP_CINEMA4K && opendcp->cinema_profile != DCP_CINEMA2K) {
        dcp_fatal(opendcp, "Invalid profile argument, must be cinema2k or cinema4k");
    }

    /* end frame check */
    if (opendcp->j2k.end_frame < 0) {
        dcp_fatal(opendcp, "End frame  must be greater than 0");
    }

    /* start frame check */
    if (opendcp->j2k.start_frame < 1) {
        dcp_fatal(opendcp, "Start frame must be greater than 0");
    }

    /* frame rate check */
    if (opendcp->frame_rate > 60 || opendcp->frame_rate < 1 ) {
        dcp_fatal(opendcp, "Invalid frame rate. Must be between 1 and 60.");
    }

    /* encoder check */
    if (opendcp->j2k.encoder == OPENDCP_ENCODER_KAKADU) {
        result = system("kdu_compress -u >/dev/null 2>&1");

        if (result >> 8 != 0) {
            dcp_fatal(opendcp, "kdu_compress was not found. Either add to path or remove -e 1 flag");
        }
    }

    /* bandwidth check */
    if (opendcp->j2k.bw < 10 || opendcp->j2k.bw > 500) {
        dcp_fatal(opendcp, "Bandwidth must be between 10 and 500, but %d was specified", opendcp->j2k.bw);
    }
    else {
        opendcp->j2k.bw *= 1000000;
    }

    /* input path check */
    if (in_path == NULL) {
        dcp_fatal(opendcp, "Missing input file");
    }

    if (in_path[strlen(in_path) - 1] == '/') {
        in_path[strlen(in_path) - 1] = '\0';
    }

    /* output path check */
    if (out_path == NULL) {
        dcp_fatal(opendcp, "Missing output file");
    }

    if (out_path[strlen(out_path) - 1] == '/') {
        out_path[strlen(out_path) - 1] = '\0';
    }

    /* make sure path modes are ok */
    if (is_dir(in_path) && !is_dir(out_path)) {
        dcp_fatal(opendcp, "Input is a directory, so output must also be a directory");
    }

    /* get file list */
    OPENDCP_LOG(LOG_DEBUG, "searching path %s", in_path);

    char *extensions = opendcp_decoder_extensions();

    filelist = get_filelist(in_path, extensions);

    if (extensions != NULL) {
        free(extensions);
    }

    if (filelist == NULL || filelist->nfiles < 1) {
        dcp_fatal(opendcp, "No input files located");
    }

    /* end frame check */
    if (opendcp->j2k.end_frame) {
        if (opendcp->j2k.end_frame > filelist->nfiles) {
            dcp_fatal(opendcp, "End frame is greater than the actual frame count");
        }
    }
    else {
        opendcp->j2k.end_frame = filelist->nfiles;
    }

    /* start frame check */
    if (opendcp->j2k.start_frame > opendcp->j2k.end_frame) {
        dcp_fatal(opendcp, "Start frame must be less than end frame");
    }

    OPENDCP_LOG(LOG_DEBUG, "checking file sequence", in_path);

    /* Sort files by index, and make sure they're sequential. */
    if (order_indexed_files(filelist->files, filelist->nfiles) != OPENDCP_NO_ERROR) {
        dcp_fatal(opendcp, "Could not order image files");
    }

    rc = ensure_sequential(filelist->files, filelist->nfiles);

    if (rc != OPENDCP_NO_ERROR) {
        OPENDCP_LOG(LOG_WARN, "Filenames not sequential between %s and %s.", filelist->files[rc], filelist->files[rc + 1]);
    }

    if (opendcp->log_level > 0 && opendcp->log_level < 3) {
        progress_bar(0, 0);
    }

    input_type = get_input_type(filelist->files[0]);

#ifdef OPENMP
    omp_set_num_threads(opendcp->threads);
    OPENDCP_LOG(LOG_DEBUG, "OpenMP Enable");
#endif

    frame_index = opendcp->j2k.start_frame;
    frame_count = opendcp->j2k.end_frame - (opendcp->j2k.start_frame -1);

    opendcp_reader_t *reader = reader_new(filelist);
    opendcp_writer_t *writer = writer_new(filelist);

    #pragma omp parallel for private(c)
    for (c = opendcp->j2k.start_frame - 1; c < opendcp->j2k.end_frame; c++) {
        #pragma omp flush(SIGINT_received)

        /* check for non-ascii filenames under windows */
#ifdef _WIN32

        if (is_filename_ascii(filelist->files[c]) == 0) {
            OPENDCP_LOG(LOG_WARN, "Filename %s contains non-ascii characters, skipping", filelist->files[c]);
            continue;
        }

#endif

        char out[MAX_FILENAME_LENGTH];
        build_j2k_filename(filelist->files[c], out_path, out);

        if (!SIGINT_received) {
            OPENDCP_LOG(LOG_INFO, "JPEG2000 conversion %s started OPENMP: %d", filelist->files[c], openmp_flag);

            if(access(out, F_OK) != 0 || opendcp->j2k.no_overwrite == 0) {
                if (input_type == VIDEO) {
                    result = decode_video(opendcp, filelist->files[c]);
                } else {
                    opendcp_image_t *image;
                    result = reader->read_frame(reader, c, &image);
                    result = writer->write_frame(writer, c, opendcp, image);
                    opendcp_image_free(image);
                }
            }
            else {
                result = OPENDCP_NO_ERROR;
            }

            if (count) {
                if (opendcp->log_level > 0 && opendcp->log_level < 3) {
                    progress_bar(count, opendcp->j2k.end_frame);
                }
            }

            if (result == OPENDCP_ERROR) {
                OPENDCP_LOG(LOG_ERROR, "JPEG2000 conversion %s failed", filelist->files[c]);
                dcp_fatal(opendcp, "Exiting...");
            }
            else {
                OPENDCP_LOG(LOG_INFO, "JPEG2000 conversion %s complete", filelist->files[c]);
            }

            count++;
        }
    }

    if (opendcp->log_level > 0 && opendcp->log_level < 3) {
        progress_bar(opendcp->j2k.end_frame, opendcp->j2k.end_frame);
    }

    filelist_free(filelist);

    if (opendcp->log_level > 0) {
        printf("\n");
    }

    reader_free(reader);  
    writer_free(writer);  
    opendcp_delete(opendcp);

    exit(0);
}
