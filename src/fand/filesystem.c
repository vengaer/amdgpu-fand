#include "filesystem.h"
#include "strutils.h"

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <syslog.h>
#include <sys/stat.h>
#include <unistd.h>

enum { FSYS_PATH_MAX_LENGTH = 256 };
enum { INOTIFY_BUF_LENGTH = 32 * sizeof(struct inotify_event) };

static int fsys_strip_filename(char *path) {
    char *dir = strrchr(path, '/');
    if(!dir) {
        return -ENOENT;
    }

    *dir = '\0';
    return 0;
}

static inline int fsys_check_entry(char const *path) {
    struct stat sb;
    if(stat(path, &sb)) {
        return errno;
    }
    return 0;
}

bool fsys_dir_exists(char const *path) {
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
}

bool fsys_file_exists(char const *path) {
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISREG(sb.st_mode);
}

ssize_t fsys_append(char *stem, char const *app, size_t bufsize) {

    ssize_t pos = strlen(stem);
    pos += strscpy(stem + pos, "/", bufsize - pos);
    if(pos < 0) {
        return pos;
    }
    pos += strscpy(stem + pos, app, bufsize - pos);
    if(pos < 0) {
        return pos;
    }

    return pos;
}

int fsys_watch_init(char const *path, struct inotify_watch *watch, int flags) {
    char abspath[FSYS_PATH_MAX_LENGTH];
    int err = fsys_check_entry(path);
    if(err) {
        syslog(LOG_ERR, "Failed to initiate watch: %s", strerror(err));
        return -1;
    }

    if(fsys_abspath(abspath, path, sizeof(abspath)) < 0) {
        syslog(LOG_ERR, "Could not determine absolue path of %s", path);
        return -1;
    }

    if(!fsys_dir_exists(abspath)) {
        err = fsys_strip_filename(abspath);
        if(err) {
            syslog(LOG_ERR, "%s does not appear to be a path", abspath);
            return -1;
        }
    }

    if(!fsys_dir_exists(abspath)) {
        syslog(LOG_ERR, "Could not determine directory to watch");
        return -1;
    }

    int fd = inotify_init1(IN_NONBLOCK);
    if(fd == -1) {
        syslog(LOG_ERR, "Could not initialize inotify: %s", strerror(errno));
        return -1;
    }

    int wd = inotify_add_watch(fd, abspath, flags);
    if(wd == -1) {
        syslog(LOG_ERR, "Failed to add inotify watch: %s", strerror(errno));
        close(fd);
        return -1;
    }

    watch->fd = fd;
    watch->wd = wd;
    watch->flags = flags;
    watch->triggered = false;

    return 0;
}

int fsys_watch_event(char const *path, struct inotify_watch *watch) {
    unsigned char inotify_buf[INOTIFY_BUF_LENGTH];
    unsigned char *ibufp;
    struct inotify_event *inevent;
    ssize_t nbytes;
    char const *filename;

    watch->triggered = false;

    filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

    nbytes = read(watch->fd, inotify_buf, sizeof(inotify_buf));
    if(nbytes == -1) {
        return 0;
    }

    ibufp = inotify_buf;
    while(ibufp < inotify_buf + nbytes) {
        inevent = (struct inotify_event *)ibufp;
        if((inevent->mask & watch->flags) && inevent->len && strcmp(inevent->name, filename) == 0) {
            watch->triggered = true;
            break;
        }
        ibufp += sizeof(struct inotify_event) + inevent->len;
    }

    return 0;
}

int fsys_watch_clear(struct inotify_watch const *watch) {
    if(watch->fd == -1 && watch->wd == -1) {
        return 0;
    }
    int status = 0;

    if(inotify_rm_watch(watch->fd, watch->wd) == -1) {
        syslog(LOG_ERR, "Could not remove inotify watch: %s", strerror(errno));
        status = -1;
    }
    if(close(watch->fd) == -1) {
        syslog(LOG_ERR, "Could not close inotify fd: %s", strerror(errno));
        status = -1;
    }
    return status;
}

ssize_t fsys_abspath(char *dst, char const *path, size_t dstsize) {
    char buffer[PATH_MAX];
    if(!realpath(path, buffer)) {
        syslog(LOG_ERR, "Failed to determine absolute path of %s: %s", path, strerror(errno));
        return -1;
    }

    ssize_t nbytes = strscpy(dst, buffer, dstsize);
    if(nbytes < 0) {
        syslog(LOG_ERR, "Absolue path %s overflows buffer (size %zu)", path, dstsize);
        return -1;
    }

    return nbytes;
}
