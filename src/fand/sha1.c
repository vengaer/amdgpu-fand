#include "arch.h"
#include "sha1.h"
#include "macro.h"

#include <string.h>

#include <arpa/inet.h>

union sha1_block {
    uint8_t as_bytes[SHA1_BLOCKSIZE];
    uint32_t as_dwords[SHA1_BLOCKSIZE / sizeof(uint32_t)];
};

static inline uint32_t rol(uint32_t value, uint32_t bits) {
    return (value >> (32u - bits)) | (value << bits);
}

static inline uint32_t expand(union sha1_block *block, uint32_t i) {
    block->as_dwords[i & 0xf] = rol(block->as_dwords[(i + 0xd) & 0xf] ^
                                    block->as_dwords[(i + 0x8) & 0xf] ^
                                    block->as_dwords[(i + 0x2) & 0xf] ^
                                    block->as_dwords[(i + 0x0) & 0xf],
                                    1u);
    return block->as_dwords[i & 0xf];
}

static inline void r0(union sha1_block *block, uint32_t v, uint32_t *w, uint32_t x, uint32_t y, uint32_t *z, uint32_t i) {
    block->as_dwords[i] = htonl(block->as_dwords[i]);
    *z += ((*w & (x ^ y)) ^ y) + block->as_dwords[i] + 0x5a827999u + rol(v, 5u);
    *w  = rol(*w, 30u);
}

static inline void r1(union sha1_block *block, uint32_t v, uint32_t *w, uint32_t x, uint32_t y, uint32_t *z, uint32_t i) {
    *z += ((*w & (x ^ y)) ^ y) + expand(block, i) + 0x5a827999u + rol(v, 5u);
    *w  = rol(*w, 30u);
}

static inline void r2(union sha1_block *block, uint32_t v, uint32_t *w, uint32_t x, uint32_t y, uint32_t *z, uint32_t i) {
    *z += (*w ^ x ^ y) + expand(block, i) + 0x6ed9eba1u + rol(v, 5u);
    *w  = rol(*w, 30u);
}

static inline void r3(union sha1_block *block, uint32_t v, uint32_t *w, uint32_t x, uint32_t y, uint32_t *z, uint32_t i) {
    *z += (((*w | x) & y) | (*w & x)) + expand(block, i) + 0x8f1bbcdcu + rol(v, 5u);
    *w  = rol(*w, 30u);
}

static inline void r4(union sha1_block *block, uint32_t v, uint32_t *w, uint32_t x, uint32_t y, uint32_t *z, uint32_t i) {
    *z += (*w ^ x ^ y) + expand(block, i) + 0xca62c1d6u + rol(v, 5u);
    *w = rol(*w, 30u);
}

static inline void sha1_transform(sha1_ctx *ctx, unsigned char const *buffer) {
    uint32_t h0 = ctx->h0;
    uint32_t h1 = ctx->h1;
    uint32_t h2 = ctx->h2;
    uint32_t h3 = ctx->h3;
    uint32_t h4 = ctx->h4;

    union sha1_block block;
    memcpy(block.as_bytes, buffer, sizeof(block));

    r0(&block, h0, &h1, h2, h3, &h4, 0u);
    r0(&block, h4, &h0, h1, h2, &h3, 1u);
    r0(&block, h3, &h4, h0, h1, &h2, 2u);
    r0(&block, h2, &h3, h4, h0, &h1, 3u);
    r0(&block, h1, &h2, h3, h4, &h0, 4u);
    r0(&block, h0, &h1, h2, h3, &h4, 5u);
    r0(&block, h4, &h0, h1, h2, &h3, 6u);
    r0(&block, h3, &h4, h0, h1, &h2, 7u);
    r0(&block, h2, &h3, h4, h0, &h1, 8u);
    r0(&block, h1, &h2, h3, h4, &h0, 9u);
    r0(&block, h0, &h1, h2, h3, &h4, 10u);
    r0(&block, h4, &h0, h1, h2, &h3, 11u);
    r0(&block, h3, &h4, h0, h1, &h2, 12u);
    r0(&block, h2, &h3, h4, h0, &h1, 13u);
    r0(&block, h1, &h2, h3, h4, &h0, 14u);
    r0(&block, h0, &h1, h2, h3, &h4, 15u);
    r1(&block, h4, &h0, h1, h2, &h3, 16u);
    r1(&block, h3, &h4, h0, h1, &h2, 17u);
    r1(&block, h2, &h3, h4, h0, &h1, 18u);
    r1(&block, h1, &h2, h3, h4, &h0, 19u);

    r2(&block, h0, &h1, h2, h3, &h4, 20u);
    r2(&block, h4, &h0, h1, h2, &h3, 21u);
    r2(&block, h3, &h4, h0, h1, &h2, 22u);
    r2(&block, h2, &h3, h4, h0, &h1, 23u);
    r2(&block, h1, &h2, h3, h4, &h0, 24u);
    r2(&block, h0, &h1, h2, h3, &h4, 25u);
    r2(&block, h4, &h0, h1, h2, &h3, 26u);
    r2(&block, h3, &h4, h0, h1, &h2, 27u);
    r2(&block, h2, &h3, h4, h0, &h1, 28u);
    r2(&block, h1, &h2, h3, h4, &h0, 29u);
    r2(&block, h0, &h1, h2, h3, &h4, 30u);
    r2(&block, h4, &h0, h1, h2, &h3, 31u);
    r2(&block, h3, &h4, h0, h1, &h2, 32u);
    r2(&block, h2, &h3, h4, h0, &h1, 33u);
    r2(&block, h1, &h2, h3, h4, &h0, 34u);
    r2(&block, h0, &h1, h2, h3, &h4, 35u);
    r2(&block, h4, &h0, h1, h2, &h3, 36u);
    r2(&block, h3, &h4, h0, h1, &h2, 37u);
    r2(&block, h2, &h3, h4, h0, &h1, 38u);
    r2(&block, h1, &h2, h3, h4, &h0, 39u);

    r3(&block, h0, &h1, h2, h3, &h4, 40u);
    r3(&block, h4, &h0, h1, h2, &h3, 41u);
    r3(&block, h3, &h4, h0, h1, &h2, 42u);
    r3(&block, h2, &h3, h4, h0, &h1, 43u);
    r3(&block, h1, &h2, h3, h4, &h0, 44u);
    r3(&block, h0, &h1, h2, h3, &h4, 45u);
    r3(&block, h4, &h0, h1, h2, &h3, 46u);
    r3(&block, h3, &h4, h0, h1, &h2, 47u);
    r3(&block, h2, &h3, h4, h0, &h1, 48u);
    r3(&block, h1, &h2, h3, h4, &h0, 49u);
    r3(&block, h0, &h1, h2, h3, &h4, 50u);
    r3(&block, h4, &h0, h1, h2, &h3, 51u);
    r3(&block, h3, &h4, h0, h1, &h2, 52u);
    r3(&block, h2, &h3, h4, h0, &h1, 53u);
    r3(&block, h1, &h2, h3, h4, &h0, 54u);
    r3(&block, h0, &h1, h2, h3, &h4, 55u);
    r3(&block, h4, &h0, h1, h2, &h3, 56u);
    r3(&block, h3, &h4, h0, h1, &h2, 57u);
    r3(&block, h2, &h3, h4, h0, &h1, 58u);
    r3(&block, h1, &h2, h3, h4, &h0, 59u);

    r4(&block, h0, &h1, h2, h3, &h4, 60u);
    r4(&block, h4, &h0, h1, h2, &h3, 61u);
    r4(&block, h3, &h4, h0, h1, &h2, 62u);
    r4(&block, h2, &h3, h4, h0, &h1, 63u);
    r4(&block, h1, &h2, h3, h4, &h0, 64u);
    r4(&block, h0, &h1, h2, h3, &h4, 65u);
    r4(&block, h4, &h0, h1, h2, &h3, 66u);
    r4(&block, h3, &h4, h0, h1, &h2, 67u);
    r4(&block, h2, &h3, h4, h0, &h1, 68u);
    r4(&block, h1, &h2, h3, h4, &h0, 69u);
    r4(&block, h0, &h1, h2, h3, &h4, 70u);
    r4(&block, h4, &h0, h1, h2, &h3, 71u);
    r4(&block, h3, &h4, h0, h1, &h2, 72u);
    r4(&block, h2, &h3, h4, h0, &h1, 73u);
    r4(&block, h1, &h2, h3, h4, &h0, 74u);
    r4(&block, h0, &h1, h2, h3, &h4, 75u);
    r4(&block, h4, &h0, h1, h2, &h3, 76u);
    r4(&block, h3, &h4, h0, h1, &h2, 77u);
    r4(&block, h2, &h3, h4, h0, &h1, 78u);
    r4(&block, h1, &h2, h3, h4, &h0, 79u);

    ctx->h0 += h0;
    ctx->h1 += h1;
    ctx->h2 += h2;
    ctx->h3 += h3;
    ctx->h4 += h4;
}

void sha1_init(sha1_ctx *ctx) {
    ctx->h0 = 0x67452301u;
    ctx->h1 = 0xefcdab89u;
    ctx->h2 = 0x98badcfeu;
    ctx->h3 = 0x10325476u;
    ctx->h4 = 0xc3d2e1f0u;
    ctx->n0 = 0u;
    ctx->n1 = 0u;
}

void sha1_update(sha1_ctx *ctx, unsigned char const *data, uint32_t length) {
    uint32_t i, j;
    uint32_t nbits = length << 3u;

    i = ctx->n0;
    ctx->n0 += nbits;

    /* Addition caused wrapping */
    if(ctx->n0 < i) {
        ++ctx->n1;
    }

    ctx->n1 += (length >> 29u);

    i = ((i >> 3u) & 0x3fu);

    if(i + length < SHA1_BLOCKSIZE) {
        j = 0u;
    }
    else {
        j = SHA1_BLOCKSIZE - i;
        memcpy(&ctx->buffer[i], data, j);
        sha1_transform(ctx, ctx->buffer);
        for(; j + SHA1_BLOCKSIZE - 1 < length; j += SHA1_BLOCKSIZE) {
            sha1_transform(ctx, &data[j]);
        }
        i = 0u;
    }

    memcpy(&ctx->buffer[i], &data[j], length - j);
}

static inline unsigned char sha1_compute_final(uint32_t n, uint32_t i) {
    return (n >> ((3u - (i & 3u)) << 3u)) & 0xffu;
}

static inline unsigned char sha1_compute_digest_byte(sha1_ctx const *ctx, uint32_t i) {
    uintptr_t offset_table[] = {
        (uintptr_t)&ctx->h0 - (uintptr_t)ctx,
        (uintptr_t)&ctx->h1 - (uintptr_t)ctx,
        (uintptr_t)&ctx->h2 - (uintptr_t)ctx,
        (uintptr_t)&ctx->h3 - (uintptr_t)ctx,
        (uintptr_t)&ctx->h4 - (uintptr_t)ctx
    };
    return (*(uint32_t const*)((uintptr_t)ctx + offset_table[i >> 2u]) >> ((3u - (i & 3u)) << 3u)) & 0xffu;
}

void sha1_final(sha1_ctx *ctx, unsigned char *digest) {
    unsigned char final[8];
    uint32_t i;
    for(i = 0; i < array_size(final) / 2u; i++) {
        final[i] = sha1_compute_final(ctx->n1, i);
    }
    for(; i < array_size(final); i++) {
        final[i] = sha1_compute_final(ctx->n0, i);
    }

    sha1_update(ctx, &(unsigned char){ 0x80 }, 1u);

    while((ctx->n0 & 0x1f8u) != 0x1c0u) {
        sha1_update(ctx, &(unsigned char){ 0x0 }, 1u);
    }

    sha1_update(ctx, final, array_size(final));

    for(i = 0; i < SHA1_DIGESTSIZE; i++) {
        digest[i] = sha1_compute_digest_byte(ctx, i);
    }
}
