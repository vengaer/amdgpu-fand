#ifndef RECORD_QUEUE_H
#define RECORD_QUEUE_H

#include "ipc.h"

enum { RECORD_QUEUE_SIZE = 32 };

struct ipc_record {
    union unsockaddr sender;
    enum ipc_cmd cmd;
};

struct record_queue {
    unsigned idx;
    unsigned size;
    struct ipc_record records[RECORD_QUEUE_SIZE];
};

int record_queue_push(struct record_queue *queue, struct ipc_record const *record);
int record_queue_peek(struct record_queue *queue, struct ipc_record *record);
int record_queue_pop(struct record_queue *queue, struct ipc_record *record);
void record_queue_flush(struct record_queue *queue);

#endif /* RECORD_QUEUE_H */
