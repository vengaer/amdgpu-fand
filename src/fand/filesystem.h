#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdbool.h>
#include <stddef.h>

#include <sys/types.h>

bool fsys_dir_exists(char const *path);
bool fsys_file_exists(char const *path);
ssize_t fsys_append(char *stem, char const *app, size_t bufsize);

#endif /* FILESYSTEM_H */
