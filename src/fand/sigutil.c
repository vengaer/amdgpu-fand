#include "sigutil.h"

#include <errno.h>
#include <string.h>

#include <syslog.h>

sig_atomic_t volatile sigbits = 0;

int sigutil_sethandler(int signal, int flags, sighandler_function handler) {

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
