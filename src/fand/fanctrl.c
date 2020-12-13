#include "drm.h"
#include "fanctrl.h"
#include "fandcfg.h"
#include "file.h"
#include "filesystem.h"
#include "hwmon.h"
#include "interpolation.h"
#include "strutils.h"

#include <errno.h>
#include <stdbool.h>

#include <fcntl.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct fanctrl_matrix {
    unsigned char rows;
    unsigned char temps[MATRIX_MAX_SIZE / 2];
    unsigned char speeds[MATRIX_MAX_SIZE / 2];
};

static struct fanctrl_matrix matrix;
static unsigned char hysteresis;
static bool throttle;

static bool dryrun = false;

static unsigned long fanctrl_percentage_to_pwm(unsigned long percentage) {
    float frac = (float)percentage / 100.f;
    unsigned long pwm = lerp(PWM_MIN, PWM_MAX, frac);
    return pwm;
}

static int fanctrl_set_matrix(unsigned char const* mat, unsigned char nrows) {
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

int fanctrl_init(bool init_dryrun) {
    if(init_dryrun) {
        dryrun = init_dryrun;
        return 0;
    }

    int status = 0;
    int card_idx = hwmon_open();

    if(card_idx < 0) {
        return card_idx;
    }

    #ifdef FAND_DRM_SUPPORT

    status = drm_open(card_idx);

    #endif

    return status;
}

int fanctrl_release(void) {
    if(dryrun) {
        /* Neither drm nor hwmon are opened in dry runs */
        return 0;
    }

    #ifdef FAND_DRM_SUPPORT

    drm_close();

    #endif
    return hwmon_close();
}

int fanctrl_configure(struct fand_config *config) {
    hysteresis = config->hysteresis;
    throttle = config->throttle;
    return fanctrl_set_matrix(config->matrix, config->matrix_rows);
}

int fanctrl_adjust(void) {
    int temp, speed;
    float frac;

    if(dryrun) {
        return 0;
    }

    if(matrix.rows == 0) {
        syslog(LOG_EMERG, "Matrix is empty");
        return FAND_FATAL_ERR;
    }

    speed = matrix.speeds[matrix.rows - 1];

    temp = fanctrl_get_temp();
    if(temp < 0) {
        return temp;
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

    return hwmon_write_pwm(fanctrl_percentage_to_pwm(speed));
}

int fanctrl_get_temp(void) {
    if(dryrun) {
        return 0;
    }

    #ifdef FAND_DRM_SUPPORT

    return drm_get_temp();

    #else

    return hwmon_read_temp();

    #endif
}

int fanctrl_get_speed(void) {
    if(dryrun) {
        return 0;
    }

    int pwm = hwmon_read_pwm();
    if(pwm < 0) {
        return -1;
    }
    float frac = inverse_lerp(PWM_MIN, PWM_MAX, pwm);
    return (int)(100 * frac);
}

