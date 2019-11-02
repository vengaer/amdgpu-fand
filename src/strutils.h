#ifndef STRUTILS_H
#define STRUTILS_H

#include <stddef.h>

#ifndef E2BIG
#define E2BIG 7
#endif

int strscpy(char *restrict dst, char const *restrict src, size_t count);
int strscat(char *restrict dst, char const *restrict src, size_t count);

void replace_char(char *string, char from, char to);

#endif
