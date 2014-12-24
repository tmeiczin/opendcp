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

#ifndef _OPENDCP_CLI_ENCODE_H_
#define _OPENDCP_CLI_ENCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_POSITIONALS 64

typedef struct {
    /* commands */
    int j2k;
    int mxf;
    int stereoscopic;
    /* arguments */
    char *input;
    char *input_left;
    char *input_right;
    char *output;
    char *output_left;
    char *output_right;
    /* flags */
    int no_overwrite;
    int no_xyz;
    int resize;
    /* options */
    char *bw;
    char *colorspace;
    char *encoder;
    char *profile;
    char *rate;
    char *type;
    char *start;
    char *end;
    char *slideshow;
    char *log_level;
    char *key;
    char *key_id;
    char *threads;
    char *tmp_dir;
} opendcp_args_t;

typedef struct {
    const char *name;
    int         value;
} command_t;

typedef struct {
    const char *name;
    char       *value;
} argument_t;

typedef struct {
    const char *name;
    int         value_required;
    char       *value;
} option_t;

typedef struct {
    char *name;
} positional_t;

typedef struct {
    int           n_commands;
    int           n_arguments;
    int           n_options;
    int           n_positional;
    command_t    *commands;
    argument_t   *arguments;
    option_t     *options;
    positional_t *positionals;
} cli_t;

typedef struct {
    int argc;
    char **argv;
    int i;
    char *current;
} argv_t;

int opendcp_command_j2k(opendcp_t *opendcp, opendcp_args_t *args);

#ifdef __cplusplus
}
#endif

#endif // _OPENDCP_CLI_ENCODE_H_
