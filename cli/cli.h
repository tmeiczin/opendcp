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

#ifndef OPENDCP_CLI_H
#define OPENDCP_CLI_H

/* prototypes */
int   check_extension(char *filename, char *pattern);
int   find_ext_offset(char str[]);
int   find_seq_offset (char str1[], char str2[]);
char *get_basename(const char *filename);
void  build_output_filename(const char *in, const char *path, char *out, const char *ext);
filelist_t *get_filelist(const char *path, const char *filter);
filelist_t *get_output_filelist(filelist_t *in, const char *path, const char *ext);

#endif
