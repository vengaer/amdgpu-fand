#include "test.h"
#include "test_sha1.h"
#include "test_record_queue.h"

int main(void) {
    run(test_sha1);

    run(test_record_queue_push_pop);
    run(test_record_queue_peek);
    run(test_record_queue_flush);
    run(test_record_queue_full);

    return test_summary();;
}
