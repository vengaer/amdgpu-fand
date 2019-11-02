#ifndef STRUTILS_H
#define STRUTILS_H

#include <stddef.h>

#ifndef E2BIG
#define E2BIG 7
#endif

int strscat(char *restrict dst, char const *restrict src, size_t count);

#endif
