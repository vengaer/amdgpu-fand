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

struct dfa_node;

struct dfa_edge {
    unsigned char c;
    struct dfa_node *end;
};

struct dfa_node {
    unsigned char flags;
    struct dfa_edge edges[5];
};

static struct dfa_node dfa_reject = {
    .flags = DFA_REJECT,
    .edges = { { 0 } }
};

static struct dfa_node dfa_node_d = {
    .flags = DFA_ACCEPT | DFA_SIGNED,
    .edges = { { 0 } }
};

static struct dfa_node dfa_node_u = {
    .flags = DFA_ACCEPT | DFA_UNSIGNED,
    .edges = { { 0 } }
};

static struct dfa_node dfa_node_l1 = {
    .flags = DFA_LONG1,
    .edges = {
        { .c = 'd',     .end = &dfa_node_d },
        { .c = 'u',     .end = &dfa_node_u },
        { .c = DFA_ANY, .end = &dfa_reject },
        { 0 }
    }
};

static struct dfa_node dfa_node_l0 = {
    .flags = DFA_LONG0,
    .edges = {
        { .c = 'd',     .end = &dfa_node_d  },
        { .c = 'u',     .end = &dfa_node_u  },
        { .c = 'l',     .end = &dfa_node_l1 },
        { .c = DFA_ANY, .end = &dfa_reject  },
        { 0 }
    }
};

static struct dfa_node dfa_node_h1 = {
    .flags = DFA_HALF1,
    .edges = {
        { .c = 'd',     .end = &dfa_node_d },
        { .c = 'u',     .end = &dfa_node_u },
        { .c = DFA_ANY, .end = &dfa_reject },
        { 0 }
    }
};

static struct dfa_node dfa_node_h0 = {
    .flags = DFA_HALF0,
    .edges = {
        { .c = 'h',     .end = &dfa_node_h1 },
        { .c = 'd',     .end = &dfa_node_d  },
        { .c = 'u',     .end = &dfa_node_u  },
        { .c = DFA_ANY, .end = &dfa_reject  },
        { 0 }
    }
};

static struct dfa_node dfa_start = {
    .flags = 0,
    .edges = {
        { .c = 'h',     .end = &dfa_node_h0 },
        { .c = 'l',     .end = &dfa_node_l0 },
        { .c = 'd',     .end = &dfa_node_d  },
        { .c = 'u',     .end = &dfa_node_u  },
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


static inline int dfa_flags_to_serial_fmt(unsigned char flags) {
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

static int dfa_simulate(char const *string) {
    struct dfa_node *current = &dfa_start;
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

    return dfa_flags_to_serial_fmt(flags);
}

ssize_t packf(unsigned char *restrict buffer, size_t bufsize, char const *restrict fmt, ...) {
    int fmttype;
    unsigned valsize;
    unsigned fmtlen;

    ssize_t nwritten = 0;

    union {
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
    } packval;

    va_list args;
    va_start(args, fmt);

    while(*fmt) {
        if(*fmt++ != '%') {
            nwritten = -1;
            goto cleanup;
        }

        fmttype = dfa_simulate(fmt);
        switch(fmttype) {
            case FMT_HHD:
                packval.hhd = va_arg(args, int);
                valsize = sizeof(signed char);
                fmtlen = 3u;
                break;
            case FMT_HD:
                packval.hd = va_arg(args, int);
                valsize = sizeof(short);
                fmtlen = 2u;
                break;
            case FMT_D:
                packval.d = va_arg(args, int);
                valsize = sizeof(int);
                fmtlen = 1u;
                break;
            case FMT_LD:
                packval.ld = va_arg(args, long);
                valsize = sizeof(long);
                fmtlen = 2u;
                break;
            case FMT_LLD:
                packval.lld = va_arg(args, long long);
                valsize = sizeof(long long);
                fmtlen = 3u;
                break;
            case FMT_HHU:
                packval.hhu = va_arg(args, int);
                valsize = sizeof(unsigned char);
                fmtlen = 3u;
                break;
            case FMT_HU:
                packval.hu = va_arg(args, int);
                valsize = sizeof(unsigned short);
                fmtlen = 2u;
                break;
            case FMT_U:
                packval.u = va_arg(args, unsigned);
                valsize = sizeof(unsigned);
                fmtlen = 1u;
                break;
            case FMT_LU:
                packval.lu = va_arg(args, unsigned long);
                valsize = sizeof(unsigned long);
                fmtlen = 2u;
                break;
            case FMT_LLU:
                packval.llu = va_arg(args, unsigned long long);
                valsize = sizeof(unsigned long long);
                fmtlen = 3u;
                break;
            default:
                nwritten = -1;
                goto cleanup;
        }
        if((size_t)nwritten + valsize >= bufsize) {
            nwritten = -E2BIG;
            goto cleanup;
        }

        memcpy(&buffer[nwritten], &packval, valsize);
        nwritten += valsize;
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

    ssize_t nread = 0;

    void *packval;

    va_list args;
    va_start(args, fmt);

    while(*fmt) {
        if(*fmt++ != '%') {
            nread = -1;
            goto cleanup;
        }

        fmttype = dfa_simulate(fmt);
        switch(fmttype) {
            case FMT_HHD:
                packval = va_arg(args, signed char *);
                valsize = sizeof(signed char);
                fmtlen = 3u;
                break;
            case FMT_HD:
                packval = va_arg(args,  short *);
                valsize = sizeof(short);
                fmtlen = 2u;
                break;
            case FMT_D:
                packval = va_arg(args, int *);
                valsize = sizeof(int);
                fmtlen = 1u;
                break;
            case FMT_LD:
                packval = va_arg(args, long *);
                valsize = sizeof(long);
                fmtlen = 2u;
                break;
            case FMT_LLD:
                packval = va_arg(args, long long *);
                valsize = sizeof(long long);
                fmtlen = 3u;
                break;
            case FMT_HHU:
                packval = va_arg(args, unsigned char *);
                valsize = sizeof(unsigned char);
                fmtlen = 3u;
                break;
            case FMT_HU:
                packval = va_arg(args, unsigned short *);
                valsize = sizeof(unsigned short);
                fmtlen = 2u;
                break;
            case FMT_U:
                packval = va_arg(args, unsigned *);
                valsize = sizeof(unsigned);
                fmtlen = 1u;
                break;
            case FMT_LU:
                packval = va_arg(args, unsigned long *);
                valsize = sizeof(unsigned long);
                fmtlen = 2u;
                break;
            case FMT_LLU:
                packval = va_arg(args, unsigned long long *);
                valsize = sizeof(unsigned long long);
                fmtlen = 3u;
                break;
            default:
                nread = -1;
                goto cleanup;
        }
        if((size_t)nread + valsize >= bufsize) {
            nread = -E2BIG;
            goto cleanup;
        }

        memcpy(packval, &buffer[nread], valsize);
        nread += valsize;
        fmt += fmtlen;
    }

cleanup:
    va_end(args);
    return nread;

}
