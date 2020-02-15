#include "fancontroller.h"
#include "filesystem.h"
#include "hwmon.h"
#include "logger.h"
#include "procs.h"
#include "strutils.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <sys/file.h>
#include <unistd.h>

#define TEMP_BUF_SIZE 8
#define PWM_BUF_SIZE 16

#define PWM_ENABLE_FILE "/pwm1_enable"
#define TEMP_INPUT_FILE "/temp1_input"
#define PWM_FILE "/pwm1"
#define PWM_MIN_FILE "/pwm1_min"
#define PWM_MAX_FILE "/pwm1_max"

static char pwm_enable[HWMON_PATH_LEN];
static char temp_input[HWMON_PATH_LEN];
static char pwm[HWMON_PATH_LEN];
static char pwm_min_path[HWMON_PATH_LEN];
static char pwm_max_path[HWMON_PATH_LEN];

static uint8_t pwm_min;
static uint8_t pwm_max;

static int8_t fan_speed = -1;

static pid_t ppid_override = -1;

static bool aggressive_throttle = false;
static enum interpolation_method interp = linear;
static enum speed_interface speed_iface = sifc_tacho;

static matrix mtrx;
static uint8_t mtrx_rows;

static FILE *file_open_excl(char const *restrict path, char const *restrict mode) {
    FILE *fp = fopen(path, mode);
    if(!fp) {
        return NULL;
    }

    int fd = fileno(fp);
    if(flock(fd, LOCK_EX) == -1) {
        perror("Lock file");
        fclose(fp);
        return NULL;
    }

    return fp;
}

static void file_close_excl(FILE *fp) {
    int fd = fileno(fp);
    if(flock(fd, LOCK_UN) == -1) {
        perror("Unlock file");
    }
    fclose(fp);
}

static bool read_long_from_file(char const *path, long *data) {
    FILE *fp = file_open_excl(path, "r");
    if(!fp) {
        fprintf(stderr, "Failed to open %s for reading\n", path);
        return false;
    }
    char buffer[PWM_BUF_SIZE] = { 0 };
    fgets(buffer, sizeof buffer, fp);
    file_close_excl(fp);

    replace_char(buffer, '\n', '\0');

    if(!buffer[0]) {
        fprintf(stderr, "%s is empty\n", path);
        return false;
    }
    char *endptr;

    long const value = strtol(buffer, &endptr, 10);
    if(*endptr) {
        fprintf(stderr, "%s contains non-digit characters\n", buffer);
        return false;
    }

    *data = value;

    return true;

}

static bool read_uint8_from_file(char const *path, uint8_t *data) {
    long value;
    if(!read_long_from_file(path, &value)) {
        return false;
    }
    if(value < 0 || value > UINT8_MAX) {
        fprintf(stderr, "%ld is not an unsigned 8-bit value\n", value);
        return false;
    }

    *data = (uint8_t)value;
    return true;
}

static bool read_uint32_from_file(char const *path, uint32_t *data) {
    long value;
    if(!read_long_from_file(path, &value)) {
        return false;
    }
    if(value < 0 || value > UINT32_MAX) {
        fprintf(stderr, "%ld is not an unsigned 32-bit value\n", value);
        return false;
    }
    *data = (uint32_t)value;
    return true;
}

static bool write_uint8_to_file(char const *path, uint8_t data) {
    FILE *fp = file_open_excl(path, "w");
    if(!fp) {
        fprintf(stderr, "Failed to open %s for writing\n", path);
        return false;
    }

    fprintf(fp, "%u", data);
    file_close_excl(fp);
    return true;
}

static int8_t get_lower_row_idx_of_temp(uint8_t temp, uint8_t *temps) {
    if(temp < temps[0]) {
        return -1;
    }

    for(uint8_t i = 0; i < mtrx_rows - 1; i++) {
        if(temp < temps[i + 1]) {
            return i;
        }
    }

    return mtrx_rows - 1;
}

static bool construct_file_path(char *restrict dst, char const *restrict dir, char const *restrict file, size_t count) {
    dst[0] = '\0';
    if(strscat(dst, dir, count) < 0) {
        fprintf(stderr, "Directory %s overflows the destination buffer\n", dir);
        return false;
    }
    if(strscat(dst, file, count) < 0) {
        fprintf(stderr, "Appending %s to %s would overflow the buffer\n", file, dst);
        return false;
    }
    if(!file_exists(dst)) {
        fprintf(stderr, "%s does not exist\n", dst);
        return false;
    }
    return true;
}

bool amdgpu_fan_setup_pwm_enable_file(char const *hwmon_path) {
    if(!construct_file_path(pwm_enable, hwmon_path, PWM_ENABLE_FILE, sizeof pwm_enable) || !file_accessible(pwm_enable, W_OK, NULL)) {
        return false;
    }
    LOG(VERBOSITY_LVL2, "PWM control file set to %s\n", pwm_enable);
    return true;
}

bool amdgpu_fan_setup_temp_input_file(char const *hwmon_path) {
    if(!construct_file_path(temp_input, hwmon_path, TEMP_INPUT_FILE, sizeof temp_input) || !file_accessible(temp_input, R_OK, NULL)) {
        return false;
    }
    LOG(VERBOSITY_LVL2, "Temperature file set to %s\n", temp_input);
    return true;
}

bool amdgpu_fan_setup_pwm_file(char const *hwmon_path) {
    if(!construct_file_path(pwm, hwmon_path, PWM_FILE, sizeof pwm) || !file_accessible(pwm, W_OK, NULL)) {
        return false;
    }
    LOG(VERBOSITY_LVL2, "PWM level file set to %s\n", pwm);
    return true;
}

bool amdgpu_fan_setup_pwm_min_file(char const *hwmon_path) {
    if(!construct_file_path(pwm_min_path, hwmon_path, PWM_MIN_FILE, sizeof pwm_min_path) || !file_accessible(pwm_min_path, R_OK, NULL)) {
        return false;
    }
    LOG(VERBOSITY_LVL2, "PWM min file set to %s\n", pwm_min_path);
    return true;
}

bool amdgpu_fan_setup_pwm_max_file(char const *hwmon_path) {
    if(!construct_file_path(pwm_max_path, hwmon_path, PWM_MAX_FILE, sizeof pwm_max_path) || !file_accessible(pwm_max_path, R_OK, NULL)) {
        return false;
    }
    LOG(VERBOSITY_LVL2, "PWM max file set to %s\n", pwm_max_path);
    return true;
}

bool amdgpu_fan_store_pwm_min(void) {
    return read_uint8_from_file(pwm_min_path, &pwm_min);
}

bool amdgpu_fan_store_pwm_max(void) {
    return read_uint8_from_file(pwm_max_path, &pwm_max);
}

void amdgpu_fan_set_aggressive_throttle(bool throttle) {
    aggressive_throttle = throttle;
}

void amdgpu_fan_set_interpolation_method(enum interpolation_method method) {
    interp = method;
}

void amdgpu_fan_set_speed_interface(enum speed_interface iface) {
    speed_iface = iface;
}

void amdgpu_fan_set_matrix(matrix m, uint8_t m_rows) {
    mtrx_rows = m_rows;
    for(size_t i = 0; i < m_rows; i++) {
        mtrx[i][0] = m[i][0];
        mtrx[i][1] = m[i][1];
    }
}

ssize_t amdgpu_fan_get_pwm_path(char *buffer, size_t count) {
    ssize_t len = strscpy(buffer, pwm, count);
    if(len < 0) {
        fprintf(stderr, "Pwm path overflows buffer\n");
    }
    return len;
}

bool amdgpu_fan_get_aggressive_throttle(void) {
    return aggressive_throttle;
}

enum interpolation_method amdgpu_fan_get_interpolation_method(void) {
    return interp;
}

void amdgpu_fan_get_matrix(matrix m, uint8_t *m_rows) {
    *m_rows = mtrx_rows;
    for(uint8_t i = 0; i < mtrx_rows; i++) {
        m[i][0] = mtrx[i][0];
        m[i][1] = mtrx[i][1];
    }
}

bool amdgpu_fan_set_mode(enum fanmode mode) {
    LOG(VERBOSITY_LVL1, "Setting fan mode: %s\n", mode == manual ? "manual" : "auto");
    return write_uint8_to_file(pwm_enable, mode);
}

bool amdgpu_fan_get_percentage(uint8_t *percentage) {
    if(fan_speed < 0 || speed_iface == sifc_tacho) {
        uint8_t npwm;
        if(!read_uint8_from_file(pwm, &npwm)) {
            return false;
        }
        double const frac = (double)npwm / (double)(pwm_max - pwm_min);
        *percentage = (uint8_t)round((frac * 100));
        fan_speed = *percentage;
        LOG(VERBOSITY_LVL3, "Current pwm: %u, corresponding to percentage: %u\n", npwm, *percentage);
    }
    else {
        *percentage = (uint8_t)fan_speed;
    }
    return true;
}

bool amdgpu_fan_set_percentage(uint8_t percentage) {
    if(percentage == fan_speed) {
        LOG(VERBOSITY_LVL3, "Fan speed unchanged (%u%%)\n", percentage);
        return true;
    }
    fan_speed = percentage;
    uint8_t const npwm = (uint8_t)((double)percentage / 100.0 * (double)(pwm_max - pwm_min));
    LOG(VERBOSITY_LVL3, "Setting pwm %u (%f%%)\n", npwm, 100.0 * (double)npwm / (double)(pwm_max - pwm_min));
    return write_uint8_to_file(pwm, npwm);
}

bool amdgpu_get_temp(uint8_t *temp) {
    uint32_t value;
    if(read_uint32_from_file(temp_input, &value)) {
        /* Remove 3 trailing zeroes */
        *temp = value / 1000;
        LOG(VERBOSITY_LVL3, "Temperature is %u\n", *temp);
        return true;
    }

    return false;
}

void amdgpu_fan_set_override_speed(uint8_t speed, pid_t ppid) {
    LOG(VERBOSITY_LVL1, "Setting override speed to %u, tied to pid %d\n", speed, ppid);
    ppid_override = ppid;
    amdgpu_fan_set_percentage(speed);
}

void amdgpu_fan_reset_override_speed(void) {
    LOG(VERBOSITY_LVL1, "Resetting override speed, falling back on matrix\n");
    ppid_override = -1;
}

bool amdgpu_fan_update_speed(void) {
    if(ppid_override != -1) {
        bool success = amdgpu_fan_set_percentage(fan_speed);
        if(ppid_override != DETACH_FROM_SHELL && !proc_alive(ppid_override)) {
            LOG(VERBOSITY_LVL1, "Process %d no longer alive, falling back on matrix\n", ppid_override);
            ppid_override = -1;
        }
        return success;
    }

    uint8_t temp;
    if(!amdgpu_get_temp(&temp)) {
        return false;
    }

    uint8_t temps[MATRIX_ROWS] = { 0 };
    matrix_extract_temps(temps, mtrx, mtrx_rows);

    int8_t idx = get_lower_row_idx_of_temp(temp, temps);

    uint8_t speeds[MATRIX_ROWS];
    matrix_extract_speeds(speeds, mtrx, mtrx_rows);

    if(idx < 0 || (aggressive_throttle && idx == 0)) {
        return amdgpu_fan_set_percentage(speeds[0]);
    }
    else if(idx >= mtrx_rows - 1) {
        return amdgpu_fan_set_percentage(speeds[mtrx_rows - 1u]);
    }

    uint8_t const percentage = inverse_lerp_uint8(temps[idx], temps[idx + 1], temp);
    uint8_t const npwm = interp == linear ? lerp_uint8(speeds[idx], speeds[idx + 1], (double)percentage / 100.0) :
                                            cosine_interpolate_uint8(speeds[idx], speeds[idx + 1], (double)percentage / 100.0);
    return amdgpu_fan_set_percentage(npwm);
}
