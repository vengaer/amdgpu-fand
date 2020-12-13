#ifndef ARCH_H
#define ARCH_H

#include <stdbool.h>

static inline bool arch_is_little_endian(void) {
    return *(unsigned char *)&(unsigned short){ 1u };
}

#endif /* ARCH_H */
