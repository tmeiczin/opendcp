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
#include <time.h>
#include <inttypes.h>
#include <sys/stat.h>
#include "opendcp.h"
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#define XML_ENCODING "UTF-8"

extern int write_dsig_template(opendcp_t *opendcp, xmlTextWriterPtr xml);

#ifdef WIN32
char *strsep (char **stringp, const char *delim) {
    register char *s;
    register const char *spanp;
    register int c, sc;
    char *tok;

    if ((s = *stringp) == NULL)
        return (NULL);

    for (tok = s;;) {
        c = *s++;
        spanp = delim;

        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;

                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
}
#endif

char *get_aspect_ratio(char *dimension_string) {
    char *p, *ratio;
    int n, d;
    float a = 0.0;

    ratio = malloc(sizeof(char) * 5);
    p = malloc(strlen(dimension_string) + 1);
    strcpy(p, dimension_string);
    n = atoi(strsep(&p, " "));
    d = atoi(strsep(&p, " "));

    if (d > 0) {
        a = (n * 1.00) / (d * 1.00);
    }

    if ( a >= 1.77 && a <= 1.78) {
        a = 1.77;
    }

    sprintf(ratio, "%-3.2f", a);

    return(ratio);
}

int is_valid_asset(asset_t asset) {
    if (asset.essence_class == ACT_PICTURE ||
            asset.essence_class == ACT_SOUND ||
            asset.essence_class == ACT_TIMED_TEXT ) {

        return 1;
    }

    return 0;
}

int write_cpl_asset(opendcp_t *opendcp, xmlTextWriterPtr xml, asset_t asset) {
    if (!is_valid_asset(asset)) {
        return OPENDCP_NO_ERROR;
    }

    if (asset.essence_class == ACT_PICTURE) {
        if (asset.stereoscopic) {
            xmlTextWriterStartElement(xml, BAD_CAST "msp-cpl:MainStereoscopicPicture");
            xmlTextWriterWriteAttribute(xml, BAD_CAST "xmlns:msp-cpl", BAD_CAST NS_CPL_3D[opendcp->ns]);
        } else {
            xmlTextWriterStartElement(xml, BAD_CAST "MainPicture");
        }
    } else if (asset.essence_class == ACT_SOUND) {
        xmlTextWriterStartElement(xml, BAD_CAST "MainSound");
    } else if (asset.essence_class == ACT_TIMED_TEXT) {
        xmlTextWriterStartElement(xml, BAD_CAST "MainSubtitle");
    } else {
        return OPENDCP_NO_ERROR;
    }

    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id", "%s%s", "urn:uuid:", asset.uuid);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "AnnotationText", "%s", asset.annotation);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "EditRate", "%s", asset.edit_rate);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "IntrinsicDuration", "%d", asset.intrinsic_duration);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "EntryPoint", "%d", asset.entry_point);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Duration", "%d", asset.duration);
    if ( opendcp->dcp.digest_flag ) {
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Hash", "%s", asset.digest);
    }

    if (asset.essence_class == ACT_PICTURE) {
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "FrameRate", "%s", asset.frame_rate);

        if (opendcp->ns == XML_NS_SMPTE) {
            xmlTextWriterWriteFormatElement(xml, BAD_CAST "ScreenAspectRatio", "%s", asset.aspect_ratio);
        } else {
            xmlTextWriterWriteFormatElement(xml, BAD_CAST "ScreenAspectRatio", "%s", get_aspect_ratio(asset.aspect_ratio));
        }
    }

    xmlTextWriterEndElement(xml); /* end asset */

    return OPENDCP_NO_ERROR;
}

int write_pkl_asset(opendcp_t *opendcp, xmlTextWriterPtr xml, asset_t asset) {
    if (!is_valid_asset(asset)) {
        return OPENDCP_NO_ERROR;
    }

    xmlTextWriterStartElement(xml, BAD_CAST "Asset");
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id", "%s%s", "urn:uuid:", asset.uuid);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "AnnotationText", "%s", asset.annotation);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Hash", "%s", asset.digest);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Size", "%s", asset.size);

    if (opendcp->ns == XML_NS_SMPTE) {
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Type", "%s", "application/mxf");
    } else {
        if (asset.essence_class == ACT_PICTURE) {
            xmlTextWriterWriteFormatElement(xml, BAD_CAST "Type", "%s", "application/x-smpte-mxf;asdcpKind=Picture");
        } else if (asset.essence_class == ACT_SOUND) {
            xmlTextWriterWriteFormatElement(xml, BAD_CAST "Type", "%s", "application/x-smpte-mxf;asdcpKind=Sound");
        } else if (asset.essence_class == ACT_TIMED_TEXT) {
            xmlTextWriterWriteFormatElement(xml, BAD_CAST "Type", "%s", "application/x-smpte-mxf;asdcpKind=Subtitle");
        } else {
            return OPENDCP_NO_ERROR;
        }
    }

    xmlTextWriterWriteFormatElement(xml, BAD_CAST "OriginalFileName", "%s", asset.filename);
    xmlTextWriterEndElement(xml);      /* end asset */

    return OPENDCP_NO_ERROR;
}

int write_assetmap_asset(xmlTextWriterPtr xml, asset_t asset) {
    if (!is_valid_asset(asset)) {
        return OPENDCP_NO_ERROR;
    }

    if (asset.uuid != NULL) {
        xmlTextWriterStartElement(xml, BAD_CAST "Asset");
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id", "%s%s", "urn:uuid:", asset.uuid);
        xmlTextWriterStartElement(xml, BAD_CAST "ChunkList");
        xmlTextWriterStartElement(xml, BAD_CAST "Chunk");
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Path", "%s", basename(asset.filename));
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "VolumeIndex", "%d", 1);
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Offset", "%d", 0);
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Length", "%s", asset.size);
        xmlTextWriterEndElement(xml); /* end chunk */
        xmlTextWriterEndElement(xml); /* end chunklist */
        xmlTextWriterEndElement(xml); /* end cpl asset */
    }

    return OPENDCP_NO_ERROR;
}

int write_cpl(opendcp_t *opendcp, cpl_t *cpl) {
    int r, rc;
    struct stat st;
    xmlIndentTreeOutput = 1;
    xmlDocPtr        doc;
    xmlTextWriterPtr xml;

    /* create XML document */
    xml = xmlNewTextWriterDoc(&doc, 0);

    /* cpl start */
    rc = xmlTextWriterStartDocument(xml, NULL, XML_ENCODING, NULL);

    if (rc < 0) {
        OPENDCP_LOG(LOG_ERROR, "xmlTextWriterStartDocument failed");
        return OPENDCP_ERROR;
    }

    xmlTextWriterStartElement(xml, BAD_CAST "CompositionPlaylist");
    xmlTextWriterWriteAttribute(xml, BAD_CAST "xmlns", BAD_CAST NS_CPL[opendcp->ns]);

    if (opendcp->xml_signature.sign) {
        xmlTextWriterWriteAttribute(xml, BAD_CAST "xmlns:dsig", BAD_CAST DS_DSIG);
    }

    /* cpl attributes */
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id", "%s%s", "urn:uuid:", cpl->uuid);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "AnnotationText", "%s", cpl->annotation);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "IssueDate", "%s", opendcp->dcp.timestamp);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Issuer", "%s", opendcp->dcp.issuer);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Creator", "%s", opendcp->dcp.creator);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "ContentTitleText", "%s", cpl->title);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "ContentKind", "%s", cpl->kind);

    /* content version */
    if (opendcp->ns == XML_NS_SMPTE) {
        xmlTextWriterStartElement(xml, BAD_CAST "ContentVersion");
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id", "%s%s_%s", "urn:uri:", cpl->uuid, cpl->timestamp);
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "LabelText", "%s_%s", cpl->uuid, cpl->timestamp);
        xmlTextWriterEndElement(xml);
    }

    /* rating */
    xmlTextWriterStartElement(xml, BAD_CAST "RatingList");

    if (strcmp(cpl->rating, "")) {
        xmlTextWriterStartElement(xml, BAD_CAST "Rating");
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Agency", "%s", RATING_AGENCY[1]);
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Label", "%s", cpl->rating);
        xmlTextWriterEndElement(xml);
    }

    xmlTextWriterEndElement(xml);

    /* reel(s) Start */
    xmlTextWriterStartElement(xml, BAD_CAST "ReelList");

    for (r = 0; r < cpl->reel_count; r++) {
        reel_t reel = cpl->reel[r];
        xmlTextWriterStartElement(xml, BAD_CAST "Reel");
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id", "%s%s", "urn:uuid:", reel.uuid);
        xmlTextWriterStartElement(xml, BAD_CAST "AssetList");

        /* write picture first, unless stereoscopic */
        if (cpl->reel[r].main_picture.stereoscopic) {
            write_cpl_asset(opendcp, xml, cpl->reel[r].main_sound);
            write_cpl_asset(opendcp, xml, cpl->reel[r].main_subtitle);
            write_cpl_asset(opendcp, xml, cpl->reel[r].main_picture);
        } else {
            write_cpl_asset(opendcp, xml, cpl->reel[r].main_picture);
            write_cpl_asset(opendcp, xml, cpl->reel[r].main_sound);
            write_cpl_asset(opendcp, xml, cpl->reel[r].main_subtitle);
        }

        xmlTextWriterEndElement(xml);     /* end assetlist */
        xmlTextWriterEndElement(xml);     /* end reel */
    }

    xmlTextWriterEndElement(xml);         /* end reel list */

#ifdef XMLSEC

    if (opendcp->xml_signature.sign) {
        write_dsig_template(opendcp, xml);
    }

#endif

    xmlTextWriterEndElement(xml);         /* end compositionplaylist */

    rc = xmlTextWriterEndDocument(xml);

    if (rc < 0) {
        OPENDCP_LOG(LOG_ERROR, "xmlTextWriterEndDocument failed %s", cpl->filename);
        return OPENDCP_ERROR;
    }

    xmlFreeTextWriter(xml);
    xmlSaveFormatFile(cpl->filename, doc, 1);
    xmlFreeDoc(doc);

#ifdef XMLSEC

    /* sign the XML file */
    if (opendcp->xml_signature.sign) {
        xml_sign(opendcp, cpl->filename);
    }

#endif

    /* store CPL file size */
    OPENDCP_LOG(LOG_INFO, "writing CPL file info");
    stat(cpl->filename, &st);
    sprintf(cpl->size, "%"PRIu64, st.st_size);
    calculate_digest(opendcp, cpl->filename, cpl->digest);

    return OPENDCP_NO_ERROR;
}

int write_cpl_list(opendcp_t *opendcp) {
    UNUSED(opendcp);
    int placeholder = 0;
    return placeholder;
}

int write_pkl_list(opendcp_t *opendcp) {
    UNUSED(opendcp);
    int placeholder = 0;
    return placeholder;
}

int write_pkl(opendcp_t *opendcp, pkl_t *pkl) {
    int r, c, rc;
    struct stat st;
    xmlIndentTreeOutput = 1;
    xmlDocPtr        doc;
    xmlTextWriterPtr xml;

    /* create XML document */
    xml = xmlNewTextWriterDoc(&doc, 0);

    /* pkl start */
    rc = xmlTextWriterStartDocument(xml, NULL, XML_ENCODING, NULL);

    if (rc < 0) {
        OPENDCP_LOG(LOG_ERROR, "xmlTextWriterStartDocument failed");
        return OPENDCP_ERROR;
    }

    xmlTextWriterStartElement(xml, BAD_CAST "PackingList");
    xmlTextWriterWriteAttribute(xml, BAD_CAST "xmlns", BAD_CAST NS_PKL[opendcp->ns]);

    if (opendcp->xml_signature.sign) {
        xmlTextWriterWriteAttribute(xml, BAD_CAST "xmlns:dsig", BAD_CAST DS_DSIG);
    }

    /* pkl attributes */
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id", "%s%s", "urn:uuid:", pkl->uuid);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "AnnotationText", "%s", pkl->annotation);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "IssueDate", "%s", opendcp->dcp.timestamp);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Issuer", "%s", opendcp->dcp.issuer);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Creator", "%s", opendcp->dcp.creator);

    OPENDCP_LOG(LOG_INFO, "cpls: %d", pkl->cpl_count);

    /* asset(s) Start */
    xmlTextWriterStartElement(xml, BAD_CAST "AssetList");

    for (c = 0; c < pkl->cpl_count; c++) {
        cpl_t cpl = pkl->cpl[c];
        OPENDCP_LOG(LOG_INFO, "reels: %d", cpl.reel_count);

        for (r = 0; r < cpl.reel_count; r++) {
            write_pkl_asset(opendcp, xml, cpl.reel[r].main_picture);
            write_pkl_asset(opendcp, xml, cpl.reel[r].main_sound);
            write_pkl_asset(opendcp, xml, cpl.reel[r].main_subtitle);
        }

        /* cpl */
        xmlTextWriterStartElement(xml, BAD_CAST "Asset");
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id", "%s%s", "urn:uuid:", cpl.uuid);
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Hash", "%s", cpl.digest);
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Size", "%s", cpl.size);

        if (opendcp->ns == XML_NS_SMPTE) {
            xmlTextWriterWriteFormatElement(xml, BAD_CAST "Type", "%s", "text/xml");
        } else {
            xmlTextWriterWriteFormatElement(xml, BAD_CAST "Type", "%s", "text/xml;asdcpKind=CPL");
        }

        xmlTextWriterWriteFormatElement(xml, BAD_CAST "OriginalFileName", "%s", cpl.filename);
        xmlTextWriterEndElement(xml);      /* end cpl asset */
    }

    xmlTextWriterEndElement(xml);      /* end assetlist */

#ifdef XMLSEC

    if (opendcp->xml_signature.sign) {
        write_dsig_template(opendcp, xml);
    }

#endif

    xmlTextWriterEndElement(xml);      /* end packinglist */

    rc = xmlTextWriterEndDocument(xml);

    if (rc < 0) {
        OPENDCP_LOG(LOG_ERROR, "xmlTextWriterEndDocument failed %s", pkl->filename);
        return OPENDCP_ERROR;
    }

    xmlFreeTextWriter(xml);
    xmlSaveFormatFile(pkl->filename, doc, 1);
    xmlFreeDoc(doc);

#ifdef XMLSEC

    /* sign the XML file */
    if (opendcp->xml_signature.sign) {
        xml_sign(opendcp, pkl->filename);
    }

#endif

    /* store PKL file size */
    stat(pkl->filename, &st);
    sprintf(pkl->size, "%"PRIu64, st.st_size);

    return OPENDCP_NO_ERROR;
}

int write_assetmap(opendcp_t *opendcp) {
    xmlIndentTreeOutput = 1;
    xmlDocPtr        doc;
    xmlTextWriterPtr xml;
    int              c, r, rc;
    char             uuid_s[40];
    reel_t           reel;

    assetmap_t assetmap = opendcp->dcp.assetmap;

    /* generate assetmap UUID */
    uuid_random(uuid_s);

    OPENDCP_LOG(LOG_INFO, "writing ASSETMAP file %.256s", assetmap.filename);

    /* create XML document */
    xml = xmlNewTextWriterDoc(&doc, 0);

    /* assetmap XML Start */
    rc = xmlTextWriterStartDocument(xml, NULL, XML_ENCODING, NULL);

    if (rc < 0) {
        OPENDCP_LOG(LOG_ERROR, "xmlTextWriterStartDocument failed");
        return OPENDCP_ERROR;
    }

    xmlTextWriterStartElement(xml, BAD_CAST "AssetMap");
    xmlTextWriterWriteAttribute(xml, BAD_CAST "xmlns", BAD_CAST NS_AM[opendcp->ns]);

    /* assetmap attributes */
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id", "%s%s", "urn:uuid:", uuid_s);

    if (opendcp->ns == XML_NS_SMPTE) {
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Creator", "%s", opendcp->dcp.creator);
    }

    xmlTextWriterWriteFormatElement(xml, BAD_CAST "VolumeCount", "%d", 1);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "IssueDate", "%s", opendcp->dcp.timestamp);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Issuer", "%s", opendcp->dcp.issuer);

    if (opendcp->ns == XML_NS_INTEROP) {
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Creator", "%s", opendcp->dcp.creator);
    }

    xmlTextWriterStartElement(xml, BAD_CAST "AssetList");

    OPENDCP_LOG(LOG_INFO, "writing ASSETMAP PKL");

    /* PKL */
    pkl_t pkl = opendcp->dcp.pkl[0];
    xmlTextWriterStartElement(xml, BAD_CAST "Asset");
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id", "%s%s", "urn:uuid:", pkl.uuid);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "PackingList", "%s", "true");
    xmlTextWriterStartElement(xml, BAD_CAST "ChunkList");
    xmlTextWriterStartElement(xml, BAD_CAST "Chunk");
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Path", "%s", basename(pkl.filename));
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "VolumeIndex", "%d", 1);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Offset", "%d", 0);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Length", "%s", pkl.size);
    xmlTextWriterEndElement(xml); /* end chunk */
    xmlTextWriterEndElement(xml); /* end chunklist */
    xmlTextWriterEndElement(xml); /* end pkl asset */

    OPENDCP_LOG(LOG_INFO, "writing ASSETMAP CPLs");

    /* CPL */
    for (c = 0; c < pkl.cpl_count; c++) {
        cpl_t cpl = pkl.cpl[c];
        xmlTextWriterStartElement(xml, BAD_CAST "Asset");
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Id", "%s%s", "urn:uuid:", cpl.uuid);
        xmlTextWriterStartElement(xml, BAD_CAST "ChunkList");
        xmlTextWriterStartElement(xml, BAD_CAST "Chunk");
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Path", "%s", basename(cpl.filename));
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "VolumeIndex", "%d", 1);
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Offset", "%d", 0);
        xmlTextWriterWriteFormatElement(xml, BAD_CAST "Length", "%s", cpl.size);
        xmlTextWriterEndElement(xml); /* end chunk */
        xmlTextWriterEndElement(xml); /* end chunklist */
        xmlTextWriterEndElement(xml); /* end cpl asset */

        /* assets(s) start */
        for (r = 0; r < cpl.reel_count; r++) {
            reel = cpl.reel[r];

            write_assetmap_asset(xml, reel.main_picture);
            write_assetmap_asset(xml, reel.main_sound);
            write_assetmap_asset(xml, reel.main_subtitle);
        }
    }

    xmlTextWriterEndElement(xml); /* end assetlist */
    xmlTextWriterEndElement(xml); /* end assetmap */

    rc = xmlTextWriterEndDocument(xml);

    if (rc < 0) {
        OPENDCP_LOG(LOG_ERROR, "xmlTextWriterEndDocument failed %s", opendcp->dcp.assetmap.filename);
        return OPENDCP_ERROR;
    }

    xmlFreeTextWriter(xml);
    xmlSaveFormatFile(assetmap.filename, doc, 1);
    xmlFreeDoc(doc);

    return OPENDCP_NO_ERROR;
}

int write_volumeindex(opendcp_t *opendcp) {
    xmlIndentTreeOutput = 1;
    xmlDocPtr        doc;
    xmlTextWriterPtr xml;
    int              rc;

    volindex_t volindex = opendcp->dcp.volindex;

    OPENDCP_LOG(LOG_INFO, "writing VOLINDEX file %.256s", volindex.filename);

    /* create XML document */
    xml = xmlNewTextWriterDoc(&doc, 0);

    /* volumeindex XML Start */
    rc = xmlTextWriterStartDocument(xml, NULL, XML_ENCODING, NULL);

    if (rc < 0) {
        OPENDCP_LOG(LOG_ERROR, "xmlTextWriterStartDocument failed");
        return OPENDCP_ERROR;
    }

    xmlTextWriterStartElement(xml, BAD_CAST "VolumeIndex");
    xmlTextWriterWriteAttribute(xml, BAD_CAST "xmlns", BAD_CAST NS_AM[opendcp->ns]);
    xmlTextWriterWriteFormatElement(xml, BAD_CAST "Index", "%d", 1);
    xmlTextWriterEndElement(xml);

    rc = xmlTextWriterEndDocument(xml);

    if (rc < 0) {
        OPENDCP_LOG(LOG_ERROR, "xmlTextWriterEndDocument failed %s", volindex.filename);
        return OPENDCP_ERROR;
    }

    xmlFreeTextWriter(xml);
    xmlSaveFormatFile(volindex.filename, doc, 1);
    xmlFreeDoc(doc);

    return OPENDCP_NO_ERROR;
}
