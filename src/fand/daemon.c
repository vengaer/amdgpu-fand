#include "daemon.h"

#include <signal.h>
#include <stdlib.h>

#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DAEMON_WORKING_DIR "/var/run/amdgpu-fand"

static sig_atomic_t volatile daemon_alive = 1;

static void daemon_sig_handler(int signal) {
    switch(signal) {
        case SIGCHLD:
            /* TODO */
            break;
        case SIGHUP:
            /* TODO */
            break;
        case SIGINT:
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

    signal(SIGCHLD, daemon_sig_handler);
    signal(SIGHUP, daemon_sig_handler);

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
    if(fork && daemon_fork()) {
        return 1;
    }

    umask(0);
    mkdir(DAEMON_WORKING_DIR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

    chdir(DAEMON_WORKING_DIR);

    openlog(0, !fork * LOG_PERROR, LOG_DAEMON);

    return 0;
}

static void daemon_kill(void) {
    closelog();
}

bool daemon_main(bool fork) {
    signal(SIGINT, daemon_sig_handler);

    if(daemon_init(fork)) {
        return false;
    }

    while(daemon_alive) {

    }

    daemon_kill();
    return true;
}
