#include "defs.h"
#include "ipc.h"
#include "server.h"
#include "strutils.h"

#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

static int server_fd;

int server_init(void) {
    union unsockaddr addr;

    server_fd = socket(PF_UNIX, SOCK_DGRAM, 0);
    if(server_fd == -1) {
        syslog(LOG_ERR, "Failed to create socket: %s", strerror(errno));
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.addr_un.sun_family = AF_UNIX;
    if(strscpy(addr.addr_un.sun_path, SERVER_SOCKET, sizeof(addr.addr_un.sun_path)) < 0) {
        syslog(LOG_ERR, "Socket path %s overflows the socket buffer", SERVER_SOCKET);
        (void)server_kill();
        return 1;
    }

    if(bind(server_fd, (struct sockaddr *)&addr.addr_un, sizeof(addr.addr_un)) == -1) {
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
