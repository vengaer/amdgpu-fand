#ifndef SERVER_H
#define SERVER_H

#include "ipc.h"

#include <stddef.h>

#include <sys/types.h>

int server_init(void);
int server_kill(void);
int server_peek_request(enum ipc_cmd *cmd);
ssize_t server_try_poll(void);
ssize_t server_respond(unsigned char const *buffer, size_t bufsize);

#endif /* SERVER_H */
