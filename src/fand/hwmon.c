#include "defs.h"
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
enum { HWMON_PATH_SIZE = 256 };

static int hwmon_pwm_enable_fd = -1;

static char hwmon_pwm[HWMON_PATH_SIZE];
static char hwmon_pwm_enable[HWMON_PATH_SIZE];
static char hwmon_temp_input[HWMON_PATH_SIZE];

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

    /* + 1 for null byte */
    int pos = snprintf(buffer, sizeof(buffer), SYSFS_HWMON_PATH_FMT, card_idx) + 1;
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
    }

    /* Found matching entry */
    if(dp) {
        status = readlink(buffer, dst, dstsize);
        if(status == -1) {
            syslog(LOG_ERR, "Following hwmon iface %s symlink overflows internal buffer of size %zu", buffer, dstsize);
            goto cleanup;
        }
    }

cleanup:
    closedir(dir);

    return status;
}

static ssize_t hwmon_init_sysfs_path(char *dst, char const *hwmon_iface_dir, char const *filename, size_t dstsize) {
    ssize_t pos = strscpy(dst, hwmon_iface_dir, dstsize);
    if(pos < 0) {
        return pos;
    }
    pos = strscpy(dst + pos, "/", dstsize  - pos);
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

int hwmon_open(void) {
    /* TODO: check cache */
    char hwmon_iface[HWMON_PATH_SIZE];
    ssize_t status;

    int card_idx = hwmon_detect_card_index();
    if(card_idx < 0) {
        return card_idx;
    }

    status = hwmon_detect_iface_dir(hwmon_iface, card_idx, sizeof(hwmon_iface));
    if(status < 0) {
        return status;
    }
    #ifndef FAND_DRM_SUPPORT

    status = hwmon_init_sysfs_path(hwmon_temp_input, hwmon_iface, SYSFS_TEMP_INPUT, sizeof(hwmon_temp_input));
    if(status < 0) {
        return status;
    }

    #endif

    status = hwmon_init_sysfs_path(hwmon_pwm, hwmon_iface, SYSFS_PWM, sizeof(hwmon_pwm));
    if(status < 0) {
        return status;
    }

    status = hwmon_init_sysfs_path(hwmon_pwm_enable, hwmon_iface, SYSFS_PWM_ENABLE, sizeof(hwmon_pwm_enable));
    if(status < 0) {
        return status;
    }

    return hwmon_set_pwm_mode_manual();
}

int hwmon_close(void) {
    if(hwmon_pwm_enable_fd == -1) {
        syslog(LOG_ERR, "No open control mode file descriptor");
        return -1;
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

