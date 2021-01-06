#include "cache.h"
#include "sha1.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FAND_CACHE_FILE "/tmp/amdgpu-fand.cache"

bool cache_struct_is_padded(void) {
    return true;
}

int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    if(!size) {
        return 0;
    }
    cache_struct_is_padded();

    unsigned char *buffer = malloc(size + SHA1_DIGESTSIZE);
    if(!buffer) {
        fputs("malloc failed\n", stderr);
        return 0;
    }

    memcpy(buffer, data, size);

    /* Append checksum */
    sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, data, size);
    sha1_final(&ctx, &buffer[size]);

    size += SHA1_DIGESTSIZE;

    int fd = open(FAND_CACHE_FILE, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if(fd == -1) {
        perror("open");
        goto free_mem;
    }

    if(write(fd, buffer, size) != (ssize_t)size) {
        perror("write");
        goto close_fd;
    }

    setlogmask(0);
    openlog(0, 0, LOG_DAEMON);

    cache_load();

    closelog();

close_fd:
    if(close(fd) == -1) {
        perror("close");
    }
free_mem:
    free(buffer);
    return 0;
}
