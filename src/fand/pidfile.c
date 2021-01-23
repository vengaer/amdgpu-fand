#include "pidfile.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FAND_PIDFILE_PATH "/var/run/amdgpu-fand/pid"

enum { PIDBUF_SIZE = 32 };

static pid_t pidfile_get_existing(void) {
    char buffer[PIDBUF_SIZE];
    pid_t pid = 0;

    int fd = open(FAND_PIDFILE_PATH, O_RDONLY);

    if(fd == -1) {
        syslog(LOG_ERR, "Could not open existing PID file: %s", strerror(errno));
        return -1;
    }

    if(read(fd, buffer, sizeof(buffer)) == -1) {
        syslog(LOG_ERR, "Could not read existing PID file: %s", strerror(errno));
        pid = -1;
        goto cleanup;
    }

    char *endp;
    pid = strtoul(buffer, &endp, 10);

    if(*endp != '\n') {
        syslog(LOG_WARNING, "PID file corrupted");
    }

    if(!pid) {
        pid = -1;
    }

cleanup:
    close(fd);
    return pid;
}

int pidfile_write(void) {
    char buffer[PIDBUF_SIZE];
    int fd;
    pid_t pid;
    ssize_t pidlen;
    ssize_t nwritten;
    int status = 0;

    fd = open(FAND_PIDFILE_PATH, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if(fd == -1) {
        if(errno == EEXIST) {
            syslog(LOG_ERR, "Daemon already running, PID: %lld", (long long)pidfile_get_existing());
        }
        else {
            syslog(LOG_ERR, "Opening PID file failed: %s", strerror(errno));
        }
        return -1;
    }

    if(flock(fd, LOCK_EX) == -1) {
        syslog(LOG_WARNING, "Could not lock PID file: %s", strerror(errno));
    }

    pid = getpid();

    if((size_t)snprintf(buffer,  sizeof(buffer), "%lld\n", (long long)pid) >= sizeof(buffer)) {
        syslog(LOG_ERR, "PID %lld overflows the internal buffer", (long long)pid);
        status = -1;
        goto cleanup;
    }
    pidlen = strlen(buffer);
    nwritten = write(fd, buffer, pidlen);
    if(nwritten == -1) {
        syslog(LOG_ERR, "Could not write PID file: %s", strerror(errno));
        status = -1;
        goto cleanup;
    }
    else if(nwritten != pidlen) {
        syslog(LOG_ERR, "PID file truncated, could only write %ld/%zu bytes", (long)nwritten, pidlen);
        status = -1;
        goto cleanup;
    }

    if(chmod(FAND_PIDFILE_PATH, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1) {
        syslog(LOG_ERR, "Error while setting PID file permissions: %s", strerror(errno));
        status = -1;
        goto cleanup;
    }

cleanup:
    close(fd);
    return status;
}

int pidfile_unlink(void) {
    int status = 0;

    if(unlink(FAND_PIDFILE_PATH) == -1) {
        syslog(LOG_ERR, "PID file unlinking failed: %s", strerror(errno));
        status = -1;
    }

    return status;
}
