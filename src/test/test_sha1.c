#include "sha1.h"
#include "test.h"
#include "test_sha1.h"

#include <string.h>

void test_sha1(void) {
    char const *short_input = "abcdef";
    char const *long_input  = "There is a risk that, when the data to be hashed "
                              "is way longer than the block size, the hashing might "
                              "fail. While unlikely, this should still be tested in "
                              "order to ensure that the implementation is correct. "
                              "Who knows, if this isn't tested, the caching might "
                              "just not work.";

    unsigned char const short_hash[] = {
        0x1f, 0x8a, 0xc1, 0x0f, 0x23, 0xc5, 0xb5, 0xbc, 0x11, 0x67,
        0xbd, 0xa8, 0x4b, 0x83, 0x3e, 0x5c, 0x05, 0x7a, 0x77, 0xd2
    };
    unsigned char const long_hash[]  = {
        0x0e, 0x6d, 0xcc, 0x8c, 0x49, 0x14, 0x44, 0x66, 0xad, 0x01,
        0xde, 0x64, 0x83, 0x6c, 0x77, 0x53, 0x8b, 0x14, 0xf4, 0x93
    };

    unsigned char digest[SHA1_DIGESTSIZE];

    sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, (unsigned char const *)short_input, strlen(short_input));
    sha1_final(&ctx, digest);

    fand_assert(memcmp(digest, short_hash, SHA1_DIGESTSIZE) == 0);

    sha1_init(&ctx);
    sha1_update(&ctx, (unsigned char const *)long_input, strlen(long_input));
    sha1_final(&ctx, digest);

    fand_assert(memcmp(digest, long_hash, SHA1_DIGESTSIZE) == 0);
}
