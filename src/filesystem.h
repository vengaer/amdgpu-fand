#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdbool.h>
#include <stddef.h>

#include <sys/types.h>

ssize_t parent_dir(char *restrict dst, char const *restrict src, size_t count);
bool find_dir_matching_pattern(char *restrict dst, size_t count, char const *restrict pattern, char const *restrict parent);

bool file_exists(char const *path);
bool file_accessible(char const *path, int amode, int *errnum);

ssize_t readlink_safe(char const *restrict link, char *restrict dst, size_t count);
ssize_t readlink_absolute(char const *restrict link, char *restrict dst, size_t count);

int rmdir_force(char const *path);

#endif
