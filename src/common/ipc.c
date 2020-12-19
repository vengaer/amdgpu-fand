#include "ipc.h"

unsigned char ipc_valid_requests[4] = {
    ipc_exit_req,
    ipc_speed_req,
    ipc_temp_req,
    ipc_matrix_req
};


