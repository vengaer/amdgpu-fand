#include "cache.h"
#include "cache_mock.h"
#include "mock.h"
#include "regutils.h"
#include "sha1.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <regex.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FAND_CACHE_FILE "/tmp/amdgpu-fand.cache"

static bool is_padded(void) {
    return true;
}

bool exists_in_sysfs(char const *file) {
    regex_t regex;
    bool exists;

    if(regcomp_info(&regex, "^/sys/devices/pci[0-9:]+(/[a-zA-Z0-9:.]+){4}/hwmon/hwmon[0-9]+/(pwm[0-9]+(_input)?|temp[0-9]+_input)", REG_EXTENDED, "sysfs mock")) {
        return false;
    }

    exists = regexec(&regex, file, 0, 0, 0) == 0;

    regfree(&regex);
    return exists;
}


int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    if(!size) {
        return 0;
    }

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

    mock_guard {
        mock_cache_struct_is_padded(is_padded);
        mock_cache_file_exists_in_sysfs(exists_in_sysfs);

        cache_load();
    }

    closelog();

close_fd:
    if(close(fd) == -1) {
        perror("close");
    }
free_mem:
    free(buffer);
    return 0;
}
