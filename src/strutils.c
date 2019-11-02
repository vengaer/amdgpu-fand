#include "strutils.h"

#include <string.h>

int strscat(char *restrict dst, char const *restrict src, size_t count) {
    size_t const src_len = strlen(src);
    size_t const dst_len = strlen(dst);

    strncat(dst, src, count - dst_len);
    dst[count - 1] = '\0';

    if(src_len >= count - dst_len) {
        return -E2BIG;
    }

    return src_len;
}
