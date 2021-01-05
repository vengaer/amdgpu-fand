#include "fanctrl.h"
#include "fandcfg.h"
#include "ipc.h"
#include "macro.h"
#include "serialize.h"
#include "server.h"
#include "strutils.h"

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <poll.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

enum { SRVBACKLOG = 8 };

static struct pollfd pollfd;

static int server_validate_request(int fd, ipc_request request) {
    struct ucred clientcreds;
    bool valid = false;

    for(unsigned i = 0; i < array_size(ipc_valid_requests); i++) {
        if(request == ipc_valid_requests[i]) {
            valid = true;
            break;
        }
    }

    if(!valid) {
        syslog(LOG_INFO, "Received invalid request %hhu", request);
        return EINVAL;
    }

    if(request < ipc_req_unprivileged) {
        if(getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &clientcreds, &(socklen_t){ sizeof(clientcreds) }) == -1) {
            syslog(LOG_ERR, "Could not get peer credentials: %s", strerror(errno));
            return errno;
        }
        if(clientcreds.uid) {
            syslog(LOG_INFO, "Attempt to execute privileged action %hhu by uid %d", request, (int)clientcreds.uid);
            return EACCES;
        }
    }

    return 0;
}

static ssize_t server_pack_result(unsigned char *buffer, size_t bufsize, ipc_request request) {
    int rspval;
    switch(request) {
        case ipc_req_speed:
            rspval = fanctrl_get_speed();
            break;
        case ipc_req_temp:
            rspval = fanctrl_get_temp();
            break;
        default:
            rspval = -1;
            break;
    }

    if(rspval < 0) {
        return pack_error(buffer, bufsize, EAGAIN);
    }
    return request == ipc_req_speed ? pack_speed(buffer, bufsize, rspval) :
                                      pack_temp(buffer, bufsize, rspval);
}

int server_init(void) {
    union unsockaddr srvaddr;
    int status = 0;

    int srvfd = socket(PF_UNIX, SOCK_STREAM, 0);
    if(srvfd == -1) {
        syslog(LOG_ERR, "Error while creating socket: %s", strerror(errno));
        return -1;
    }

    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.addr_un.sun_family = AF_UNIX;
    if(strscpy(srvaddr.addr_un.sun_path, DAEMON_SERVER_SOCKET, sizeof(srvaddr.addr_un.sun_path)) < 0) {
        syslog(LOG_ERR, "Socket path %s overflows the internal buffer", DAEMON_SERVER_SOCKET);
        status = -1;
        goto closesock;
    }

    if(bind(srvfd, &srvaddr.addr, sizeof(srvaddr)) == -1) {
        syslog(LOG_ERR, "Error while binding socket: %s", strerror(errno));
        status = -1;
        goto closesock;
    }

    if(chmod(DAEMON_SERVER_SOCKET, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) == -1) {
        syslog(LOG_ERR, "Error while changing socket permissions: %s", strerror(errno));
        status = -1;
        goto rmsock;
    }

    if(listen(srvfd, SRVBACKLOG) == -1) {
        syslog(LOG_ERR, "Error on listen: %s", strerror(errno));
        status = -1;
        goto rmsock;
    }

    pollfd.fd = srvfd;
    pollfd.events = POLLIN;

    syslog(LOG_INFO, "Opened socket: %s", DAEMON_SERVER_SOCKET);

    return status;

rmsock:
    unlink(DAEMON_SERVER_SOCKET);
closesock:
    close(srvfd);
    return status;
}

int server_kill(void) {
    int status = 0;
    if(close(pollfd.fd) == -1) {
        syslog(LOG_WARNING, "Error closing socket: %s", strerror(errno));
        status = -1;
    }

    if(unlink(DAEMON_SERVER_SOCKET) == -1) {
        syslog(LOG_WARNING, "Error unlinking socket: %s", strerror(errno));
        status = -1;
    }
    return status;
}

int server_recv_and_respond(int fd, struct fand_config const *config) {
    unsigned char buffer[IPC_MAX_MSG_LENGTH];
    ipc_request request;
    int status;
    int exitcode = 0;
    ssize_t rsplen;
    ssize_t nsent;
    ssize_t nbytes = recv(fd, &request, sizeof(request), 0);

    switch(nbytes) {
        case -1:
            syslog(LOG_ERR, "Error on recv: %s", strerror(errno));
            return 1;
        case 0:
            syslog(LOG_INFO, "Connection reset by peer");
            return 1;
        default:
            /* NOP */
            break;
    }

    status = server_validate_request(fd, request);

    if(status) {
        rsplen = pack_error(buffer, sizeof(buffer), status);
    }
    else {
        switch(request) {
            case ipc_req_exit:
                syslog(LOG_INFO, "Exit request received, signalling parent");
                rsplen = pack_exit_rsp(buffer, sizeof(buffer));
                exitcode = FAND_SERVER_EXIT;
                break;
            case ipc_req_speed:
                rsplen = server_pack_result(buffer, sizeof(buffer), request);
                break;
            case ipc_req_temp:
                rsplen = server_pack_result(buffer, sizeof(buffer), request);
                break;
            case ipc_req_matrix:
                rsplen = pack_matrix(buffer, sizeof(buffer), config->matrix, config->matrix_rows);
                break;
            default:
                syslog(LOG_WARNING, "Received invalid request %hhu, this should never happen!", request);
                rsplen = pack_error(buffer, sizeof(buffer), EINVAL);
                break;
        }
    }

    if(rsplen < 0) {
        syslog(LOG_ERR, "Error while packing response for %hhu", request);
        return exitcode | 1;
    }

    nsent = send(fd, buffer, rsplen, 0);

    if(nsent == -1) {
        syslog(LOG_ERR, "Error on send: %s", strerror(errno));
        exitcode |= 1;
    }

    return exitcode;
}


int server_poll(struct fand_config const *config) {
    union unsockaddr clientaddr;
    int newfd;
    int status = 0;

    int nready = poll(&pollfd, 1u, config->interval * 1000);

    switch(nready) {
        case -1:
            if(errno == EINTR) {
                /* SIGCHLD caught */
                return 0;
            }
            syslog(LOG_ERR, "Error on poll: %s", strerror(errno));
            return -1;
        case 0:
            return 0;
        default:
            /* NOP */
            break;
    }

    newfd = accept(pollfd.fd, &clientaddr.addr, &(socklen_t){ sizeof(clientaddr) });
    if(newfd == -1) {
        syslog(LOG_ERR, "Error while accepting client connection: %s", strerror(errno));
        return -1;
    }

#ifdef FAND_FUZZ_CONFIG
    // Fuzz targets may not call exit
    status = server_recv_and_respond(newfd, config);
#else
    int pid = fork();
    if(pid == -1) {
        syslog(LOG_ERR, "Unable to fork to respond to incoming connection: %s", strerror(errno));
        return -1;
    }

    if(!pid) {
        /* Child process */
        close(pollfd.fd);
        status = server_recv_and_respond(newfd, config);
        close(newfd);
        exit(status);
    }
#endif

    close(newfd);

    return status;
}
