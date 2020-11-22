#include "daemon.h"
#include "defs.h"
#include "server.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static sig_atomic_t volatile daemon_alive = 1;

static void daemon_sig_handler(int signal) {
    switch(signal) {
        case SIGINT:
        case SIGTERM:
            daemon_alive = 0;
            break;
    }
}

static int daemon_fork(void) {
    pid_t pid = fork();

    if(pid < 0) {
        return 1;
    }

    if(pid > 0) {
        exit(0);
    }

    if(setsid() < 0) {
        return 1;
    }

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    pid = fork();

    if(pid < 0) {
        return 1;
    }

    if(pid > 0) {
        exit(0);
    }

    for(int fd = sysconf(_SC_OPEN_MAX); fd >= 0; --fd) {
        close(fd);
    }

    return 0;
}

static int daemon_init(bool fork) {
    signal(SIGINT, daemon_sig_handler);
    signal(SIGTERM, daemon_sig_handler);

    if(fork && daemon_fork()) {
        return 1;
    }

    openlog(0, !fork * LOG_PERROR, LOG_DAEMON);

    umask(0);

    if(mkdir(DAEMON_WORKING_DIR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
        syslog(LOG_ERR, "Failed to create working directory: %s", strerror(errno));
        return 1;
    }

    if(chdir(DAEMON_WORKING_DIR)) {
        syslog(LOG_ERR, "Failed to set working directory: %s", strerror(errno));
        return 1;
    }

    return server_init();
}

static int daemon_kill(void) {
    int rv = 0;

    if(server_kill()) {
        rv = 1;
    }
    if(rmdir(DAEMON_WORKING_DIR)) {
        syslog(LOG_ERR, "Failed to remove working directory: %s", strerror(errno));
        rv = 1;
    }

    closelog();
    return rv;
}

int daemon_main(bool fork) {
    if(daemon_init(fork)) {
        return 1;
    }

    while(daemon_alive) {

    }

    return daemon_kill();
}
