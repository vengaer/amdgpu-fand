#include "interpolation_test.h"
#include "record_queue_test.h"
#include "sha1_test.h"
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

    return test_summary();;
}
