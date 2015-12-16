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
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <inttypes.h>
#include <time.h>
#include <sys/stat.h>
#include "opendcp.h"

int main (int argc, char **argv)
{
    struct stat st;

    if (argc < 2) {
        printf("\nTest to see if large file support is enabled. Should return correct\n");
        printf("size of files larger than 4gb\n\n");
        printf("usage: opendcp_largefile <file>\n");

        return -1;
    }

    char *filename = argv[1];
    stat(filename, &st);
    printf("file: %s size: %"PRIu64 "\n", filename, st.st_size);

    return 0;
}
