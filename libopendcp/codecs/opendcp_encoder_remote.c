#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#if WIN32
#include <winsock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#include <pthread.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#include "opendcp.h"
#include "opendcp_image.h"

#define OPENDCP_ERROR 1
#define OPENDCP_NO_ERROR 0

#define MD5_DIGEST_LENGTH 16
#define CHUNK_SIZE 1024
#define READ_SIZE 512
#define HEADER_LENGTH 1024

char* itoa(int value, char* result, int base) {
    if (base < 2 || base > 36) { 
        *result = '\0';
        return result;
    }
    
    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;
    
    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );
    
    // Apply negative sign
    if (tmp_value < 0) {
        *ptr++ = '-';
    }

    *ptr-- = '\0';

    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }

    return result;
}

typedef struct {
    char header[HEADER_LENGTH];
    int  data_length;
    char *data;
} message_t;

void copy_var(const char *src, char *dst, int len) {
    int c;
    
    for (c = 0; c < len; c++) {
        dst[c] = src[c];
    }
    dst[len] = '\0';
}

int get_parameter(const char *data, size_t data_len, const char *name, char *dst, int dst_len) {
    const char *value_start, *data_end, *value_end;
    int len;
    char search[data_len];
    
    if (dst == NULL || dst_len == 0) {
        len = -2;
    } else if (data == NULL || name == NULL || data_len == 0) {
        len = -1;
        dst[0] = '\0';
    } else {
        data_end = data + data_len;
        len = -1;
        dst[0] = '\0';
        
        snprintf(search, data_len, ":%s=", name);
        
        if ((value_start = strstr(data, search))) {
            value_start += strlen(search);
            value_end = (const char *) memchr(value_start, ':', (size_t)(data_end - value_start));
            
            if (value_end == NULL) {
                value_end = data_end;
            }
                     
            len = value_end - value_start < dst_len ? value_end - value_start : dst_len;
            copy_var(value_start, dst, len);
        } 
    }
    
    return len;
}

char *calculate_checksum(char *file) {
    FILE *fp;
    int bytes, i;
    md5_t ctx;
    unsigned char data[1024];
    unsigned char c[MD5_DIGEST_LENGTH];

    char *md5 = malloc(MD5_DIGEST_LENGTH *2 + 1);
    
    fp = fopen(file, "rb");
    
    if (!fp) {
      return NULL;
    }
    
    md5_init(&ctx);
    
    while ((bytes = fread(data, 1, 1024, fp)) != 0) {
        md5_update(&ctx, data, bytes);
    }
    
    md5_final(c, &ctx);
    
    for (i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        snprintf(&md5[i*2], MD5_DIGEST_LENGTH*2, "%02x", c[i]);
    }
    
    fclose(fp);

    return md5;
}

int get_file_length(char *filename) {
    FILE *fp;
    int file_length;

    fp = fopen(filename, "rb");

    if (!fp) {
        printf("could not open file\n");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    file_length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    fclose(fp);

    return file_length;
}

int read_file(char *filename, char *buffer, int buffer_size) {
    FILE *fp;

    fp = fopen(filename, "rb");
    fread(buffer, 1, buffer_size, fp);
    fclose(fp);

    return OPENDCP_NO_ERROR;
}

int write_file(char *file, char *data, int data_length) {
    FILE *fp = fopen(file, "wb");
    
    if (fp == NULL) {
        fprintf(stderr, "Could not open file %s\n", file);
        return 1;
    }
    
    printf("writing %d\n", data_length);
    fwrite(data, 1, data_length, fp);
    fclose(fp);
    
    return 0;
}

static int set_sock_timeout(int sockfd, int milliseconds) {
    struct timeval t;
    t.tv_sec = milliseconds / 1000;
    t.tv_usec = (milliseconds * 1000) % 1000000;

    return setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *) &t, sizeof(t)) ||
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (void *) &t, sizeof(t));
}

int receive_data(int connection, char *buffer, int buffer_size) {
    int total = 0;
    int n = 0;
    int recv_size;
    int left = buffer_size;

    while (total < buffer_size) {
        recv_size = left > CHUNK_SIZE ? CHUNK_SIZE : left;
        n = recv(connection, buffer + total, recv_size, 0);
        
        if (!n) {
            break;
        }
    
        total += n;
        left -= n;
    }
    
    return n < 0 ? OPENDCP_ERROR:OPENDCP_NO_ERROR;
}

int send_data(int connection, char *buffer, int buffer_size) {
    int total = 0;
    int left = buffer_size;
    int n = 0;
    int send_size;

    while (total < buffer_size) {
        send_size = left > CHUNK_SIZE ? CHUNK_SIZE : left;
        n = send(connection, buffer + total, send_size, 0);
        total += n;
        left -= n;
    }
    
    return n < 0 ? OPENDCP_ERROR:OPENDCP_NO_ERROR;
}

#define BUF_SIZE 512
#define MIN_BUF_SPACE_THRESHOLD (BUF_SIZE / 2)

char *read_response(int sock) {
    int bytes_read;
    char *buf = (char*)malloc(BUF_SIZE);
    int cur_position = 0;
    int space_left = BUF_SIZE;

    if (buf == NULL) {
        exit(1); /* or try to cope with out-of-memory situation */
    }

    while ((bytes_read = read(sock, buf + cur_position, space_left)) > 0) {
        cur_position += bytes_read;
        space_left -= bytes_read;
        if (space_left < MIN_BUF_SPACE_THRESHOLD) {
            buf = realloc(buf, cur_position + space_left + BUF_SIZE);
            if (buf == NULL) {
                exit(1); /* or try to cope with out-of-memory situation */
            }
            space_left += BUF_SIZE;
        }
    }
    
    return buf;
}

int create_connection(const char *host, const char *port) {
    struct addrinfo hints, *server;
    int sockfd, rc;
    char port_s[5];
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    rc = getaddrinfo(host, port, &hints, &server);
    
    if (rc) {
        printf("error: %d\n", rc);
    }
    
    if (server == NULL) {
        printf("error no such host\n");
        return -1;
    }
    
    printf("creating connection to %s %s\n", host, port);
    sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

    if (sockfd < 0) {
        printf("failed to create socket\n");
        freeaddrinfo(server);
        return -1;
    }

    if (connect(sockfd, server->ai_addr, server->ai_addrlen) < 0 ) {
        printf("failed to connect to server %s\n", host);
        freeaddrinfo(server);
        return -1;
    }
    
    freeaddrinfo(server);
    
    printf("connected!\n");

    return sockfd;
}

int close_connection(int connection) {
    printf("closed connection");
    close(connection);

    return OPENDCP_NO_ERROR;
}

int receive_message(int connection, message_t *m) {
    char param[HEADER_LENGTH];
    
    receive_data(connection, m->header, sizeof(m->header));
    get_parameter(m->header, strlen(m->header), "length", param, sizeof(param));
    m->data_length = atoi(param);

    if (m->data_length) {
        m->data = malloc(m->data_length);
        receive_data(connection, m->data, m->data_length);
    }
    
    return 0;
}

int send_message(int connection, const message_t *msg) {
    char *m;
    int  m_length;

    /* allocate message */
    m_length = sizeof(msg->header) + msg->data_length;
    m = malloc(m_length);
    
    if (!m) {
        return 1;
    }

    /* copy header into message */
    memset(m, 0, m_length);
    memcpy(m, msg->header, sizeof(msg->header));

    /* copy data into message */
    if (msg->data && msg->data_length) {
        memcpy(m + sizeof(msg->header), msg->data, msg->data_length);
    }

    /* send message */
    printf("sending message %d\n", (m_length));
    send_data(connection, m, m_length);
    
    free(m);

    return 0;
}

int opendcp_request_encoded_file(opendcp_t *opendcp, char *file) {
    int        connection;
    char       md5[40];
    char       *checksum;
    message_t  message;

    connection = create_connection(opendcp->remote.host, opendcp->remote.port);
    
    if (connection < 0 ) {
        return OPENDCP_ERROR;
    }
    
    /* send file request */
    snprintf(message.header, sizeof(message.header), ":action=%s:file=%s:length=0", "encoded_file_request", file);
    message.data = NULL;
    message.data_length = 0;
    send_message(connection, &message);
    
    /* receive file */
    receive_message(connection, &message);
    get_parameter(message.header, strlen(message.header), "md5", md5, sizeof(md5));

    if (write_file("foo.tif", message.data, message.data_length)) {
        printf("File write error");
        free(message.data);
        return 1;
    }
    
    if (message.data) {
        free(message.data);
    }
    
    close(connection);
    
    //checksum = malloc(sizeof(MD5_DIGEST_LENGTH*2) + 1);
    checksum = calculate_checksum("foo.tif");
    
    if (strcmp(checksum, md5)) {
        printf("checksum mismatch\n");
        free(checksum);
        return 1;
    }
    
    free(checksum);
    
    return 0;
}

int opendcp_encode_request(const opendcp_t *opendcp, opendcp_image_t *image) {
    int        connection;
    char       *checksum = NULL;
    message_t  message;
    
    connection = create_connection(opendcp->remote.host, opendcp->remote.port);
    
    if (connection < 0 ) {
        return OPENDCP_ERROR;
    }
    
    /* get file details and read */
    message.data_length = opendcp_image_size(image);
    
    if (message.data_length < 0) {
        return OPENDCP_ERROR;
    }
    
    /* read data */
    message.data = malloc(message.data_length);
    //read_file(file, message.data, message.data_length);
    //checksum = calculate_checksum(file);

    /* build request */
    snprintf(message.header, sizeof(message.header), ":action=encode_request:length=%d:md5=%s:profile=cinema2k:bw=%d", message.data_length, checksum, opendcp->j2k.bw);

    /* send encode request */
    send_message(connection, &message);
    
    if (message.data) {
        free(message.data);
    }
    
    /* wait for encode to complete */
    receive_message(connection, &message);

    if (message.data_length) {
        free(message.data);
    }
    
    free(checksum);

    close(connection);

    return OPENDCP_NO_ERROR;
}

int opendcp_encode_remote(opendcp_t *opendcp, opendcp_image_t *simage, char *dfile) {
    int rc;
    char encoded_file[512];
    
    opendcp->remote.host = "localhost";
    opendcp->remote.port = "8080";
    opendcp->j2k.bw = 125;
    
    //for (;;) {
    printf("encode request\n");
    rc = opendcp_encode_request(opendcp, simage);
    sleep(1);
    
    if (rc) {
        return rc;
    }

    printf("request file\n");
    rc = opendcp_request_encoded_file(opendcp, "test.tif");
    //}
    
    /* encode request complete */
    return rc;
}
