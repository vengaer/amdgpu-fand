#include "interpolation_test.h"
#include "sha1_test.h"
#include "serialize_test.h"
#include "test.h"

int main(void) {
    section(sha1);
    run(test_sha1);

    section(interpolation);
    run(test_lerp);
    run(test_lerp_inverse);

    section(serialize);
    run(test_packf);
    run(test_packf_insufficient_bufsize);
    run(test_packf_invalid_fmtstring);
    run(test_packf_repeat);
    run(test_unpackf);
    run(test_unpackf_insufficient_bufsize);
    run(test_unpackf_invalid_fmtstring);
    run(test_unpackf_repeat);

    return test_summary();;
}
