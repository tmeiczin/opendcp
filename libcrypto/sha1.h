/*********************************************************************
* Filename:   sha1.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding SHA1 implementation.
*********************************************************************/

#ifndef SHA1_H
#define SHA1_H

#ifdef __cplusplus
extern "C" {
#endif

/*************************** HEADER FILES ***************************/
#include <stddef.h>

/****************************** MACROS ******************************/
#define SHA1_BLOCK_SIZE 20              // SHA1 outputs a 20 byte digest

/**************************** DATA TYPES ****************************/
typedef unsigned char BYTE;             // 8-bit byte
typedef unsigned int  WORD;             // 32-bit word, change to "long" for 16-bit machines

typedef struct {
    BYTE data[64];
    WORD datalen;
    unsigned long long bitlen;
    WORD state[5];
    WORD k[4];
} sha1_t;

/*********************** FUNCTION DECLARATIONS **********************/
void sha1_init(sha1_t *ctx);
void sha1_update(sha1_t *ctx, const BYTE data[], size_t len);
void sha1_final(sha1_t *ctx, BYTE hash[]);

/*********************** OPENSSL COMPATIBLE DECLARATIONS **********************/
void SHA1_Init(sha1_t *ctx);
void SHA1_Update(sha1_t *ctx, const BYTE data[], size_t len);
void SHA1_Final(BYTE hash[], sha1_t *ctx);

#ifdef __cplusplus
}
#endif



#endif   // SHA1_H
