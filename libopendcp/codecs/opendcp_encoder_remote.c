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
#include "opendcp_remote.h"
#include <openssl/evp.h>

int create_connection(char *host, int port) {
    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd;

    OPENDCP_LOG(LOG_DEBUG, "creating connection");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
            OPENDCP_LOG(LOG_ERROR, "failed to create socket");
            return -1;
    }

    server = gethostbyname(host);

    if (server == NULL) {
        OPENDCP_LOG(LOG_ERROR, "error no such host");
        return -1;
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    OPENDCP_LOG(LOG_DEBUG, "connecting to %s", server->h_name);
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            OPENDCP_LOG(LOG_ERROR, "failed to connect to server %s", server->h_name);
            return -1;
    }

    return sockfd;
}

int close_connection(int connection) {
    OPENDCP_LOG(LOG_DEBUG, "closed connection");
    close(connection);

    return OPENDCP_NO_ERROR;
}

int opendcp_encode_remote(opendcp_t *opendcp, char *sfile, char *dfile) {
    int     slength;
    char    *sdata;
    char    *ddata;
    int     connection;
    FILE    *fp;
    encode_request_t request;
    char    smd5[EVP_MAX_MD_SIZE];

    /* create connection to server */
    connection = create_connection(opendcp->remote.host, opendcp->remote.port);

    if (connection < 0 ) {
       OPENDCP_LOG(LOG_ERROR, "failed to connect to server");
       return OPENDCP_ERROR;
    }

    /* initialize encode requet */
    memset(&request, 0, sizeof(encode_request_t));
    
    /* get file details and read */
    slength = get_file_length(sfile);
    sdata = malloc(slength + 1);
    read_file(sfile, sdata, slength);
    calculate_data_digest(smd5, "md5", sdata, slength);

    /* fill in request details */
    request.id             = opendcp->remote.id;
    request.message        = ENCODE_REQUEST;
    request.filename       = sfile;
    request.md5            = smd5;
    request.file_size      = slength;
    request.stereoscopic   = opendcp->stereoscopic;
    request.bw             = opendcp->j2k.bw;
    request.cinema_profile = opendcp->cinema_profile;
    request.xyz            = opendcp->j2k.xyz;
    request.resize         = opendcp->j2k.resize;

    /* send encode request */
    OPENDCP_LOG(LOG_DEBUG, "sending encode request");
    send_request(connection, &request);

    /* wait for ok */
    if (receive_response(connection, OK) == OPENDCP_NO_ERROR) {
        OPENDCP_LOG(LOG_DEBUG, "Ok received");
    } else {
        OPENDCP_LOG(LOG_DEBUG, "Ok NOT received");
        free(sdata);

        return OPENDCP_ERROR;
    }

    /* send data */
    OPENDCP_LOG(LOG_DEBUG, "sending data");
    send_data(connection, sdata, slength);
    free(sdata);

    /* wait for encode to complete */
    OPENDCP_LOG(LOG_DEBUG, "waiting for server to complete encode");
    memset(&request, 0, sizeof(encode_request_t));
    receive_request(connection, &request);

    /* send OK */
    send_response(connection, OK);

    /* receive the data */
    OPENDCP_LOG(LOG_DEBUG, "receiving encoded file");
    ddata = malloc(request.file_size);
    receive_data(connection, ddata, request.file_size);

     OPENDCP_LOG(LOG_DEBUG, "writing file %s", dfile);
     fp = fopen(dfile, "wb");
     fwrite(ddata, 1, request.file_size, fp);
     fclose(fp);

     free(ddata);

    /* ok, done... server will call us back when it's done */
    close(connection);

    return OPENDCP_NO_ERROR;
}