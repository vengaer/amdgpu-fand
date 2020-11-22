#include "strutils.h"

ssize_t strscpy(char *restrict dst, char const *restrict src, size_t n) {
    size_t const srclen = strlen(src);
    size_t i = n;
    char *d = dst;

    while(i-- && (*d++ = *src++));

    if(srclen >= n) {
        dst[n - 1] = '\0';
        return -E2BIG;
    }

    return srclen;
}
