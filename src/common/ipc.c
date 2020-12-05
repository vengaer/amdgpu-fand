#include "ipc.h"

#include <errno.h>
#include <string.h>

#define IPC_HEADER_SIZE 2u

unsigned char ipc_valid_requests[4] = {
    ipc_exit_req,
    ipc_speed_req,
    ipc_temp_req,
    ipc_matrix_req
};

int ipc_pack_errno(unsigned char *restrict buf, size_t bufsize, int err) {
    if(bufsize < sizeof(int) + 1) {
        return -E2BIG;
    }

    buf[0] = ipc_err;
    memcpy(&buf[1], &err, sizeof(err));

    return sizeof(int) + 1;
}

int ipc_pack_exit_rsp(unsigned char *restrict buf, size_t bufsize) {
    if(bufsize < IPC_HEADER_SIZE) {
        return -E2BIG;
    }
    buf[0] = ipc_ok;
    buf[1] = ipc_exit_rsp;
    return 2;
}

int ipc_pack_temp_rsp(unsigned char *restrict buf, size_t bufsize, unsigned char temp) {
    if(bufsize < IPC_HEADER_SIZE + 1) {
        return -E2BIG;
    }
    buf[0] = ipc_ok;
    buf[1] = ipc_temp_rsp;
    buf[2] = temp;
    return 3;
}

int ipc_pack_speed_rsp(unsigned char *restrict buf, size_t bufsize, unsigned char speed) {
    if(bufsize < IPC_HEADER_SIZE + 1) {
        return -E2BIG;
    }
    buf[0] = ipc_ok;
    buf[1] = ipc_speed_rsp;
    buf[2] = speed;
    return 3;
}

int ipc_pack_matrix_rsp(unsigned char *restrict buf, size_t bufsize, unsigned char const *restrict matrix, unsigned char nrows) {
    if(bufsize < nrows + IPC_HEADER_SIZE) {
        return -E2BIG;
    }
    buf[0] = ipc_ok;
    buf[1] = ipc_matrix_rsp;
    buf[2] = nrows;
    memcpy(&buf[3], matrix, nrows);
    return nrows + 3;
}

/* Return values signifying errors on the client side are negative,
 * errors from the server positive. */
int ipc_unpack_errno(unsigned char const *restrict buf, size_t bufsize) {
    if(bufsize < sizeof(int) + 1) {
        return -E2BIG;
    }

    if(buf[0] != ipc_err) {
        return -EINVAL;
    }

    int err;
    memcpy(&err, &buf[1], sizeof(err));
    return err;
}

int ipc_unpack_exit_rsp(unsigned char const *restrict buf, size_t bufsize) {
    if(bufsize < IPC_HEADER_SIZE) {
        return -E2BIG;
    }

    if(buf[0] == ipc_err) {
        return ipc_unpack_errno(buf, bufsize);
    }

    return -1 * (buf[1] != ipc_exit_rsp);
}

int ipc_unpack_temp_rsp(unsigned char const *restrict buf, size_t bufsize, unsigned char *temp) {
    if(bufsize < IPC_HEADER_SIZE + 1) {
        return -E2BIG;
    }
    if(buf[0] == ipc_err) {
        return ipc_unpack_errno(buf, bufsize);
    }

    if(buf[1] != ipc_temp_rsp) {
        return -EINVAL;
    }

    *temp = buf[2];

    return 0;
}

int ipc_unpack_speed_rsp(unsigned char const *restrict buf, size_t bufsize, unsigned char *speed) {
    if(bufsize < IPC_HEADER_SIZE + 1) {
        return -E2BIG;
    }
    if(buf[0] == ipc_err) {
        return ipc_unpack_errno(buf, bufsize);
    }

    if(buf[1] != ipc_temp_rsp) {
        return -EINVAL;
    }

    *speed = buf[2];
    return 0;
}

int ipc_unpack_matrix_rsp(unsigned char const* restrict buf, size_t bufsize, unsigned char *restrict matrix, unsigned char *nrows, size_t maxsize) {
    unsigned char rows;
    if(bufsize < IPC_HEADER_SIZE + 1) {
        return -E2BIG;
    }

    if(buf[0] == ipc_err) {
        return ipc_unpack_errno(buf, bufsize);
    }

    if(buf[1] != ipc_matrix_rsp) {
        return -EINVAL;
    }

    rows = buf[3];
    if(rows * 2 > maxsize || bufsize < rows + IPC_HEADER_SIZE  + 1) {
        return -1;
    }
    memcpy(matrix, &buf[4], rows);
    *nrows = rows;
    return 0;
}

