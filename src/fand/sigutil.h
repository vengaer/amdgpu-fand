#ifndef SIGUTIL_H
#define SIGUTIL_H

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>

#include <sys/types.h>

typedef void (*sighandler)(int);

int sigutil_sethandler(int signal, int flags, sighandler handler);

#endif /* SIGUTIL_H */
