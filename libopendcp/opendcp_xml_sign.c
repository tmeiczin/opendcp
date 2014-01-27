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
#include <libxml/xmlwriter.h>

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

extern int write_dsig_template(opendcp_t *opendcp, xmlTextWriterPtr xml);
extern int xml_sign(opendcp_t *opendcp, char *filename);

char *dn_oneline(X509_NAME *xn) {
    BIO* bio;
    int n;
    char *result;
    unsigned long flags = XN_FLAG_RFC2253;

    if ((bio = BIO_new(BIO_s_mem())) == NULL) {
        return NULL;
    }

    X509_NAME_print_ex(bio, xn, 0, flags);
    n = BIO_pending(bio);
    result = (char *)malloc(sizeof(char *) * (n + 1));
    n = BIO_read(bio, result, n);
    result[n] = '\0';
    BIO_free(bio);

    return result;
}

char *strip_cert(const char *data) {
    int i, len;
    int offset = 28;
    char *buffer;

    len = strlen((char *)data) - 53;
    buffer = (char *)malloc(len);
    memset(buffer, 0, (len));

    for (i = 0; i < len - 2; i++) {
        buffer[i] = data[i + offset];
    }

    return buffer;
}

char *strip_cert_file(char *filename) {
    int i = 0;
    char text[5000];
    char *ptr;
    FILE *fp = fopen(filename, "rb");

    while (!feof(fp)) {
        text[i++] = fgetc(fp);
    }

    text[i - 1] = '\0';
    ptr = strip_cert(text);
    return(ptr);
}

int write_dsig_template(opendcp_t *opendcp, xmlTextWriterPtr xml) {
    BIO *bio[3];
    X509 *x[3];
    X509_NAME *issuer_xn[3];
    X509_NAME *subject_xn[3];
    char *cert[3];
    int i;

    OPENDCP_LOG(LOG_DEBUG, "xml_sign: write_dsig_template");

    if (opendcp->xml_signature.use_external) {
        /* read certificates from file */
        FILE *cp;

        cp = fopen(opendcp->xml_signature.signer, "rb");

        if (cp) {
            x[0] = PEM_read_X509(cp, NULL, NULL, NULL);
            fclose(cp);
        }

        cp = fopen(opendcp->xml_signature.ca, "rb");

        if (cp) {
            x[1] = PEM_read_X509(cp, NULL, NULL, NULL);
            fclose(cp);
        }

        cp = fopen(opendcp->xml_signature.root, "rb");

        if (cp) {
            x[2] = PEM_read_X509(cp, NULL, NULL, NULL);
            fclose(cp);
        }

        cert[0] = strip_cert_file(opendcp->xml_signature.signer);
        cert[1] = strip_cert_file(opendcp->xml_signature.ca);
        cert[2] = strip_cert_file(opendcp->xml_signature.root);
    } else {
        /* read certificate from memory */
        bio[0] = BIO_new_mem_buf((void *)opendcp_signer_cert, -1);
        bio[1] = BIO_new_mem_buf((void *)opendcp_ca_cert, -1);
        bio[2] = BIO_new_mem_buf((void *)opendcp_root_cert, -1);

        /* save a copy with the BEGIN/END stripped */
        cert[0] = strip_cert(opendcp_signer_cert);
        cert[1] = strip_cert(opendcp_ca_cert);
        cert[2] = strip_cert(opendcp_root_cert);

        for (i = 0; i < 3; i++) {
            if (bio[i] == NULL) {
                OPENDCP_LOG(LOG_ERROR, "could allocate certificate from memory");
                return OPENDCP_ERROR;
            }

            x[i] = PEM_read_bio_X509(bio[i], NULL, NULL, NULL);

            if (!BIO_set_close(bio[i], BIO_NOCLOSE)) {
                OPENDCP_LOG(LOG_ERROR, "could set BIO close flag");
                return OPENDCP_ERROR;
            }

            if (x[i] == NULL) {
                OPENDCP_LOG(LOG_ERROR, "could not read certificate");
                return OPENDCP_ERROR;
            }
        }
    }

    /* get issuer, subject */
    for (i = 0; i < 3; i++) {
        issuer_xn[i]  =  X509_get_issuer_name(x[i]);
        subject_xn[i] =  X509_get_subject_name(x[i]);

        if (issuer_xn[i] == NULL || subject_xn[i] == NULL) {
            OPENDCP_LOG(LOG_ERROR, "could not parse certificate data");
            return OPENDCP_ERROR;
        }
    }

    OPENDCP_LOG(LOG_DEBUG, "xml_sign: write_dsig_template: start signer");

    /* signer */
    xmlTextWriterStartElement(xml, BAD_CAST "Signer");
    xmlTextWriterStartElementNS(xml, BAD_CAST "dsig",
                                BAD_CAST "X509Data", NULL);

    xmlTextWriterStartElementNS(xml, BAD_CAST "dsig",
                                BAD_CAST "X509IssuerSerial", NULL);

    xmlTextWriterWriteFormatElementNS(xml, BAD_CAST "dsig",
                                      BAD_CAST "X509IssuerName", NULL, "%s",
                                      dn_oneline(issuer_xn[0]));

    xmlTextWriterWriteFormatElementNS(xml, BAD_CAST "dsig",
                                      BAD_CAST "X509SerialNumber", NULL, "%ld",
                                      ASN1_INTEGER_get(X509_get_serialNumber(x[0])));
    xmlTextWriterEndElement(xml);

    xmlTextWriterWriteFormatElementNS(xml, BAD_CAST "dsig",
                                      BAD_CAST "X509SubjectName", NULL,
                                      "%s", dn_oneline(subject_xn[0]));

    xmlTextWriterEndElement(xml);
    xmlTextWriterEndElement(xml);

    /* template */
    xmlTextWriterStartElementNS(xml, BAD_CAST "dsig",
                                BAD_CAST "Signature", NULL);

    xmlTextWriterStartElementNS(xml, BAD_CAST "dsig",
                                BAD_CAST "SignedInfo", NULL);

    xmlTextWriterStartElementNS(xml, BAD_CAST "dsig",
                                BAD_CAST "CanonicalizationMethod", NULL);

    xmlTextWriterWriteAttribute(xml, BAD_CAST "Algorithm",
                                BAD_CAST DS_CMA);

    xmlTextWriterEndElement(xml);

    xmlTextWriterStartElementNS(xml, BAD_CAST "dsig",
                                BAD_CAST "SignatureMethod", NULL);

    xmlTextWriterWriteAttribute(xml, BAD_CAST "Algorithm",
                                BAD_CAST DS_SMA[opendcp->ns]);

    xmlTextWriterEndElement(xml);

    xmlTextWriterStartElementNS(xml, BAD_CAST "dsig",
                                BAD_CAST "Reference",
                                NULL);

    xmlTextWriterWriteAttribute(xml, BAD_CAST "URI", BAD_CAST NULL);

    xmlTextWriterStartElementNS(xml, BAD_CAST "dsig",
                                BAD_CAST"Transforms",
                                NULL);

    xmlTextWriterStartElementNS(xml, BAD_CAST "dsig",
                                BAD_CAST "Transform",
                                NULL);

    xmlTextWriterWriteAttribute(xml, BAD_CAST "Algorithm",
                                BAD_CAST DS_TMA);

    xmlTextWriterEndElement(xml);
    xmlTextWriterEndElement(xml);

    xmlTextWriterStartElementNS(xml, BAD_CAST "dsig",
                                BAD_CAST "DigestMethod", NULL);

    xmlTextWriterWriteAttribute(xml, BAD_CAST "Algorithm",
                                BAD_CAST DS_DMA);

    xmlTextWriterEndElement(xml);

    xmlTextWriterWriteElementNS(xml, BAD_CAST "dsig",
                                BAD_CAST "DigestValue",
                                NULL, BAD_CAST "");

    xmlTextWriterEndElement(xml);
    xmlTextWriterEndElement(xml);

    xmlTextWriterWriteElementNS(xml, BAD_CAST "dsig",
                                BAD_CAST "SignatureValue",
                                NULL, BAD_CAST "");

    xmlTextWriterStartElementNS(xml, BAD_CAST "dsig",
                                BAD_CAST "KeyInfo", NULL);

    for (i = 0; i < 3; i++) {
        xmlTextWriterStartElementNS(xml, BAD_CAST "dsig",
                                    BAD_CAST "X509Data", NULL);
        xmlTextWriterStartElementNS(xml, BAD_CAST "dsig",
                                    BAD_CAST "X509IssuerSerial", NULL);

        xmlTextWriterWriteFormatElementNS(xml, BAD_CAST "dsig",
                                          BAD_CAST "X509IssuerName", NULL, "%s",
                                          dn_oneline(issuer_xn[i]));

        xmlTextWriterWriteFormatElementNS(xml, BAD_CAST "dsig",
                                          BAD_CAST "X509SerialNumber", NULL, "%ld",
                                          ASN1_INTEGER_get(X509_get_serialNumber(x[i])));
        xmlTextWriterEndElement(xml);

        xmlTextWriterWriteFormatElementNS(xml, BAD_CAST "dsig",
                                          BAD_CAST "X509Certificate", NULL, "%s",
                                          cert[i]);

        xmlTextWriterEndElement(xml);
    }

    xmlTextWriterEndElement(xml); /* KeyInfo */
    xmlTextWriterEndElement(xml); /* Signature */

    if (subject_xn[0]) {
        free(subject_xn[0]);
    }

    for (i = 0; i < 3; i++) {
        if (issuer_xn[i]) {
            free(issuer_xn[i]);
        }
    }

    return OPENDCP_NO_ERROR;
}

int xmlsec_init() {
    /* init libxml lib */
    xmlInitParser();
    xmlIndentTreeOutput = 1;
    LIBXML_TEST_VERSION
    xmlLoadExtDtdDefaultValue = XML_DETECT_IDS | XML_COMPLETE_ATTRS;
    xmlSubstituteEntitiesDefault(1);

    /* init xmlsec lib */
    if (xmlSecInit() < 0) {
        OPENDCP_LOG(OPENDCP_ERROR, "xmlsec initialization failed.");
        return(OPENDCP_ERROR);
    }

    /* Check loaded library version */
    if (xmlSecCheckVersion() != 1) {
        OPENDCP_LOG(OPENDCP_ERROR, "loaded xmlsec library version is not compatible.");
        return(OPENDCP_ERROR);
    }

    /* Init crypto library */
    if (xmlSecCryptoAppInit(NULL) < 0) {
        OPENDCP_LOG(OPENDCP_ERROR, "crypto initialization failed.");
        return(OPENDCP_ERROR);
    }

    /* Init xmlsec-crypto library */
    if (xmlSecCryptoInit() < 0) {
        OPENDCP_LOG(OPENDCP_ERROR, "xmlsec-crypto initialization failed.");
        return(OPENDCP_ERROR);
    }

    return(OPENDCP_NO_ERROR);
}

xmlSecKeysMngrPtr load_certificates_verify() {
    xmlSecKeysMngrPtr key_manager;

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

xmlSecKeysMngrPtr load_certificates_sign(opendcp_t *opendcp) {
    xmlSecKeysMngrPtr key_manager;
    xmlSecKeyPtr      key;

    /* create and initialize keys manager */
    key_manager = xmlSecKeysMngrCreate();

    if (key_manager == NULL) {
        fprintf(stderr, "Error: failed to create keys manager.\n");
        return(NULL);
    }

    if (xmlSecCryptoAppDefaultKeysMngrInit(key_manager) < 0) {
        fprintf(stderr, "Error: failed to initialize keys manager.\n");
        xmlSecKeysMngrDestroy(key_manager);
        return(NULL);
    }

    /* read key file */
    if (opendcp->xml_signature.private_key) {
        key = xmlSecCryptoAppKeyLoad(opendcp->xml_signature.private_key, xmlSecKeyDataFormatPem, NULL, NULL, NULL);
    } else {
        key = xmlSecCryptoAppKeyLoadMemory((unsigned char *)opendcp_private_key, strlen((char *)opendcp_private_key), xmlSecKeyDataFormatPem, NULL, NULL, NULL);
    }

    if (xmlSecCryptoAppDefaultKeysMngrAdoptKey(key_manager, key) < 0) {
        fprintf(stderr, "Error: failed to initialize keys manager.\n");
        xmlSecKeysMngrDestroy(key_manager);
        return(NULL);
    }

    /* load root certificate */
    if (opendcp->xml_signature.root) {
        if (xmlSecCryptoAppKeysMngrCertLoad(key_manager, opendcp->xml_signature.root, xmlSecKeyDataFormatPem, xmlSecKeyDataTypeTrusted) < 0) {
            fprintf(stderr, "Error: failed to load pem certificate \"%s\"\n", opendcp->xml_signature.root);
            xmlSecKeysMngrDestroy(key_manager);
            return(NULL);
        }
    } else {
        if (xmlSecCryptoAppKeysMngrCertLoadMemory(key_manager, (unsigned char *)opendcp_root_cert, strlen((char* )opendcp_root_cert), xmlSecKeyDataFormatPem, xmlSecKeyDataTypeTrusted) < 0) {
            fprintf(stderr, "Error: failed to load pem certificate from memory\n");
            xmlSecKeysMngrDestroy(key_manager);
            return(NULL);
        }
    }

    /* load ca (intermediate) certificate */
    if (opendcp->xml_signature.ca) {
        if (xmlSecCryptoAppKeysMngrCertLoad(key_manager, opendcp->xml_signature.ca, xmlSecKeyDataFormatPem, xmlSecKeyDataTypeTrusted) < 0) {
            fprintf(stderr, "Error: failed to load pem certificate \"%s\"\n", opendcp->xml_signature.ca);
            xmlSecKeysMngrDestroy(key_manager);
            return(NULL);
        }
    } else {
        if (xmlSecCryptoAppKeysMngrCertLoadMemory(key_manager, (unsigned char *)opendcp_ca_cert, strlen((char *)opendcp_ca_cert), xmlSecKeyDataFormatPem, xmlSecKeyDataTypeTrusted) < 0) {
            fprintf(stderr, "Error: failed to load pem certificate from memory\n");
            xmlSecKeysMngrDestroy(key_manager);
            return(NULL);
        }
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

    return(OPENDCP_NO_ERROR);
}

int xml_sign(opendcp_t *opendcp, char *filename) {
    xmlSecDSigCtxPtr dsig_ctx = NULL;
    xmlDocPtr        doc = NULL;
    xmlNodePtr       root_node;
    xmlNodePtr       sign_node;
    FILE *fp;
    int result = OPENDCP_ERROR;
    xmlSecKeysMngrPtr key_manager = NULL;

    OPENDCP_LOG(LOG_DEBUG, "xmlsec_init");
    xmlsec_init();

    /* load doc file */
    OPENDCP_LOG(LOG_DEBUG, "parse file %s", filename);
    doc = xmlParseFile(filename);

    if (doc == NULL) {
        OPENDCP_LOG(OPENDCP_ERROR, "unable to parse file %s", filename);
        goto done;
    }

    /* find root node */
    OPENDCP_LOG(LOG_DEBUG, "find root node");
    root_node = xmlDocGetRootElement(doc);

    if (root_node == NULL) {
        OPENDCP_LOG(OPENDCP_ERROR, "unable to find root node");
        goto done;
    }

    /* find signature node */
    OPENDCP_LOG(LOG_DEBUG, "find signature node");
    sign_node = xmlSecFindNode(root_node, xmlSecNodeSignature, xmlSecDSigNs);

    if (sign_node == NULL) {
        OPENDCP_LOG(LOG_ERROR, "start node not found");
        goto done;
    }

    /* create keys manager */
    OPENDCP_LOG(LOG_DEBUG, "load certificates");
    key_manager = load_certificates_sign(opendcp);

    if (key_manager == NULL) {
        OPENDCP_LOG(LOG_ERROR, "failed to create key manager\n");
        goto done;
    }

    /* create signature opendcp */
    OPENDCP_LOG(LOG_DEBUG, "create signature context");
    dsig_ctx = xmlSecDSigCtxCreate(key_manager);

    if (dsig_ctx == NULL) {
        OPENDCP_LOG(LOG_ERROR, "failed to create signature context");
        goto done;
    }

    /* sign the template */
    OPENDCP_LOG(LOG_DEBUG, "sign template");

    if (xmlSecDSigCtxSign(dsig_ctx, sign_node) < 0) {
        OPENDCP_LOG(LOG_ERROR, "Error: signature failed\n");
        goto done;
    }

    /* open xml file */
    OPENDCP_LOG(LOG_DEBUG, "open file for writing %s", filename);
    fp = fopen(filename, "wb");

    if (fp == NULL) {
        OPENDCP_LOG(LOG_ERROR, "Error: could not open output file\n");
        goto done;
    }

    /* write the xml file */
    OPENDCP_LOG(LOG_DEBUG, "write signed XML document");

    if (xmlDocDump(fp, doc) < 0) {
        OPENDCP_LOG(LOG_ERROR, "Error: writing XML document failed\n");
        goto done;
    }

    /* close the file */
    fclose(fp);

    /* success */
    result = 0;

done:
    /* destroy keys manager */
    OPENDCP_LOG(LOG_DEBUG, "destroy key manager");
    xmlSecKeysMngrDestroy(key_manager);

    /* destroy signature context */
    OPENDCP_LOG(LOG_DEBUG, "destroy signature context");

    if (dsig_ctx != NULL) {
        xmlSecDSigCtxDestroy(dsig_ctx);
    }

    /* destroy xml doc */
    OPENDCP_LOG(LOG_DEBUG, "free document memory");

    if (doc != NULL) {
        xmlFreeDoc(doc);
    }

    xmlsec_close();

    return(result);
}

int xml_verify(char *filename) {
    xmlSecDSigCtxPtr dsig_ctx = NULL;
    xmlDocPtr        doc = NULL;
    xmlNodePtr       root_node;
    xmlNodePtr       sign_node;
    xmlNodePtr       cert_node;
    xmlNodePtr       x509d_node;
    xmlNodePtr       cur_node;
    int              result = OPENDCP_ERROR;
    xmlSecKeysMngrPtr key_manager = NULL;
    unsigned char cert[5000];
    int  cert_l;

    xmlsec_init();

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
    key_manager = load_certificates_verify();

    if (key_manager == NULL) {
        OPENDCP_LOG(LOG_ERROR, "create key manager failed");
        goto done;
    }

    /* find certificates */
    cur_node = sign_node;

    while ((x509d_node = xmlSecFindNode(cur_node, xmlSecNodeX509Data, xmlSecDSigNs))) {
        cert_node = xmlSecFindNode(x509d_node, xmlSecNodeX509Certificate, xmlSecDSigNs);

        if (cert_node == NULL) {
            OPENDCP_LOG(LOG_ERROR, "X509certficate node not found");
            goto done;
        }

        sprintf((char *)cert, "-----BEGIN CERTIFICATE-----\n%s\n-----END CERTIFICATE-----\n", xmlNodeGetContent(cert_node));
        cert_l = strlen((char *)cert);

        if (xmlSecCryptoAppKeysMngrCertLoadMemory(key_manager, cert, cert_l, xmlSecKeyDataFormatPem, xmlSecKeyDataTypeTrusted) < 0) {
            OPENDCP_LOG(LOG_ERROR, "couldn't read X509certificate node value");
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

    /* verify the signature */
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
