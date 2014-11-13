#include "codecs/opendcp_decoder.h"

typedef struct {
    int total;
    int current;
    void *context;
    opendcp_decoder_t *decoder;
    int (*read_next)();
    int (*read_frame)();
} opendcp_reader_t;

opendcp_reader_t *reader_new(filelist_t *filelist);
void reader_free(opendcp_reader_t *self);
