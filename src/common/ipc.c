#include "ipc.h"

ipc_request ipc_valid_requests[4] = {
    ipc_req_exit,
    ipc_req_speed,
    ipc_req_temp,
    ipc_req_matrix
};

struct ipc_pair ipc_request_map[4] = {
    { "speed",       ipc_req_speed  },
    { "temp",        ipc_req_temp   },
    { "temperature", ipc_req_temp   },
    { "matrix",      ipc_req_matrix }
};
