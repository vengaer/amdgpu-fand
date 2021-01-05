#include "config.h"
#include "ipc.h"
#include "server.h"
#include "strutils.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int clientfd;

ssize_t sockput_raw(unsigned char const *data, size_t size) {
    union unsockaddr srvaddr;;
    int status = 0;
    clientfd  = socket(PF_UNIX, SOCK_STREAM, 0);

    if(clientfd == -1) {
        perror("socket");
        return -1;
    }

    srvaddr.addr_un.sun_family = AF_UNIX;
    if(strscpy(srvaddr.addr_un.sun_path, DAEMON_SERVER_SOCKET, sizeof(srvaddr.addr_un.sun_path)) < 0) {
        fputs("socket path overflow\n", stderr);
        status = -1;
        goto sockerr;
    }

    if(connect(clientfd, &srvaddr.addr, sizeof(srvaddr)) == -1) {
        perror("connect");
        status = -1;
        goto sockerr;
    }

    if(send(clientfd, data, size, 0) == -1) {
        perror("send");
        status = -1;
        goto sockerr;
    }

    return 0;

sockerr:
    close(clientfd);
    return status;
}

int sock_consume_and_close(void) {
    unsigned char rsp[IPC_MAX_MSG_LENGTH];

    ssize_t nrecv = recv(clientfd, rsp, sizeof(rsp), 0);
    if(nrecv == -1) {
        perror("recv");
    }

    if(close(clientfd) == -1) {
        perror("close");
        return -1;
    }

    return 0;
}

int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    if(!size) {
        return 0;
    }
    struct fand_config config = {
        .throttle = false,
        .matrix_rows = 1,
        .hysteresis = 3,
        .interval = 2,
        .matrix = { 0 }
    };

    if(server_init()) {
        fputs("Error starting server\n", stderr);
        return 0;
    }

    setlogmask(0);
    openlog(0, 0, LOG_DAEMON);

    if(sockput_raw(data, size) == -1) {
        fputs("Could not write to socket\n", stderr);
        goto cleanup;
    }

    server_poll(&config);

cleanup:
    sock_consume_and_close();

    if(server_kill()) {
        fputs("Error stopping server\n", stderr);
    }
    closelog();

    return 0;
}
