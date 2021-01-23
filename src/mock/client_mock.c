#include "client_mock.h"
#include "mock.h"

#include <stdio.h>

static ssize_t(*send_and_recv)(unsigned char *, size_t, ipc_request) = 0;

void mock_client_send_and_recv(ssize_t(*mock)(unsigned char *, size_t, ipc_request)) {
    mock_function(send_and_recv, mock);
}

ssize_t client_send_and_recv(unsigned char *buffer, size_t bufsize, ipc_request request) {
    validate_mock(client_send_and_recv, send_and_recv);
    return send_and_recv(buffer, bufsize, request);
}
