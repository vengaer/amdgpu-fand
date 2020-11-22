#include "ipc.h"

unsigned char ipc_valid_requests[4] = {
    ipc_exit,
    ipc_get_speed,
    ipc_get_temp,
    ipc_get_matrix
};
