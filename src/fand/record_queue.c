#include "record_queue.h"

#include <string.h>

int record_queue_push(struct record_queue *queue, struct ipc_record const *record) {
    if(queue->size - queue->idx >= RECORD_QUEUE_SIZE) {
        return 1;
    }
    queue->records[queue->size++ % RECORD_QUEUE_SIZE] = *record;
    return 0;
}

int record_queue_peek(struct record_queue *queue, struct ipc_record *record) {
    if(queue->idx == queue->size) {
        record_queue_flush(queue);
        return 1;
    }

    *record = queue->records[queue->idx % RECORD_QUEUE_SIZE];
    return 0;
}

int record_queue_pop(struct record_queue *queue, struct ipc_record *record) {
    if(queue->idx == queue->size) {
        record_queue_flush(queue);
        return 1;
    }
    *record = queue->records[queue->idx++ % RECORD_QUEUE_SIZE];

    if(queue->idx == queue->size) {
        record_queue_flush(queue);
    }

    return 0;
}

void record_queue_flush(struct record_queue *queue) {
    memset(queue, 0, sizeof(*queue));
}
