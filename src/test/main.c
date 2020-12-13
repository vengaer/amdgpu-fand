#include "test.h"
#include "test_sha1.h"
#include "test_record_queue.h"

int main(void) {
    test_sha1();
    test_record_queue_push_pop();
    test_record_queue_peek();
    test_record_queue_flush();
    test_record_queue_full();

    return test_summary();;
}
