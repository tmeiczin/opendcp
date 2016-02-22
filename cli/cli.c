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
#if defined(WIN32) || defined(APPLE)
#include "win32/opendcp_win32_string.h"
#endif
#ifdef WIN32
#include "win32/opendcp_win32_dirent.h"
#else
#include <dirent.h>
#endif
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <stdint.h>

#include "opendcp.h"
#include "cli.h"


int is_dir(const char *path) {
    struct stat st_in;

    if (stat(path, &st_in) != 0 ) {
        return 0;
    }

    if (S_ISDIR(st_in.st_mode)) {
        return 1;
    }

    return 0;
}

int check_extension(char *filename, char *pattern) {
    char *extension;

    extension = strrchr(filename, '.');

    if ( extension == NULL ) {
        return 0;
    }

    extension++;

    if (strncasecmp(extension, pattern, 3) != 0) {
        return 0;
    }

    return 1;
}

char *get_basename(const char *filename) {
    char *extension;
    char *base = 0;

    base = (char *)filename;
    extension = strrchr(filename, '.');
    base[(strlen(filename) - strlen(extension))] = '\0';

    return(base);
}

char *basename_noext(const char *str) {
    if (str == 0 || strlen(str) == 0) {
        return NULL;
    }

    char *base = strrchr(str, '/') + 1;
    char *ext  = strrchr(str, '.');

    return strndup(base, ext - base);
}

int file_selector(const char *filename, const char *filter) {
    char *extension;

    extension = strrchr(filename, '.');

    if ( extension == NULL ) {
        return 0;
    }

    extension++;

    if (strlen(extension) < 3) {
        return 0;
    }

    if (strstr(filter, extension) != NULL) {
        return 1;
    }

    return 0;
}

void build_output_filename(const char *in, const char *path, char *out, const char *ext) {
    OPENDCP_LOG(LOG_DEBUG, "Building filename from %s", in);

    if (!is_dir(path)) {
        snprintf(out, MAX_FILENAME_LENGTH, "%s", path);
    }
    else {
        char *base = basename_noext(in);
        snprintf(out, MAX_FILENAME_LENGTH, "%s/%s.%s", path, base, ext);

        if (base) {
            free(base);
        }
    }
}

filelist_t *get_output_filelist(filelist_t *in, const char *path, const char *ext) {
    size_t i, count;

    count = in->nfiles;
    filelist_t *out = filelist_alloc(count);

    for (i = 0; i < count; i++) {
        build_output_filename(in->files[i], path, out->files[i], ext); 
    }

    return out;
}

filelist_t *get_filelist(const char *path, const char *filter) {
    DIR *d;
    struct stat st_in;
    struct dirent *de;
    char **names = 0, **tmp;

    size_t cnt = 0, len = 0;
    filelist_t *filelist;

    if (stat(path, &st_in) != 0 ) {
        OPENDCP_LOG(LOG_DEBUG, "path not found %s", path);
        return NULL;
    }

    if (!S_ISDIR(st_in.st_mode)) {
        OPENDCP_LOG(LOG_DEBUG, "single file mode");
        filelist = filelist_alloc(1);
        snprintf(filelist->files[0], MAX_FILENAME_LENGTH, "%s", path);
        return filelist;
    }

    if ((d = opendir(path)) == NULL) {
        return(NULL);
    }

    OPENDCP_LOG(LOG_DEBUG, "reading directory");

    while ((de = readdir(d))) {
        if (!file_selector(de->d_name, filter)) {
            continue;
        }

        if (cnt >= len) {
            len = 2 * len + 1;

            if (len > SIZE_MAX / sizeof * names) {
                break;
            }

            tmp = realloc(names, len * sizeof * names);

            if (!tmp) {
                break;
            }

            names = tmp;
        }

        names[cnt] = malloc(strlen(de->d_name) + 1);

        if (!names[cnt]) {
            break;
        }

        snprintf(names[cnt++], strlen(de->d_name) + 1, "%s", de->d_name);
        OPENDCP_LOG(LOG_DEBUG, "Found %s", de->d_name);
    }

    closedir(d);

    OPENDCP_LOG(LOG_DEBUG, "found %d files", cnt);
    filelist = filelist_alloc(cnt);

    if (names) {
        while (cnt-- > 0) {
            OPENDCP_LOG(LOG_DEBUG, "Adding file %s", names[cnt]);
            snprintf(filelist->files[cnt], MAX_FILENAME_LENGTH, "%s/%s", path, names[cnt]);
            free(names[cnt]);
        }

        free(names);
    }

    return filelist;
}

int find_seq_offset(char str1[], char str2[]) {
    unsigned int i;
    unsigned int offset = 0;

    for (i = 0; (i < strlen(str1)) && (offset == 0); i++) {
        if(str1[i] != str2[i]) {
            offset = i;
        }
    }

    return offset;
}

int find_ext_offset(char str[]) {
    int i = strlen(str);

    while(i) {
        if(str[i] == '.') {
            return i;
        }

        i--;
    }

    return 0;
}

char *append(const char *str1, const char *str2) {
    const size_t len = strlen(str1) + strlen(str2) + 2;
    char *out = malloc(len);

    if (strlen(str1) < 1) {
        snprintf(out, len, "%s", str2);
    } else if (strlen(str2) < 1) {
        snprintf(out, len, "%s", str1);
    } else {
        snprintf(out, len, "%s %s", str1, str2);
    }

    return out;
}

char *substring(const char *str, size_t begin, size_t len) {
    char   *result;

    if (str == 0 || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin + len)) {
        return NULL;
    }

    result = (char *)malloc(len);

    if (!result) {
        return NULL;
    }

    result[0] = '\0';
    strncat(result, str + begin, len);

    return result;
}
