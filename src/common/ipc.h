#ifndef IPC_H
#define IPC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <sys/types.h>

#define SOCK_DIR "/tmp/amdgpu-fanctl"
#define SERVER_SOCK_RESTRICT_FILE SOCK_DIR"/amdgpu-fanctl-server-restr.sock"
#define CLIENT_SOCK_RESTRICT_FILE SOCK_DIR"/amdgpu-fanctl-client-restr.sock"
#define SERVER_SOCK_FILE SOCK_DIR"/amdgpu-fanctl-server.sock"
#define CLIENT_SOCK_FILE SOCK_DIR"/amdgpu-fanctl-client.sock"
#define IPC_BUF_SIZE 512

#define IPC_MATRIX_OVERFLOW 0x80
#define IPC_EMPTY_REQUEST_FIELD 0x200
#define IPC_PERCENTAGE_BIT 0x400

extern char const *ipc_request_type_value[4];
extern char const *ipc_request_target_value[5];

enum ipc_request_type {
    ipc_get,
    ipc_set,
    ipc_reset,
    ipc_invalid_type
};

enum ipc_request_target {
    ipc_temp,
    ipc_speed,
    ipc_matrix,
    ipc_pwm_path,
    ipc_invalid_target
};

enum ipc_request_state {
    ipc_invalid_state,
    ipc_server_state,
    ipc_client_state
};

struct ipc_request {
    enum ipc_request_type type;
    enum ipc_request_target target;
    int16_t value;
    pid_t ppid;
};

bool parse_ipc_param(char const *request_param, size_t param_idx, struct ipc_request *result);
enum ipc_request_state get_ipc_state(struct ipc_request *request);

bool ipc_server_running(void);

#endif
