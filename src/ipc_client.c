#include "filesystem.h"
#include "ipc.h"
#include "ipc_client.h"
#include "logger.h"
#include "strutils.h"

#include <stdio.h>
#include <string.h>

#include <regex.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static int fd;

static bool parse_ipc_request(char const *buf, struct ipc_request *request) {
    regex_t ipc_rgx, speed_rgx;
    regmatch_t pmatch[3];
    int reti = regcomp(&ipc_rgx, "^\\s*(set|get)\\s*(temp(erature)?|(fan)?speed)\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile ipc regex\n");
        return false;
    }

    reti = regcomp(&speed_rgx, "speed", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile speed regex\n");
        return false;
    }

    if(regexec(&ipc_rgx, buf, 3, pmatch, 0)) {
        fprintf(stderr, "%s if not a valid ipc request\n", buf);
        return false;
    }

    if(strncmp(buf + pmatch[1].rm_so, "set", pmatch[1].rm_eo - pmatch[1].rm_so) == 0) {
        request->type = ipc_set;
    }
    else {
        request->type = ipc_get;
    }

    if(regexec(&speed_rgx, buf + pmatch[2].rm_so, 0, NULL, 0) == 0) {
        request->value = ipc_speed;
    }
    else {
        request->value = ipc_temp;
    }

    return true;
}

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
    struct ipc_request sreq;
    if(!parse_ipc_request(request, &sreq)) {
        return -1;
    }
    if(send(fd, (char *)&sreq, sizeof(struct ipc_request), 0) == -1) {
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
