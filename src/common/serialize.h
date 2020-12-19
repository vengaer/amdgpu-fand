#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <stddef.h>

#include <sys/types.h>

ssize_t packf(unsigned char *restrict buffer, size_t bufsize, char const *restrict fmt, ...);
ssize_t unpackf(unsigned char const *restrict buffer, size_t bufsize, char const *restrict fmt, ...);

#endif /* SERIALIZE_H */
