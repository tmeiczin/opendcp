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
#include "cli.h"
#include "cli_parser.h"
#include "extract.h"

void progress_bar();

void version() {
    FILE *fp;

    fp = stdout;
    fprintf(fp, "\n%s version %s %s\n\n", OPENDCP_NAME, OPENDCP_VERSION, OPENDCP_COPYRIGHT);

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

    printf("  MXF Extraction [");

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

int set_args(opendcp_t *opendcp,  args_t *args) {
    opendcp->mxf.start_frame = atoi(args->start);
    opendcp->log_level = atoi(args->log_level);

    if (opendcp->mxf.start_frame < 1) {
        dcp_fatal(opendcp, "Start frame must be greater than 0");
    }

    if (args->key) {
        if (!is_key(args->key)) {
            fprintf(stderr, "Invalid encryption key format %s\n", args->key);
            return 1;
        }

        if (hex2bin(args->key, opendcp->mxf.key_value, 16)) {
            fprintf(stderr, "Invalid encryption key format\n");
            return 1;
        }
        opendcp->mxf.key_flag = 1;
    }

    return 0;
}

int main (int argc, char **argv) {
    opendcp_t *opendcp;
    char *filename = NULL;

    CLI_PARSER

    if (cli_result) {
        exit(1);
    }

    /* check if version was invoked */
    if  (args.version) {
        fprintf(stdout, "%s\n", OPENDCP_VERSION);
        exit(0);
    }

    opendcp = opendcp_create();

    /* set initial values */
    opendcp->log_level = LOG_WARN;
    opendcp->ns = XML_NS_SMPTE;

    set_args(opendcp, &args);

    filename = args.file;

    opendcp_log_init(opendcp->log_level);

    if (opendcp->log_level > 0) {
        printf("\nOpenDCP Extract %s %s\n", OPENDCP_VERSION, OPENDCP_COPYRIGHT);
    }

    /* set the callbacks (optional) for the mxf writer */
    if (opendcp->log_level > 0 && opendcp->log_level < 3) {
        opendcp->mxf.frame_done.callback = frame_done_cb;
        opendcp->mxf.file_done.callback  = write_done_cb;
    }

    if (opendcp->log_level > 0 && opendcp->log_level < 3) { progress_bar(); }

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
