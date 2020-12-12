#ifndef SHA1_H
#define SHA1_H

#include <stdint.h>

enum { SHA1_BLOCKSIZE = 64 };
enum { SHA1_DIGESTSIZE = 20 };

typedef struct {
    uint32_t h0, h1, h2, h3, h4;
    uint32_t n0, n1;
    unsigned char buffer[SHA1_BLOCKSIZE];
} sha1_ctx;

void sha1_init(sha1_ctx *ctx);
void sha1_update(sha1_ctx *ctx, unsigned char const *data, uint32_t length);
void sha1_final(sha1_ctx *ctx, unsigned char *digest);

#endif /* SHA1_H */
