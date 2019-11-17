#include "filesystem.h"
#include "ipc.h"
#include "ipc_client.h"
#include "logger.h"
#include "strutils.h"

#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static int fd;

bool ipc_client_open_socket(void) {
    struct sockaddr_un addr;

    switch(directory_exists(SOCK_DIR)) {
        case status_existing:
            break;
        case status_unexistant:
            fprintf(stderr, "Server not running\n");
            return false;
        case status_error:
        default:
            fprintf(stderr, "Failed determining server status\n");
            return false;
    }

    fd = socket(PF_UNIX, SOCK_DGRAM, 0);
    if(fd == -1) {
        perror("Failed to open socket");
        return false;
    }

    memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    if(strscpy(addr.sun_path, CLIENT_SOCK_FILE, sizeof addr.sun_path) < 0) {
        fprintf(stderr, "%s overflowed the socket path\n", CLIENT_SOCK_FILE);
        close(fd);
        return false;
    }
    unlink(CLIENT_SOCK_FILE);

    if(bind(fd, (struct sockaddr *)&addr, sizeof addr) == -1) {
        perror("Failed to bind socket");
        close(fd);
        return false;
    }

    memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    if(strscpy(addr.sun_path, SERVER_SOCK_FILE, sizeof addr.sun_path) < 0) {
        fprintf(stderr, "%s overflows the socket path\n", SERVER_SOCK_FILE);
        close(fd);
        return false;
    }

    if(connect(fd, (struct sockaddr *)&addr, sizeof addr) == -1) {
        perror("Failed to connect to server");
        close(fd);
        return false;
    }

    LOG(VERBOSITY_LVL2, "Connected to server\n");
    return true;
}

void ipc_client_close_socket(void) {
    LOG(VERBOSITY_LVL2, "Closing client socket\n");
    close(fd);
    unlink(CLIENT_SOCK_FILE);
}

ssize_t ipc_client_send_request(char *restrict response, char const *restrict request, size_t count) {
    ssize_t nbytes;
    if(send(fd, request, strlen(request) + 1, 0) == -1) {
        perror("Failed to send request");
        return -1;
    }
    LOG(VERBOSITY_LVL3, "Sent request %s\n", request);
    if((nbytes = recv(fd, response, count, 0)) < 0) {
        perror("Failed to receive server response");
        return -E2BIG;
    }
    return strlen(response);
}
