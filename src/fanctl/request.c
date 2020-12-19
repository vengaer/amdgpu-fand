#include "client.h"
#include "format.h"
#include "ipc.h"
#include "macro.h"
#include "request.h"
#include "serialize.h"

#include <stdio.h>

int request_convert(char const *target, ipc_request *request) {
    *request = ipc_req_inval;
    if(!target) {
        return 0;
    }
    for(unsigned i = 0; i < array_size(ipc_request_map); i++) {
        if(strcmp(target, ipc_request_map[i].name) == 0) {
            *request = ipc_request_map[i].code;
            break;
        }
    }

    if(*request == ipc_req_inval) {
        fprintf(stderr, "Invalid request: %s\n", target);
        return -1;
    }

    return 0;
}

int request_process_get(ipc_request request) {
    unsigned char rspbuffer[IPC_MAX_MSG_LENGTH];
    ssize_t rsplen;
    union unpack_result result;
    rsplen = client_send_and_recv(rspbuffer, sizeof(rspbuffer), request);
    ipc_response rsp;

    switch(request) {
        case ipc_req_speed:
            rsp = unpack_speed(rspbuffer, rsplen, &result);
            break;
        case ipc_req_temp:
            rsp = unpack_temp(rspbuffer, rsplen, &result);
            break;
        case ipc_req_matrix:
            rsp = unpack_matrix(rspbuffer, rsplen, &result);
            break;
        default:
            fprintf(stderr, "Invalid request %hhu", request);
            return -1;
    }

    if(format(&result, request, rsp)) {
        return -1;
    }

    return 0;
}

int request_process_exit(void) {
    unsigned char rspbuffer[IPC_MAX_MSG_LENGTH];
    ssize_t rsplen;
    ipc_response rsp;
    union unpack_result result;

    rsplen = client_send_and_recv(rspbuffer, sizeof(rspbuffer), ipc_req_exit);
    rsp = unpack_exit_rsp(rspbuffer, rsplen, &result);

    if(rsp) {
        fprintf(stderr, "%s\n", strerror(result.error));
        return -1;
    }

    return 0;
}
