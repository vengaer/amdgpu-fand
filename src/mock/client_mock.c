#include "client_mock.h"

#include <stdio.h>

static ssize_t(*send_and_recv)(unsigned char *, size_t, ipc_request) = 0;

void mock_client_send_and_recv(ssize_t(*mock)(unsigned char *, size_t, ipc_request)) {
    send_and_recv = mock;
}

ssize_t client_send_and_recv(unsigned char *buffer, size_t bufsize, ipc_request request) {
    if(!send_and_recv) {
        fputs("client_send_and_recv not mocked\n", stderr);
        return -1;
    }
    return send_and_recv(buffer, bufsize, request);
}
