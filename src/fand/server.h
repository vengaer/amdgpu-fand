#ifndef SERVER_H
#define SERVER_H

#include "config.h"

enum { FAND_SERVER_EXIT = 3 };

int server_init(void);
int server_kill(void);
int server_poll(struct fand_config const *config);

#endif /* SERVER_H */
