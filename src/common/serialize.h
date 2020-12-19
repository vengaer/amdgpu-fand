#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "fandcfg.h"

#include <stddef.h>

#include <sys/types.h>

union unpack_result {
    union {
        int temp;
        int speed;
        /* Fat pointer containing number of
         * rows followed by matrix values */
        unsigned char matrix[2 * MAX_TEMP_THRESHOLDS + 1];
    };
    int error;
};

ssize_t packf(unsigned char *restrict buffer, size_t bufsize, char const *restrict fmt, ...);
ssize_t unpackf(unsigned char const *restrict buffer, size_t bufsize, char const *restrict fmt, ...);

ssize_t pack_error(unsigned char *restrict buffer, size_t bufsize, int error);

ssize_t pack_speed(unsigned char *restrict buffer, size_t bufsize, int speed);
ssize_t unpack_speed(unsigned char const *restrict buffer, size_t bufsize, union unpack_result *restrict result);

ssize_t pack_temp(unsigned char *restrict buffer, size_t bufsize, int temp);
ssize_t unpack_temp(unsigned char const* restrict buffer, size_t bufsize, union unpack_result *restrict result);

ssize_t pack_matrix(unsigned char *restrict buffer, size_t bufsize, unsigned char const *restrict matrix, unsigned char nrows);
ssize_t unpack_matrix(unsigned char const *restrict buffer, size_t bufsize, union unpack_result *restrict result);

ssize_t pack_exit_rsp(unsigned char *restrict buffer, size_t bufsize);
ssize_t unpack_exit_rsp(unsigned char const *restrict buffer, size_t bufsize, union unpack_result *restrict result);

#endif /* SERIALIZE_H */
