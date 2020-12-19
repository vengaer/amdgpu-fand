#ifndef IPC_H
#define IPC_H

#include <sys/socket.h>
#include <sys/un.h>

enum { IPC_MAX_MSG_LENGTH = 48 };

typedef unsigned char ipc_request;
typedef unsigned char ipc_response;

enum {
    ipc_req_privileged = 0,
    ipc_req_exit,
    ipc_req_unprivileged = 32,
    ipc_req_speed,
    ipc_req_temp,
    ipc_req_matrix,
    ipc_req_inval = 0xff
};

struct ipc_pair {
    char const *name;
    ipc_request code;
};

enum {
    ipc_rsp_ok,
    ipc_rsp_err
};

union unsockaddr {
    struct sockaddr addr;
    struct sockaddr_un addr_un;
};

extern ipc_request ipc_valid_requests[4];
extern struct ipc_pair ipc_request_map[4];

#endif /* IPC_H */
