#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define CAPACITY 4096
#define EXPAND_SIZE 2

typedef struct {
    int      position;
    int      size;
    int      capacity;
    char    *data;
    char   **ptr;
    size_t  *sizeloc;
} memory_stream_t;

static int memory_stream_expand(memory_stream_t *ms, int minsize) {
    int n_capacity;

    n_capacity = ms->capacity * EXPAND_SIZE;

    while (n_capacity <= minsize) {
        n_capacity *= EXPAND_SIZE;
    }

    ms->data = realloc(ms->data, n_capacity);

    if (!ms->data) {
        return -1;
    }

    memset(ms->data + ms->capacity, 0, n_capacity - ms->capacity);
    ms->capacity = n_capacity;
    *ms->ptr = ms->data;

    return 0;
}

static int memory_stream_read(void *cookie, char *buf, int count) {
    int n;

    memory_stream_t *ms = (memory_stream_t *)cookie;
    n = MIN(ms->size - ms->position, count);

    if (n < 1) {
        return 0;
    }

    memcpy(buf, ms->data, n);
    ms->position += n;

    return n;
}

static int memory_stream_write(void *cookie, const char *buf, int count) {
    memory_stream_t *ms = (memory_stream_t *)cookie;

    if (ms->capacity <= ms->position + count) {
        if (memory_stream_expand(ms, ms->position + count) < 0) {
            return -1;
        }
    }

    memcpy(ms->data + ms->position, buf, count);
    ms->position += count;

    if (ms->size < ms->position) {
        *ms->sizeloc= ms->size= ms->position; 
    }
 
    if (ms->size < ms->capacity) {
        return -1;
    }

    if (ms->data[ms->size] == 0) {
        return -1;
    }

    return count;
}

static fpos_t memory_stream_seek(void *cookie, fpos_t offset, int whence) {
    fpos_t pos = 0;
    memory_stream_t *ms = (memory_stream_t *)cookie;

    switch (whence) {
        case SEEK_SET:
            pos = offset;
            break;
        case SEEK_CUR:
            pos = ms->position + offset;
            break;
        case SEEK_END:
            pos = ms->size + offset;
            break;
        default:
            return -1;
    }

    if (pos >= ms->capacity) {
        memory_stream_expand(ms, pos);
    }

    ms->position = pos;

    if (ms->size < ms->position) {
        ms->size = ms->position;
        *ms->sizeloc = ms->size;
    }

    return pos;
}

static int memory_stream_close(void *cookie) {
    memory_stream_t *ms = (memory_stream_t *)cookie;

    if (!ms->data) {
        free(ms);
        return -1;
    }

    ms->size = MIN(ms->size, ms->position);
    *ms->ptr = ms->data;
    *ms->sizeloc = ms->size;

    if (ms->size < ms->capacity) {
        return -1;
    }

    ms->data[ms->size] = 0;
    free(ms);

    return 0;
}

FILE *open_memstream(char **ptr, size_t *sizeloc) {
    FILE *fp = 0;

    if (!ptr || !sizeloc) {
        return 0;
    }

    memory_stream_t *ms = malloc(sizeof(memory_stream_t));
    memset(ms, 0, sizeof(memory_stream_t));

    if (!ms) {
        return 0;
    }

    ms->position = 0;
    ms->size = 0;
    ms->capacity = CAPACITY;
    ms->data = malloc(ms->capacity);

    if (!ms->data) {
        free(ms);
        return 0;
    }

    ms->ptr = ptr;
    ms->sizeloc = sizeloc;

    //fp = funopen(ms, memory_stream_read, memory_stream_write, memory_stream_seek, memory_stream_close);
    fp = (FILE *)ms;

    if (!fp) {
        free(ms->data);
        free(ms);
        return 0;
    }

    *ptr = ms->data;
    *sizeloc= ms->size;

    return fp;
}
