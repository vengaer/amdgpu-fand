#ifndef SERVER_H
#define SERVER_H

#include "config.h"

#define SRVCHLD_CON_RESET 0x100
#define SRVCHLD_RECV_ERR  0x200
#define SRVCHLD_PACK_ERR  0x400
#define SRVCHLD_SEND_ERR  0x800
#define SRVCHLD_INVAL     0x1000
#define SRVCHLD_EXIT      0x2000

int server_init(void);
int server_kill(void);
int server_recv_and_respond(int fd, struct fand_config const *config);
int server_poll(struct fand_config const *config);

#endif /* SERVER_H */
