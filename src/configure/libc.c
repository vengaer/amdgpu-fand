#include "libc.h"

#include <errno.h>
#include <stdio.h>

#include <regex.h>

enum { POPEN_BUFSIZE = 1024 };
enum { REGEX_ERRBUF_SIZE = 256 };

char const *libc_vendor_strings[3] = {
    "glibc",
    "musl",
    "unknown"
};

static inline int regcomp_desc(regex_t *regex, char const *pattern, int cflags, char const *desc) {
    char errbuf[REGEX_ERRBUF_SIZE];
    int reti = regcomp(regex, pattern, cflags);

    if(reti) {
        regerror(reti, regex, errbuf, sizeof(errbuf));
        fprintf(stderr, "Failed to compile %s regex: %s\n", desc, errbuf);
        return -1;
    }

    return 0;
}

enum libc_vendor libc_get_vendor(void) {
    char buffer[POPEN_BUFSIZE];

    enum libc_vendor vendor = libc_vendor_unknown;

    regex_t glibc_regex;
    regex_t musl_regex;

    if(regcomp_desc(&glibc_regex, "GNU\\s+libc", REG_EXTENDED | REG_ICASE, "glibc")) {
        return vendor;
    }

    if(regcomp_desc(&musl_regex, "musl\\s+libc", REG_EXTENDED | REG_ICASE, "musl")) {
        goto glibc_cleanup;
    }

    /* errno might be set by internal calls made in popen.
    * If it is indeed set, we might as well use the information */
    int prev_errno = errno;
    /* musl's ldd writes its version message to stderr */
    FILE *fp = popen("/usr/bin/env ldd --version 2>&1", "r");

    if(!fp) {
        if(errno != prev_errno) {
            perror("Error on popen");
        }
        else {
            fputs("Error on popen\n", stderr);
        }
        goto musl_cleanup;
    }

    while(fgets(buffer, sizeof(buffer), fp)) {
        if(regexec(&glibc_regex, buffer, 0, 0, 0) == 0) {
            vendor = libc_vendor_gnu;
            break;
        }
        else if(regexec(&musl_regex, buffer, 0, 0, 0) == 0) {
            vendor = libc_vendor_musl;
            break;
        }
    }

    pclose(fp);
musl_cleanup:
    regfree(&musl_regex);
glibc_cleanup:
    regfree(&glibc_regex);

    return vendor;
}
