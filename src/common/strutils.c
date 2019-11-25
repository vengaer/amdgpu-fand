#include "strutils.h"

#include <stddef.h>
#include <string.h>

char *strncpy_fast(char *restrict dst, char const *restrict src, size_t n) {
    char *d = dst;
    if(n) {
        char const *s = src;
        while((*d++ = *s++) && --n);
    }
    return d;
}

ssize_t strscpy(char *restrict dst, char const *restrict src, size_t count) {
    size_t const src_len = strlen(src);
    strncpy_fast(dst, src, count);

    if(src_len >= count) {
        dst[count - 1] = '\0';
        return -E2BIG;
    }

    return src_len;
}

ssize_t strscat(char *restrict dst, char const *restrict src, size_t count) {
    size_t const src_len = strlen(src);
    size_t const dst_len = strlen(dst);
    size_t const rem = count - dst_len - 1;

    strncat(dst, src, rem);

    return src_len >= rem ? -E2BIG : (ssize_t)src_len;
}

ssize_t strsncpy(char *restrict dst, char const *restrict src, size_t n, size_t count) {
    size_t const src_bytes = strlen(src) + 1;
    size_t const nbytes = src_bytes < n ? src_bytes : n;

    if(nbytes >= count) {
        strncpy_fast(dst, src, count - 1);
        dst[count - 1] = '\0';
        return -E2BIG;
    }
    strncpy_fast(dst, src, nbytes);

    if(nbytes < src_bytes) {
        dst[nbytes] = '\0';
    }

    return nbytes;
}

void replace_char(char *string, char from, char to) {
    for(size_t i = 0; i < strlen(string); i++) {
        if(string[i] == from) {
            string[i] = to;
        }
    }
}
