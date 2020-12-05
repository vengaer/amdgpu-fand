#include "defs.h"
#include "fanctrl.h"
#include "file.h"
#include "filesystem.h"
#include "strutils.h"

#include <errno.h>

#include <fcntl.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


enum { FANCTRL_MODE_MANUAL = 1 };
enum { FANCTRL_MODE_AUTO = 2 };

static int ctrl_mode_fd = -1;
static unsigned long pwm_min, pwm_max;

static char sysdir[DEVICE_PATH_MAX_SIZE];
static char ctrl_mode_path[DEVICE_PATH_MAX_SIZE];
static char temp_path[DEVICE_PATH_MAX_SIZE];

static int fanctrl_init_sysdir(char const *restrict devpath, char const *restrict hwmon) {
    ssize_t pos = strscpy(sysdir, devpath, sizeof(sysdir));
    if(pos < 0) {
        return pos;
    }

    pos = fsys_append(sysdir, hwmon, sizeof(sysdir));
    if(pos < 0) {
        return pos;
    }

    if(!fsys_dir_exists(sysdir)) {
        return -ENOENT;
    }

    return 0;
}

static int fanctrl_init_ctrl_mode_path(char const *ctrl_mode_filename) {
    ssize_t pos = strscpy(ctrl_mode_path, sysdir, sizeof(ctrl_mode_path));
    if(pos < 0) {
        return pos;
    }
    pos = fsys_append(ctrl_mode_path, ctrl_mode_filename, sizeof(ctrl_mode_path));
    if(pos < 0) {
        return pos;
    }

    if(!fsys_file_exists(ctrl_mode_path)) {
        return -ENOENT;
    }

    return 0;
}

static int fanctrl_init_temp_path(char const *temp_filename) {
    ssize_t pos = strscpy(temp_path, sysdir, sizeof(temp_path));
    if(pos < 0) {
        return pos;
    }
    pos = fsys_append(temp_path, temp_filename, sizeof(temp_path));
    if(pos < 0) {
        return pos;
    }

    if(!fsys_file_exists(temp_path)) {
        return -ENOENT;
    }

    return 0;
}

static int fanctrl_init_pwm_from_path(char *restrict buffer, size_t bufsize, char const *restrict filename, unsigned long *restrict pwm) {
    ssize_t pos = fsys_append(buffer, filename, bufsize);
    if(pos < 0) {
        return pos;
    }

    if(!fsys_file_exists(buffer)) {
        return -1;
    }

    return fread_ulong_excl(buffer, pwm);
}

static int fanctrl_init_pwm_limits(char const *restrict pwm_min_filename, char const* restrict pwm_max_filename) {
    char buffer[DEVICE_PATH_MAX_SIZE];
    ssize_t pos;

    ssize_t baselen = strscpy(buffer, sysdir, sizeof(buffer));
    if(baselen < 0) {
        return baselen;
    }
    pos = fanctrl_init_pwm_from_path(buffer, sizeof(buffer), pwm_min_filename, &pwm_min);
    if(pos) {
        return pos;
    }

    buffer[baselen] = '\0';

    return fanctrl_init_pwm_from_path(buffer, sizeof(buffer), pwm_max_filename, &pwm_max);
}

int fanctrl_init(struct fand_config *config) {
    int status = fanctrl_init_sysdir(config->devpath, config->hwmon);
    if(status) {
        return status;
    }
    status = fanctrl_init_ctrl_mode_path(config->ctrl_mode);
    if(status) {
        return status;
    }
    status = fanctrl_init_temp_path(config->temp_sensor);
    if(status) {
        return status;
    }
    return fanctrl_init_pwm_limits(config->pwm_min, config->pwm_max);
}

int fanctrl_acquire(void) {
    ctrl_mode_fd = fdopen_excl(ctrl_mode_path, O_RDONLY);

    if(ctrl_mode_fd == -1) {
        return -1;
    }

    if(fdwrite_ulong(ctrl_mode_fd, 1ul)) {
        return -1;
    }

    return 0;
}

int fanctrl_release(void) {
    if(ctrl_mode_fd == -1) {
        syslog(LOG_ERR, "No open control mode file descriptor");
        return -1;
    }

    (void)fdwrite_ulong(ctrl_mode_fd, 2ul);

    return fdclose_excl(ctrl_mode_fd);
}
