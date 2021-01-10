#ifndef REGUTILS_H
#define REGUTILS_H

#include <regex.h>

int regcomp_info(regex_t *restrict regex, char const *restrict pattern, int cflags, char const *restrict desc);

#endif /* REGUTILS_H */
