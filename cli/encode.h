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

#ifndef _ENCODE_H_
#define _ENCODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cli_parser.h"

/* (command, desc, arg count, args) */
#define COMMANDS(GENERATOR) \
     GENERATOR(j2k,    "Encode JPEG2000",              input,output)  \
     GENERATOR(j2k_3d, "Encode JPEG2000 Sterepscopic", input_left,input_right,output_left,output_right)  \
     GENERATOR(mxf,    "Encode MXF",                   input,output)  \
     GENERATOR(mxf_3d, "Encode MXF Stereoscopic",      input_left,input_right,output)

/* (argument, desc) */
#define ARGUMENTS(GENERATOR) \
    GENERATOR(input,        "Input file or directory") \
    GENERATOR(input_left,   "Left input file or directory in 3D mode") \
    GENERATOR(input_right,  "Right input file or directory in 3D mode") \
    GENERATOR(output,       "Output file or directory") \
    GENERATOR(output_left,  "Left output file or directory in 3D mode") \
    GENERATOR(output_right, "Right output file or directory in 3D mode")

/* (option, value_required, default, desc) */
#define OPTIONS(GENERATOR) \
    GENERATOR(help,           0, NULL,     "Show help") \
    GENERATOR(version,        0, NULL,     "Show version") \
    GENERATOR(bw,             1, 125,      "The bandwidth in MB/s for codestream") \
    GENERATOR(overwrite,      1, true,     "Overwrite existing files") \
    GENERATOR(xyz,            1, true,     "Perform XYZ<->RGB conversion") \
    GENERATOR(resize,         1, false,    "Resize image to closest DCI compliant resolution") \
    GENERATOR(colorspace,     1, rec709,   "Source colorspace (srgb|rec709|p3|srgb_complex|rec709_complex)") \
    GENERATOR(encoder,        1, openjpeg, "JPEG2000 j2k (openjpeg|kakadu)") \
    GENERATOR(profile,        1, 2k,       "The cinema profile (2k | 4k)") \
    GENERATOR(frame_rate,     1, 24,       "Frame rate of source") \
    GENERATOR(type,           1, smpte,    "Generate SMPTE or MXF Interop labels (smpte|interop)") \
    GENERATOR(start,          1, 1,        "The start frame") \
    GENERATOR(end,            1, 0,        "The end frame") \
    GENERATOR(slideshow,      1, 0,        "Create a slideshow with each image having duration specified in seconds") \
    GENERATOR(log_level,      1, 2,        "Sets the log level 0-4. Higher means more logging") \
    GENERATOR(key,            1, NULL,     "Set encryption key and enable encryption (not recommended)") \
    GENERATOR(key_id,         1, NULL,     "Set encryption key id (leaving blank generates a random uuid)") \
    GENERATOR(threads,        1, 0,        "The number of threads to use") \
    GENERATOR(tmp_path,       1, NULL,     "Temporary directory for intermediate files")

/* include arg structure */
CLI_PARSER_STRUCT

/* function declarations */
int opendcp_command_j2k(args_t *args);

#ifdef __cplusplus
}
#endif

#endif // _ENCODE_H_
