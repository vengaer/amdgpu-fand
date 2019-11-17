#ifndef STRUTILS_H
#define STRUTILS_H

#include <stddef.h>

#include <errno.h>
#include <sys/types.h>

char *strncpy_fast(char *restrict dst, char const *restrict src, size_t n);

ssize_t strscpy(char *restrict dst, char const *restrict src, size_t count);
ssize_t strscat(char *restrict dst, char const *restrict src, size_t count);

ssize_t strsncpy(char *restrict dst, char const *restrict src, size_t n, size_t count);

void replace_char(char *string, char from, char to);

#endif
