#include "serialize.h"
#include "serialize_test.h"
#include "test.h"

#include <string.h>

#include <sys/types.h>

void test_packf(void) {
    unsigned char buffer[32];

    fand_assert(packf(buffer, sizeof(buffer), "%hhu%hu%lld", 28, 32, 212ll) == sizeof(unsigned char)  +
                                                                               sizeof(unsigned short) +
                                                                               sizeof(long long));
    unsigned char hhu;
    unsigned short hu;
    long long lld;

    memcpy(&hhu, buffer, sizeof(hhu));
    memcpy(&hu, buffer + sizeof(hhu), sizeof(hu));
    memcpy(&lld, buffer + sizeof(hhu) + sizeof(hu), sizeof(lld));

    fand_assert(hhu == 28);
    fand_assert(hu == 32);
    fand_assert(lld = 212ll);
}

void test_packf_insufficient_bufsize(void) {
    unsigned char buffer[5];

    fand_assert(packf(buffer, sizeof(buffer), "%hhu%hu%lld", 28, 32, 212ll) < 0);
}

void test_packf_invalid_fmtstring(void) {
    unsigned char buffer[32];
    fand_assert(packf(buffer, sizeof(buffer), "%hhuc%d", 28, 33) < 0);
    fand_assert(packf(buffer, sizeof(buffer), "%hhhu", 23) < 0);
}

void test_packf_repeat(void) {
    unsigned char packbuf[32];
    unsigned char inbuf[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    };
    fand_assert(packf(packbuf, sizeof(packbuf), "%*hhu%d", sizeof(inbuf), inbuf, 30) == sizeof(inbuf) + sizeof(int));
    int out;

    for(unsigned i = 0; i < sizeof(inbuf); i++) {
        fand_assert(inbuf[i] == packbuf[i]);
    }
    memcpy(&out, &packbuf[sizeof(inbuf)], sizeof(out));
    fand_assert(out == 30);
}

void test_unpackf(void) {
    unsigned char buffer[32];
    ssize_t size = sizeof(unsigned char) +
                   sizeof(unsigned short) +
                   sizeof(unsigned long);
    fand_assert(packf(buffer, sizeof(buffer), "%hhu%hu%lu", 10, 20, 30) == size);

    unsigned char hhu;
    unsigned short hu;
    unsigned long lu;
    fand_assert(unpackf(buffer, sizeof(buffer), "%hhu%hu%lu", &hhu, &hu, &lu) == size);
    fand_assert(hhu == 10);
    fand_assert(hu == 20);
    fand_assert(lu == 30);
}

void test_unpackf_insufficient_bufsize(void) {
    unsigned char buffer[5] = { 0 };

    unsigned char hhu = 0xff;
    unsigned short hu = 0xff;
    long long lld = 0xff;

    fand_assert(packf(buffer, sizeof(buffer), "%hhu%hu%lld", &hhu, &hu, &lld) < 0);
    fand_assert(hhu == 0xff);
    fand_assert(hu == 0xff);
    fand_assert(lld == 0xff);
}

void test_unpackf_invalid_fmtstring(void) {
    unsigned char buffer[32] = { 0 };

    unsigned char hhu = 0xff;
    int d = 0xff;
    fand_assert(packf(buffer, sizeof(buffer), "%hhuc%d", &hhu, &d) < 0);
    fand_assert(packf(buffer, sizeof(buffer), "%hhhu", &hhu) < 0);

    fand_assert(hhu == 0xff);
    fand_assert(d == 0xff);
}

void test_unpackf_repeat(void) {
    unsigned char packbuf[32];
    unsigned char inbuf[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9
    };
    fand_assert(packf(packbuf, sizeof(packbuf), "%*hhu%d", sizeof(inbuf), inbuf, 30) == sizeof(inbuf) + sizeof(int));
    unsigned char outbuf[sizeof(inbuf)];
    int outd;
    fand_assert(unpackf(packbuf, sizeof(packbuf), "%*hhu%d", sizeof(outbuf), outbuf, &outd) == sizeof(outbuf) + sizeof(int));
    for(unsigned i = 0; i < sizeof(outbuf); i++) {
        fand_assert(outbuf[i] == inbuf[i]);
    }
    fand_assert(outd == 30);
}
