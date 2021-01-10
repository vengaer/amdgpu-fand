#include "client_mock.h"
#include "ipc.h"
#include "request.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

ssize_t send_and_recv(unsigned char *buffer, size_t bufsize, ipc_request request) {
    (void)buffer;
    (void)bufsize;
    (void)request;
    return -1;
}

int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    if(!size) {
        return 0;
    }

    mock_client_send_and_recv(send_and_recv);

    ipc_request request;

    char *buffer = malloc(size + 1);

    if(!buffer) {
        fputs("malloc failure\n", stderr);
        return 0;
    }

    memcpy(buffer, data, size);
    buffer[size] = '\0';

    if(request_convert(buffer, &request)) {
        goto freemem;
    }
    if(request_process_get(request)) {
        goto freemem;
    }


freemem:
    free(buffer);

    return 0;
}
