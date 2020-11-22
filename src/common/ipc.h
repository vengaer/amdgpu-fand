#ifndef IPC_H
#define IPC_H

#include <sys/socket.h>
#include <sys/un.h>

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

#endif /* IPC_H */
