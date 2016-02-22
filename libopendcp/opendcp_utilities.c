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

#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <sys/stat.h>
#include <dirent.h>
#include <inttypes.h>
#include <ctype.h>
#include <time.h>
#include "opendcp.h"


bool is_dir(const char *path) {
    struct stat st_in;

    if (stat(path, &st_in) != 0 ) {
        return false;
    }

    if (S_ISDIR(st_in.st_mode)) {
        return true;
    }

    return false;
}

int htob(int x) {
    if (x >= '0' && x <= '9')
        return x - '0';

    if (x >= 'A' && x <= 'F')
        return x - 'A' + 10;

    if (x >= 'a' && x <= 'f')
        return x - 'a' + 10;

    return 0;
}

int hex2bin(const char *str, byte_t *buf, unsigned int buf_len) {
    int i, x = 0;

    /* nothing to do */
    if (str[0] == 0) {
        return 0;
    }

    /* buffer isn't big enough */
    if (strlen(str)/2 > buf_len) {
        return 1;
    }

    /* iterate and convert */
    for ( i = 0; str[i]; i+=2 ) {
        if (!isxdigit(str[i]) || !isxdigit(str[i+1])) {
            return 1;
        }
        buf[x++] = (htob(str[i]) << 4) | htob(str[i+1]);
    }

    return OPENDCP_NO_ERROR;
}

bool is_key_value_set(byte_t *key, int len) {
    int i;

    for(i = 0; i < len / (int)sizeof(byte_t); i++) {
        if (key[i]) {
            return true;
        }
    }

    return false;
}

bool is_uuid(const char *s) {
    return is_key(s);
}

bool is_key(const char *s) {
    int i, len;
    char strip[40] = "";

    strnchrdel(s, strip, sizeof(strip), '-');

    len = strlen(strip);

    if (len != 32) {
        return false;
    }

    for (i = 0; i < len; i++) {
        if (isxdigit(strip[i]) == 0) {
            return false;
        }
    }

    return true;
}

bool is_filename_ascii(const char *s) {
    int i, len;

    len = strlen(s);

    for (i = 0; i < len; i++) {
        if (isascii(s[i]) == 0) {
            return false;
        }
    }

    return true;
}

/**
This function will generate a timestamp string based on the current local time.

@param
  timestamp     buffer that will hold the timestamp

@return
  void          the result is placed in the timestamp buffer
*/
void generate_timestamp(char *timestamp) {
    time_t time_ptr;
    struct tm *time_struct;
    char buffer[30];

    time(&time_ptr);
    time_struct = localtime(&time_ptr);
    strftime(buffer, 30, "%Y-%m-%dT%I:%M:%S+00:00", time_struct);
    sprintf(timestamp, "%.30s", buffer);
}

/**
Allocate a list of filenames.

This function allocates memory for a list of filenames.

@param
  nfiles         number of files to allocate

@return
  filelist_t     filelist_t pointer
*/
filelist_t *filelist_alloc(int nfiles) {
    int x;
    filelist_t *filelist;

    filelist = malloc(sizeof(filelist_t));

    filelist->nfiles = nfiles;
    filelist->files  = malloc(filelist->nfiles * sizeof(char*));

    if (filelist->nfiles) {
        for (x = 0; x < filelist->nfiles; x++) {
            filelist->files[x]  = malloc(MAX_FILENAME_LENGTH * sizeof(char));
        }
    }

    return filelist;
}

/**
free a filelist_t structure

This function frees memory used by filelist_t.

@param
  filelist_t         a pointer to a filelist
@return
   void 
*/
void filelist_free(filelist_t *filelist) {
    int x;

    if (filelist == NULL) {
        return;
    }

    if (filelist->files) {
        for (x = 0; x < filelist->nfiles; x++) {
            free(filelist->files[x]);
        }

        free(filelist->files);
    }

    free(filelist);
}

/**
Remove the character d from the src string and place the result in dst.

@param
  src          source string
  dst          destination string
  dst_len      length of the destination buffer
  d            character to delete from source 

@return
  void         result is place in dst string
*/
void strnchrdel(const char *src, char *dst, int dst_len, char d) {
    int i, len, count = 0;

    /* get the number of occurrences */
    for (i = 0; src[i]; i++) {
        if (src[i] == d) {
            count++;
        }
    }

    /* adjust length to be the smaller of the two */
    len = dst_len > (int)strlen(src) - count ? (int)strlen(src) - count : dst_len;

    for (i = 0; i < len; i++) {
        if (src[i] != d) {
            *dst++ = src[i];
        }
        else {
            len++;
        }
    }
    *dst = '\0';
}

/**
Case insensitive substring search.

@params
  s           string to search in
  find        string to search for

@return
  bool        True if substring is found
*/
bool strcasefind(const char *s, const char *find) {
    char c, sc;
    size_t len;

    if ((c = *find++) != 0) {
        c = tolower((unsigned char)c);
        len = strlen(find);

        do {
            do {
                if ((sc = *s++) == 0) {
                    return false;
                }
            }
            while ((char)tolower((unsigned char)sc) != c);
        }
        while (strncasecmp(s, find, len) != 0);

        s--;
    }

    return true;
}

/**
Populates prefix with the longest common prefix of s1 and s2.

@params
  s1            string filename 
  s2            strinf filename
  prefix       the prefix string to populate
@return
  void
*/
static void common_prefix(const char *s1, const char *s2, char *prefix) {
    int i;

    for (i = 0; s1[i] && s2[i] && s1[i] == s2[i]; i++) {
        prefix[i] = s1[i];
    }

    prefix[i] = '\0';
}

/**
Populates prefix with the longest common prefix of all the files.

@params
  files        array of filenames
  nfiles       number of files in array
  prefix       the prefix string to populate

@return
  void 
*/
static void prefix_of_all(char *files[], int nfiles, char *prefix) {
    int i;

    common_prefix(files[0], files[1], prefix);

    for (i = 2; i < nfiles; i++) {
        common_prefix(files[i], prefix, prefix);
    }
}

/* a filename of the form <prefix>N*.<ext> paired with its index. */
typedef struct {
    char *file;
    int   index;
} opendcp_sort_t;

/**
Compare 2 file opendcp_sort_t structs by their index.

@params
  a           opendcp_sort_t
  b           opendcp_sort_t

@return
  
*/
static int file_cmp(const void *a, const void *b) {
    const opendcp_sort_t *fa, *fb;
    fa = a;
    fb = b;

    return (fa->index) - (fb->index);
}

/**
Return the index of a filename.

@params
  file        string filename
  prefix_len  length of the file prefix

@return
  integer value of the index portion
*/
static int get_index(char *file, int prefix_len) {
    int index;

    char *index_string = file + prefix_len;

    if (!isdigit(index_string[0])) {
        return OPENDCP_ERROR;
    }

    index = atoi(index_string);

    return index;
}

/**
Ensure a list of ordered filenames are sequential.

@params
  files       array of file names
  nfiles      the number of files
@return
  0 if files are sequential, otherwise it returns the index
  the first out of order file.
*/
int ensure_sequential(char *files[], int nfiles) {
    int  prefix_len, i;
    char prefix_buffer[MAX_FILENAME_LENGTH];

    if (nfiles <= 1) {
        return OPENDCP_NO_ERROR;
    }

    prefix_of_all(files, nfiles, prefix_buffer);
    prefix_len = strlen(prefix_buffer);

    for (i = 0; i < nfiles - 1; i++) {
        if (get_index(files[i], prefix_len) + 1 != get_index(files[i + 1], prefix_len)) {
            if (!i) {
                i++;
            }
            return i;
        }
    }

    return OPENDCP_NO_ERROR;
}

/**
Order a list of filenames using a version type sort.

This function will order a list of filenames of the form:

  <index>N* or N*<index>

where:
  <index> is the longest (though possibly empty) common prefix/suffix of all the
  files.
  N is some decimal number which I call its "index".
  N is unique for each file.
  1 <= N <= nfiles  OR  0 <= N < nfiles

@params
  files       array of file names
  nfile       number of files
@return
  OPENDCP_ERROR_CODE
*/
int order_indexed_files(char *files[], int nfiles) {
    int  prefix_len, i;
    char prefix_buffer[MAX_FILENAME_LENGTH];
    opendcp_sort_t *fis;

    /* A single file is trivially sorted. */
    if (nfiles < 2) {
        return OPENDCP_NO_ERROR;
    }

    prefix_of_all(files, nfiles, prefix_buffer);
    prefix_len = strlen(prefix_buffer);

    /* Create an array of files and their indices to sort. */
    fis = malloc(sizeof(*fis) * nfiles);

    for (i = 0; i < nfiles; i++) {
        fis[i].file  = files[i];
        fis[i].index = get_index(files[i], prefix_len);
    }

    qsort(fis, nfiles, sizeof(*fis), file_cmp);

    /* Reorder the original file array. */
    for (i = 0; i < nfiles; i++) {
        files[i] = fis[i].file;
    }

    free(fis);

    return OPENDCP_NO_ERROR;
}
