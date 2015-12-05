/*********************************************************************
* Filename:   md5.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding MD5 implementation.
*********************************************************************/

#ifndef MD5_H
#define MD5_H

/*************************** HEADER FILES ***************************/
#include <stddef.h>

/****************************** MACROS ******************************/
#define MD5_BLOCK_SIZE 16               // MD5 outputs a 16 byte digest

/**************************** DATA TYPES ****************************/
typedef unsigned char byte_t;             // 8-bit byte
typedef unsigned int  word_t;             // 32-bit word, change to "long" for 16-bit machines

typedef struct {
   byte_t data[64];
   word_t datalen;
   unsigned long long bitlen;
   word_t state[4];
} md5_t;

/*********************** FUNCTION DECLARATIONS **********************/
void md5_init(md5_t *ctx);
void md5_update(md5_t *ctx, const byte_t data[], size_t len);
void md5_final(md5_t *ctx, byte_t hash[]);

#endif   // MD5_H
