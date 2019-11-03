#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdbool.h>
#include <stddef.h>

#include <unistd.h>

bool find_dir_matching_pattern(char *restrict dst, size_t count, char const *restrict pattern, char const *restrict parent);

static inline bool file_exists(char const *path) {
    return access(path, F_OK) != -1;
}

bool is_valid_hwmon_dir(char const *dir);

#endif
