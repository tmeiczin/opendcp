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
#define SHA_CTX sha1_t
#define SHA_DIGEST_LENGTH SHA1_BLOCK_SIZE

/**************************** DATA TYPES ****************************/
typedef unsigned char byte_t;             // 8-bit byte
typedef unsigned int  word_t;             // 32-bit word, change to "long" for 16-bit machines

typedef struct {
    byte_t data[64];
    word_t datalen;
    unsigned long long bitlen;
    word_t state[5];
    word_t k[4];
} sha1_t;

/*********************** FUNCTION DECLARATIONS **********************/
void sha1_init(sha1_t *ctx);
void sha1_update(sha1_t *ctx, const byte_t data[], size_t len);
void sha1_final(sha1_t *ctx, byte_t hash[]);

/*********************** OPENSSL COMPATIBLE DECLARATIONS **********************/
void SHA1_Init(sha1_t *ctx);
void SHA1_Update(sha1_t *ctx, const byte_t data[], size_t len);
void SHA1_Final(byte_t hash[], sha1_t *ctx);

#ifdef __cplusplus
}
#endif

#endif   // SHA1_H
