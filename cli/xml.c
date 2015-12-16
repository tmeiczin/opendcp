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

#include "opendcp.h"

void progress_bar();

typedef struct {
    char filename[FILENAME_MAX];
} asset_list_t;

typedef struct {
    int          asset_count;
    asset_list_t asset_list[3];
} reel_list_t;

void version() {
    FILE *fp;

    fp = stdout;
    fprintf(fp, "\n%s version %s %s\n\n", OPENDCP_NAME, OPENDCP_VERSION, OPENDCP_COPYRIGHT);

    exit(0);
}

void dcp_usage() {
    FILE *fp;
    fp = stdout;

    fprintf(fp, "\n%s version %s %s\n\n", OPENDCP_NAME, OPENDCP_VERSION, OPENDCP_COPYRIGHT);
    fprintf(fp, "Usage:\n");
    fprintf(fp, "       opendcp_xml --reel <mxf mxf mxf> [options]\n\n");
    fprintf(fp, "Ex:\n");
    fprintf(fp, "       opendcp_xml --reel picture.mxf sound.mxf\n");
    fprintf(fp, "       opendcp_xml --reel picture.mxf sound.mxf --reel picture.mxf sound.mxf\n");
    fprintf(fp, "       opendcp_xml --reel picture.mxf sound.mxf subtitle.mxf --reel picture.mxf sound.mxf\n");
    fprintf(fp, "       opendcp_xml --reel picture.mxf sound.mxf --digest OpenDCP --kind trailer\n");
    fprintf(fp, "\n");
    fprintf(fp, "Required: At least 1 reel is required:\n");
    fprintf(fp, "       -r | --reel <mxf mxf mxf>      - Creates a reel of MXF elements. The first --reel is reel 1, second --reel is reel 2, etc.\n");
    fprintf(fp, "                                        The argument is a space separated list of the essence elemements.\n");
    fprintf(fp, "                                        Picture/Sound/Subtitle (order of the mxf files in the list doesn't matter)\n");
    fprintf(fp, "                                        *** a picture mxf is required per reel ***\n");
    fprintf(fp, "Options:\n");
    fprintf(fp, "       -h | --help                    - Show help\n");
    fprintf(fp, "       -v | --version                 - Show version\n");
    fprintf(fp, "       -d | --digest                  - Generates digest (used to validate DCP asset integrity)\n");
#ifdef XMLSEC
    fprintf(fp, "       -s | --sign                    - Writes XML digital signature\n");
    fprintf(fp, "       -1 | --root                    - Root pem certificate used to sign XML files\n");
    fprintf(fp, "       -2 | --ca                      - CA (intermediate) pem certificate used to sign XML files\n");
    fprintf(fp, "       -3 | --signer                  - Signer (leaf) pem certificate used to sign XML files\n");
    fprintf(fp, "       -p | --privatekey              - Private (signer) pem key used to sign XML files\n");
#endif
    fprintf(fp, "       -i | --issuer <issuer>         - Issuer details\n");
    fprintf(fp, "       -a | --annotation <annotation> - Asset annotations\n");
    fprintf(fp, "       -t | --title <title>           - DCP content title\n");
    fprintf(fp, "       -b | --base <basename>         - Prepend CPL/PKL filenames with basename rather than UUID\n");
    fprintf(fp, "       -n | --duration <duration>     - Set asset durations in frames\n");
    fprintf(fp, "       -m | --rating <rating>         - Set DCP MPAA rating G PG PG-13 R NC-17 (default none)\n");
    fprintf(fp, "       -e | --entry <entry point>     - Set asset entry point (offset) frame\n");
    fprintf(fp, "       -k | --kind <kind>             - Content kind (test, feature, trailer, policy, teaser, etc)\n");
    fprintf(fp, "       -x | --width                   - Force aspect width (overrides detect value)\n");
    fprintf(fp, "       -y | --height                  - Force aspect height (overrides detected value)\n");
    fprintf(fp, "       -l | --log_level <level>       - Set the log level 0:Quiet, 1:Error, 2:Warn (default),  3:Info, 4:Debug\n");
    fprintf(fp, "\n\n");

    fclose(fp);
    exit(0);
}

/* gross globals, but... quicker than implementing callback arguments */
int  total = 0;
int  val   = 0;
char progress_string[80];
int  read_size = 16384;

int sha1_update_done_cb(void *p) {
    val++;
    UNUSED(p);
    progress_bar();

    return 0;
}

int sha1_digest_done_cb(void *p) {
    UNUSED(p);
    printf("\n  Digest Complete\n");

    return 0;
}

void progress_bar() {
    int x;
    int step = 20;
    float c = (float)step / total * (float)val;

    printf("%-50s [", progress_string);

    for (x = 0; x < step; x++) {
        if (c > x) {
            printf("=");
        }
        else {
            printf(" ");
        }
    }

    printf("] 100%%\r");
    fflush(stdout);
}

int main (int argc, char **argv) {
    int c, j;
    int reel_count = 0;
    int height = 0;
    int width  = 0;
    char buffer[80];
    opendcp_t *opendcp;
    reel_list_t reel_list[MAX_REELS];

    if ( argc <= 1 ) {
        dcp_usage();
    }

    opendcp = opendcp_create();

    /* parse options */
    while (1)
    {
        static struct option long_options[] =
        {
            {"annotation",     required_argument, 0, 'a'},
            {"base",           required_argument, 0, 'b'},
            {"digest",         no_argument,       0, 'd'},
            {"duration",       required_argument, 0, 'n'},
            {"entry",          required_argument, 0, 'e'},
            {"help",           no_argument,       0, 'h'},
            {"issuer",         required_argument, 0, 'i'},
            {"kind",           required_argument, 0, 'k'},
            {"log_level",      required_argument, 0, 'l'},
            {"rating",         required_argument, 0, 'm'},
            {"reel",           required_argument, 0, 'r'},
            {"title",          required_argument, 0, 't'},
            {"root",           required_argument, 0, '1'},
            {"ca",             required_argument, 0, '2'},
            {"signer",         required_argument, 0, '3'},
            {"privatekey",     required_argument, 0, 'p'},
            {"sign",           no_argument,       0, 's'},
            {"height",         required_argument, 0, 'y'},
            {"width",          required_argument, 0, 'x'},
            {"version",        no_argument,       0, 'v'},
            {0, 0, 0, 0}
        };

        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "a:b:e:svdhi:k:r:l:m:n:t:x:y:p:1:2:3:",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
        { break; }

        switch (c)
        {
            case 0:

                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                { break; }

                break;

            case 'a':
                sprintf(opendcp->dcp.annotation, "%.128s", optarg);
                break;

            case 'b':
                sprintf(opendcp->dcp.basename, "%.80s", optarg);
                break;

            case 'd':
                opendcp->dcp.digest_flag = 1;
                break;

            case 'e':
                opendcp->entry_point = atoi(optarg);
                break;

            case 'h':
                dcp_usage();
                break;

            case 'i':
                sprintf(opendcp->dcp.issuer, "%.80s", optarg);
                break;

            case 'k':
                sprintf(opendcp->dcp.kind, "%.15s", optarg);
                break;

            case 'l':
                opendcp->log_level = atoi(optarg);
                break;

            case 'm':
                if ( !strcmp(optarg, "G")
                        || !strcmp(optarg, "PG")
                        || !strcmp(optarg, "PG-13")
                        || !strcmp(optarg, "R")
                        || !strcmp(optarg, "NC-17") ) {
                    sprintf(opendcp->dcp.rating, "%.5s", optarg);
                }
                else {
                    sprintf(buffer, "Invalid rating %s\n", optarg);
                    dcp_fatal(opendcp, buffer);
                }

                break;

            case 'n':
                opendcp->duration = atoi(optarg);
                break;

            case 'r':
                j = 0;
                optind--;

                while ( optind < argc && strncmp("-", argv[optind], 1) != 0) {
                    sprintf(reel_list[reel_count].asset_list[j++].filename, "%s", argv[optind++]);
                }

                reel_list[reel_count++].asset_count = j--;
                break;

#ifdef XMLSEC

            case 's':
                opendcp->xml_signature.sign = 1;
                break;
#endif

            case 't':
                sprintf(opendcp->dcp.title, "%.80s", optarg);
                break;

            case 'x':
                width = atoi(optarg);
                break;

            case 'y':
                height = atoi(optarg);
                break;

            case '1':
                opendcp->xml_signature.root = optarg;
                opendcp->xml_signature.use_external = 1;
                break;

            case '2':
                opendcp->xml_signature.ca = optarg;
                opendcp->xml_signature.use_external = 1;
                break;

            case '3':
                opendcp->xml_signature.signer = optarg;
                opendcp->xml_signature.use_external = 1;
                break;

            case 'p':
                opendcp->xml_signature.private_key = optarg;
                opendcp->xml_signature.use_external = 1;
                break;

            case 'v':
                version();
                break;

            default:
                dcp_usage();
        }
    }

    opendcp_log_init(opendcp->log_level);

    if (opendcp->log_level > 0) {
        printf("\nOpenDCP XML %s %s\n", OPENDCP_VERSION, OPENDCP_COPYRIGHT);
    }

    if (reel_count < 1) {
        dcp_fatal(opendcp, "No reels supplied");
    }

    /* check cert files */
    if (opendcp->xml_signature.sign && opendcp->xml_signature.use_external == 1) {
        FILE *tp;

        if (opendcp->xml_signature.root) {
            tp = fopen(opendcp->xml_signature.root, "rb");

            if (tp) {
                fclose(tp);
            }
            else {
                dcp_fatal(opendcp, "Could not read root certificate");
            }
        }
        else {
            dcp_fatal(opendcp, "XML digital signature certifcates enabled, but root certificate file not specified");
        }

        if (opendcp->xml_signature.ca) {
            tp = fopen(opendcp->xml_signature.ca, "rb");

            if (tp) {
                fclose(tp);
            }
            else {
                dcp_fatal(opendcp, "Could not read ca certificate");
            }
        }
        else {
            dcp_fatal(opendcp, "XML digital signature certifcates enabled, but ca certificate file not specified");
        }

        if (opendcp->xml_signature.signer) {
            tp = fopen(opendcp->xml_signature.signer, "rb");

            if (tp) {
                fclose(tp);
            }
            else {
                dcp_fatal(opendcp, "Could not read signer certificate");
            }
        }
        else {
            dcp_fatal(opendcp, "XML digital signature certifcates enabled, but signer certificate file not specified");
        }

        if (opendcp->xml_signature.private_key) {
            tp = fopen(opendcp->xml_signature.private_key, "rb");

            if (tp) {
                fclose(tp);
            }
            else {
                dcp_fatal(opendcp, "Could not read private key file");
            }
        }
        else {
            dcp_fatal(opendcp, "XML digital signature certifcates enabled, but private key file not specified");
        }
    }

    /* set aspect ratio override */
    if (width || height) {
        if (!height) {
            dcp_fatal(opendcp, "You must specify height, if you specify width");
        }

        if (!width) {
            dcp_fatal(opendcp, "You must specify widht, if you specify height");
        }

        sprintf(opendcp->dcp.aspect_ratio, "%d %d", width, height);
    }

    /* add pkl to the DCP (only one PKL currently support) */
    pkl_t pkl;
    create_pkl(opendcp->dcp, &pkl);
    add_pkl_to_dcp(&opendcp->dcp, pkl);

    /* add cpl to the DCP/PKL (only one CPL currently support) */
    cpl_t cpl;
    create_cpl(opendcp->dcp, &cpl);
    add_cpl_to_pkl(&opendcp->dcp.pkl[0], cpl);

    /* set the callbacks (optional) for the digest generator */
    if (opendcp->log_level > 0 && opendcp->log_level < 3) {
        opendcp->dcp.sha1_update.callback = sha1_update_done_cb;
    }

    /* Add and validate reels */
    for (c = 0; c < reel_count; c++) {
        int a;
        reel_t reel;
        create_reel(opendcp->dcp, &reel);

        for (a = 0; a < reel_list[c].asset_count; a++) {
            val   = 0;
            asset_t asset;
            add_asset(opendcp, &asset, reel_list[c].asset_list[a].filename);
            sprintf(progress_string, "%-.25s %.25s", asset.filename, "Digest Calculation");
            total = atoi(asset.size) / read_size;

            if (opendcp->log_level > 0 && opendcp->log_level < 3) {
                printf("\n");
                progress_bar();
            }

            calculate_digest(opendcp, asset.filename, asset.digest);
            add_asset_to_reel(opendcp, &reel, asset);
        }

        if (validate_reel(opendcp, &reel, c) == OPENDCP_NO_ERROR) {
            add_reel_to_cpl(&opendcp->dcp.pkl[0].cpl[0], reel);
        }
        else {
            sprintf(buffer, "Could not validate reel %d\n", c + 1);
            dcp_fatal(opendcp, buffer);
        }
    }

    /* set ASSETMAP/VOLINDEX path */
    if (opendcp->ns == XML_NS_SMPTE) {
        sprintf(opendcp->dcp.assetmap.filename, "%s", "ASSETMAP.xml");
        sprintf(opendcp->dcp.volindex.filename, "%s", "VOLINDEX.xml");
    }
    else {
        sprintf(opendcp->dcp.assetmap.filename, "%s", "ASSETMAP");
        sprintf(opendcp->dcp.volindex.filename, "%s", "VOLINDEX");
    }

    /* Write CPL File */
    if (opendcp->log_level > 0 && opendcp->log_level < 3) {
        printf("\n");
        sprintf(progress_string, "%-.50s", opendcp->dcp.pkl[0].cpl[0].filename);
        progress_bar();
    }

    if (write_cpl(opendcp, &opendcp->dcp.pkl[0].cpl[0]) != OPENDCP_NO_ERROR)
    { dcp_fatal(opendcp, "Writing composition playlist failed"); }

    /* Write PKL File */
    if (opendcp->log_level > 0 && opendcp->log_level < 3) {
        printf("\n");
        sprintf(progress_string, "%-.50s", opendcp->dcp.pkl[0].filename);
        progress_bar();
    }

    if (write_pkl(opendcp, &opendcp->dcp.pkl[0]) != OPENDCP_NO_ERROR)
    { dcp_fatal(opendcp, "Writing packing list failed"); }

    if (opendcp->log_level > 0 && opendcp->log_level < 3) {
        printf("\n");
        sprintf(progress_string, "%-.50s", opendcp->dcp.assetmap.filename);
        progress_bar();
    }

    if (write_volumeindex(opendcp) != OPENDCP_NO_ERROR)
    { dcp_fatal(opendcp, "Writing volume index failed"); }

    if (opendcp->log_level > 0 && opendcp->log_level < 3) {
        printf("\n");
        sprintf(progress_string, "%-.50s", opendcp->dcp.volindex.filename);
        progress_bar();
    }

    if (write_assetmap(opendcp) != OPENDCP_NO_ERROR)
    { dcp_fatal(opendcp, "Writing asset map failed"); }

    OPENDCP_LOG(LOG_INFO, "DCP Complete");

    opendcp_delete(opendcp);

    exit(0);
}
