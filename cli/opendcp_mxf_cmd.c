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

#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <opendcp.h>
#include "opendcp_cli.h"

void progress_bar();

void version() {
    FILE *fp;

    fp = stdout;
    fprintf(fp, "\n%s version %s %s\n\n", OPENDCP_NAME, OPENDCP_VERSION, OPENDCP_COPYRIGHT);

    exit(0);
}

void dcp_usage() {
    FILE *fp;
    fp = stdout;

    fprintf(fp, "\n%s version %s %s\n\n", OPENDCP_NAME, OPENDCP_VERSION, OPENDCP_COPYRIGHT);
    fprintf(fp, "Usage:\n");
    fprintf(fp, "       opendcp_mxf -i <file> -o <file> [options ...]\n\n");
    fprintf(fp, "Required:\n");
    fprintf(fp, "       -i | --input <file | dir>      - input file or directory.\n");
    fprintf(fp, "       -1 | --left <dir>              - left channel input images when creating a 3D essence\n");
    fprintf(fp, "       -2 | --right <dir>             - right channel input images when creating a 3D essence\n");
    fprintf(fp, "       -o | --output <file>           - output mxf file\n");
    fprintf(fp, "\n");
    fprintf(fp, "Options:\n");
    fprintf(fp, "       -n | --ns <interop | smpte>    - Generate SMPTE or MXF Interop labels (default smpte)\n");
    fprintf(fp, "       -r | --rate <rate>             - frame rate (default 24)\n");
    fprintf(fp, "       -s | --start <frame>           - start frame\n");
    fprintf(fp, "       -d | --end  <frame>            - end frame\n");
    fprintf(fp, "       -p | --slideshow  <duration>   - create slideshow with each image having duration specified (in seconds)\n");
    fprintf(fp, "       -l | --log_level <level>       - Sets the log level 0:Quiet, 1:Error, 2:Warn (default),  3:Info, 4:Debug\n");
    fprintf(fp, "       -h | --help                    - show help\n");
    fprintf(fp, "       -v | --version                 - show version\n");
    fprintf(fp, "\n\n");

    fclose(fp);
    exit(0);
}

filelist_t *get_filelist_3d(char *in_path_left, char *in_path_right) {
    int x = 0;
    int y = 0;
    int rc;
    filelist_t *left, *right, *filelist;

    left  = get_filelist(in_path_left, "j2c,j2k");
    right = get_filelist(in_path_right, "j2c,j2k");

    if (left->nfiles != right->nfiles) {
        OPENDCP_LOG(LOG_ERROR, "Mismatching file count for 3D images left: %d right: %d", left->nfiles, right->nfiles);
        filelist_free(left);
        filelist_free(right);
        return NULL;
    }

    /* Sort files by index, and make sure they're sequential. */
    if (order_indexed_files(left->files, left->nfiles) != OPENDCP_NO_ERROR ||
            order_indexed_files(right->files, right->nfiles) != OPENDCP_NO_ERROR) {
        OPENDCP_LOG(LOG_WARN, "Could not order image files");
        filelist_free(left);
        filelist_free(right);
        return NULL;
    }

    rc = ensure_sequential(left->files, left->nfiles);

    if (rc != OPENDCP_NO_ERROR) {
        OPENDCP_LOG(LOG_WARN, "Filenames not sequential between %s and %s.", left->files[rc], left->files[rc + 1]);
        filelist_free(left);
        filelist_free(right);
        return NULL;
    }

    rc = ensure_sequential(right->files, left->nfiles);

    if (rc != OPENDCP_NO_ERROR) {
        OPENDCP_LOG(LOG_WARN, "Filenames not sequential between %s and %s.", right->files[rc], right->files[rc + 1]);
        filelist_free(left);
        filelist_free(right);
        return NULL;
    }

    filelist = filelist_alloc(left->nfiles + right->nfiles);

    for (x = 0; x < filelist->nfiles; y++, x += 2) {
        strcpy(filelist->files[x],   left->files[y]);
        strcpy(filelist->files[x + 1], right->files[y]);
    }

    filelist_free(left);
    filelist_free(right);

    return filelist;
}

int total = 0;
int val   = 0;

int frame_done_cb(void *p) {
    val++;
    UNUSED(p);
    progress_bar();

    return 0;
}

int write_done_cb(void *p) {
    UNUSED(p);
    printf("\n  MXF Complete\n");

    return 0;
}

void progress_bar() {
    int x;
    int step = 20;
    float c = (float)step / total * (float)val;

    printf("  MXF Creation [");

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

int main (int argc, char **argv) {
    int c;
    opendcp_t *opendcp;
    char *in_path = NULL;
    char *in_path_left = NULL;
    char *in_path_right = NULL;
    char *out_path = NULL;
    filelist_t *filelist;

    if (argc <= 1) {
        dcp_usage();
    }

    opendcp = opendcp_create();

    /* set initial values */
    opendcp->log_level = LOG_WARN;
    opendcp->ns = XML_NS_SMPTE;
    opendcp->frame_rate = 24;
    opendcp->threads = 4;

    /* parse options */
    while (1)
    {
        static struct option long_options[] =
        {
            {"help",           required_argument, 0, 'h'},
            {"input",          required_argument, 0, 'i'},
            {"left",           required_argument, 0, '1'},
            {"right",          required_argument, 0, '2'},
            {"ns",             required_argument, 0, 'n'},
            {"output",         required_argument, 0, 'o'},
            {"start",          required_argument, 0, 's'},
            {"end",            required_argument, 0, 'd'},
            {"rate",           required_argument, 0, 'r'},
            {"slideshow",      required_argument, 0, 'p'},
            {"log_level",      required_argument, 0, 'l'},
            {"version",        no_argument,       0, 'v'},
            {0, 0, 0, 0}
        };

        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "1:2:d:i:n:o:r:s:p:l:3hv",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
        { break; }

        switch (c)
        {
            case 0:

                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                { break; }

                printf ("option %s", long_options[option_index].name);

                if (optarg)
                { printf (" with arg %s", optarg); }

                printf ("\n");
                break;

            case '3':
                opendcp->stereoscopic = 1;
                break;

            case 'd':
                opendcp->mxf.end_frame = atoi(optarg);

                if (opendcp->mxf.end_frame < 1) {
                    dcp_fatal(opendcp, "End frame  must be greater than 0");
                }

                break;

            case 'p':
                opendcp->mxf.slide = 1;
                opendcp->mxf.duration = atoi(optarg);

                if (opendcp->mxf.duration < 1) {
                    dcp_fatal(opendcp, "Slide duration  must be greater than 0");
                }

                break;

            case 's':
                opendcp->mxf.start_frame = atoi(optarg);

                if (opendcp->mxf.start_frame < 1) {
                    dcp_fatal(opendcp, "Start frame must be greater than 0");
                }

                break;

            case 'n':
                if (!strcmp(optarg, "smpte")) {
                    opendcp->ns = XML_NS_SMPTE;
                }
                else if (!strcmp(optarg, "interop")) {
                    opendcp->ns = XML_NS_INTEROP;
                }
                else {
                    dcp_fatal(opendcp, "Invalid profile argument, must be smpte or interop");
                }

                break;

            case 'i':
                in_path = optarg;
                break;

            case '1':
                in_path_left = optarg;
                opendcp->stereoscopic = 1;
                break;

            case '2':
                in_path_right = optarg;
                opendcp->stereoscopic = 1;
                break;

            case 'l':
                opendcp->log_level = atoi(optarg);
                break;

            case 'o':
                out_path = optarg;
                break;

            case 'h':
                dcp_usage();
                break;

            case 'r':
                opendcp->frame_rate = atoi(optarg);

                if (opendcp->frame_rate > 60 || opendcp->frame_rate < 1 ) {
                    dcp_fatal(opendcp, "Invalid frame rate. Must be between 1 and 60.");
                }

                break;

            case 'v':
                version();
                break;
        }
    }

    opendcp_log_init(opendcp->log_level);

    if (opendcp->log_level > 0) {
        printf("\nOpenDCP MXF %s %s\n", OPENDCP_VERSION, OPENDCP_COPYRIGHT);
    }

    if (opendcp->stereoscopic) {
        if (in_path_left == NULL) {
            dcp_fatal(opendcp, "3D input detected, but missing left image input path");
        }
        else if (in_path_right == NULL) {
            dcp_fatal(opendcp, "3D input detected, but missing right image input path");
        }
    }
    else {
        if (in_path == NULL) {
            dcp_fatal(opendcp, "Missing input file");
        }
    }

    if (out_path == NULL) {
        dcp_fatal(opendcp, "Missing output file");
    }

    if (opendcp->stereoscopic) {
        filelist = get_filelist_3d(in_path_left, in_path_right);
    }
    else {
        filelist = get_filelist(in_path, "j2c,j2k,wav");

        /* Sort files by index, and make sure they're sequential. */
        if (order_indexed_files(filelist->files, filelist->nfiles) != OPENDCP_NO_ERROR) {
            dcp_fatal(opendcp, "Could not order image files");
        }

        int rc = ensure_sequential(filelist->files, filelist->nfiles);

        if (rc != OPENDCP_NO_ERROR) {
            OPENDCP_LOG(LOG_WARN, "Filenames not sequential between %s and %s.", filelist->files[rc], filelist->files[rc + 1]);
        }
    }

    if (!filelist) {
        dcp_fatal(opendcp, "Could not read input files");
    }

    if (filelist->nfiles < 1) {
        dcp_fatal(opendcp, "No input files located");
    }

#ifdef _WIN32

    /* check for non-ascii filenames under windows */
    for (c = 0; c < filelist->nfiles; c++) {
        if (is_filename_ascii(filelist->files[c]) == 0) {
            OPENDCP_LOG(LOG_ERROR, "Filename %s contains non-ascii characters, skipping", filelist->files[c]);
            dcp_fatal(opendcp, "Filenames cannot contain non-ascii characters");
        }
    }

#endif

    if (opendcp->mxf.end_frame) {
        if (opendcp->mxf.end_frame > filelist->nfiles) {
            dcp_fatal(opendcp, "End frame is greater than the actual frame count");
        }
    }
    else {
        opendcp->mxf.end_frame = filelist->nfiles;
    }

    if (opendcp->mxf.start_frame) {
        if (opendcp->mxf.start_frame > opendcp->mxf.end_frame) {
            dcp_fatal(opendcp, "Start frame must be less than end frame");
        }
    }
    else {
        opendcp->mxf.start_frame = 1;
    }

    if (opendcp->mxf.slide) {
        opendcp->mxf.duration = opendcp->mxf.duration * opendcp->frame_rate * filelist->nfiles;
    }
    else {
        opendcp->mxf.duration = opendcp->mxf.end_frame - (opendcp->mxf.start_frame - 1);
    }

    if (opendcp->mxf.duration < 1) {
        dcp_fatal(opendcp, "Duration must be at least 1 frame");
    }

    /* set the callbacks (optional) for the mxf writer */
    if (opendcp->log_level > 0 && opendcp->log_level < 3) {
        opendcp->mxf.frame_done.callback = frame_done_cb;
        opendcp->mxf.file_done.callback  = write_done_cb;
    }

    int class = get_file_essence_class(filelist->files[0], 1);

    if (opendcp->log_level > 0 && opendcp->log_level < 3) { progress_bar(); }

    if (class == ACT_SOUND) {
        total = get_wav_duration(filelist->files[0], opendcp->frame_rate);
    }
    else {
        total = opendcp->mxf.duration;
    }

    if (write_mxf(opendcp, filelist, out_path) != 0 )  {
        OPENDCP_LOG(LOG_INFO, "Could not create MXF file");
    }
    else {
        OPENDCP_LOG(LOG_INFO, "MXF creation complete");
    }

    filelist_free(filelist);

    if (opendcp->log_level > 0) {
        printf("\n");
    }

    opendcp_delete(opendcp);

    exit(0);
}
