#ifndef STRUTILS_H
#define STRUTILS_H

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <sys/types.h>

ssize_t strscpy(char *restrict dst, char const *restrict src, size_t dstlen);
ssize_t strsncpy(char *restrict dst, char const *restrict src, size_t dstlen, size_t n);
void strtolower(char *str);

#endif /* STRUTILS_H */
