#ifndef IPC_H
#define IPC_H

#define SOCK_DIR "/tmp/amdgpu-fanctl"
#define SERVER_SOCK_FILE SOCK_DIR"/amdgpu-fanctl-server.sock"
#define CLIENT_SOCK_FILE SOCK_DIR"/amdgpu-fanctl-client.sock"
#define IPC_BUF_SIZE 8192

enum ipc_request_type {
    ipc_get,
    ipc_set
};

enum ipc_request_value {
    ipc_temp,
    ipc_speed
};

struct ipc_request {
    enum ipc_request_type type;
    enum ipc_request_value value;
};

#endif
