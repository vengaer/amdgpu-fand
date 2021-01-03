#include "ipc.h"
#include "macro.h"
#include "serialize.h"

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#define DFA_ACCEPT   0x1u
#define DFA_REJECT   0x2u
#define DFA_HALF0    0x4u
#define DFA_HALF1    0x8u
#define DFA_LONG0    0x10u
#define DFA_LONG1    0x20u
#define DFA_SIGNED   0x40u
#define DFA_UNSIGNED 0x80u

#define DFA_FMTMASK (DFA_HALF0 | DFA_HALF1 | DFA_LONG0 | DFA_LONG1 | DFA_SIGNED | DFA_UNSIGNED)

enum {
    FMT_HHD, FMT_HD, FMT_D, FMT_LD, FMT_LLD,
    FMT_HHU, FMT_HU, FMT_U, FMT_LU, FMT_LLU
};

enum { DFA_ANY = 128 };

union packtype {
    signed char hhd;
    short hd;
    int d;
    long ld;
    long long lld;
    unsigned char hhu;
    unsigned short hu;
    unsigned u;
    unsigned long lu;
    unsigned long long llu;
};

struct dfa_state;

struct dfa_edge {
    unsigned char c;
    struct dfa_state const *end;
};

struct dfa_state {
    unsigned char flags;
    struct dfa_edge edges[5];
};

static struct dfa_state const dfa_reject = {
    .flags = DFA_REJECT,
    .edges = { { 0 } }
};

/* @dot: signed -> accept [ label="epsilon" ] */
static struct dfa_state const dfa_state_d = {
    .flags = DFA_ACCEPT | DFA_SIGNED,
    .edges = { { 0 } }
};

/* @dot: unsigned -> accept [ label="epsilon" ] */
static struct dfa_state const dfa_state_u = {
    .flags = DFA_ACCEPT | DFA_UNSIGNED,
    .edges = { { 0 } }
};

/* @dot: long_long -> signed   [ label="d" ] */
/* @dot: long_long -> unsigned [ label="u" ] */
/* @dot: long_long -> reject   [ label="." ] */
static struct dfa_state const dfa_state_l1 = {
    .flags = DFA_LONG1,
    .edges = {
        { .c = 'd',     .end = &dfa_state_d },
        { .c = 'u',     .end = &dfa_state_u },
        { .c = DFA_ANY, .end = &dfa_reject },
        { 0 }
    }
};

/* @dot: long -> signed    [ label="d" ] */
/* @dot: long -> unsigned  [ label="u" ] */
/* @dot: long -> long_long [ label="l" ] */
/* @dot: long -> reject    [ label="." ] */
static struct dfa_state const dfa_state_l0 = {
    .flags = DFA_LONG0,
    .edges = {
        { .c = 'd',     .end = &dfa_state_d  },
        { .c = 'u',     .end = &dfa_state_u  },
        { .c = 'l',     .end = &dfa_state_l1 },
        { .c = DFA_ANY, .end = &dfa_reject  },
        { 0 }
    }
};

/* @dot: half_half -> signed   [ label="d" ] */
/* @dot: half_half -> unsigned [ label="u" ] */
/* @dot: half_half -> reject   [ label="." ] */
static struct dfa_state const dfa_state_h1 = {
    .flags = DFA_HALF1,
    .edges = {
        { .c = 'd',     .end = &dfa_state_d },
        { .c = 'u',     .end = &dfa_state_u },
        { .c = DFA_ANY, .end = &dfa_reject },
        { 0 }
    }
};

/* @dot: half -> half_half [ label="h" ] */
/* @dot: half -> signed    [ label="d" ] */
/* @dot: half -> unsigned  [ label="u" ] */
/* @dot: half -> reject    [ label="." ] */
static struct dfa_state const dfa_state_h0 = {
    .flags = DFA_HALF0,
    .edges = {
        { .c = 'h',     .end = &dfa_state_h1 },
        { .c = 'd',     .end = &dfa_state_d  },
        { .c = 'u',     .end = &dfa_state_u  },
        { .c = DFA_ANY, .end = &dfa_reject  },
        { 0 }
    }
};

/* @dot: start -> half     [ label="h" ] */
/* @dot: start -> long     [ label="l" ] */
/* @dot: start -> signed   [ label="d" ] */
/* @dot: start -> unsigned [ label="u" ] */
/* @dot: start -> reject   [ label="." ] */
static struct dfa_state const dfa_start = {
    .flags = 0,
    .edges = {
        { .c = 'h',     .end = &dfa_state_h0 },
        { .c = 'l',     .end = &dfa_state_l0 },
        { .c = 'd',     .end = &dfa_state_d  },
        { .c = 'u',     .end = &dfa_state_u  },
        { .c = DFA_ANY, .end = &dfa_reject  }
    }
};

static inline bool dfa_edge_match(char ic, unsigned char ec) {
    return ec == DFA_ANY || (unsigned char)ic == ec;
}

static inline bool dfa_bitflag_set(unsigned char flags, unsigned char bit) {
    return !!(flags & bit);
}

static inline bool dfa_accept(unsigned char flags) {
    return dfa_bitflag_set(flags, DFA_ACCEPT);
}


static inline int dfa_flags_to_fmttype(unsigned char flags) {
    switch(flags & DFA_FMTMASK) {
        case DFA_SIGNED   | DFA_LONG0 | DFA_LONG1:
        case DFA_SIGNED   | DFA_HALF0 | DFA_HALF1:
        case DFA_SIGNED   | DFA_LONG0:
        case DFA_SIGNED   | DFA_HALF0:
        case DFA_SIGNED:
        case DFA_UNSIGNED | DFA_LONG0 | DFA_LONG1:
        case DFA_UNSIGNED | DFA_HALF0 | DFA_HALF1:
        case DFA_UNSIGNED | DFA_LONG0:
        case DFA_UNSIGNED | DFA_HALF0:
        case DFA_UNSIGNED:
            /* NOP */
            break;
        default:
            /* Invalid combination of flags */
            return -1;
    }
    return dfa_bitflag_set(flags, DFA_SIGNED)   * 2    +
           dfa_bitflag_set(flags, DFA_UNSIGNED) * 7    +
           dfa_bitflag_set(flags, DFA_HALF0)    * (-1) +
           dfa_bitflag_set(flags, DFA_HALF1)    * (-1) +
           dfa_bitflag_set(flags, DFA_LONG0)           +
           dfa_bitflag_set(flags, DFA_LONG1);
}

static int dfa_simulate(char const *restrict string) {
    struct dfa_state const *current = &dfa_start;
    unsigned char flags = 0;

    while(true) {
        for(unsigned i = 0; i < array_size(current->edges); i++) {
            flags |= current->flags;

            if(dfa_accept(flags)) {
                goto loop_end;
            }

            if(!current->edges[i].c) {
                /* Invalid path */
                return -1;
            }

            if(dfa_edge_match(*string, current->edges[i].c)) {
                current = current->edges[i].end;
                break;
            }
        }

        if(!*string) {
            break;
        }

        ++string;
    }

loop_end:
    if(!dfa_accept(flags)) {
        return -1;
    }
    return dfa_flags_to_fmttype(flags);
}

static inline size_t dfa_valsize(int fmttype) {
    static size_t sizelist[] = {
        sizeof(unsigned char),
        sizeof(unsigned short),
        sizeof(unsigned),
        sizeof(unsigned long),
        sizeof(unsigned long long)
    };

    return sizelist[fmttype % array_size(sizelist)];
}

static inline unsigned dfa_fmtlen(int fmttype) {
    static size_t fmtlist[] = {
        3u, 2u, 1u, 2u, 3u
    };
    return fmtlist[fmttype % array_size(fmtlist)];
}

static int valist_strip_integral(int fmttype, union packtype *restrict packval,  va_list args, unsigned *restrict valsize, unsigned *restrict fmtlen) {
    switch(fmttype) {
        case FMT_HHD:
            packval->hhd = va_arg(args, int);
            break;
        case FMT_HD:
            packval->hd  = va_arg(args, int);
            break;
        case FMT_D:
            packval->d   = va_arg(args, int);
            break;
        case FMT_LD:
            packval->ld  = va_arg(args, long);
            break;
        case FMT_LLD:
            packval->lld = va_arg(args, long long);
            break;
        case FMT_HHU:
            packval->hhu = va_arg(args, int);
            break;
        case FMT_HU:
            packval->hu  = va_arg(args, int);
            break;
        case FMT_U:
            packval->u   = va_arg(args, unsigned);
            break;
        case FMT_LU:
            packval->lu  = va_arg(args, unsigned long);
            break;
        case FMT_LLU:
            packval->llu = va_arg(args, unsigned long long);
            break;
        default:
            return -1;
    }
    *valsize = dfa_valsize(fmttype);
    *fmtlen = dfa_fmtlen(fmttype);
    return 0;
}

static int valist_strip_pointer(int fmttype, void **out,  va_list args, unsigned *restrict valsize, unsigned *restrict fmtlen) {
    switch(fmttype) {
        case FMT_HHD:
            *out = va_arg(args, signed char *);
            break;
        case FMT_HD:
            *out = va_arg(args, short *);
            break;
        case FMT_D:
            *out = va_arg(args, int *);
            break;
        case FMT_LD:
            *out = va_arg(args, long *);
            break;
        case FMT_LLD:
            *out = va_arg(args, long long *);
            break;
        case FMT_HHU:
            *out = va_arg(args, unsigned char *);
            break;
        case FMT_HU:
            *out = va_arg(args, unsigned short *);
            break;
        case FMT_U:
            *out = va_arg(args, unsigned *);
            break;
        case FMT_LU:
            *out = va_arg(args, unsigned long *);
            break;
        case FMT_LLU:
            *out = va_arg(args, unsigned long long *);
            break;
        default:
            return -1;
    }
    *valsize = dfa_valsize(fmttype);
    *fmtlen = dfa_fmtlen(fmttype);
    return 0;
}

ssize_t packf(unsigned char *restrict buffer, size_t bufsize, char const *restrict fmt, ...) {
    int fmttype;
    unsigned valsize;
    unsigned fmtlen;
    unsigned nrepeat;

    ssize_t nwritten = 0;

    union {
        union packtype pv;
        void *pptr;
    } packval;

    void *srcaddr;
    ssize_t nbytes;

    va_list args;
    va_start(args, fmt);

    while(*fmt) {
        nrepeat = 1u;

        if(*fmt++ != '%') {
            nwritten = -1;
            goto cleanup;
        }

        if(*fmt == '*') {
            nrepeat = va_arg(args, unsigned);
            if(!nrepeat) {
                nwritten = -1;
                goto cleanup;
            }

            ++fmt;
        }

        fmttype = dfa_simulate(fmt);
        if(nrepeat == 1) {
            if(valist_strip_integral(fmttype, &packval.pv, args, &valsize, &fmtlen) < 0) {
                nwritten = -1;
                goto cleanup;
            }
            srcaddr = &packval.pv;
        }
        else {
            if(valist_strip_pointer(fmttype, &packval.pptr, args, &valsize, &fmtlen) < 0) {
                nwritten = -1;
                goto cleanup;
            }
            srcaddr = packval.pptr;
        }

        nbytes = nrepeat * valsize;

        if((size_t)nwritten + nbytes > bufsize) {
            nwritten = -E2BIG;
            goto cleanup;
        }
        memcpy(&buffer[nwritten], srcaddr, nbytes);
        nwritten += nbytes;
        fmt += fmtlen;

    }

cleanup:
    va_end(args);
    return nwritten;
}

ssize_t unpackf(unsigned char const *restrict buffer, size_t bufsize, char const *restrict fmt, ...) {
    int fmttype;
    unsigned fmtlen;
    unsigned valsize;
    unsigned nrepeat;

    ssize_t nread = 0;

    void *packval;
    ssize_t nbytes;

    va_list args;
    va_start(args, fmt);

    while(*fmt) {
        nrepeat = 1u;

        if(*fmt++ != '%') {
            nread = -1;
            goto cleanup;
        }

        if(*fmt == '*') {
            nrepeat = va_arg(args, unsigned);
            if(!nrepeat) {
                nread = -1;
                goto cleanup;
            }

            ++fmt;
        }

        fmttype = dfa_simulate(fmt);
        if(valist_strip_pointer(fmttype, &packval, args, &valsize, &fmtlen) < 0) {
            nread = -1;
            goto cleanup;
        }

        nbytes = nrepeat * valsize;

        if((size_t)nread + nbytes > bufsize) {
            nread = -E2BIG;
            goto cleanup;
        }

        memcpy(packval, &buffer[nread], nbytes);
        nread += nbytes;
        fmt += fmtlen;
    }

cleanup:
    va_end(args);
    return nread;
}

static ssize_t unpack_int(unsigned char const *restrict buffer, size_t bufsize, union unpack_result *restrict result) {
    unsigned char len;
    ipc_response rsp;
    ssize_t rsplen = unpackf(buffer, bufsize, "%hhu%hhu", &len, &rsp);

    if(rsplen < 0) {
        return rsplen;
    }

    if(rsp) {
        rsplen += unpackf(&buffer[rsplen], bufsize - rsplen, "%d", &result->error);
    }
    else {
        rsplen += unpackf(&buffer[rsplen], bufsize - rsplen, "%d", &result->temp);
    }

    if(rsplen != len) {
        return -1;
    }

    return rsp;
}

ssize_t pack_error(unsigned char *restrict buffer, size_t bufsize, int error) {
    return packf(buffer, bufsize, "%hhu%hhu%d", sizeof(unsigned char) + sizeof(ipc_response) + sizeof(error), ipc_rsp_err, error);
}

ssize_t pack_speed(unsigned char *restrict buffer, size_t bufsize, int speed) {
    return packf(buffer, bufsize, "%hhu%hhu%d", sizeof(unsigned char) + sizeof(ipc_response) + sizeof(speed), ipc_rsp_ok, speed);
}

ssize_t unpack_speed(unsigned char const *restrict buffer, size_t bufsize, union unpack_result *restrict result) {
    return unpack_int(buffer, bufsize, result);
}

ssize_t pack_temp(unsigned char *restrict buffer, size_t bufsize, int temp) {
    return packf(buffer, bufsize, "%hhu%hhu%d", sizeof(unsigned char) + sizeof(ipc_response) + sizeof(temp), ipc_rsp_ok, temp);
}

ssize_t unpack_temp(unsigned char const* restrict buffer, size_t bufsize, union unpack_result *restrict result) {
    return unpack_int(buffer, bufsize, result);
}

ssize_t pack_matrix(unsigned char *restrict buffer, size_t bufsize, unsigned char const *restrict matrix, unsigned char nrows) {
    unsigned char const len = sizeof(ipc_response) + sizeof(unsigned char) * (nrows * 2 + 2);
    return packf(buffer, bufsize, "%hhu%hhu%hhu%*hhu", len, ipc_rsp_ok, nrows, 2 * nrows, matrix);
}

ssize_t unpack_matrix(unsigned char const *restrict buffer, size_t bufsize, union unpack_result *restrict result) {
    unsigned char len;
    ipc_response rsp;
    ssize_t rsplen = unpackf(buffer, bufsize, "%hhu%hhu", &len, &rsp);
    if(rsplen < 0) {
        return rsplen;
    }

    if(rsp) {
        rsplen += unpackf(&buffer[rsplen], bufsize - rsplen, "%d", &result->error);
    }
    else {
        rsplen += unpackf(&buffer[rsplen], bufsize - rsplen, "%hhu", &result->matrix[0]);
        if(rsplen < 0) {
            return rsplen;
        }

        rsplen += unpackf(&buffer[rsplen], bufsize - rsplen, "%*hhu", 2 * result->matrix[0], &result->matrix[1]);
    }

    if(rsplen < 0) {
        return rsplen;
    }

    if(rsplen != len) {
        return -1;
    }

    return rsp;
}

ssize_t pack_exit_rsp(unsigned char *restrict buffer, size_t bufsize) {
    return packf(buffer, bufsize, "%hhu%hhu", sizeof(unsigned char) + sizeof(ipc_response), ipc_rsp_ok);
}

ssize_t unpack_exit_rsp(unsigned char const *restrict buffer, size_t bufsize, union unpack_result *restrict result) {
    unsigned char len;
    ipc_response rsp;
    ssize_t rsplen = unpackf(buffer, bufsize, "%hhu%hhu", &len, &rsp);

    if(rsplen < 0) {
        return rsplen;
    }

    if(rsp) {
        rsplen += unpackf(&buffer[rsplen], bufsize - rsplen, "%d", &result->error);
    }

    if(len != rsplen) {
        return -1;
    }

    return rsp;
}
