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
    fprintf(fp, "       opendcp_extract -i <file> [options ...]\n\n");
    fprintf(fp, "Required:\n");
    fprintf(fp, "       -i | --input <file>            - input mxf file\n");
    fprintf(fp, "\n");
    fprintf(fp, "Options:\n");
    fprintf(fp, "       -s | --start <frame>           - start frame\n");
    fprintf(fp, "       -d | --end  <frame>            - end frame\n");
    fprintf(fp, "       -l | --log_level <level>       - Sets the log level 0:Quiet, 1:Error, 2:Warn (default),  3:Info, 4:Debug\n");
    fprintf(fp, "       -k | --key <key>               - set encryption key (this enables encryption)\n");
    fprintf(fp, "       -h | --help                    - show help\n");
    fprintf(fp, "       -v | --version                 - show version\n");
    fprintf(fp, "\n\n");

    fclose(fp);
    exit(0);
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
    char *filename = NULL;
    int key_id_flag = 0;

    if (argc <= 1) {
        dcp_usage();
    }

    opendcp = opendcp_create();

    /* set initial values */
    opendcp->log_level = LOG_WARN;
    opendcp->ns = XML_NS_SMPTE;

    /* parse options */
    while (1)
    {
        static struct option long_options[] =
        {
            {"key",            required_argument, 0, 'k'},
            {"help",           required_argument, 0, 'h'},
            {"input",          required_argument, 0, 'i'},
            {"start",          required_argument, 0, 's'},
            {"log_level",      required_argument, 0, 'l'},
            {"version",        no_argument,       0, 'v'},
            {0, 0, 0, 0}
        };

        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "i:k:s:l:hv",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1) {
            break;
        }

        switch (c)
        {
            case 0:

                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0) {
                    break;
                }

                printf ("option %s", long_options[option_index].name);

                if (optarg) {
                    printf (" with arg %s", optarg);
                }

                printf ("\n");
                break;

            case 's':
                opendcp->mxf.start_frame = atoi(optarg);

                if (opendcp->mxf.start_frame < 1) {
                    dcp_fatal(opendcp, "Start frame must be greater than 0");
                }

                break;

            case 'i':
                filename = optarg;
                break;

            case 'l':
                opendcp->log_level = atoi(optarg);
                break;

            case 'h':
                dcp_usage();
                break;

            case 'k':
                if (!is_key(optarg)) {
                    dcp_fatal(opendcp, "Invalid encryption key format");
                }

                if (hex2bin(optarg, opendcp->mxf.key_value, 16)) {
                    dcp_fatal(opendcp, "Invalid encryption key format");
                }

                opendcp->mxf.key_flag = 1;

                break;

            case 'v':
                version();
                break;
        }
    }

    opendcp_log_init(opendcp->log_level);

    if (opendcp->log_level > 0) {
        printf("\nOpenDCP Extract %s %s\n", OPENDCP_VERSION, OPENDCP_COPYRIGHT);
    }

    /* set the callbacks (optional) for the mxf writer */
    if (opendcp->log_level > 0 && opendcp->log_level < 3) {
        opendcp->mxf.frame_done.callback = frame_done_cb;
        opendcp->mxf.file_done.callback  = write_done_cb;
    }

    int class = get_file_essence_class(filename, 1);

    if (opendcp->log_level > 0 && opendcp->log_level < 3) {
        progress_bar();
    }

    total = opendcp->mxf.duration;

    if (read_j2k_mxf(opendcp, filename) != 0 )  {
        OPENDCP_LOG(LOG_INFO, "Could not READ MXF file");
    }
    else {
        OPENDCP_LOG(LOG_INFO, "MXF extract complete");
    }

    if (opendcp->log_level > 0) {
        printf("\n");
    }

    opendcp_delete(opendcp);

    exit(0);
}
