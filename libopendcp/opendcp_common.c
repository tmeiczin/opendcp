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
#include "opendcp.h"

const char *XML_HEADER  = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";

const char *NS_CPL[]    = { "none",
                            "http://www.digicine.com/PROTO-ASDCP-CPL-20040511#", /* MXF Interop */
                            "http://www.smpte-ra.org/schemas/429-7/2006/CPL"     /* SMPTE */
                          };

const char *NS_CPL_3D[] = { "none",
                            "http://www.digicine.com/schemas/437-Y/2007/Main-Stereo-Picture-CPL",   /* MXF Interop */
                            "http://www.smpte-ra.org/schemas/429-10/2008/Main-Stereo-Picture-CPL"   /* SMPTE */
                          };

const char *NS_PKL[]    = { "none",
                            "http://www.digicine.com/PROTO-ASDCP-PKL-20040311#", /* MXF Interop */
                            "http://www.smpte-ra.org/schemas/429-8/2007/PKL"     /* SMPTE */
                          };

const char *NS_AM[]     = { "none",
                            "http://www.digicine.com/PROTO-ASDCP-AM-20040311#",  /* MXF Interop */
                            "http://www.smpte-ra.org/schemas/429-9/2007/AM"      /* SMPTE */
                          };

const char *DS_DSIG = "http://www.w3.org/2000/09/xmldsig#";                      /* digial signature */
const char *DS_CMA  = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315";         /* canonicalization method */
const char *DS_DMA  = "http://www.w3.org/2000/09/xmldsig#sha1";                  /* digest method */
const char *DS_TMA  = "http://www.w3.org/2000/09/xmldsig#enveloped-signature";   /* transport method */

const char *DS_SMA[] = { "none",
                         "http://www.w3.org/2000/09/xmldsig#rsa-sha1",           /* MXF Interop */
                         "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256"     /* SMPTE */
                       };

const char *RATING_AGENCY[] = { "none",
                                "http://www.mpaa.org/2003-ratings",
                                "http://rcq.qc.ca/2003-ratings"
                              };

const char *OPENDCP_LOGLEVEL_NAME[] = { "NONE",
                                        "ERROR",
                                        "WARN",
                                        "INFO",
                                        "DEBUG"
                                      };

void dcp_fatal(opendcp_t *opendcp, char *format, ...) {
    char msg[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(msg, sizeof(msg), format, args);
    OPENDCP_LOG(LOG_ERROR, msg);
    va_end(args);
    opendcp_delete(opendcp);
    exit(OPENDCP_ERROR);
}

/*
char *base64(const unsigned char *data, int length) {
    int len;
    char *b_ptr;

    BIO *b64 = BIO_new(BIO_s_mem());
    BIO *cmd = BIO_new(BIO_f_base64());
    b64 = BIO_push(cmd, b64);

    BIO_write(b64, data, length);
    BIO_flush(b64);

    len = BIO_get_mem_data(b64, &b_ptr);
    b_ptr[len-1] = '\0';

    return b_ptr;
}
*/

int is_filename_ascii(const char *s) {
    int i, len;

    len = strlen(s);

    for (i = 0; i < len; i++) {
        if (isascii(s[i]) == 0) {
            return 0;
        }
    }

    return 1;
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

int opendcp_callback_null(void *args) {
    UNUSED(args);

    return OPENDCP_NO_ERROR;
}

/**
create an opendcp context

This function will allocate and initialize an opendcp context.

@param  NONE
@return An initialized opendcp_t structure on success, otherwise returns NULL
*/
opendcp_t *opendcp_create() {
    opendcp_t *opendcp;

    /* allocation opendcp memory */
    opendcp = malloc(sizeof(opendcp_t));

    if (!opendcp) {
        return NULL;
    }

    memset(opendcp, 0, sizeof (opendcp_t));

    /* initialize opendcp */
    opendcp->log_level = LOG_WARN;
    sprintf(opendcp->dcp.issuer, "%.80s %.80s", OPENDCP_NAME, OPENDCP_VERSION);
    sprintf(opendcp->dcp.creator, "%.80s %.80s", OPENDCP_NAME, OPENDCP_VERSION);
    sprintf(opendcp->dcp.annotation, "%.128s", DCP_ANNOTATION);
    sprintf(opendcp->dcp.title, "%.80s", DCP_TITLE);
    sprintf(opendcp->dcp.kind, "%.15s", DCP_KIND);
    get_timestamp(opendcp->dcp.timestamp);

    /* initialize callbacks */
    opendcp->mxf.frame_done.callback  = opendcp_callback_null;
    opendcp->mxf.frame_done.argument  = NULL;
    opendcp->mxf.file_done.callback   = opendcp_callback_null;
    opendcp->mxf.file_done.argument   = NULL;
    opendcp->dcp.sha1_update.callback = opendcp_callback_null;
    opendcp->dcp.sha1_update.argument = NULL;
    opendcp->dcp.sha1_done.callback = opendcp_callback_null;
    opendcp->dcp.sha1_done.argument = NULL;

    return opendcp;
}

/**
delete an opendcp context

This function will de-allocate an opendcp context.

@param  opendcp an opendcp_t structure
@return returns OPENDCP_NO_ERROR on success
*/
int opendcp_delete(opendcp_t *opendcp) {
    if ( opendcp != NULL) {
        free(opendcp);
    }

    return OPENDCP_NO_ERROR;
}

/**
create a pkl and add information

This function populates a pkl data structure with DCP information
from a dcp_t structure.

@param  dcp dcp_t structure
@param  pkl pkl_t  structure
@return NONE
*/
void create_pkl(dcp_t dcp, pkl_t *pkl) {
    char uuid_s[40];

    memset(pkl, 0, sizeof(pkl_t));

    strcpy(pkl->issuer,     dcp.issuer);
    strcpy(pkl->creator,    dcp.creator);
    strcpy(pkl->annotation, dcp.annotation);
    strcpy(pkl->timestamp,  dcp.timestamp);
    pkl->cpl_count = 0;

    /* Generate UUIDs */
    uuid_random(uuid_s);
    sprintf(pkl->uuid, "%.36s", uuid_s);

    /* Generate XML filename */
    if ( !strcmp(dcp.basename, "") ) {
        sprintf(pkl->filename, "PKL_%.40s.xml", pkl->uuid);
    } else {
        sprintf(pkl->filename, "PKL_%.40s.xml", dcp.basename);
    }

    return;
}

/**
add packaging list to dcp

This function adds a pkl to a dcp_t structure

@param  dcp dcp_t structure
@param  pkl pkl_t structure
@return NONE
*/
void add_pkl_to_dcp(dcp_t *dcp, pkl_t pkl) {
    memcpy(dcp[dcp->pkl_count].pkl, &pkl, sizeof(pkl_t));
    dcp->pkl_count++;

    return;
}

/**
add content playlist to packaging list

This function populates a cpl data structure with DCP information
from a dcp_t structure.

@param  dcp dcp_t structure
@param  cpl cpl_t structure
@return NONE
*/
void create_cpl(dcp_t dcp, cpl_t *cpl) {
    char uuid_s[40];

    memset(cpl, 0, sizeof(cpl_t));

    strcpy(cpl->annotation, dcp.annotation);
    strcpy(cpl->issuer,     dcp.issuer);
    strcpy(cpl->creator,    dcp.creator);
    strcpy(cpl->title,      dcp.title);
    strcpy(cpl->kind,       dcp.kind);
    strcpy(cpl->rating,     dcp.rating);
    strcpy(cpl->timestamp,  dcp.timestamp);
    cpl->reel_count = 0;

    uuid_random(uuid_s);
    sprintf(cpl->uuid, "%.36s", uuid_s);

    /* Generate XML filename */
    if ( !strcmp(dcp.basename, "") ) {
        sprintf(cpl->filename, "CPL_%.40s.xml", cpl->uuid);
    } else {
        sprintf(cpl->filename, "CPL_%.40s.xml", dcp.basename);
    }

    return;
}

/**
add packaging list to packaging list

This functio adds a cpl to a pkl structure

@param  dcp dcp_t structure
@param  cpl cpl_t structure
@return NONE
*/
void add_cpl_to_pkl(pkl_t *pkl, cpl_t cpl) {
    memcpy(&pkl->cpl[pkl->cpl_count], &cpl, sizeof(cpl_t));
    pkl->cpl_count++;

    return;
}

int init_asset(asset_t *asset) {
    memset(asset, 0, sizeof(asset_t));

    return OPENDCP_NO_ERROR;
}

void create_reel(dcp_t dcp, reel_t *reel) {
    char uuid_s[40];

    memset(reel, 0, sizeof(reel_t));

    strcpy(reel->annotation, dcp.annotation);

    /* Generate UUIDs */
    uuid_random(uuid_s);
    sprintf(reel->uuid, "%.36s", uuid_s);
}

int validate_reel(opendcp_t *opendcp, reel_t *reel, int reel_number) {
    int d = 0;
    int picture = 0;
    int duration_mismatch = 0;
    UNUSED(opendcp);

    /* change reel to 1 based for user */
    reel_number += 1;

    OPENDCP_LOG(LOG_DEBUG, "validate_reel: validating reel %d", reel_number);

    /* check if reel has a picture track */
    if (reel->main_picture.essence_class == ACT_PICTURE) {
        picture++;
    }

    if (picture < 1) {
        OPENDCP_LOG(LOG_ERROR, "Reel %d has no picture track", reel_number);
        return OPENDCP_NO_PICTURE_TRACK;
    } else if (picture > 1) {
        OPENDCP_LOG(LOG_ERROR, "Reel %d has multiple picture tracks", reel_number);
        return OPENDCP_MULTIPLE_PICTURE_TRACK;
    }

    d = reel->main_picture.duration;

    /* check durations */
    if (reel->main_sound.duration && reel->main_sound.duration != d) {
        duration_mismatch = 1;

        if (reel->main_sound.duration < d) {
            d = reel->main_sound.duration;
        }
    }

    if (reel->main_subtitle.duration && reel->main_subtitle.duration != d) {
        duration_mismatch = 1;

        if (reel->main_subtitle.duration < d) {
            d = reel->main_subtitle.duration;
        }
    }

    if (duration_mismatch) {
        reel->main_picture.duration = d;
        reel->main_sound.duration = d;
        reel->main_subtitle.duration = d;
        OPENDCP_LOG(LOG_WARN, "Asset duration mismatch, adjusting all durations to shortest asset duration of %d frames", d);
    }

    return OPENDCP_NO_ERROR;
}

void add_reel_to_cpl(cpl_t *cpl, reel_t reel) {
    memcpy(&cpl->reel[cpl->reel_count], &reel, sizeof(reel_t));
    cpl->reel_count++;
}

int add_asset(opendcp_t *opendcp, asset_t *asset, char *filename) {
    struct stat st;
    FILE   *fp;
    int    result;

    OPENDCP_LOG(LOG_INFO, "Adding asset %s", filename);

    init_asset(asset);

    /* check if file exists */
    if ((fp = fopen(filename, "r")) == NULL) {
        OPENDCP_LOG(LOG_ERROR, "add_asset: Could not open file: %s", filename);
        return OPENDCP_FILEOPEN;
    } else {
        fclose (fp);
    }

    sprintf(asset->filename, "%s", filename);
    sprintf(asset->annotation, "%s", basename(filename));

    /* get file size */
    stat(filename, &st);
    sprintf(asset->size, "%"PRIu64, st.st_size);

    /* read asset information */
    OPENDCP_LOG(LOG_DEBUG, "add_asset: Reading %s asset information", filename);

    result = read_asset_info(asset);

    if (result == OPENDCP_ERROR) {
        OPENDCP_LOG(LOG_ERROR, "%s is not a proper essence file", filename);
        return OPENDCP_INVALID_TRACK_TYPE;
    }

    /* force aspect ratio, if specified */
    if (strcmp(opendcp->dcp.aspect_ratio, "") ) {
        sprintf(asset->aspect_ratio, "%s", opendcp->dcp.aspect_ratio);
    }

    /* Set duration, if specified */
    if (opendcp->duration) {
        if  (opendcp->duration < asset->duration) {
            asset->duration = opendcp->duration;
        } else {
            OPENDCP_LOG(LOG_WARN, "Desired duration %d cannot be greater than assset duration %d, ignoring value", opendcp->duration, asset->duration);
        }
    }

    /* Set entry point, if specified */
    if (opendcp->entry_point) {
        if (opendcp->entry_point < asset->duration) {
            asset->entry_point = opendcp->entry_point;
        } else {
            OPENDCP_LOG(LOG_WARN, "Desired entry point %d cannot be greater than assset duration %d, ignoring value", opendcp->entry_point, asset->duration);
        }
    }

    /* calculate digest */
    // calculate_digest(opendcp, filename, asset->digest);

    return OPENDCP_NO_ERROR;
}

int add_asset_to_reel(opendcp_t *opendcp, reel_t *reel, asset_t asset) {
    int result;

    OPENDCP_LOG(LOG_INFO, "Adding asset to reel");

    if (opendcp->ns == XML_NS_UNKNOWN) {
        opendcp->ns = asset.xml_ns;
        OPENDCP_LOG(LOG_DEBUG, "add_asset_to_reel: Label type detected: %d", opendcp->ns);
    } else {
        if (opendcp->ns != asset.xml_ns) {
            OPENDCP_LOG(LOG_ERROR, "Warning DCP specification mismatch in assets. Please make sure all assets are MXF Interop or SMPTE");
            return OPENDCP_SPECIFICATION_MISMATCH;
        }
    }

    result = get_asset_type(asset);

    switch (result) {
        case ACT_PICTURE:
            OPENDCP_LOG(LOG_DEBUG, "add_asset_to_reel: adding picture");
            reel->main_picture = asset;
            break;

        case ACT_SOUND:
            OPENDCP_LOG(LOG_DEBUG, "add_asset_to_reel: adding sound");
            reel->main_sound = asset;
            break;

        case ACT_TIMED_TEXT:
            OPENDCP_LOG(LOG_DEBUG, "add_asset_to_reel: adding subtitle");
            reel->main_subtitle = asset;
            break;

        default:
            return OPENDCP_ERROR;
    }

    return OPENDCP_NO_ERROR;
}
