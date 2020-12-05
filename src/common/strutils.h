#ifndef STRUTILS_H
#define STRUTILS_H

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <sys/types.h>

ssize_t strscpy(char *restrict dst, char const *restrict src, size_t dstlen);
ssize_t strsncpy(char *restrict dst, char const *restrict src, size_t dstlen, size_t n);
void strtolower(char *str);

ssize_t strstoul(char const *restrict src, unsigned long *dst);
ssize_t strstoul_range(char const *restrict src, unsigned long *dst, unsigned long low, unsigned long high);

#endif /* STRUTILS_H */
