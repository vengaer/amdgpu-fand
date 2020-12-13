#include "macro.h"
#include "test.h"
#include "record_queue.h"
#include "record_queue_test.h"

#include <string.h>

#include <sys/socket.h>
#include <sys/un.h>


static struct ipc_record record0 = {
    .sender.addr_un = {
        .sun_family = AF_UNIX,
        .sun_path = "addr0"
    },
    .cmd = ipc_exit_req
};

static struct ipc_record record1 = {
    .sender.addr_un = {
        .sun_family = AF_UNIX,
        .sun_path = "addr1"
    },
    .cmd = ipc_speed_req
};

static inline void assert_ipc_records_eq(struct ipc_record const *r0, struct ipc_record const *r1) {
    fand_assert(r0->sender.addr_un.sun_family == r1->sender.addr_un.sun_family);
    fand_assert(strcmp(r0->sender.addr_un.sun_path, r1->sender.addr_un.sun_path) == 0);
    fand_assert(r0->cmd == r1->cmd);
}

void test_record_queue_push_pop(void) {
    struct record_queue queue = { 0 };

    struct ipc_record out;

    fand_assert(record_queue_push(&queue, &record0) == 0);
    fand_assert(record_queue_push(&queue, &record1) == 0);

    fand_assert(record_queue_pop(&queue, &out) == 0);
    assert_ipc_records_eq(&out, &record0);

    fand_assert(record_queue_pop(&queue, &out) == 0);
    assert_ipc_records_eq(&out, &record1);

    fand_assert(record_queue_pop(&queue, &out));
}

void test_record_queue_peek(void) {
    struct record_queue queue = { 0 };
    struct ipc_record out;

    fand_assert(record_queue_push(&queue, &record0) == 0);
    fand_assert(record_queue_push(&queue, &record1) == 0);

    fand_assert(record_queue_peek(&queue, &out) == 0);
    assert_ipc_records_eq(&out, &record0);

    fand_assert(record_queue_peek(&queue, &out) == 0);
    assert_ipc_records_eq(&out, &record0);
}

void test_record_queue_flush(void) {
    struct record_queue queue = { 0 };

    fand_assert(record_queue_push(&queue, &record0) == 0);
    fand_assert(record_queue_push(&queue, &record1) == 0);

    record_queue_flush(&queue);

    fand_assert(queue.size == 0);
}

void test_record_queue_full(void) {
    struct record_queue queue = { 0 };
    struct ipc_record out;
    for(unsigned i = 0; i < array_size(queue.records) - 1; i++) {
        fand_assert(record_queue_push(&queue, &record0) == 0);
    }

    fand_assert(record_queue_push(&queue, &record1) == 0);

    fand_assert(record_queue_push(&queue, &record0));

    for(unsigned i = 0; i < array_size(queue.records) - 1; i++) {
        fand_assert(record_queue_pop(&queue, &out) == 0);
        assert_ipc_records_eq(&out, &record0);
    }

    fand_assert(record_queue_pop(&queue, &out) == 0);
    assert_ipc_records_eq(&out, &record1);
}
