#ifndef CLIENT_H
#define CLIENT_H

#include "ipc.h"

#include <stddef.h>

#include <sys/types.h>

int client_init(void);
int client_kill(void);
ssize_t client_send_and_recv(unsigned char *buffer, size_t bufsize, ipc_request request);

#endif /* CLIENT_H */
