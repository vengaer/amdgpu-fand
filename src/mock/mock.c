#include "mock.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { ADDR_BLOCK_SIZE = 32 };

struct mock_record {
    void *addr;
    size_t size;
};

static struct mock_record *mock_records;
static unsigned addr_cap;
static unsigned naddrs;

void mock_guard_init(void) {
    mock_records = malloc(ADDR_BLOCK_SIZE * sizeof (*mock_records));

    if(!mock_records) {
        fputs("Malloc failure during mock guard initialization\n", stderr);
        exit(1);
    }
    addr_cap = ADDR_BLOCK_SIZE;
    naddrs = 0;
}

void mock_guard_add(void *addr, size_t size) {
    if(naddrs >= addr_cap) {
        mock_records = realloc(mock_records, 2 * addr_cap);
        addr_cap *= 2;
        if(!mock_records) {
            fputs("Realloc failure while adding mock to guard\n", stderr);
            exit(1);
        }
    }

    mock_records[naddrs++] = (struct mock_record){ addr, size };
}

bool mock_guard_active(void) {
    return mock_records;
}

void mock_guard_cleanup(void) {
    for(unsigned i = 0; i < naddrs; i++) {
        memset(mock_records[i].addr, 0, mock_records[i].size);
    }
    free(mock_records);
    mock_records = 0;
}
