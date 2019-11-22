#include "filesystem.h"
#include "ipc.h"
#include "ipc_client.h"
#include "ipc_server.h"
#include "logger.h"
#include "matrix.h"
#include "strutils.h"

#include <stdio.h>
#include <string.h>

#include <regex.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static int fd;

static bool sysfs_writable(void) {
    struct ipc_request request = {
        .type = ipc_get,
        .target = ipc_pwm_path,
        .value = -1
    };

    char path[IPC_BUF_SIZE];
    ssize_t len = ipc_client_send_request(path, &request, sizeof path);
    if(len < 0) {
        fprintf(stderr, "Failed to get path path\n");
        return false;
    }

    int errnum;
    if(!file_accessible(path, R_OK | W_OK, &errnum)) {
        fprintf(stderr, "Failed to access sysfs: %s\n", strerror(errnum));
        return false;
    }
    return true;
}

bool ipc_client_open_socket(void) {
    struct sockaddr_un addr;
    
    if(!ipc_server_running()) {
        fprintf(stderr, "Server not running\n");
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

ssize_t ipc_client_send_request(char *response, struct ipc_request *request, size_t count) {
    ssize_t nbytes;
    if(send(fd, (char *)request, sizeof(struct ipc_request), 0) == -1) {
        perror("Failed to send request");
        return -1;
    }
    LOG(VERBOSITY_LVL3, "Sent request: " IPC_REQUEST_FMT(request));
    if((nbytes = recv(fd, response, count, 0)) < 0) {
        perror("Failed to receive server response");
        return -E2BIG;
    }
    return nbytes;
}

bool ipc_client_handle_request(struct ipc_request *request) {
    char response[IPC_BUF_SIZE];
    if(!ipc_client_open_socket()) {
        return false;
    }

    if(request->type == ipc_set) {
        if(!sysfs_writable()) {
            return false;
        }
    }

    ssize_t recv = ipc_client_send_request(response, request, sizeof response);
    ipc_client_close_socket();

    if(recv < 0) {
        return false;
    }
    if(request->target == ipc_matrix) {
        uint8_t const rows = *(uint8_t*)response;
        if(MATRIX_OVERFLOW & rows) {
            fprintf(stderr, "Matrix overflows the buffer\n");
            return false;
        }
        matrix_print((uint8_t (*)[2])(response + sizeof(uint8_t)), rows);
    }
    else {
        printf("%s\n", response);
    }
    return true;
}


