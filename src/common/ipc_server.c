#include "fancontroller.h"
#include "filesystem.h"
#include "ipc.h"
#include "ipc_server.h"
#include "logger.h"
#include "matrix.h"
#include "strutils.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define IPC_RESPONSE_BUF_SIZE 32

static int fd;

static void create_tmp_dir(void) {
    mode_t old = umask(0);
    mkdir(SOCK_DIR, S_IRWXU | S_IRWXG | S_IRWXO);
    umask(old);
}

static size_t write_matrix_to_buffer(char *response, size_t count) {
    matrix m;
    uint8_t rows;
    amdgpu_fan_get_matrix(m, &rows);
    if(count < sizeof(uint8_t) * (1 + rows * MATRIX_COLS)) {
        memset(response, MATRIX_OVERFLOW, sizeof(uint8_t));
        return sizeof(uint8_t);
    }

    memset(response, rows, sizeof(uint8_t));
    memcpy(response + sizeof(uint8_t), m, rows * MATRIX_COLS * sizeof(uint8_t));
    return sizeof(uint8_t) + sizeof(matrix);

}

static size_t construct_ipc_response(char *response, struct ipc_request *request, size_t count) {
    char buffer[IPC_RESPONSE_BUF_SIZE];
    uint8_t value;
    if(request->type == ipc_get) {
        if(request->target == ipc_temp) {
            if(!amdgpu_get_temp(&value)) {
                if(strscpy(response, "Failed to get chip temp", count) < 0) {
                    fprintf(stderr, "Ipc response overflowed the buffer\n");
                }
                return strlen(response) + 1;
            }
            sprintf(buffer, "%u", value);
        }
        else if(request->target == ipc_speed) {
            if(!amdgpu_fan_get_percentage(&value)) {
                if(strscpy(response, "Failed to get fan percentage", count) < 0) {
                    fprintf(stderr, "Ipc response overflows the buffer\n");
                }
                return strlen(response) + 1;
            }
            sprintf(buffer, "%u", value);
        }
        else if(request->target == ipc_matrix) {    
            return write_matrix_to_buffer(response, count);
        }
        else if(request->target == ipc_pwm_path) {
            (void)amdgpu_fan_get_pwm_path(response, count);
            return strlen(response) + 1;
        }
        if(strscpy(response, buffer, count) < 0) {
            fprintf(stderr, "%u overflows the destination buffer\n", value);
        }
    }
    else if(request->type == ipc_set) {
        sprintf(buffer, "Speed set to constant %d%%", request->value);
        if(strscpy(response, buffer, count) < 0)  {
            fprintf(stderr, "Ipc response overflows the buffer\n");
        }
        amdgpu_fan_set_override_speed(request->value, request->ppid);
    }
    return strlen(response) + 1;
}

bool ipc_server_running(void) {
    return file_exists(SERVER_SOCK_FILE);
}

bool ipc_server_open_socket(void) {
    struct sockaddr_un addr;

    fd = socket(PF_UNIX, SOCK_DGRAM, 0);
    if(fd == -1) {
        perror("Failed to open socket");
        return false;
    }

    create_tmp_dir();

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if(strscpy(addr.sun_path, SERVER_SOCK_FILE, sizeof addr.sun_path) < 0) {
        fprintf(stderr, "%s overflows the socket address path\n", SERVER_SOCK_FILE);
        ipc_server_close_socket();
        return false;
    }
    unlink(SERVER_SOCK_FILE);

    if(bind(fd, (struct sockaddr *)&addr, sizeof addr) == -1) {
        perror("Failed to bind socket");
        ipc_server_close_socket();
        return false;
    }
    if(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) == -1) {
        perror("Failed to prevent socket from blocking");
        ipc_server_close_socket();
        return false;
    }
    chmod(SERVER_SOCK_FILE, S_IRWXU | S_IRWXG | S_IRWXO);

    LOG(VERBOSITY_LVL2, "Opened server socket\n");
    return true;
}

void ipc_server_close_socket(void) {
    LOG(VERBOSITY_LVL2, "Closing server socket\n");
    close(fd);
    unlink(SERVER_SOCK_FILE);
    rmdir_force(SOCK_DIR);
}

void ipc_server_handle_request(void) {
    ssize_t nbytes, ret;
    char request[sizeof(struct ipc_request)];
    char response[IPC_BUF_SIZE];
    struct sockaddr_un sender;
    socklen_t sender_len = sizeof sender;

    while((nbytes = recvfrom(fd, request, sizeof request, 0, (struct sockaddr *)&sender, &sender_len)) > 0) {
        LOG(VERBOSITY_LVL3, "Received request: " IPC_REQUEST_FMT((struct ipc_request *)request));
    
        size_t len = construct_ipc_response(response, (struct ipc_request *)request, sizeof response);

        ret = sendto(fd, response, len, 0, (struct sockaddr *)&sender, sender_len);
        if(ret == -1) {
            perror("Failed to send server response");
            break;
        }
    }
}