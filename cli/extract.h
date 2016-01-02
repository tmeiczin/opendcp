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
     GENERATOR(mxf,    "Extract MXF file",              file)

/* (argument, desc) */
#define ARGUMENTS(GENERATOR) \
    GENERATOR(file,        "Input mxf file") \

/* (option, value_required, default, desc) */
#define OPTIONS(GENERATOR) \
    GENERATOR(help,           0, NULL,     "Show help") \
    GENERATOR(version,        0, NULL,     "Show version") \
    GENERATOR(start,          1, 1,        "Frame to start decode") \
    GENERATOR(end,            1, 0,        "Frame to stop decode") \
    GENERATOR(log_level,      1, 2,        "Log level (0:Quiet, 1:Error, 2:Warn (default),  3:Info, 4:Debug") \
    GENERATOR(key,            1, NULL,     "Set decryption key (for encrypted assets)")

/* include arg structure */
CLI_PARSER_STRUCT

#ifdef __cplusplus
}
#endif

#endif // _ENCODE_H_
