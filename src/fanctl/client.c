#include "client.h"
#include "fandcfg.h"
#include "strutils.h"

#include <errno.h>
#include <stdio.h>

#include <sys/stat.h>
#include <unistd.h>

static int clientfd;

int client_init(void) {
    clientfd = socket(PF_UNIX, SOCK_STREAM, 0);
    union unsockaddr srvaddr;
    int status = 0;

    if(clientfd == -1) {
        perror("Failed to create socket");
        return -1;
    }

    struct timeval tv = {
        .tv_sec = 5,
        .tv_usec = 0
    };

    if(setsockopt(clientfd, SOL_SOCKET, SO_RCVTIMEO, &tv, (socklen_t)sizeof(tv)) == -1) {
        perror("Could not set socket timeout");
        status = -1;
        goto closesock;
    }

    srvaddr.addr_un.sun_family = AF_UNIX;
    if(strscpy(srvaddr.addr_un.sun_path, DAEMON_SERVER_SOCKET, sizeof(srvaddr.addr_un.sun_path)) < 0) {
        fprintf(stderr, "Socket path %s overflows the internal buffer", DAEMON_SERVER_SOCKET);
        status = -1;
        goto closesock;
    }

    if(connect(clientfd, &srvaddr.addr, sizeof(srvaddr)) == -1) {
        perror("Could not connect to server");
        status = -1;
        goto closesock;
    }

    return status;

closesock:
    close(clientfd);
    return status;
}

int client_kill(void) {
    if(close(clientfd) == -1) {
        perror("Could not close file descriptor");
        return -1;
    }
    return 0;
}

ssize_t client_send_and_recv(unsigned char *buffer, size_t bufsize, ipc_request request) {
    ssize_t nrecv;

    if(client_init()) {
        return -1;
    }

    if(send(clientfd, &request, sizeof(request), 0) == -1) {
        perror("Failed to send request");
        nrecv = -1;
        goto cleanup;
    }

    nrecv = recv(clientfd, buffer, bufsize, 0);
    if(nrecv == -1) {
        perror("No server response");
        nrecv = -1;
        goto cleanup;
    }

cleanup:
    client_kill();
    return nrecv;
}
