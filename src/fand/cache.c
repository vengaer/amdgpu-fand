#include "cache.h"
#include "filesystem.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <fcntl.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define FAND_CACHE_DIR "/var/cache/amdgpu-fand"
#define FAND_CACHE_FILE FAND_CACHE_DIR "/amdgpu-fand.cache"

struct fand_cache fand_cache;

enum {
    CACHE_SIZE = sizeof(((struct fand_cache *)0)->pwm) +
                 sizeof(((struct fand_cache *)0)->pwm_enable) +
                 sizeof(((struct fand_cache *)0)->temp_input) +
                 sizeof(((struct fand_cache *)0)->card_idx) +
                 sizeof(((struct fand_cache *)0)->checksum)
};

static inline bool cache_struct_is_padded(void) {
    return sizeof(fand_cache) > CACHE_SIZE;
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

    if(!fsys_file_exists(fand_cache.pwm)) {
        syslog(LOG_WARNING, "Cached pwm file %s does not exist", fand_cache.pwm);
        status = -1;
    }
    if(!fsys_file_exists(fand_cache.pwm_enable)) {
        syslog(LOG_WARNING, "Cached pwm enable file %s does not exist", fand_cache.pwm_enable);
        status = -1;
    }
    if(!fsys_file_exists(fand_cache.temp_input)) {
        syslog(LOG_WARNING, "Cached temp input file %s does not exist", fand_cache.temp_input);
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

    memcpy(buffer, fand_cache.pwm, sizeof(fand_cache.pwm));
    nbytes += sizeof(fand_cache.pwm);
    memcpy(buffer + nbytes, fand_cache.pwm_enable, sizeof(fand_cache.pwm_enable));
    nbytes += sizeof(fand_cache.pwm_enable);
    memcpy(buffer + nbytes, fand_cache.temp_input, sizeof(fand_cache.temp_input));
    nbytes += sizeof(fand_cache.temp_input);
    memcpy(buffer + nbytes, &fand_cache.card_idx, sizeof(fand_cache.card_idx));
    nbytes += sizeof(fand_cache.card_idx);

    sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, buffer, nbytes);
    sha1_final(&ctx, digest);

    memcpy(buffer + nbytes, digest, sizeof(digest));

    return nbytes + sizeof(digest);
}

static ssize_t cache_unpack(unsigned char *buffer, size_t bufsize) {
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
        memcpy(fand_cache.pwm, buffer, sizeof(fand_cache.pwm));
        nbytes += sizeof(fand_cache.pwm);
        memcpy(fand_cache.pwm_enable, buffer + nbytes, sizeof(fand_cache.pwm_enable));
        nbytes += sizeof(fand_cache.pwm_enable);
        memcpy(fand_cache.temp_input, buffer + nbytes, sizeof(fand_cache.temp_input));
        nbytes += sizeof(fand_cache.temp_input);
        memcpy(&fand_cache.card_idx, buffer + nbytes, sizeof(fand_cache.card_idx));
        nbytes += sizeof(fand_cache.card_idx);
        memcpy(&fand_cache.checksum, buffer + nbytes, sizeof(fand_cache.checksum));
        nbytes += sizeof(fand_cache.checksum);
    }
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
        syslog(LOG_INFO, "Corrupted cache file found, expected %zu bytes but read %zu", sizeof(fand_cache), (size_t)nbytes);
        status = -1;
    }

    if(close(fd) == -1) {
        syslog(LOG_WARNING, "Failed to close cache file descriptor: %s", strerror(errno));
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
            syslog(LOG_WARNING, "Failed to create cache directory: %s", strerror(errno));
            return -1;
        }
    }

    if(cache_pack(buffer, sizeof(buffer)) < 0) {
        return -1;
    }

    int fd = open(FAND_CACHE_FILE, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if(fd < 0) {
        syslog(LOG_WARNING, "Could not open cache file for writing: %s", strerror(errno));
        return -1;
    }

    if(write(fd, buffer, sizeof(buffer)) != sizeof(buffer)) {
        syslog(LOG_WARNING, "Failed to write cache file: %s", strerror(errno));
        status = -1;
    }

    if(close(fd) == -1) {
        syslog(LOG_WARNING, "Could not close cache file: %s", strerror(errno));
        status = -1;
    }

    return status;
}
