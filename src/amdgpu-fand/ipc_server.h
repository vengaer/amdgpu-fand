#ifndef IPC_SERVER_H
#define IPC_SERVER_H

#include <stdbool.h>

bool ipc_server_open_socket(void);
void ipc_server_close_socket(void);
void ipc_server_handle_request(void);

#endif
