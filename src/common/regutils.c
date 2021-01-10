#include "regutils.h"

#include <syslog.h>

enum { REGEX_ERRBUF_SIZE = 128 };

int regcomp_info(regex_t *restrict regex, char const *restrict pattern, int cflags, char const *restrict desc) {
    char errbuf[REGEX_ERRBUF_SIZE];
    int reti = regcomp(regex, pattern, cflags);

    if(reti) {
        regerror(reti, regex, errbuf, sizeof(errbuf));
        syslog(LOG_ERR, "Failed to compile %s regex: %s", desc, errbuf);
        return -1;
    }

    return 0;
}
