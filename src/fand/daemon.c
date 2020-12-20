#include "config.h"
#include "daemon.h"
#include "fanctrl.h"
#include "fandcfg.h"
#include "filesystem.h"
#include "ipc.h"
#include "pidfile.h"
#include "server.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <syslog.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static sig_atomic_t volatile daemon_alive = 1;
static sig_atomic_t volatile sighup_recvd = 0;

static void daemon_kill(void) {
    daemon_alive = 0;
}

static void daemon_sig_handler(int signal) {
    int childstatus;

    switch(signal) {
        case SIGINT:
        case SIGTERM:
            daemon_kill();
            break;
        case SIGPIPE:
            syslog(LOG_WARNING, "Broken pipe");
            break;
        case SIGHUP:
            sighup_recvd = 1;
            break;
        case SIGCHLD:
            while(waitpid(-1, &childstatus, WNOHANG) > 0);

            if(WEXITSTATUS(childstatus) == FAND_SERVER_EXIT) {
                daemon_alive = 0;
            }

            break;
    }
}

static int daemon_sigset(int signal, int flags) {
    struct sigaction sa;

    sa.sa_handler = daemon_sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = flags;
    if(sigaction(signal, &sa, 0) == -1) {
        syslog(LOG_ERR, "Failed to set %s handler: %s", strsignal(signal), strerror(errno));
        return -1;
    }
    return 0;
}

static inline int daemon_set_sigacts(void) {
    return daemon_sigset(SIGINT,  SA_RESTART) |
           daemon_sigset(SIGTERM, SA_RESTART) |
           daemon_sigset(SIGPIPE, SA_RESTART) |
           daemon_sigset(SIGHUP,  SA_RESTART) |
           daemon_sigset(SIGCHLD, SA_RESTART);
}

static int daemon_fork(void) {
    pid_t pid = fork();

    if(pid < 0) {
        return -1;
    }

    if(pid > 0) {
        exit(0);
    }

    if(setsid() < 0) {
        return -1;
    }

    pid = fork();

    if(pid < 0) {
        return -1;
    }

    if(pid > 0) {
        exit(0);
    }

    for(int fd = sysconf(_SC_OPEN_MAX); fd >= 0; --fd) {
        close(fd);
    }

    return 0;
}

static void daemon_openlog(bool fork, bool verbose) {
    /* Enable LOG_WARNING through LOG_EMERG by default,
     * if verbose is true, enable LOG_NOTICE, LOG_INFO and
     * LOG_DEBUG as well */
    int mask = 0x1f | (0xe0 * verbose);

#ifdef FAND_FUZZ_CONFIG
    /* Don't log at all while fuzzing */
    mask = 0;
#endif

    setlogmask(mask);
    openlog(0, !fork * LOG_PERROR, LOG_DAEMON);
}

static int daemon_init(bool fork, bool verbose, char const *config, struct fand_config *data, struct inotify_watch *watch) {
    daemon_openlog(fork, verbose);

    if(daemon_set_sigacts()) {
        return -1;
    }

    if(fork && daemon_fork()) {
        return -1;
    }
    umask(0);

    if(mkdir(DAEMON_WORKING_DIR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
        syslog(LOG_ERR, "Failed to create working directory: %s", strerror(errno));
        return -1;
    }

    if(pidfile_write()) {
        return -1;
    }

    if(chdir(DAEMON_WORKING_DIR)) {
        syslog(LOG_ERR, "Failed to set working directory: %s", strerror(errno));
        return -1;
    }

    if(config_parse(config, data)) {
        return -1;
    }

    if(server_init()) {
        return -1;
    }

    if(fsys_watch_init(config, watch, IN_MODIFY)) {
        return -1;
    }

    if(fanctrl_init() < 0) {
        syslog(LOG_EMERG, "Fancontroller initialization failed");
        return -1;
    }

    if(fanctrl_configure(data) < 0) {
        syslog(LOG_EMERG, "Fancontroller configuration failed");
        return -1;
    }

    return 0;
}

static int daemon_reload(char const *path, struct fand_config *data) {
    struct fand_config tmpdata;

    if(config_parse(path, &tmpdata)) {
        syslog(LOG_WARNING, "Failed to reload config");
        return -1;
    }
    *data = tmpdata;

    if(fanctrl_configure(data)) {
        syslog(LOG_EMERG, "Fancontroller reconfiguration failed");
        return FAND_FATAL_ERR;
    }

    syslog(LOG_INFO, "Config reloaded");

    return 0;
}

static int daemon_free(struct inotify_watch const *watch) {
    int status = 0;

    if(fsys_watch_clear(watch)) {
        status = -1;
    }

    if(server_kill()) {
        status = -1;
    }

    if(pidfile_unlink()) {
        status = -1;
    }

    if(rmdir(DAEMON_WORKING_DIR)) {
        syslog(LOG_ERR, "Failed to remove working directory: %s", strerror(errno));
        status = -1;
    }

    if(fanctrl_release()) {
        syslog(LOG_EMERG, "Could not release control of fans");
        status = -1;
    }

    closelog();
    return status;
}

static inline int daemon_adjust_fanspeed(void) {
    int status = fanctrl_adjust();

    if(status == FAND_FATAL_ERR) {
        syslog(LOG_EMERG, "Fatal error encountered, exiting");
        daemon_kill();
    }

    return status;
}

static inline void daemon_watch_event(char const *config, struct fand_config *data, struct inotify_watch *watch) {
    if(fsys_watch_event(config, watch)) {
        syslog(LOG_WARNING, "Failed to poll inotify events");
    }

    /* Reload on SIGHUP or if config has been modified */
    if(sighup_recvd || watch->triggered) {
        sighup_recvd = 0;

        if(daemon_reload(config, data) == FAND_FATAL_ERR) {
            syslog(LOG_EMERG, "Fatal error encountered, exiting");
            daemon_kill();
        }
    }
}

int daemon_main(bool fork, bool verbose, char const *config) {
    int status;
    struct fand_config data = { 0 };
    struct inotify_watch watch = {
        .fd = -1,
        .wd = -1,
        .flags = 0,
        .triggered = false
    };

    if(daemon_init(fork, verbose, config, &data, &watch)) {
        status = 1;
        daemon_kill();
    }

    while(daemon_alive) {
        status = daemon_adjust_fanspeed();
        daemon_watch_event(config, &data, &watch);
        server_poll(&data);
    }

    return status | daemon_free(&watch);
}
