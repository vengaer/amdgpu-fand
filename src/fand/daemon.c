#include "config.h"
#include "daemon.h"
#include "fanctrl.h"
#include "fandcfg.h"
#include "filesystem.h"
#include "ipc.h"
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
        return -1;
    }

    if(pid > 0) {
        exit(0);
    }

    if(setsid() < 0) {
        return -1;
    }

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

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

static int daemon_init(bool fork, char const *config, struct fand_config *data, struct inotify_watch *watch) {
    signal(SIGINT, daemon_sig_handler);
    signal(SIGTERM, daemon_sig_handler);

    if(fork && daemon_fork()) {
        return -1;
    }

    openlog(0, !fork * LOG_PERROR, LOG_DAEMON);
    umask(0);

    if(mkdir(DAEMON_WORKING_DIR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
        syslog(LOG_ERR, "Failed to create working directory: %s", strerror(errno));
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

static int daemon_kill(struct inotify_watch const *watch) {
    int status = 0;

    if(fsys_watch_clear(watch)) {
        status = -1;
    }

    if(server_kill()) {
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

static int daemon_process_messages(struct fand_config *data) {
    unsigned char rspbuf[IPC_MAX_MSG_LENGTH];
    int rsplen, speed, temp;
    ssize_t nmessages;
    enum ipc_cmd cmd;

    nmessages = server_try_poll();
    if(nmessages < 0) {
        return -0;
    }

    for(int i = 0; i < nmessages; i++) {
        if(server_peek_request(&cmd)) {
            syslog(LOG_ERR, "Internal ipc processing error");
            return -1;
        }

        switch(cmd) {
            case ipc_exit_req:
                daemon_alive = 0;
                rsplen = ipc_pack_exit_rsp(rspbuf, sizeof(rspbuf));
                break;
            case ipc_speed_req:
                speed = fanctrl_get_speed();
                if(speed < 0) {
                    rsplen = ipc_pack_errno(rspbuf, sizeof(rspbuf), -speed);
                }
                else {
                    rsplen = ipc_pack_speed_rsp(rspbuf, sizeof(rspbuf), fanctrl_get_speed());
                }
                break;
            case ipc_temp_req:
                temp = fanctrl_get_temp();
                if(temp < 0) {
                    rsplen = ipc_pack_errno(rspbuf, sizeof(rspbuf), -temp);
                }
                else {
                    rsplen= ipc_pack_temp_rsp(rspbuf, sizeof(rspbuf), fanctrl_get_temp());
                }
                break;
            case ipc_matrix_req:
                rsplen = ipc_pack_matrix_rsp(rspbuf, sizeof(rspbuf), data->matrix, data->matrix_rows);
                break;
            default:
                syslog(LOG_WARNING, "Unexpected ipc command: %hhu", (unsigned char)cmd);
                continue;
        }
        if(rsplen < 0) {
            syslog(LOG_ERR, "Failed to serialize response to request %hhu", (unsigned char)cmd);
            return -1;
        }

        (void)server_respond(rspbuf, (size_t)rsplen);
    }

    return 0;
}

static inline int daemon_adjust_fanspeed(void) {
    int status = fanctrl_adjust();

    if(status == FAND_FATAL_ERR) {
        syslog(LOG_EMERG, "Fatal error encountered, exiting");
        daemon_alive = 0;
    }

    return status;
}

static inline void daemon_watch_event(char const *config, struct fand_config *data, struct inotify_watch *watch) {
    if(fsys_watch_event(config, watch)) {
        syslog(LOG_WARNING, "Failed to poll inotify events");
    }

    if(watch->triggered) {
        if(daemon_reload(config, data) == FAND_FATAL_ERR) {
            syslog(LOG_EMERG, "Fatal error encountered, exiting");
            daemon_alive = 0;
        }
    }
}

int daemon_main(bool fork, char const *config) {
    int status;
    struct fand_config data = { 0 };
    struct inotify_watch watch = {
        .fd = -1,
        .wd = -1,
        .flags = 0,
        .triggered = false
    };

    if(daemon_init(fork, config, &data, &watch)) {
        status = 1;
        daemon_alive = 0;
    }

    while(daemon_alive) {
        status = daemon_adjust_fanspeed();
        daemon_watch_event(config, &data, &watch);
        daemon_process_messages(&data);
        sleep(data.interval);
    }

    return status | daemon_kill(&watch);
}
