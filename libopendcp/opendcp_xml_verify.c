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

#define XMLSEC_CRYPTO_OPENSSL

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#ifndef XMLSEC_NO_XSLT
#include <libxslt/xslt.h>
#include <libxslt/extensions.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>
#include <libexslt/exslt.h>
#endif

#include <xmlsec/xmlsec.h>
#include <xmlsec/xmltree.h>
#include <xmlsec/xmldsig.h>
#include <xmlsec/templates.h>
#include <xmlsec/crypto.h>

#include "opendcp.h"
#include "opendcp_certificates.h"

int xmlsec_verify_init() {
    /* init libxml lib */
    xmlInitParser();
    xmlIndentTreeOutput = 1;
    LIBXML_TEST_VERSION
    xmlLoadExtDtdDefaultValue = XML_DETECT_IDS | XML_COMPLETE_ATTRS;
    xmlSubstituteEntitiesDefault(1);

    /* init xmlsec lib */
    if (xmlSecInit() < 0) {
        OPENDCP_LOG(DCP_FATAL, "xmlsec initialization failed");
        return(DCP_FATAL);
    }

    /* Check loaded library version */
    if (xmlSecCheckVersion() != 1) {
        OPENDCP_LOG(DCP_FATAL, "loaded xmlsec library version is not compatible");
        return(DCP_FATAL);
    }

    /* Init crypto library */
    if (xmlSecCryptoAppInit(NULL) < 0) {
        OPENDCP_LOG(DCP_FATAL, "crypto initialization failed");
        return(DCP_FATAL);
    }

    /* Init xmlsec-crypto library */
    if (xmlSecCryptoInit() < 0) {
        OPENDCP_LOG(DCP_FATAL, "xmlsec-crypto initialization failed");
        return(DCP_FATAL);
    }

    return(DCP_SUCCESS);
}

xmlSecKeysMngrPtr load_certificates() {
    xmlSecKeysMngrPtr key_manager;
    xmlSecKeyPtr      key;

    /* create and initialize keys manager */
    key_manager = xmlSecKeysMngrCreate();

    if (key_manager == NULL) {
        return(NULL);
    }

    if (xmlSecCryptoAppDefaultKeysMngrInit(key_manager) < 0) {
        xmlSecKeysMngrDestroy(key_manager);
        return(NULL);
    }

    return(key_manager);
}

int xmlsec_close() {
    /* Shutdown xmlsec-crypto library */
    xmlSecCryptoShutdown();

    /* Shutdown crypto library */
    xmlSecCryptoAppShutdown();

    /* Shutdown xmlsec library */
    xmlSecShutdown();

    /* Shutdown libxslt/libxml */
    xmlCleanupParser();

    return(DCP_SUCCESS);
}

int xml_verify(char *filename) {
    xmlSecDSigCtxPtr dsig_ctx = NULL;
    xmlDocPtr        doc = NULL;
    xmlNodePtr       root_node;
    xmlNodePtr       sign_node;
    xmlNodePtr       cert_node;
    xmlNodePtr       x509d_node;
    xmlNodePtr       cur_node;
    int              result = DCP_FATAL;
    xmlSecKeysMngrPtr key_manager;
    char cert[5000];
    int  cert_l;
    xmlsec_verify_init();

    /* load doc file */
    doc = xmlParseFile(filename);

    if (doc == NULL) {
        OPENDCP_LOG(LOG_ERROR, "unable to parse file %s", filename);
        goto done;
    }

    /* find root node */
    root_node = xmlDocGetRootElement(doc);

    if (root_node == NULL) {
        OPENDCP_LOG(LOG_ERROR, "unable to find root node");
        goto done;
    }

    /* find signature node */
    sign_node = xmlSecFindNode(root_node, xmlSecNodeSignature, xmlSecDSigNs);

    if (sign_node == NULL) {
        OPENDCP_LOG(LOG_ERROR, "signature node not found");
        goto done;
    }

    /* create keys manager */
    key_manager = load_certificates();

    if (key_manager == NULL) {
        OPENDCP_LOG(LOG_ERROR, "create key manager failed");
        goto done;
    }

    /* find certificates */
    cur_node = sign_node;

    while (x509d_node = xmlSecFindNode(cur_node, xmlSecNodeX509Data, xmlSecDSigNs)) {
        cert_node = xmlSecFindNode(x509d_node, xmlSecNodeX509Certificate, xmlSecDSigNs);

        if (cert_node == NULL) {
            OPENDCP_LOG(LOG_ERROR, "X509certficate node not found");
            goto done;
        }

        sprintf(cert, "-----BEGIN CERTIFICATE-----\n%s\n-----END CERTIFICATE-----\n", xmlNodeGetContent(cert_node));
        cert_l = strlen(cert);

        if (xmlSecCryptoAppKeysMngrCertLoadMemory(key_manager, cert, cert_l, xmlSecKeyDataFormatPem, xmlSecKeyDataTypeTrusted) < 0) {
            OPENDCP_LOG(LOG_ERROR, "could read X509certificate node value");
            goto done;
        }

        cur_node = xmlNextElementSibling(x509d_node);
    }

    /* create signature context */
    dsig_ctx = xmlSecDSigCtxCreate(key_manager);

    if (dsig_ctx == NULL) {
        OPENDCP_LOG(LOG_ERROR, "create signature opendcp failed");
        goto done;
    }

    /* sign the template */
    if (xmlSecDSigCtxVerify(dsig_ctx, sign_node) < 0) {
        OPENDCP_LOG(LOG_ERROR, "signature verify failed");
        goto done;
    }

    if (dsig_ctx->status != xmlSecDSigStatusSucceeded) {
        OPENDCP_LOG(LOG_ERROR, "signature validation failed");
        goto done;
    }

    /* success */
    result = 0;

done:
    /* destroy keys manager */
    xmlSecKeysMngrDestroy(key_manager);

    /* destroy signature context */
    if (dsig_ctx != NULL) {
        xmlSecDSigCtxDestroy(dsig_ctx);
    }

    /* destroy xml doc */
    if (doc != NULL) {
        xmlFreeDoc(doc);
    }

    xmlsec_close();
    return(result);
}
