#ifndef CRYPTO_ERR_H
#define CRYPTO_ERR_H 

#ifdef __cplusplus
extern "C" {
#endif

unsigned long ERR_get_error() {
    return 0;
}

char *ERR_error_string(int errval, char *err_buf) {
    errval = 0;
    return(err_buf);
}

#ifdef __cplusplus
}
#endif

#endif   // SHA1_H
