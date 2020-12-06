#include "defs.h"
#include "fanctrl.h"
#include "file.h"
#include "filesystem.h"
#include "interp.h"
#include "strutils.h"

#include <errno.h>
#include <stdbool.h>

#include <fcntl.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

enum { FANCTRL_MODE_MANUAL = 1 };
enum { FANCTRL_MODE_AUTO = 2 };
enum { FANCTRL_TEMP_ADJUST = 1000 };

struct fanctrl_matrix {
    unsigned char rows;
    unsigned char temps[MATRIX_MAX_SIZE / 2];
    unsigned char speeds[MATRIX_MAX_SIZE / 2];
};

static int ctrl_mode_fd = -1;
static unsigned long pwm_min, pwm_max;

static struct fanctrl_matrix matrix;
static unsigned char hysteresis;
static bool throttle;

static char sysdir[DEVICE_PATH_MAX_SIZE];
static char ctrl_mode_path[DEVICE_PATH_MAX_SIZE];
static char temp_path[DEVICE_PATH_MAX_SIZE];
static char pwm_ctrl_path[DEVICE_PATH_MAX_SIZE];

static int fanctrl_read_temp(unsigned long *temp) {
    int status = fread_ulong_excl(temp_path, temp);
    if(status) {
        return status;
    }
    *temp /= FANCTRL_TEMP_ADJUST;
    return 0;
}

static int fanctrl_read_speed(unsigned long *speed) {
    unsigned long pwm;
    int status = fread_ulong_excl(pwm_ctrl_path, &pwm);
    if(status) {
        return status;
    }

    float frac = inverse_lerp(pwm_min, pwm_max, pwm);
    *speed = (unsigned long)(100 * frac);
    return 0;
}

static int fanctrl_write_speed(unsigned long percentage) {
    if(percentage > 100) {
        syslog(LOG_ERR, "Invalid percentage %lu", percentage);
        return FAND_FATAL_ERR;
    }

    float frac = (float)percentage / 100.f;
    unsigned long pwm = lerp(pwm_min, pwm_max, frac);

    return fwrite_ulong_excl(pwm_ctrl_path, pwm);
}

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

static int fanctrl_init_pwm_ctrl_path(char const *pwm_ctrl_filename) {
    ssize_t pos = strscpy(pwm_ctrl_path, sysdir, sizeof(pwm_ctrl_path));
    if(pos < 0) {
        return pos;
    }
    pos = fsys_append(pwm_ctrl_path, pwm_ctrl_filename, sizeof(pwm_ctrl_path));
    if(pos < 0) {
        return pos;
    }

    if(!fsys_file_exists(pwm_ctrl_path)) {
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

static int fanctrl_init_matrix(unsigned char const* mat, unsigned char nrows) {
    if(nrows > MATRIX_MAX_SIZE / 2) {
        return -1;
    }

    matrix.rows = nrows;
    for(unsigned i = 0; i < nrows; i++) {
        matrix.temps[i] = mat[2 * i];
        matrix.speeds[i] = mat[2 * i + 1];
    }
    return 0;
}

int fanctrl_init(struct fand_config *config) {
    int status = fanctrl_init_sysdir(config->devpath, config->hwmon)      |
                 fanctrl_init_ctrl_mode_path(config->ctrl_mode)           |
                 fanctrl_init_temp_path(config->temp_sensor)              |
                 fanctrl_init_pwm_ctrl_path(config->pwm_ctrl)             |
                 fanctrl_init_pwm_limits(config->pwm_min, config->pwm_max);
    if(status) {
        return status;
    }

    hysteresis = config->hysteresis;
    throttle = config->throttle;

    return fanctrl_init_matrix(config->matrix, config->matrix_rows);
}

int fanctrl_acquire(void) {
    ctrl_mode_fd = fdopen_excl(ctrl_mode_path, O_WRONLY);

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

int fanctrl_adjust(void) {
    unsigned long temp, speed;
    int status;
    float frac;

    if(matrix.rows == 0) {
        syslog(LOG_EMERG, "Matrix is empty");
        return FAND_FATAL_ERR;
    }

    speed = matrix.speeds[matrix.rows - 1];

    status = fanctrl_read_temp(&temp);
    if(status) {
        return status;
    }

    /* TODO: add hysteresis support */

    /* Below low threshold */
    if(temp <= matrix.temps[0]) {
        speed = matrix.speeds[0] * !throttle;
    }
    else if(temp <= matrix.temps[matrix.rows - 1]) {
        /* Between two thresholds */
        for(unsigned i = 0; i < matrix.rows - 1u; i++) {
            if(matrix.temps[i] < temp && matrix.temps[i + 1] >= temp) {
                frac = inverse_lerp(matrix.temps[i], matrix.temps[i + 1], temp);
                speed = lerp(matrix.speeds[i], matrix.speeds[i + 1], frac);
                break;
            }
        }
    }
    else {
        /* Above high threshold */
        speed = matrix.speeds[matrix.rows - 1];
    }

    return fanctrl_write_speed(speed);
}

int fanctrl_get_speed(void) {
    unsigned long speed;
    int status = fanctrl_read_speed(&speed);

    if(status) {
        return -1;
    }

    return (int)speed;
}

int fanctrl_get_temp(void) {
    unsigned long temp;
    int status = fanctrl_read_temp(&temp);

    if(status) {
        return -1;
    }

    return (int)temp;
}
