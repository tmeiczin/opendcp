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
#include <opendcp.h>

#include "opendcp_cli_encode.h"


/* prototypes */
int   options_to_args(opendcp_t *opendcp, cli_t *elements, opendcp_args_t *args);
int   commands_to_args(cli_t *elements, opendcp_args_t *args);
int   arguments_to_args(cli_t *elements, opendcp_args_t *args);

const char usage[] =
"Usage:\n"
"  opendcp_encode j2k [options] <input> <output>\n"
"  opendcp_encode j2k stereoscopic [options] <input> <output>\n"
"  opendcp_encode mxf [options] <input> <output>\n"
"  opendcp_encode mxf stereoscopic [options] <input_left> <input_right> <output>\n"
"\n"
"Options:\n"
"     --help                         Show this screen.\n"
"     --version                      Show version.\n"
"     --no_overwrite                 Do no overwrite existing files\n"
"     --no_xyz                       Disable XYZ<->RGB conversion                  \n"
"     --resize                       Resize image to closest DCI compliant resolution\n"
"     --bw         <bw>              Maximum JPEG2000 bandwidth [default: 125]\n"
"     --colorspace <colorspace>      Source colorspace (srgb|rec709|p3|srgb_complex|rec709_complex) [default rec709]\n"
"     --encoder    <encoder>         JPEG2000 j2kr (openjpeg|kakadu) [default openjpeg]\n"
"     --profile    <profile>         Cinema profile (2k|4k) [default 2k]\n"
"     --rate       <frame_rate>      Frame rate of source [default 24]\n"
"     --type       <type>            Generate SMPTE or MXF Interop labels (smpte|interop)  [default smpte]\n"
"     --start      <start_frame>     The start frame [default 0]\n"
"     --end        <end_frame>       The end frame\n"
"     --slideshow  <duration>        Create a slideshow with each image having duration specified in seconds\n"
"     --log_level  <log_level>       Sets the log level 0-4. Higher means more logging [default 2]\n"
"     --key        <key>             Set encryption key and enable encryption (not recommended)\n"
"     --key_id     <key_id>          Set encryption key id (leaving blank generates a random uuid)\n"
"     --threads    <threads>         The number of threads to use for encoding\n"
"     --tmp_dir    <tmp_dir>         Temporary directory for intermediate files\n"
"\n"
"Commands:\n"
"  j2k       Encode to JPEG2000\n"
"  mxf       Encode to MXF\n"
"\n"
"  Supported formats are tif, dpx, bmp, jpeg2000, and some video formats\n"
"\n"
"Examples:\n"
"  opendcp_encode j2k frame.tif frame_001.j2c\n"
"  opendcp_encode j2k stereoscopic frame_left.tif frame_right.tif frame_left.j2c frame_right.j2c\n"
"  opendcp_encode j2k tif_frames/ j2c_frames/\n"
"  opendcp_encode mxf --rate 25 frames/ my.mxf\n"
"";

const char usage_j2k[] =
"Usage:\n"
"  opendcp_encode j2k [options] <input> <output>\n"
"  opendcp_encode j2k stereoscopic [options] <input_left> <input_right> <output>\n"
"\n"
"Options:\n"
"     --help                         Show this screen.\n"
"     --version                      Show version.\n"
"     --no_overwrite                 Do no overwrite existing files\n"
"     --no_xyz                       Disable XYZ<->RGB conversion                  \n"
"     --resize                       Resize image to closest DCI compliant resolution\n"
"     --bw         <bw>              Maximum JPEG2000 bandwidth [default: 125]\n"
"     --colorspace <colorspace>      Source colorspace (srgb|rec709|p3|srgb_complex|rec709_complex) [default rec709]\n"
"     --encoder    <encoder>         JPEG2000 j2kr (openjpeg|kakadu) [default openjpeg]\n"
"     --profile    <profile>         Cinema profile (2k|4k) [default 2k]\n"
"     --rate       <frame_rate>      Frame rate of source [default 24]\n"
"     --log_level  <log_level>       Sets the log level 0-4. Higher means more logging [default 2]\n"
"     --threads    <threads>         The number of threads to use for encoding\n"
"     --tmp_dir    <tmp_dir>         Temporary directory for intermediate files\n"
"\n";

const char usage_mxf[] = 
"Usage:\n"
"  opendcp_encode mxf [options] <input> <output>\n"
"  opendcp_encode mxf stereoscopic [options] <input_left> <input_right> <output>\n"
"\n"
"Options:\n"
"     --help                         Show this screen.\n"
"     --version                      Show version.\n"
"     --rate       <frame_rate>      Frame rate of source [default 24]\n"
"     --type       <type>            Generate SMPTE or MXF Interop labels (smpte|interop)  [default smpte]\n"
"     --start      <start_frame>     Start frame [default 0]\n"
"     --end        <end_frame>       End frame\n"
"     --slideshow  <duration>        Create a slideshow with each image having duration specified in seconds\n"
"     --log_level  <log_level>       Sets the log level 0-4. Higher means more logging [default 2]\n"
"     --key        <key>             Set encryption key and enable encryption (not recommended)\n"
"     --key_id     <key_id>          Set encryption key id (leaving blank generates a random uuid)\n"
"     --threads    <threads>         The number of threads to use for encoding\n"
"\n";

argv_t argv_create(int argc, char **argv) {
    argv_t a = {argc, argv, 0, argv[0]};

    return a;
}

argv_t *argv_increment(argv_t *a) {
    if (a->i < a->argc) {
        a->current = a->argv[++a->i];
    }

    if (a->i == a->argc) {
        a->current = NULL;
    }

    return a;
}

int parse_options(argv_t *a, cli_t *elements) {
    int i;
    int len_prefix;
    int n_options = elements->n_options;
    char *eq = strchr(a->current, '=');
    option_t *option;
    option_t *options = elements->options;

    len_prefix = (eq-(a->current)) / sizeof(char);

    for (i=0; i < n_options; i++) {
        option = &options[i];
        if (!strncmp(a->current, option->name, len_prefix)) {
            break;
        }
    }

    if (i == n_options) {
        fprintf(stderr, "%s is not recognized\n", a->current);
        return 1;
    }

    argv_increment(a);

    if (option->value_required) {
        if (eq == NULL) {
            if (a->current == NULL) {
                fprintf(stderr, "%s requires argument\n", option->name);
                return 1;
            }
            option->value = a->current;
            argv_increment(a);
        } else {
            option->value = eq + 1;
        }
    } else {
        if (eq != NULL) {
            fprintf(stderr, "%s must not have an argument\n", option->name);
            return 1;
        }
        option->value = "1";
    }

    return 0;
}

int parse_commands(argv_t *a, cli_t *elements) {
    int i;
    int n_commands = elements->n_commands;
    command_t *command;
    command_t *commands = elements->commands;

    for (i=0; i < n_commands; i++) {
        command = &commands[i];

        if (!strcmp(command->name, a->current) && !command->seen) {
            command->value = true;
            command->seen = true;
            return 0;
        }
    }

    return 1;
}

int set_positional(const char *name, positional_t positional, cli_t *elements) {
    int i;
    argument_t *argument;
    argument_t *arguments = elements->arguments;

    for (i=0; i < elements->n_arguments; i++) {
        argument = &arguments[i];
        if (!strcmp(name, argument->name)) {
            argument->value = positional.name;
            return 0;
        }
    }

    return 1;
}

int parse_positional(cli_t *elements, opendcp_args_t *args) {

    /* j2k command */
    if (args->j2k) {
        if (args->stereoscopic) {
            if (elements->n_positional != 3) {
                fprintf(stderr, "j2k requires <input-left> <input-right> <output>\n");
                return 1;
            }
            set_positional("<input_left>", elements->positionals[0], elements);
            set_positional("<input_right>", elements->positionals[1], elements);
            set_positional("<output_left>", elements->positionals[2], elements);
        }
        else {
            if (elements->n_positional != 2) {
                fprintf(stderr, "j2k requires <input> <output>\n");
                return 1;
            }

            set_positional("<input>", elements->positionals[0], elements);
            set_positional("<output>", elements->positionals[1], elements);
        }

        return 0;
    }

    /* mxf command */
    if (args->mxf) {
        if (args->stereoscopic) {
            if (elements->n_positional != 4) {
                fprintf(stderr, "mxf stereoscopic requires <input_left> <input_right> <output_left> <output_right>\n");
                return 1;
            }
            set_positional("<input_left>", elements->positionals[0], elements);
            set_positional("<input_right>", elements->positionals[1], elements);
            set_positional("<output_left>", elements->positionals[2], elements);
            set_positional("<output_right>", elements->positionals[3], elements);
        }
        else {
            if (elements->n_positional != 2) {
                fprintf(stderr, "mxf requires <input> <output>\n");
                return 1;
            }
            set_positional("<input>", elements->positionals[0], elements);
            set_positional("<output>", elements->positionals[1], elements);
        }

        return 0;
    }

    return 0;
}

int parse_args(argv_t *a, cli_t *elements) {
    int ret;

    /* first argv is the command, skip that */
    argv_increment(a);

    while (a->current != NULL) {
        if (a->current[0] == '-' && a->current[1] == '-') {
            ret = parse_options(a, elements);
        }
        else {
            ret = parse_commands(a, elements);
            if (ret) {
                elements->positionals[elements->n_positional++].name = a->current;
                ret = 0;
            }
            argv_increment(a);
        }

        if (ret) {
            return ret;
        }
    }

    return ret;
}

int options_to_args(opendcp_t *opendcp, cli_t *elements, opendcp_args_t *args) {
    option_t *option;
    int       value, i;
    char      key_id[40];

    for (i=0; i < elements->n_options; i++) {
        option = &elements->options[i];
        value = option->value != NULL ? 1:0;

        if (!strcmp(option->name, "--help")) {
            if (value) {
                fprintf(stdout, "%s version %s %s\n\n", OPENDCP_NAME, OPENDCP_VERSION, OPENDCP_COPYRIGHT);
                if (args->j2k) {
                    fprintf(stdout, "%s", usage_j2k);
                } else if (args->mxf) {
                    fprintf(stdout, "%s", usage_mxf);
                } else {
                    fprintf(stdout, "%s", usage);
                }
                exit(0);
            }
        }

        if  (!strcmp(option->name, "--version")) {
            if (value) {
                fprintf(stdout, "%s\n", OPENDCP_VERSION);
                exit(0);
            }
        }

        /* flags */
        if (!strcmp(option->name, "--no_overwrite")) {
            opendcp->j2k.no_overwrite = value;
        }

        if (!strcmp(option->name, "--no_xyz")) {
            opendcp->j2k.xyz = !value;
        }

        if (!strcmp(option->name, "--resize")) {
            opendcp->j2k.resize = value;
        }

        /* options */
        if (!option->value) {
            continue;
        }

        if (!strcmp(option->name, "--bw")) {
            opendcp->j2k.bw = atoi(option->value);
        }

        if (!strcmp(option->name, "--colorspace")) {
            if (!strcmp(option->value, "srgb")) {
                opendcp->j2k.lut = CP_SRGB;
            }
            else if (!strcmp(option->value, "rec709")) {
                opendcp->j2k.lut = CP_REC709;
            }
            else if (!strcmp(option->value, "p3")) {
                opendcp->j2k.lut = CP_P3;
            }
            else if (!strcmp(option->value, "srgb_complex")) {
                opendcp->j2k.lut = CP_SRGB_COMPLEX;
            }
            else if (!strcmp(option->value, "rec709_complex")) {
                opendcp->j2k.lut = CP_REC709_COMPLEX;
            }
            else {
                fprintf(stderr, "Invalid colorspace argument\n");
                exit(1);
            }
        }

        if (!strcmp(option->name, "--encoder")) {
            if (!strcmp(option->value, "openjpeg")) {
                opendcp->j2k.encoder = OPENDCP_ENCODER_OPENJPEG;
            }
            else if (!strcmp(option->value, "kakadu")) {
                opendcp->j2k.encoder = OPENDCP_ENCODER_KAKADU;
            }
            else if (!strcmp(option->value, "ragnarok")) {
                opendcp->j2k.encoder = OPENDCP_ENCODER_RAGNAROK;
            }
            else if (!strcmp(option->value, "remote")) {
                opendcp->j2k.encoder = OPENDCP_ENCODER_REMOTE;
            }
            else {
                fprintf(stderr, "Invalid encoder argument\n");
                exit(1);
            }
        }

        if (!strcmp(option->name, "--profile")) {
            if (!strcmp(option->value, "2k")) {
                opendcp->cinema_profile = DCP_CINEMA2K;
            }
            else if (!strcmp(option->value, "4k")) {
                opendcp->cinema_profile = DCP_CINEMA4K;
            }
            else {
                fprintf(stderr, "Invalid cinema profile argument\n");
                exit(1);
            }
        }

        if (!strcmp(option->name, "--rate")) {
            opendcp->frame_rate = atoi(option->value);
        }

        if (!strcmp(option->name, "--type")) {
            if (!strcmp(option->value, "smpte")) {
                opendcp->ns = XML_NS_SMPTE;
            }
            else if (!strcmp(option->value, "interop")) {
                opendcp->ns = XML_NS_INTEROP;
            }
            else {
                fprintf(stderr, "Invalid profile argument, must be smpte or interop");
                exit(1);
            }
        }

        if (!strcmp(option->name, "--start")) {
            opendcp->j2k.start_frame = atoi(option->value);
        }

        if (!strcmp(option->name, "--end")) {
            opendcp->j2k.end_frame = strtol(option->value, NULL, 10);
        }

        if (!strcmp(option->name, "--slideshow")) {
            opendcp->mxf.slide = 1;
            opendcp->mxf.frame_duration = atoi(option->value);

            if (opendcp->mxf.frame_duration < 1) {
                fprintf(stderr, "Slide duration  must be greater than 0");
                exit(1);
            }
        }

        if (!strcmp(option->name, "--log_level")) {
            opendcp->log_level = atoi(option->value);
        }

        if (!strcmp(option->name, "--key")) {
            if (!is_key(option->value)) {
               fprintf(stderr, "Invalid encryption key format");
            }

            if (hex2bin(option->value, opendcp->mxf.key_value, 16)) {
               fprintf(stderr, "Invalid encryption key format");
            }
            opendcp->mxf.key_flag = 1;
        }

        if (!strcmp(option->name, "--key_id")) {
            if (!is_uuid(option->value)) {
               fprintf(stderr, "Invalid encryption key id format");
            }

            strnchrdel(option->value, key_id, sizeof(key_id), '-');

            if (hex2bin(key_id, opendcp->mxf.key_id, 16)) {
               fprintf(stderr, "Invalid encryption key format");
            }
            opendcp->mxf.key_id_flag = 1;
        }

        if (!strcmp(option->name, "--threads")) {
            opendcp->threads = atoi(option->value);
        }

        if (!strcmp(option->name, "--tmp_dir")) {
            opendcp->tmp_path = option->value;
        }
    }

    return 0;
}

int commands_to_args(cli_t *elements, opendcp_args_t *args) {
    command_t *command;
    int i;

    for (i=0; i < elements->n_commands; i++) {
        command = &elements->commands[i];

        if (!strcmp(command->name, "j2k")) {
            args->j2k = command->value;
        }

        if (!strcmp(command->name, "mxf")) {
            args->mxf = command->value;
        }

        if (!strcmp(command->name, "stereoscopic")) {
            args->stereoscopic = command->value;
        }
    }

    return 0;
}

int arguments_to_args(cli_t *elements, opendcp_args_t *args) {
    argument_t *argument;
    int i;

    for (i=0; i < elements->n_arguments; i++) {
        argument = &elements->arguments[i];

        if (!strcmp(argument->name, "<input>")) {
            args->input = argument->value;
        }

        if (!strcmp(argument->name, "<input_left>")) {
            args->input_left = argument->value;
        }

        if (!strcmp(argument->name, "<input_right>")) {
            args->input_right = argument->value;
        }

        if (!strcmp(argument->name, "<output>")) {
            args->output = argument->value;
        }

        if (!strcmp(argument->name, "<output_left>")) {
            args->output_left = argument->value;
        }

        if (!strcmp(argument->name, "<output_right>")) {
            args->output_right = argument->value;
        }
    }

    return 0;
}

opendcp_args_t opendcp_args(opendcp_t *opendcp, int argc, char *argv[]) {
    int i = 0;

    opendcp_args_t args = {
        0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
    };

    argv_t a;

    command_t commands[] = {
        {"j2k",          0, 0},
        {"mxf",          0, 0},
        {"stereoscopic", 0, 0},
        {NULL,           0, 0}
    };

    argument_t arguments[] = {
        {"<input>",        NULL},
        {"<input_left>",   NULL},
        {"<input_right>",  NULL},
        {"<output>",       NULL},
        {"<output_left>",  NULL},
        {"<output_right>", NULL},
        {NULL,             NULL}
    };

    option_t options[] = {
        {"--help",         0, NULL},
        {"--version",      0, NULL},
        {"--no_overwrite", 0, NULL},
        {"--no_xyz",       0, NULL},
        {"--resize",       0, NULL},
        {"--bw",           1, "125"},
        {"--colorspace",   1, "rec709"},
        {"--encoder",      1, "openjpeg"},
        {"--profile",      1, "2k"},
        {"--rate",         1, "24"},
        {"--type",         1, "smpte"},
        {"--start",        1, "1"},
        {"--end",          1, "NULL"},
        {"--slideshow",    1, NULL},
        {"--log_level",    1, "1"},
        {"--key",          1, NULL},
        {"--key_id",       1, NULL},
        {"--threads",      1, "4"},
        {"--tmp_dir",      1, NULL},
        {NULL,             0, NULL}
    };

    positional_t positionals[MAX_POSITIONALS] = {
        {NULL}
    };

    cli_t elements = {0, 0, 0, 0, commands, arguments, options, positionals};

    for (i = 0; commands[i].name != NULL; i++) {
        elements.n_commands++;
    }

    for (i = 0; arguments[i].name != NULL; i++) {
        elements.n_arguments++;
    }

    for (i = 0; options[i].name != NULL; i++) {
        elements.n_options++;
    }

    a = argv_create(argc, argv);

    if (parse_args(&a, &elements)) {
        exit(1);
    }

    commands_to_args(&elements, &args);

    options_to_args(opendcp, &elements, &args);

    if (parse_positional(&elements, &args)) {
        exit(1);
    }

    arguments_to_args(&elements, &args);
    //options_to_args(opendcp, &elements, &args);

    return args;
}

int main(int argc, char *argv[]) {
    opendcp_t *opendcp = opendcp_create();
    opendcp_args_t args = opendcp_args(opendcp, argc, argv);

    /* set log level */
    opendcp_log_init(opendcp->log_level);

    /* set stereoscopic option */
    opendcp->stereoscopic = args.stereoscopic;

    printf("Commands:\n");
    printf("    mxf == %s\n", args.mxf ? "true" : "false");
    printf("    j2k == %s\n", args.j2k ? "true" : "false");
    printf("    stereoscopic == %s\n", args.stereoscopic ? "true" : "false");
    printf("Arguments:\n");
    printf("    input  == %s\n", args.input);
    printf("    output == %s\n", args.output);
    printf("    input_left   == %s\n", args.input_left);
    printf("    input_right  == %s\n", args.input_right);
    printf("    output_left  == %s\n", args.output_left);
    printf("    output_right == %s\n", args.output_right);

    printf("Flags:\n");
    printf("    --no_xyz       == %s\n", opendcp->j2k.xyz ? "true" : "false");
    printf("    --no_overwrite == %s\n", opendcp->j2k.no_overwrite ? "true" : "false");
    printf("    --resize       == %s\n", opendcp->j2k.resize ? "true" : "false");
    printf("Options:\n");
    printf("    --bw      == %d\n", opendcp->j2k.bw);
    printf("    --encoder == %d\n", opendcp->j2k.encoder);
    printf("    --profile == %d\n", opendcp->cinema_profile);
    printf("    --rate    == %d\n", opendcp->frame_rate);

    if (args.mxf) {
        printf("mxf\n");
    }
    else if (args.j2k) {
        opendcp_command_j2k(opendcp, &args);
    }
    else {
        fprintf(stdout, "%s", usage);
        exit(1);
    }

    return 0;
}
