#ifndef SERVER_H
#define SERVER_H

#include "config.h"

int server_init(void);
int server_kill(void);
int server_recv_and_respond(int fd, struct fand_config const *config);
int server_poll(struct fand_config const *config);

#endif /* SERVER_H */
