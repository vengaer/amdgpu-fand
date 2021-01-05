#ifndef LIBC_H
#define LIBC_H

#include <stddef.h>

enum libc_vendor {
    libc_vendor_gnu,
    libc_vendor_musl,
    libc_vendor_unknown
};

extern char const *libc_vendor_strings[3];

enum libc_vendor libc_get_vendor(void);

#endif /* LIBC_H */
