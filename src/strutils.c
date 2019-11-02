#include "strutils.h"

#include <stddef.h>
#include <string.h>

int strscpy(char *restrict dst, char const *restrict src, size_t count) {
    size_t const src_len = strlen(src);

    strncpy(dst, src, count);
    dst[count - 1] = '\0';

    if(src_len >= count) {
        return -E2BIG;
    }

    return src_len;
}

int strscat(char *restrict dst, char const *restrict src, size_t count) {
    size_t const src_len = strlen(src);
    size_t const dst_len = strlen(dst);
    size_t const rem = count - dst_len;

    strncat(dst, src, rem);
    dst[count - 1] = '\0';

    if(src_len >= rem) {
        return -E2BIG;
    }

    return src_len;
}

int strsncpy(char *restrict dst, char const *restrict src, size_t count, size_t max_count) {
    size_t const src_len = strlen(src);

    if(count > src_len) {
        dst[0] = '\0';
        return -E2BIG;
    }

    strncpy(dst, src, count);
    dst[max_count - 1] = '\0';

    if(count >= max_count) {
        return -E2BIG;
    }

    return count;
}

void replace_char(char *string, char from, char to) {
    for(size_t i = 0; i < strlen(string); i++) {
        if(string[i] == from) {
            string[i] = to;
        }
    }
}
