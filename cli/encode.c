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
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "encode.h"
#include "cli_parser.h"


/****************************/
/***** APPLICATION CODE *****/
/****************************/

/* start of application */
int main(int argc, char *argv[]) {
    /* include parser macro */
    CLI_PARSER

    printf("Commands:\n");
    printf("    mxf == %s\n", args.mxf ? "true" : "false");
    printf("    j2k == %s\n", args.j2k ? "true" : "false");
    printf("    j2k_3d == %s\n", args.j2k_3d ? "true" : "false");
    printf("Arguments:\n");
    printf("    input  == %s\n", args.input);
    printf("    output == %s\n", args.output);
    printf("    input_left   == %s\n", args.input_left);
    printf("    input_right  == %s\n", args.input_right);
    printf("    output_left  == %s\n", args.output_left);
    printf("    output_right == %s\n", args.output_right);

    printf("Flags:\n");
    printf("    --help         == %s\n", args.help ? "true" : "false");
    printf("    --version      == %s\n", args.version ? "true" : "false");
    printf("Options:\n");
    printf("    --bw      == %s\n", args.bw);
    printf("    --encoder == %s\n", args.encoder);
    printf("    --profile == %s\n", args.profile);
    printf("    --rate    == %s\n", args.frame_rate);
    printf("    --overwrite == %s\n", args.overwrite);

    if (cli_result) {
        exit(1);
    }

    if (args.mxf) {
        printf("mxf\n");
    }
    else if (args.j2k) {
        printf("j2k\n");
        opendcp_command_j2k(&args);
    }

    return 0;
}
