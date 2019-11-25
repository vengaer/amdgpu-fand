#ifndef IPC_CLIENT_H
#define IPC_CLIENT_H

#include "ipc.h"

#include <stdbool.h>
#include <stddef.h>

#include <sys/types.h>

bool ipc_client_open_socket(void);
void ipc_client_close_socket(void);
ssize_t ipc_client_send_request(char *response, struct ipc_request *request, size_t count);

bool ipc_client_handle_request(struct ipc_request *request);

#endif
