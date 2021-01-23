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

enum { MILLIDEGC_ADJUST = 1000 };

struct fanctrl_matrix {
    unsigned char rows;
    unsigned char temps[MATRIX_MAX_SIZE / 2];
    unsigned char speeds[MATRIX_MAX_SIZE / 2];
};

static struct fanctrl_matrix matrix;
static unsigned char hysteresis;
static short current_threshold;
static bool throttle;

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

int fanctrl_init(void) {
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
    #ifdef FAND_DRM_SUPPORT

    drm_close();

    #endif
    return hwmon_close();
}

int fanctrl_configure(struct fand_config *config) {
    current_threshold = -1;
    hysteresis = config->hysteresis;
    throttle = config->throttle;
    return fanctrl_set_matrix(config->matrix, config->matrix_rows);
}

int fanctrl_adjust(void) {
    int temp, speed;
    float frac;
    short threshold = -1;

    if(matrix.rows == 0) {
        syslog(LOG_ERR, "Matrix is empty");
        return FAND_FATAL_ERR;
    }

    speed = matrix.speeds[matrix.rows - 1];

    temp = fanctrl_get_temp();
    if(temp < 0) {
        return temp;
    }

    /* Below low threshold */
    if(temp <= matrix.temps[0]) {
        threshold = -1;
        speed = matrix.speeds[0] * !throttle;
    }
    else if(temp <= matrix.temps[matrix.rows - 1]) {
        /* Between two thresholds */
        for(unsigned i = 0; i < matrix.rows - 1u; i++) {
            if(matrix.temps[i] < temp && matrix.temps[i + 1] >= temp) {
                threshold = i;
                frac = lerp_inverse(matrix.temps[i], matrix.temps[i + 1], temp);
                speed = lerp(matrix.speeds[i], matrix.speeds[i + 1], frac);
                break;
            }
        }
    }
    else {
        /* Above high threshold */
        threshold = matrix.rows - 1;
        speed = matrix.speeds[matrix.rows - 1];
    }

    if(current_threshold > -1 && threshold < current_threshold && temp + hysteresis > matrix.temps[current_threshold]) {
        /* Hysteresis not yet surpassed */
        speed = matrix.speeds[current_threshold];
    }
    else {
        current_threshold = threshold;
    }

    return hwmon_write_pwm(fanctrl_percentage_to_pwm(speed));
}

int fanctrl_get_temp(void) {
    int temp;
    #ifdef FAND_DRM_SUPPORT

    temp = drm_get_temp();

    #else

    temp = hwmon_read_temp();

    #endif

    /* Millidegrees Celsius to degrees Celsius */
    return temp < 0 ? temp : temp / MILLIDEGC_ADJUST;
}

int fanctrl_get_speed(void) {
    int pwm = hwmon_read_pwm();
    if(pwm < 0) {
        return -1;
    }
    float frac = lerp_inverse(PWM_MIN, PWM_MAX, pwm);
    return (int)(100 * frac);
}

