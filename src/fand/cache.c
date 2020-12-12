#include "cache.h"
#include "filesystem.h"

#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define FAND_CACHE_DIR "/var/cache/amdgpu-fand"
#define FAND_CACHE_FILE FAND_CACHE_DIR "/amdgpu-fand.cache"

struct fand_cache fand_cache;

static int cache_validate(void) {
    /* TODO: add checksum to cache */
}

int cache_load(void) {
    int status = 0;
    if(!fsys_file_exists(FAND_CACHE_FILE)) {
        return -1;
    }

    int fd = open(FAND_CACHE_FILE, O_RDONLY);
    if(fd == -1) {
        syslog(LOG_INFO, "No readable cache found: %s", strerror(errno));
        return -1;
    }

    ssize_t nbytes = read(fd, &fand_cache, sizeof(fand_cache));

    if(nbytes == -1) {
        syslog(LOG_WARNING, "Failed to read from cache file: %s", strerror(errno));
        status = -1;
    }
    else if((size_t)nbytes != sizeof(fand_cache)) {
        syslog(LOG_INFO, "Corrupted cache file found, expected %zu bytes but read %zu", sizeof(fand_cache), (size_t)nbytes);
        status = -1;
    }

    if(close(fd) == -1) {
        syslog(LOG_WARNING, "Failed to close cache file descriptor: %s", strerror(errno));
    }
    if(status < 0) {
        return status;
    }

    return cache_validate();
}

int cache_write(void) {
    int status = 0;
    if(!fsys_dir_exists(FAND_CACHE_DIR)) {
        if(mkdir(FAND_CACHE_DIR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
            syslog(LOG_WARNING, "Failed to create cache directory: %s", strerror(errno));
            return -1;
        }
    }
    int fd = open(FAND_CACHE_FILE, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if(fd < 0) {
        syslog(LOG_WARNING, "Could not open cache file for writing: %s", strerror(errno));
        return -1;
    }

    if(write(fd, &fand_cache, sizeof(fand_cache)) != sizeof(fand_cache)) {
        syslog(LOG_WARNING, "Failed to write cache file: %s", strerror(errno));
        status = -1;
    }

    if(close(fd) == -1) {
        syslog(LOG_WARNING, "Could not close cache file: %s", strerror(errno));
        status = -1;
    }

    return status;
}
