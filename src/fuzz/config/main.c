#include "config.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <syslog.h>
#include <sys/stat.h>
#include <unistd.h>

#define FAND_CONFIG "/tmp/_fand.conf"

int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    if(!size) {
        return 0;
    }

    int fd = open(FAND_CONFIG, O_CREAT | O_WRONLY);
    if(fd == -1) {
        perror("open");
        return 0;
    }

    if(write(fd, data, size) == -1) {
        perror("write");
        goto cleanup;
    }

    setlogmask(0);
    openlog(0, 0, LOG_DAEMON);
    config_parse(FAND_CONFIG, &(struct fand_config){ 0 });
    closelog();

cleanup:
    if(close(fd) == -1) {
        perror("close");
    }
    if(unlink(FAND_CONFIG) == -1) {
        perror("unlink");
    }

    return 0;
}
