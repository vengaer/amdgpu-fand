#ifndef SIGUTIL_H
#define SIGUTIL_H

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>

#include <sys/types.h>

#define SIGPIPE_CAUGHT  0x100u
#define SIGHUP_CAUGHT   0x200u

/* Bits set in exit code when catching SIGCHLD */
#define CHLDRECV_ERR    0x400u
#define CHLDSEND_ERR    0x800u
#define CHLDINVAL       0x1000u
#define CHLDEXIT        0x2000u
#define CHLDPACK_ERR    0x4000u

#define SIGUTIL_ERR     0x8000u

#define SIGUTIL_ERRMASK 0xdc00
#define SIGUTIL_VALMASK 0xffu

extern sig_atomic_t volatile sigbits;

typedef void (*sighandler_function)(int);

int sigutil_sethandler(int signal, int flags, sighandler_function handler);
void sigutil_catch(int signal);

ssize_t sigutil_geterr(char *buffer, size_t bufsize);

static inline bool sigutil_sigpipe_caught(void) {
    return sigbits & SIGPIPE_CAUGHT;
}

static inline bool sigutil_sighup_caught(void) {
    return sigbits & SIGHUP_CAUGHT;
}

static inline bool sigutil_has_error(void) {
    return sigbits & SIGUTIL_ERRMASK;
}

static inline bool sigutil_exit(void) {
    return sigbits & CHLDEXIT;
}


#endif /* SIGUTIL_H */
