#ifndef IPC_H
#define IPC_H

#include <sys/socket.h>
#include <sys/un.h>

enum { IPC_MAX_MSG_LENGTH = 35 };

enum ipc_cmd {
    ipc_privileged = 0,
    ipc_exit_req,
    ipc_unprivileged = 32,
    ipc_speed_req,
    ipc_temp_req,
    ipc_matrix_req,
    ipc_exit_rsp,
    ipc_speed_rsp,
    ipc_temp_rsp,
    ipc_matrix_rsp
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

int ipc_pack_errno(unsigned char *restrict buf, size_t bufsize, int err);

int ipc_pack_exit_rsp(unsigned char *restrict buf, size_t bufsize);
int ipc_pack_matrix_rsp(unsigned char *restrict buf, size_t bufsize, unsigned char const *restrict matrix, unsigned char nrows);

int ipc_unpack_errno(unsigned char const *restrict buf, size_t bufsize);

int ipc_unpack_exit_rsp(unsigned char const *restrict buf, size_t bufsize);
int ipc_unpack_matrix_rsp(unsigned char const* restrict buf, size_t bufsize, unsigned char *restrict matrix, unsigned char *nrows, size_t maxrows);

#endif /* IPC_H */
