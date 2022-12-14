#include "cache.h"
#include "filesystem.h"
#include "mock.h"
#include "serialize.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <fcntl.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef FAND_TEST_CONFIG
#define FAND_CACHE_DIR "/var/cache/amdgpu-fand"
#else
#define FAND_CACHE_DIR "/tmp"
#endif

#define FAND_CACHE_FILE FAND_CACHE_DIR "/amdgpu-fand.cache"

struct fand_cache fand_cache;

enum {
    CACHE_SIZE = sizeof(((struct fand_cache *)0)->pwm) +
                 sizeof(((struct fand_cache *)0)->pwm_enable) +
                 sizeof(((struct fand_cache *)0)->temp_input) +
                 sizeof(((struct fand_cache *)0)->card_idx) +
                 sizeof(((struct fand_cache *)0)->checksum)
};

MOCKABLE(static inline)
bool cache_struct_is_padded(void) {
    return sizeof(fand_cache) > CACHE_SIZE;
}

MOCKABLE(static inline)
bool cache_file_exists_in_sysfs(char const *file) {
    static char const *sysfs_stem = "/sys/";
    static size_t const stem_len = 5u;

    return fsys_file_exists(file) && strncmp(file, sysfs_stem, stem_len) == 0;
}

static int cache_validate(unsigned char *buffer, size_t nbytes) {
    if(nbytes != CACHE_SIZE) {
        syslog(LOG_WARNING, "Cache corrupted, expected %zu bytes, found %zu", (size_t)CACHE_SIZE, nbytes);
        return -1;
    }

    unsigned char digest[SHA1_DIGESTSIZE];
    int status = 0;

    sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, buffer, nbytes - SHA1_DIGESTSIZE);
    sha1_final(&ctx, digest);

    if(memcmp(digest, fand_cache.checksum, SHA1_DIGESTSIZE) != 0) {
        syslog(LOG_WARNING, "Corrupted cache, checksums did not match");
        return -1;
    }

    if(!cache_file_exists_in_sysfs(fand_cache.pwm)) {
        syslog(LOG_WARNING, "Cached pwm file %s does not exist in /sys tree", fand_cache.pwm);
        status = -1;
    }
    else if(!cache_file_exists_in_sysfs(fand_cache.pwm_enable)) {
        syslog(LOG_WARNING, "Cached pwm enable file %s does not exist in /sys tree", fand_cache.pwm_enable);
        status = -1;
    }
    else if(!cache_file_exists_in_sysfs(fand_cache.temp_input)) {
        syslog(LOG_WARNING, "Cached temp input file %s does not exist in /sys tree", fand_cache.temp_input);
        status = -1;
    }

    return status;
}

static ssize_t cache_pack(unsigned char *buffer, size_t bufsize) {
    if(bufsize < CACHE_SIZE) {
        syslog(LOG_ERR, "Failed to pack cache file");
        return -1;
    }

    ssize_t nbytes = 0;
    unsigned char digest[SHA1_DIGESTSIZE];

    nbytes = packf(buffer, bufsize, "%*hhu%*hhu%*hhu%u", sizeof(fand_cache.pwm),        (unsigned char *)fand_cache.pwm,
                                                         sizeof(fand_cache.pwm_enable), (unsigned char *)fand_cache.pwm_enable,
                                                         sizeof(fand_cache.temp_input), (unsigned char *)fand_cache.temp_input,
                                                         fand_cache.card_idx);
    sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, buffer, nbytes);
    sha1_final(&ctx, digest);

    nbytes += packf(buffer + nbytes, bufsize - nbytes, "%*hhu", sizeof(digest), digest);

    return nbytes;
}

static ssize_t cache_unpack(unsigned char const *buffer, size_t bufsize) {
    if(bufsize < CACHE_SIZE) {
        syslog(LOG_WARNING, "Cache file corrupted");
        return -1;
    }
    ssize_t nbytes = 0;
    if(!cache_struct_is_padded()) {
        memcpy(&fand_cache, buffer, sizeof(fand_cache));
        nbytes = sizeof(fand_cache);
    }
    else {
        nbytes = unpackf(buffer, bufsize, "%*hhu%*hhu%*hhu%u%*hhu", sizeof(fand_cache.pwm),        (unsigned char *)fand_cache.pwm,
                                                                    sizeof(fand_cache.pwm_enable), (unsigned char *)fand_cache.pwm_enable,
                                                                    sizeof(fand_cache.temp_input), (unsigned char *)fand_cache.temp_input,
                                                                    &fand_cache.card_idx,
                                                                    sizeof(fand_cache.checksum), fand_cache.checksum);
    }

    /* Prevent overrun in case of cache corruption */
    fand_cache.pwm[sizeof(fand_cache.pwm) - 1] = '\0';
    fand_cache.pwm_enable[sizeof(fand_cache.pwm_enable) - 1] = '\0';
    fand_cache.temp_input[sizeof(fand_cache.temp_input) - 1] = '\0';

    return nbytes;
}

int cache_load(void) {
    unsigned char buffer[CACHE_SIZE];
    int status = 0;

    if(!fsys_file_exists(FAND_CACHE_FILE)) {
        return -1;
    }

    int fd = open(FAND_CACHE_FILE, O_RDONLY);
    if(fd == -1) {
        syslog(LOG_INFO, "No readable cache found: %s", strerror(errno));
        return -1;
    }

    ssize_t nbytes = read(fd, &buffer, sizeof(buffer));

    if(nbytes == -1) {
        syslog(LOG_WARNING, "Failed to read from cache file: %s", strerror(errno));
        status = -1;
    }
    else if((size_t)nbytes != sizeof(buffer)) {
        syslog(LOG_INFO, "Cache file corrupted, expected %zu bytes but read %zu", sizeof(fand_cache), (size_t)nbytes);
        status = -1;
    }

    if(close(fd) == -1) {
        syslog(LOG_WARNING, "Could not close cache file descriptor: %s", strerror(errno));
    }
    if(status < 0) {
        return status;
    }

    nbytes = cache_unpack(buffer, sizeof(buffer));
    if(nbytes < 0) {
        return -1;
    }

    return cache_validate(buffer, nbytes);
}

int cache_write(void) {
    unsigned char buffer[CACHE_SIZE];
    int status = 0;
    if(!fsys_dir_exists(FAND_CACHE_DIR)) {
        if(mkdir(FAND_CACHE_DIR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
            syslog(LOG_WARNING, "Could not create cache directory: %s", strerror(errno));
            return -1;
        }
    }

    if(cache_pack(buffer, sizeof(buffer)) < 0) {
        return -1;
    }

    int fd = open(FAND_CACHE_FILE, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if(fd == -1) {
        syslog(LOG_WARNING, "Could not open cache file for writing: %s", strerror(errno));
        return -1;
    }

    if(write(fd, buffer, sizeof(buffer)) != sizeof(buffer)) {
        syslog(LOG_WARNING, "Could not write cache file: %s", strerror(errno));
        status = -1;
    }

    if(close(fd) == -1) {
        syslog(LOG_WARNING, "Could not close cache file: %s", strerror(errno));
        status = -1;
    }

    return status;
}
