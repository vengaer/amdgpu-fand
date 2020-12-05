#include "file.h"
#include "strutils.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <syslog.h>
#include <sys/file.h>
#include <unistd.h>

enum { FILE_ULONG_BUFSIZE = 16 };

FILE *fopen_excl(char const *path, char const *mode) {
    FILE *fp = fopen(path, mode);
    if(!fp) {
        syslog(LOG_ERR, "Could not open %s: %s", path, strerror(errno));
        return fp;
    }

    int fd = fileno(fp);
    if(flock(fd, LOCK_EX) == -1) {
        syslog(LOG_ERR, "Failed to acquire exclusive lock for %d: %s", fd, strerror(errno));
        fclose(fp);
        return 0;
    }

    return fp;
}

int fclose_excl(FILE *fp) {
    int fd = fileno(fp);
    if(flock(fd, LOCK_UN) == -1) {
        syslog(LOG_ERR, "Failed to release exclusive lock for %d: %s", fd, strerror(errno));
    }
    return fclose(fp);
}

int fwrite_ulong_excl(char const *path, unsigned long value) {
    FILE *fp = fopen_excl(path, "w");
    if(!fp) {
        return -1;
    }

    fprintf(fp, "%lu", value);

    return fclose_excl(fp);
}

int fread_ulong_excl(char const *path, unsigned long *value) {
    char buffer[FILE_ULONG_BUFSIZE] = { 0 };
    FILE *fp = fopen_excl(path, "r");
    if(!fp) {
        return -1;
    }

    {
        char *s = fgets(buffer, sizeof(buffer), fp);
        if(fclose_excl(fp)) {
            syslog(LOG_ERR, "Could not close %s: %s", path, strerror(errno));
        }
        if(!s) {
            syslog(LOG_ERR, "Could not read from %s", path);
            return -1;
        }
    }

    buffer[strcspn(buffer, "\n")] = '\0';

    return strstoul(buffer, value);
}

int fdopen_excl(char const *path, int mode) {
    int fd = open(path, mode);
    if(fd == -1) {
        syslog(LOG_ERR, "Could not open control mode file %s: %s", path, strerror(errno));
        return fd;
    }

    if(flock(fd, LOCK_EX) == -1) {
        syslog(LOG_WARNING, "Failed to acquire lock for %d: %s", fd, strerror(errno));
    }

    return fd;
}

int fdclose_excl(int fd) {
    if(flock(fd, LOCK_UN) == -1) {
        syslog(LOG_WARNING, "Failed to release lock for %d: %s", fd, strerror(errno));
    }

    if(close(fd)) {
        syslog(LOG_ERR, "Could not close fd %d: %s", fd, strerror(errno));
        return -1;
    }
    return 0;
}

int fdwrite_ulong(int fd, unsigned long value) {
    char buffer[FILE_ULONG_BUFSIZE];

    snprintf(buffer, sizeof(buffer), "%lu", value);
    if(write(fd, buffer, strlen(buffer)) == -1) {
        syslog(LOG_ERR, "Could not write value to fd %d: %s", fd, strerror(errno));
        return -1;
    }

    return 0;
}

int fdread_ulong(int fd, unsigned long *value) {
    char buffer[FILE_ULONG_BUFSIZE];
    unsigned byteidx = 0;

    for(;read(fd, &buffer[byteidx], 1) != -1; ++byteidx) {
        switch(buffer[byteidx]) {
            case '\n':
            case '\0':
                goto convert;
            default:
                /* NOP */
                break;
        }
    }
convert:
    return strstoul(buffer, value);
}
