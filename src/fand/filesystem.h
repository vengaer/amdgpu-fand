#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdbool.h>
#include <stddef.h>

#include <sys/inotify.h>
#include <sys/types.h>

struct inotify_watch {
    int fd;
    int wd;
    int flags;
    bool triggered;
};

bool fsys_dir_exists(char const *path);
bool fsys_file_exists(char const *path);
ssize_t fsys_append(char *stem, char const *app, size_t bufsize);

int fsys_watch_init(char const *path, struct inotify_watch *watch, int flags);
int fsys_watch_event(char const *path, struct inotify_watch *watch);
int fsys_watch_clear(struct inotify_watch const *watch);

ssize_t fsys_abspath(char *dst, char const *path, size_t dstsize);

#endif /* FILESYSTEM_H */
