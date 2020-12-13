#include "record_queue_test.h"
#include "sha1_test.h"
#include "test.h"

int main(void) {
    run(test_sha1);

    run(test_record_queue_push_pop);
    run(test_record_queue_peek);
    run(test_record_queue_flush);
    run(test_record_queue_full);

    return test_summary();;
}
