/*********************************************************************
* Filename:   aes.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding AES implementation.
*********************************************************************/

#ifndef AES_H
#define AES_H

#ifdef __cplusplus
extern "C" {
#endif

/*************************** HEADER FILES ***************************/
#include <stddef.h>

/****************************** MACROS ******************************/
#define AES_BLOCK_SIZE 16               // AES operates on 16 bytes at a time

/**************************** DATA TYPES ****************************/
typedef unsigned char byte_t;            // 8-bit byte
typedef unsigned int word_t;             // 32-bit word, change to "long" for 16-bit machines

/*********************** FUNCTION DECLARATIONS **********************/
///////////////////
// AES
///////////////////
// Key setup must be done before any AES en/de-cryption functions can be used.
void aes_key_setup(const byte_t key[],          // The key, must be 128, 192, or 256 bits
                   word_t w[],                  // Output key schedule to be used later
                   int keysize);                // Bit length of the key, 128, 192, or 256

void aes_encrypt(const byte_t in[],             // 16 bytes of plaintext
                 byte_t out[],                  // 16 bytes of ciphertext
                 const word_t key[],            // From the key setup
                 int keysize);                  // Bit length of the key, 128, 192, or 256

void aes_decrypt(const byte_t in[],             // 16 bytes of ciphertext
                 byte_t out[],                  // 16 bytes of plaintext
                 const word_t key[],            // From the key setup
                 int keysize);                  // Bit length of the key, 128, 192, or 256

///////////////////
// AES - CBC
///////////////////
int aes_encrypt_cbc(const byte_t in[],          // Plaintext
                    size_t in_len,              // Must be a multiple of AES_BLOCK_SIZE
                    byte_t out[],               // Ciphertext, same length as plaintext
                    const word_t key[],         // From the key setup
                    int keysize,                // Bit length of the key, 128, 192, or 256
                    const byte_t iv[]);         // IV, must be AES_BLOCK_SIZE bytes long

// Only output the CBC-MAC of the input.
int aes_encrypt_cbc_mac(const byte_t in[],      // plaintext
                        size_t in_len,          // Must be a multiple of AES_BLOCK_SIZE
                        byte_t out[],           // Output MAC
                        const word_t key[],     // From the key setup
                        int keysize,            // Bit length of the key, 128, 192, or 256
                        const byte_t iv[]);     // IV, must be AES_BLOCK_SIZE bytes long

///////////////////
// AES - CTR
///////////////////
void increment_iv(byte_t iv[],                  // Must be a multiple of AES_BLOCK_SIZE
                  int counter_size);            // Bytes of the IV used for counting (low end)

void aes_encrypt_ctr(const byte_t in[],         // Plaintext
                     size_t in_len,             // Any byte length
                     byte_t out[],              // Ciphertext, same length as plaintext
                     const word_t key[],        // From the key setup
                     int keysize,               // Bit length of the key, 128, 192, or 256
                     const byte_t iv[]);        // IV, must be AES_BLOCK_SIZE bytes long

void aes_decrypt_ctr(const byte_t in[],         // Ciphertext
                     size_t in_len,             // Any byte length
                     byte_t out[],              // Plaintext, same length as ciphertext
                     const word_t key[],        // From the key setup
                     int keysize,               // Bit length of the key, 128, 192, or 256
                     const byte_t iv[]);        // IV, must be AES_BLOCK_SIZE bytes long

///////////////////
// AES - CCM
///////////////////
// Returns True if the input parameters do not violate any constraint.
int aes_encrypt_ccm(const byte_t plaintext[],              // IN  - Plaintext.
                    word_t plaintext_len,                  // IN  - Plaintext length.
                    const byte_t associated_data[],        // IN  - Associated Data included in authentication, but not encryption.
                    unsigned short associated_data_len,    // IN  - Associated Data length in bytes.
                    const byte_t nonce[],                  // IN  - The Nonce to be used for encryption.
                    unsigned short nonce_len,              // IN  - Nonce length in bytes.
                    byte_t ciphertext[],                   // OUT - Ciphertext, a concatination of the plaintext and the MAC.
                    word_t *ciphertext_len,                // OUT - The length of the ciphertext, always plaintext_len + mac_len.
                    word_t mac_len,                        // IN  - The desired length of the MAC, must be 4, 6, 8, 10, 12, 14, or 16.
                    const byte_t key[],                    // IN  - The AES key for encryption.
                    int keysize);                          // IN  - The length of the key in bits. Valid values are 128, 192, 256.

// Returns True if the input parameters do not violate any constraint.
// Use mac_auth to ensure decryption/validation was preformed correctly.
// If authentication does not succeed, the plaintext is zeroed out. To overwride
// this, call with mac_auth = NULL. The proper proceedure is to decrypt with
// authentication enabled (mac_auth != NULL) and make a second call to that
// ignores authentication explicitly if the first call failes.
int aes_decrypt_ccm(const byte_t ciphertext[],             // IN  - Ciphertext, the concatination of encrypted plaintext and MAC.
                    word_t ciphertext_len,                 // IN  - Ciphertext length in bytes.
                    const byte_t assoc[],                  // IN  - The Associated Data, required for authentication.
                    unsigned short assoc_len,              // IN  - Associated Data length in bytes.
                    const byte_t nonce[],                  // IN  - The Nonce to use for decryption, same one as for encryption.
                    unsigned short nonce_len,              // IN  - Nonce length in bytes.
                    byte_t plaintext[],                    // OUT - The plaintext that was decrypted. Will need to be large enough to hold ciphertext_len - mac_len.
                    word_t *plaintext_len,                 // OUT - Length in bytes of the output plaintext, always ciphertext_len - mac_len .
                    word_t mac_len,                        // IN  - The length of the MAC that was calculated.
                    int *mac_auth,                         // OUT - TRUE if authentication succeeded, FALSE if it did not. NULL pointer will ignore the authentication.
                    const byte_t key[],                    // IN  - The AES key for decryption.
                    int keysize);                          // IN  - The length of the key in BITS. Valid values are 128, 192, 256.

/*********************** OPENSSL COMPATIBLE DECLARATIONS **********************/
#define AES_MAXNR 14

typedef struct aes_key_st {
    word_t schedule[4 * (AES_MAXNR + 1)];
    int size;
} aes_key_t;
typedef aes_key_t AES_KEY;

int AES_set_encrypt_key (const unsigned char *user_key, const int bits, aes_key_t *key);
int AES_set_decrypt_key (const unsigned char *user_key, const int bits, aes_key_t *key);
void AES_encrypt(const unsigned char *in, unsigned char *out, const aes_key_t *key);
void AES_decrypt(const unsigned char *in, unsigned char *out, const aes_key_t *key);
void AES_decrypt_cbc(const unsigned char *in, int in_len, unsigned char *out, const aes_key_t *key, byte_t *iv);

///////////////////
// Test functions
///////////////////
int aes_test();
int aes_ecb_test();
int aes_cbc_test();
int aes_ctr_test();
int aes_ccm_test();

#ifdef __cplusplus
}
#endif

#endif   // AES_H
