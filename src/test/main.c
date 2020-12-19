#include "interpolation_test.h"
#include "record_queue_test.h"
#include "sha1_test.h"
#include "serialize_test.h"
#include "test.h"

int main(void) {
    section(sha1);
    run(test_sha1);

    section(record_queue);
    run(test_record_queue_push_pop);
    run(test_record_queue_peek);
    run(test_record_queue_flush);
    run(test_record_queue_full);

    section(interpolation);
    run(test_lerp);
    run(test_lerp_inverse);

    section(serialize);
    run(test_packf);
    run(test_packf_insufficient_bufsize);
    run(test_packf_invalid_fmtstring);
    run(test_unpackf);
    run(test_unpackf_insufficient_bufsize);
    run(test_unpackf_invalid_fmtstring);

    return test_summary();;
}
