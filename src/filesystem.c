#include "filesystem.h"
#include "hwmon.h"
#include "logger.h"
#include "strutils.h"

#include <stdio.h>
#include <string.h>

#include <dirent.h>
#include <errno.h>
#include <regex.h>
#include <unistd.h>

#define CURRENT_DIR "./"
#define CURRENT_DIR_SIZE 2

static ssize_t absolute_path(char *restrict dst, char const *restrict working_dir, char const *restrict path, size_t count) {
    char buffer[HWMON_PATH_LEN];
    char *c;

    regex_t path_rgx;
    int reti = regcomp(&path_rgx, "^(\\.){2}/", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile regex for detecting path paths\n");
        return -1;
    }

    if(strscpy(buffer, working_dir, sizeof buffer) < 0) {
        fprintf(stderr, "Working directory overflows the buffer\n");
        return -E2BIG;
    }

    while(regexec(&path_rgx, path, 0, NULL, 0) == 0) {
        path = strchr(path, '/') + 1;
        c = strrchr(buffer, '/');
        if(c) {
            *c = '\0';
        }
    }
    if(strscat(dst, buffer, count) < 0 || strscat(dst, "/", count) < 0 || strscat(dst, path, count) < 0) {
        fprintf(stderr, "Absolute path overflows the buffer\n");
        return -E2BIG;
    }
    return strlen(dst);
}

ssize_t parent_dir(char *restrict dst, char const *restrict src, size_t count) {
    char const *end = strrchr(src, '/');
    if(!end) {
        if(strscpy(dst, CURRENT_DIR, count) < 0) {
            return -E2BIG;
        }
        return CURRENT_DIR_SIZE;
    }
    ssize_t const nbytes = strsncpy(dst, src, end - src, count);
    if(nbytes < 0) {
        fprintf(stderr, "Parent dir does not fit in buffer\n");
        return -E2BIG;
    }
    return nbytes;
}


bool find_dir_matching_pattern(char *restrict dst, size_t count, char const *restrict pattern, char const *restrict parent) {
    regex_t rgx;
    int reti;

    reti = regcomp(&rgx, pattern, REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile regex %s\n", pattern);
        return false;
    }

    struct dirent *entry;
    DIR *sdir = opendir(parent);
    if(!sdir) {
        int const errnum = errno;
        fprintf(stderr, "Failed to open directory %s: %s\n", parent, strerror(errnum));
        return false;
    }

    while((entry = readdir(sdir))) {
        if(entry->d_type == DT_DIR && regexec(&rgx, entry->d_name, 0, NULL, 0) == 0) {
            if(strscpy(dst, entry->d_name, count) < 0) {
                fprintf(stderr, "Directory %s overflowed buffer\n", entry->d_name);
                closedir(sdir);
                return false;
            }
            closedir(sdir);
            return true;
        }
    }

    closedir(sdir);
    return false;
}

bool file_exists(char const *path) {
    return access(path, F_OK) != -1;
}

bool file_accessible(char const *path, int amode, int *errnum) {
    if(access(path, amode) == 0) {
        if(errnum) {
            *errnum = 0;
        }
        return true;
    }

    if(errnum) {
        *errnum = errno;
    }
    LOG(VERBOSITY_LVL2, "Access denied: %s\n", strerror(errno));
    return false;
}

ssize_t readlink_safe(char const *restrict link, char *restrict dst, size_t count) {
    ssize_t len = readlink(link, dst, count);
    if(len == -1) {
        int const errnum = errno;
        fprintf(stderr, "Failed to read symlink %s: %s\n", link, strerror(errnum));
        dst[0] = '\0';
        return -E2BIG;
    }
    dst[len] = '\0';

    return (size_t)len == count ? -E2BIG : len;
}

ssize_t readlink_absolute(char const *restrict link, char *restrict dst, size_t count) {
    char dir[HWMON_PATH_LEN];
    char relative[HWMON_PATH_LEN];

    if(parent_dir(dir, link, sizeof dir) < 0) {
        fprintf(stderr, "Parent dir of %s overflows the buffer\n", link);
        return -E2BIG;
    }

    if(readlink_safe(link, relative, sizeof relative) < 0) {
        fprintf(stderr, "Link path %s overflows the buffer\n", link);
        return -E2BIG;
    }

    ssize_t const len = absolute_path(dst, dir, relative, count);

    if(len < 0) {
        fprintf(stderr, "Absolute path of symbolic link overflows the buffer\n");
        return -E2BIG;
    }
    return len;
}

