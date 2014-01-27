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
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <sys/stat.h>
#include <dirent.h>
#include <inttypes.h>
#include <ctype.h>
#include <time.h>
#include <openssl/md5.h>
#include "opendcp.h"


int get_file_length(char *filename) {
    FILE *fp;
    int file_length;

    fp = fopen(filename, "rb");

    if (!fp) {
        printf("could not open file\n");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    file_length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    fclose(fp);

    return file_length;
}

int read_file(char *filename, char *buffer, int buffer_size) {
    FILE *fp;
    int  file_length;

    fp = fopen(filename, "rb");
    fread(buffer, 1, buffer_size, fp);
    fclose(fp);

    return OPENDCP_NO_ERROR;
}

static void print_md5(const unsigned char *digest, int len) {
    int i;

    for (i = 0; i < len; i++) {
        printf ("%02x", digest[i]);
    }

    printf("\n");

    return;
}

void md5sum(char *md5, char *data, int len) {
    EVP_MD_CTX mdctx;
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;

    EVP_DigestInit(&mdctx, EVP_md5());
    EVP_DigestUpdate(&mdctx, data, (size_t) len);
    EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
    EVP_MD_CTX_cleanup(&mdctx);
    print_md5(md_value, md_len);
    memcpy(md5, md_value, md_len);

    return;
}

int strcasefind(const char *s, const char *find) {
    char c, sc;
    size_t len;

    if ((c = *find++) != 0) {
        c = tolower((unsigned char)c);
        len = strlen(find);

        do {
            do {
                if ((sc = *s++) == 0) {
                    return 0;
                }
            } while ((char)tolower((unsigned char)sc) != c);
        } while (strncasecmp(s, find, len) != 0);

        s--;
    }

    return 1;
}

/* populates prefix with the longest common prefix of s1 and s2. */
static void common_prefix(const char *s1, const char *s2, char *prefix) {
    int i;

    for (i = 0; s1[i] && s2[i] && s1[i] == s2[i]; i++) {
        prefix[i] = s1[i];
    }

    prefix[i] = '\0';
}

/* populates prefix with the longest common prefix of all the files. */
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

/* compare 2 File_and_index structs by their index. */
static int file_cmp(const void *a, const void *b) {
    const opendcp_sort_t *fa, *fb;
    fa = a;
    fb = b;

    return (fa->index) - (fb->index);
}

/* return the index of a filename */
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
Ensure a list of ordered filenames are sequential

@param  files is an array of file names
@param  nfiles is the number of files
@return returns 0 if files are sequential, otherwise it returns the index
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
  <index> is the longest (though possibly empty) common prefix/suffic of all the
  files.
  N is some decimal number which I call its "index".
  N is unique for each file.
  1 <= N <= nfiles  OR  0 <= N < nfiles

@param  files is an array of file names
@param  nfiles is the number of files
@return OPENDCP_ERROR_CODE
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

/**
Allocate a list of filenames

This function allocates memory for a list of filenames.

@param  nfiles is the number of files to allocate
@return filelist_t pointer
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

@param  filelist_t a pointer to a filelist
@return NONE
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
generate a timestamp string

This function will generate a timestamp string based on the current local time.

@param  timestamp buffer that will hold the timestamp
@return NONE
*/
void get_timestamp(char *timestamp) {
    time_t time_ptr;
    struct tm *time_struct;
    char buffer[30];

    time(&time_ptr);
    time_struct = localtime(&time_ptr);
    strftime(buffer, 30, "%Y-%m-%dT%I:%M:%S+00:00", time_struct);
    sprintf(timestamp, "%.30s", buffer);
}

/**
determine an assets type

This function will return the class type of an asset essence.

@param  asset_t
@return int
*/
int get_asset_type(asset_t asset) {
    switch (asset.essence_type) {
        case AET_MPEG2_VES:
        case AET_JPEG_2000:
        case AET_JPEG_2000_S:
            return ACT_PICTURE;
            break;

        case AET_PCM_24b_48k:
        case AET_PCM_24b_96k:
            return ACT_SOUND;
            break;

        case AET_TIMED_TEXT:
            return ACT_TIMED_TEXT;
            break;

        default:
            return ACT_UNKNOWN;
    }
}
