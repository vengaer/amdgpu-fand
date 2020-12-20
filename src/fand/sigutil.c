#include "sigutil.h"
#include "strutils.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <syslog.h>

sig_atomic_t volatile sigbits = 0;

int sigutil_sethandler(int signal, int flags, sighandler_function handler) {
    static_assert(sizeof(sig_atomic_t) * CHAR_BIT >= 16, "sig_atomic_t must be capable of holding at least 15 bits");

    struct sigaction sa;

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = flags;
    if(sigaction(signal, &sa, 0) == -1) {
        syslog(LOG_ERR, "Failed to set %s handler: %s", strsignal(signal), strerror(errno));
        return -1;
    }
    return 0;
}

void sigutil_catch(int signal) {
    switch(signal) {
        case SIGHUP:
            sigbits |= SIGHUP_CAUGHT;
            break;
        case SIGPIPE:
            sigbits |= SIGPIPE_CAUGHT;
            break;
        default:
            sigbits |= SIGUTIL_ERR;
            break;
    }
}


ssize_t sigutil_geterr(char *buffer, size_t bufsize) {
    ssize_t nwritten = bufsize + 1;
    if(sigbits & CHLDRECV_ERR) {
        nwritten = snprintf(buffer, bufsize, "Error on recv: %s", strerror(sigbits & SIGUTIL_VALMASK));
    }
    else if(sigbits & CHLDSEND_ERR) {
        nwritten = snprintf(buffer, bufsize, "Error on send: %s", strerror(sigbits & SIGUTIL_VALMASK));
    }
    else if(sigbits & CHLDINVAL) {
        nwritten = snprintf(buffer, bufsize, "Received invalid request %d, this should have been caught earlier", sigbits & SIGUTIL_VALMASK);
    }
    else if(sigbits & CHLDPACK_ERR) {
        nwritten = strscpy(buffer, "Could not pack response", bufsize);
    }
    else if(sigbits & SIGUTIL_ERR) {
        nwritten = strscpy(buffer, "Internal sigutil error", bufsize);
    }

    return ((size_t)nwritten >= bufsize) * -E2BIG;
}
