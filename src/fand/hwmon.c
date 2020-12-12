#include "cache.h"
#include "fandcfg.h"
#include "file.h"
#include "filesystem.h"
#include "hwmon.h"
#include "strutils.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <regex.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SYSFS_DRI_DEBUG "/sys/kernel/debug/dri"
#define AMDGPU_PM_FILE "amdgpu_pm_info"

#define SYSFS_HWMON_PATH_FMT "/sys/class/drm/card%u/device/hwmon/"

#define SYSFS_PWM "pwm1"
#define SYSFS_PWM_ENABLE "pwm1_enable"
#define SYSFS_TEMP_INPUT "temp1_input"

enum { PWM_MODE_MANUAL = 1 };
enum { PWM_MODE_AUTO = 2 };

enum { MAX_DRI_DIR_IDX = 128 };

static int hwmon_pwm_enable_fd = -1;

static char *hwmon_pwm = fand_cache.pwm;
static char *hwmon_pwm_enable = fand_cache.pwm_enable;
static char *hwmon_temp_input = fand_cache.temp_input;

static size_t hwmon_pwm_size = sizeof(fand_cache.pwm);
static size_t hwmon_pwm_enable_size = sizeof(fand_cache.pwm_enable);
static size_t hwmon_temp_input_size = sizeof(fand_cache.temp_input);

static int hwmon_detect_card_index(void) {
    char buffer[HWMON_PATH_SIZE];
    int card_idx = -1;

    for(int i = 0; i < MAX_DRI_DIR_IDX; i++) {
        if((size_t)snprintf(buffer, sizeof(buffer), "%s/%d/%s", SYSFS_DRI_DEBUG, i, AMDGPU_PM_FILE) >= sizeof(buffer)) {
            syslog(LOG_ERR, "Sysfs kernel path %s/%d/%s overflows the internal buffer", SYSFS_DRI_DEBUG, i, AMDGPU_PM_FILE);
            return -1;
        }
        if(fsys_file_exists(buffer)) {
            card_idx = i;
            break;
        }
    }

    return card_idx;
}

static ssize_t hwmon_detect_iface_dir(char *dst, unsigned card_idx, size_t dstsize) {
    char buffer[HWMON_PATH_SIZE];

    int pos = snprintf(buffer, sizeof(buffer), SYSFS_HWMON_PATH_FMT, card_idx);
    ssize_t status = 0;

    if((size_t)pos > sizeof(buffer)) {
        syslog(LOG_ERR, "Sysfs hwmon path "SYSFS_HWMON_PATH_FMT" overflows the internal buffer", card_idx);
        return -1;
    }

    regex_t hwmon_regex;
    int reti = regcomp(&hwmon_regex, "hwmon[0-9]+", REG_EXTENDED);

    if(reti) {
        regerror(reti, &hwmon_regex, buffer, sizeof(buffer));
        syslog(LOG_EMERG, "Failed to compile hwmon regex: %s", buffer);
        return -1;
    }

    struct dirent *dp;
    DIR *dir = opendir(buffer);

    while((dp = readdir(dir))) {
        if(regexec(&hwmon_regex, dp->d_name, 0, 0, 0)) {
            continue;
        }
        if(strscpy(buffer + pos, dp->d_name, sizeof(buffer) - pos) < 0) {
            status = -1;
            goto cleanup;
        }
        break;
    }

    /* Found matching entry */
    if(dp) {
        status = fsys_abspath(dst, buffer, dstsize);
        int err = errno;
        if(status == -1 && err != EINVAL) {
            syslog(LOG_ERR, "Could not follow symlink %s: %s", buffer, strerror(err));
            goto cleanup;
        }
    }

cleanup:
    closedir(dir);

    return status;
}

static ssize_t hwmon_init_single_sysfs_path(char *dst, char const *hwmon_iface_dir, char const *filename, size_t dstsize) {
    ssize_t pos = strscpy(dst, hwmon_iface_dir, dstsize);
    if(pos < 0) {
        return pos;
    }
    pos += strscpy(dst + pos, "/", dstsize  - pos);
    if(pos < 0) {
        return pos;
    }
    return strscpy(dst + pos, filename, dstsize - pos);
}

static int hwmon_set_pwm_mode_manual(void) {
    hwmon_pwm_enable_fd = fdopen_excl(hwmon_pwm_enable, O_WRONLY);
    if(hwmon_pwm_enable_fd == -1) {
        return -1;
    }
    return -!!fdwrite_ulong(hwmon_pwm_enable_fd, PWM_MODE_MANUAL);
}


static int hwmon_set_sysfs_paths(void) {
    char hwmon_iface[HWMON_PATH_SIZE];
    ssize_t status;

    int card_idx = hwmon_detect_card_index();
    if(card_idx < 0) {
        return card_idx;
    }

    fand_cache.card_idx = (unsigned)card_idx;

    status = hwmon_detect_iface_dir(hwmon_iface, card_idx, sizeof(hwmon_iface));
    if(status < 0) {
        return status;
    }

    status = hwmon_init_single_sysfs_path(hwmon_temp_input, hwmon_iface, SYSFS_TEMP_INPUT, hwmon_temp_input_size);
    if(status < 0) {
        return status;
    }

    status = hwmon_init_single_sysfs_path(hwmon_pwm, hwmon_iface, SYSFS_PWM, hwmon_pwm_size);
    if(status < 0) {
        return status;
    }

    return hwmon_init_single_sysfs_path(hwmon_pwm_enable, hwmon_iface, SYSFS_PWM_ENABLE, hwmon_pwm_enable_size);
}

int hwmon_open(void) {
    int status;
    if(cache_load()) {
        status = hwmon_set_sysfs_paths();
        if(status < 0) {
            return status;
        }
        status = cache_write();
        if(status < 0) {
            return status;
        }
    }

    status = hwmon_set_pwm_mode_manual();
    if(status < 0) {
        return status;
    }

    return (int)fand_cache.card_idx;
}

int hwmon_close(void) {
    if(hwmon_pwm_enable_fd == -1) {
        syslog(LOG_WARNING, "No open control mode file descriptor");
        return 0;
    }

    fdwrite_ulong(hwmon_pwm_enable_fd, PWM_MODE_AUTO);

    return fdclose_excl(hwmon_pwm_enable_fd);
}

int hwmon_read_temp(void) {
    unsigned long temp;
    if(fread_ulong_excl(hwmon_temp_input, &temp)) {
        return -1;
    }
    temp /= MILLIDEGC_ADJUST;
    return (int)temp;
}

int hwmon_read_pwm(void) {
    unsigned long pwm;
    if(fread_ulong_excl(hwmon_pwm, &pwm)) {
        return -1;
    }
    return (int)pwm;
}

int hwmon_write_pwm(unsigned long pwm) {
    if(pwm > PWM_MAX) {
        syslog(LOG_ERR, "Invalid pwm %lu", pwm);
        return FAND_FATAL_ERR;
    }

    return fwrite_ulong_excl(hwmon_pwm, pwm);
}

