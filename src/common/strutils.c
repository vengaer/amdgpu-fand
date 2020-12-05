#include "strutils.h"

#include <ctype.h>

ssize_t strscpy(char *restrict dst, char const *restrict src, size_t dstlen) {
    size_t const srclen = strlen(src);
    size_t i = dstlen;
    char *d = dst;

    while(i-- && (*d++ = *src++));

    if(srclen >= dstlen) {
        dst[dstlen - 1] = '\0';
        return -E2BIG;
    }

    return srclen;
}

ssize_t strsncpy(char *restrict dst, char const *restrict src, size_t dstlen, size_t n) {
    size_t const srclen = strlen(src);
    size_t i = dstlen;
    size_t nn = n;
    char *d = dst;

    while(i-- && nn-- && (*d++ = *src++));

    if(n >= dstlen) {
        dst[dstlen - 1] = '\0';
        return -E2BIG;
    }
    if(srclen < n) {
        return -E2BIG;
    }

    dst[n] = '\0';

    return n;
}

void strtolower(char *str) {
    char c;
    while(*str) {
        c = tolower((unsigned char)*str);
        *str++ = c;
    }
}
