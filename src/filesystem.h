#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdbool.h>
#include <stddef.h>

#define __USE_XOPEN_EXTENDED
#include <sys/types.h>
#include <unistd.h>

ssize_t parent_dir(char *restrict dst, char const *restrict src, size_t count);
bool find_dir_matching_pattern(char *restrict dst, size_t count, char const *restrict pattern, char const *restrict parent);

static inline bool file_exists(char const *path) {
    return access(path, F_OK) != -1;
}

ssize_t readlink_safe(char const *restrict link, char *restrict dst, size_t count);
ssize_t readlink_absolute(char const *restrict link, char *restrict dst, size_t count);

bool is_valid_hwmon_dir(char const *dir);

#endif
