#ifndef IPC_H
#define IPC_H

#include <sys/socket.h>
#include <sys/un.h>

enum { IPC_MAX_MSG_LENGTH = 18 };

enum ipc_cmd {
    ipc_privileged = 0,
    ipc_exit,
    ipc_unprivileged = 32,
    ipc_get_speed,
    ipc_get_temp,
    ipc_get_matrix
};

enum ipc_rsp {
    ipc_ok,
    ipc_err
};

union unsockaddr {
    struct sockaddr addr;
    struct sockaddr_un addr_un;
};

extern unsigned char ipc_valid_requests[4];

#endif /* IPC_H */
