#include "codecs/opendcp_encoder.h"

typedef struct {
    int total;
    int current;
    void *context;
    opendcp_encoder_t *encoder;
    int (*write_next)();
    int (*write_frame)();
} opendcp_writer_t;

opendcp_writer_t *writer_new(filelist_t *filelist);
void writer_free(opendcp_writer_t *self);
