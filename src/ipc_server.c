#include "fancontroller.h"
#include "ipc.h"
#include "ipc_server.h"
#include "logger.h"
#include "strutils.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <regex.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

static int fd;
static regex_t ipc_rgx;

static void create_tmp_dir(void) {
    mode_t old = umask(0);
    mkdir(SOCK_DIR, S_IRWXU | S_IRWXG | S_IRWXO);
    umask(old);
}

static bool compile_ipc_regex(void) {
    int reti = regcomp(&ipc_rgx, "^\\s*(set|get)\\s*(temp(erature)?|speed)\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile ipc regex\n");
        return false;
    }
    return true;
}

static bool parse_ipc_request(char *buf, struct ipc_request *request, size_t count) {
    regmatch_t pmatch[3];
    if(regexec(&ipc_rgx, buf, 3, pmatch, 0)) {
        strncat(buf, " is not a valid ipc request", count);
        return false;
    }

    if(strncmp(buf + pmatch[1].rm_so, "set", pmatch[1].rm_eo - pmatch[1].rm_so) == 0) {
        request->type = ipc_set;
    }
    else {
        request->type = ipc_get;
    }

    if(strncmp(buf + pmatch[2].rm_so, "speed", pmatch[2].rm_eo - pmatch[2].rm_so) == 0) {
        request->value = ipc_speed;
    }
    else {
        request->value = ipc_temp;
    }

    return true;
}

static void construct_ipc_response(char *response, struct ipc_request *request, size_t count) {
    char buffer[4];
    uint8_t value;
    if(request->type == ipc_get) {
        if(request->value == ipc_temp) {
            if(!amdgpu_get_temp(&value)) {
                if(strscpy(response, "Failed to get chip temp", count) < 0) {
                    fprintf(stderr, "Ipc response overflowed the buffer\n");
                }
                return;
            }
            sprintf(buffer, "%u", value);
        }
        else if(request->value == ipc_speed) {
            if(!amdgpu_fan_get_percentage(&value)) {
                if(strscpy(response, "Failed to get fan percentage", count) < 0) {
                    fprintf(stderr, "Ipc response overflows the buffer\n");
                }
                return;
            }
            sprintf(buffer, "%u", value);
        }
    }
    else if(request->type == ipc_set) {
        // TODO
    }
    if(strscpy(response, buffer, count) < 0) {
        fprintf(stderr, "%u overflows the destination buffer\n", value);
    }
}

bool ipc_server_open_socket(void) {
    struct sockaddr_un addr;

    if(!compile_ipc_regex()) {
        return false;
    }

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
    if(rmdir(SOCK_DIR) == -1) {
        perror("Removing tmp dir");
    }
}

void ipc_server_handle_request(void) {
    ssize_t nbytes, ret;
    char buffer[IPC_BUF_SIZE];
    struct sockaddr_un sender;
    socklen_t sender_len = sizeof sender;
    struct ipc_request ipc;

    while((nbytes = recvfrom(fd, buffer, sizeof buffer, 0, (struct sockaddr *)&sender, &sender_len)) > 0) {
        LOG(VERBOSITY_LVL3, "Received request: %s\n", buffer);
    
        if(parse_ipc_request(buffer, &ipc, sizeof buffer)) {
            construct_ipc_response(buffer, &ipc, sizeof buffer);
        }


        ret = sendto(fd, buffer, strlen(buffer) + 1, 0, (struct sockaddr *)&sender, sender_len);
        if(ret == -1) {
            perror("Failed to send server response");
            break;
        }
    }
}
