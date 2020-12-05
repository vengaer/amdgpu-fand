#ifndef FILE_H
#define FILE_H

#include <stdio.h>

FILE *fopen_excl(char const *path, char const *mode);
int fclose_excl(FILE *fp);
int fwrite_ulong_excl(char const *path, unsigned long value);
int fread_ulong_excl(char const *path, unsigned long *value);

int fdopen_excl(char const *path, int mode);
int fdclose_excl(int fd);
int fdwrite_ulong(int fd, unsigned long value);
int fdread_ulong(int fd, unsigned long *value);

#endif /* FILE_H */
