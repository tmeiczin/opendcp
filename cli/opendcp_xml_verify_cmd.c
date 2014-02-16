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
#include <sys/types.h>

#include "opendcp.h"

void dcp_usage() {
    FILE *fp;
    fp = stdout;

    fprintf(fp, "\n%s version %s %s\n\n", OPENDCP_NAME, OPENDCP_VERSION, OPENDCP_COPYRIGHT);
    fprintf(fp, "Verifies the digital signature of an XML file\n\n");
    fprintf(fp, "Usage:\n");
    fprintf(fp, "       opendcp_xml_verify <xml file>\n\n");

    fclose(fp);
    exit(0);
}

int main (int argc, char **argv) {
    int result;
    char *filename;
    struct stat s;

    if ( argc <= 1 ) {
        dcp_usage();
    }

    opendcp_log_init(33);

    filename = argv[1];

    if( stat(filename, &s) == 0 ) {
        if( !(s.st_mode & S_IFREG) ) {
            OPENDCP_LOG(LOG_ERROR, "%s not a file", filename);
            exit(OPENDCP_ERROR);
        }
    }
    else {
        OPENDCP_LOG(LOG_ERROR, "could not open file: %s", filename);
        exit(OPENDCP_ERROR);
    }

    result = xml_verify(filename);

    if (result == OPENDCP_NO_ERROR) {
        OPENDCP_LOG(LOG_INFO, "%s Signature is VALID", filename);
        exit(OPENDCP_NO_ERROR);
    }
    else {
        OPENDCP_LOG(LOG_ERROR, "%s Signature is NOT VALID", filename);
        exit(OPENDCP_ERROR);
    }
}
