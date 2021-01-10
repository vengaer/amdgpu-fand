#ifndef CLIENT_MOCK_H
#define CLIENT_MOCK_H

#include "ipc.h"

#include <stddef.h>

#include <sys/types.h>

void mock_client_send_and_recv(ssize_t(*mock)(unsigned char *, size_t, ipc_request));

#endif /* CLIENT_MOCK_H */
