#include "defs.h"
#include "macro.h"
#include "server.h"
#include "strutils.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <fcntl.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

enum { SERVER_QUEUE_SIZE = 32 };

struct ipc_record {
    union unsockaddr sender;
    enum ipc_cmd cmd;
};

struct server_record_queue {
    unsigned idx;
    unsigned size;
    struct ipc_record records[SERVER_QUEUE_SIZE];
};

static int server_fd;
static struct server_record_queue record_queue;

static inline void record_queue_flush(void) {
    memset(&record_queue, 0, sizeof(record_queue));
}

static int record_queue_push(struct ipc_record const *record) {
    if(record_queue.size - record_queue.idx >= SERVER_QUEUE_SIZE) {
        return 1;
    }
    record_queue.records[record_queue.size++ % SERVER_QUEUE_SIZE] = *record;
    return 0;
}

static int record_queue_peek(struct ipc_record *record) {
    if(record_queue.idx == record_queue.size) {
        record_queue_flush();
        return 1;
    }

    *record = record_queue.records[record_queue.idx % SERVER_QUEUE_SIZE];
    return 0;
}

static int record_queue_pop(struct ipc_record *record) {
    if(record_queue.idx == record_queue.size) {
        record_queue_flush();
        return 1;
    }
    *record = record_queue.records[record_queue.idx++ % SERVER_QUEUE_SIZE];

    if(record_queue.idx == record_queue.size) {
        record_queue_flush();
    }
j
    return 0;
}

static ssize_t server_send(unsigned char const* buffer, size_t bufsize, union unsockaddr *receiver) {
    ssize_t nsent;

    if(bufsize > IPC_MAX_MSG_LENGTH) {
        syslog(LOG_ERR, "Response of %zu bytes overflows response buffer (%zu bytes)", bufsize, (size_t)IPC_MAX_MSG_LENGTH);
        return 1;
    }

    nsent = sendto(server_fd, buffer, bufsize, 0, &receiver->addr, (socklen_t)sizeof(receiver->addr));

    if(nsent == -1) {
        syslog(LOG_ERR, "Failed to send response: %s", strerror(errno));
    }

    return nsent;
}

static int server_validate_request(enum ipc_cmd cmd) {
    int err;
    bool valid = false;
    struct ucred client_creds;

    for(unsigned i = 0; i < array_size(ipc_valid_requests); i++) {
        if(cmd == ipc_valid_requests[i]) {
            valid = true;
            break;
        }
    }

    if(!valid) {
        syslog(LOG_INFO, "Received invalid request: %hhu", (unsigned char)cmd);
        return EINVAL;
    }

    if(cmd < ipc_unprivileged) {
        if(getsockopt(server_fd, SOL_SOCKET, SO_PEERCRED, &client_creds, &(socklen_t){ sizeof(client_creds) }) == -1) {
            err = errno;
            syslog(LOG_ERR, "Failed to get peer credential information: %s", strerror(err));
            return err;
        }
        if(client_creds.uid) {
            syslog(LOG_INFO, "Attempt to execute privileged command %hhu without permission, ignoring", (unsigned char)cmd);
            return EACCES;
        }
    }

    return 0;
}

static ssize_t server_invalid_request(int err, union unsockaddr *receiver) {
    unsigned char rspbuf[sizeof(err) + 1];
    unsigned rsplen = sizeof(rspbuf);

    rspbuf[0] = ipc_err;
    memcpy(&rspbuf[1], &err, sizeof(err));

    return server_send(rspbuf, rsplen, receiver);
}

int server_init(void) {
    static_assert(IPC_MAX_MSG_LENGTH >= MAX_TEMP_THRESHOLDS + 2, "Insufficient IPC buffer size");

    union unsockaddr sockaddr;

    server_fd = socket(PF_UNIX, SOCK_DGRAM, 0);
    if(server_fd == -1) {
        syslog(LOG_ERR, "Failed to create socket: %s", strerror(errno));
        return 1;
    }

    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.addr_un.sun_family = AF_UNIX;
    if(strscpy(sockaddr.addr_un.sun_path, SERVER_SOCKET, sizeof(sockaddr.addr_un.sun_path)) < 0) {
        syslog(LOG_ERR, "Socket path %s overflows the socket buffer", SERVER_SOCKET);
        (void)server_kill();
        return 1;
    }

    if(bind(server_fd, &sockaddr.addr, sizeof(sockaddr.addr)) == -1) {
        syslog(LOG_ERR, "Failed to bind socket %s: %s", SERVER_SOCKET, strerror(errno));
        (void)server_kill();
        return 1;
    }

    if(fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL) | O_NONBLOCK) == -1) {
        syslog(LOG_ERR, "Failed to make socket non-blocking: %s", strerror(errno));
        (void)server_kill();
        return 1;
    }

    if(chmod(SERVER_SOCKET, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) {
        syslog(LOG_ERR, "Failed to set socket permissions: %s", strerror(errno));
        (void)server_kill();
        return 1;
    }

    syslog(LOG_INFO, "Opened socket %s", SERVER_SOCKET);
    return 0;
}

ssize_t server_try_poll(void) {
    static unsigned char buffer[IPC_MAX_MSG_LENGTH];
    union unsockaddr client;
    int err;
    ssize_t nbytes;
    ssize_t ntotal = 0;

    while(1) {
        nbytes = recvfrom(server_fd, buffer, sizeof(buffer), 0, &client.addr, &(socklen_t){ sizeof(client) });

        switch(nbytes) {
            case -1:
                syslog(LOG_ERR, "Failed to read from socket: %s", strerror(errno));
                ntotal = -1;
                /* fallthrough */
            case 0:
                goto ret;

        }

        for(unsigned i = 0; i < nbytes; i++) {
            err = server_validate_request(buffer[i]);
            if(err) {
                record_queue_flush();
                (void)server_invalid_request(err, &client);
                ntotal = -1;
                goto ret;
            }
            if(record_queue_push(&(struct ipc_record){ client, buffer[i] })) {
                syslog(LOG_ERR, "Failed to enqueue ipc record: queue is full");
                goto ret;
            }
        }

        ntotal += nbytes;
    }

ret:
    return ntotal;
}

int server_kill(void) {
    int rv = 0;
    if(close(server_fd) == -1) {
        syslog(LOG_ERR, "Failed to close server socket: %s", strerror(errno));
        rv = 1;
    }
    if(unlink(SERVER_SOCKET) == -1) {
        syslog(LOG_ERR, "Failed to unlink server socket: %s", strerror(errno));
        rv = 1;
    }
    return rv;
}

int server_peek_request(enum ipc_cmd *cmd) {
    struct ipc_record record;
    if(record_queue_peek(&record)) {
        return 1;
    }
    *cmd = record.cmd;
    return 0;
}

ssize_t server_respond(unsigned char const *buffer, size_t bufsize) {
    struct ipc_record record;
    if(record_queue_pop(&record)) {
        return 1;
    }

    return server_send(buffer, bufsize, &record.sender);
}
