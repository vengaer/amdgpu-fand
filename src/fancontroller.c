#include "fancontroller.h"
#include "interpolation.h"
#include "strutils.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <regex.h>

#define TEMP_BUF_SIZE 8
#define PWM_BUF_SIZE 16

static char const *pwm_enable_file = "/pwm1_enable";
static char const *temp_input_file = "/temp1_input";
static char const *pwm_file = "/pwm1";
static char const *pwm_min_file = "/pwm1_min";
static char const *pwm_max_file = "/pwm1_max";

extern bool verbose;

static char pwm_enable[HWMON_PATH_LEN];
static char temp_input[HWMON_PATH_LEN];
static char pwm[HWMON_PATH_LEN];
static char pwm_min_path[HWMON_PATH_LEN];
static char pwm_max_path[HWMON_PATH_LEN];

static uint8_t pwm_min;
static uint8_t pwm_max;

static bool aggressive_throttle;

static matrix mtrx;
static uint8_t mtrx_rows;

static bool read_long_from_file(char const *path, long *data) {
    FILE *fp = fopen(path, "r");
    if(!fp) {
        fprintf(stderr, "Failed to open %s for reading\n", path);
        return false;
    }
    char buffer[PWM_BUF_SIZE] = { 0 };
    fgets(buffer, sizeof buffer, fp);
    fclose(fp);

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
    FILE *fp = fopen(path, "w");
    if(!fp) {
        fprintf(stderr, "Failed to open %s for writing\n", path);
        return false;
    }

    fprintf(fp, "%u", data);
    fclose(fp);
    return true;
}

static int16_t get_lower_row_idx_of_temp(uint8_t temp, uint8_t *temps) {
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

bool amdgpu_fan_setup_pwm_enable_file(char const *hwmon_path) {
    pwm_enable[0] = '\0';
    if(strscat(pwm_enable, hwmon_path, sizeof pwm_enable) < 0) {
        fprintf(stderr, "hwmon path overflows pwm_enable buffer\n");
        return false;
    }
    if(strscat(pwm_enable, pwm_enable_file, sizeof pwm_enable) < 0) {
        fprintf(stderr, "Appending %s to pwm_enable overflows the buffer\n", pwm_enable_file);
        return false;
    }
    if(verbose) {
        printf("PWM control file set to %s\n", pwm_enable);
    }
    return true;
}

bool amdgpu_fan_setup_temp_input_file(char const *hwmon_path) {
    temp_input[0] = '\0';
    if(strscat(temp_input, hwmon_path, sizeof temp_input) < 0) {
        fprintf(stderr, "hwmon path overflows the temp_input buffer\n");
        return false;
    }
    if(strscat(temp_input, temp_input_file, sizeof temp_input) < 0) {
        fprintf(stderr, "Appending %s to temp_input overflows the buffer\n", temp_input_file);
        return false;
    }
    if(verbose) {
        printf("Temperature file set to %s\n", temp_input);
    }
    return true;
}

bool amdgpu_fan_setup_pwm_file(char const *hwmon_path) {
    pwm[0] = '\0';
    if(strscat(pwm, hwmon_path, sizeof pwm) < 0) {
        fprintf(stderr, "hwmon path overflows the pwm buffer\n");
        return false;
    }
    if(strscat(pwm, pwm_file, sizeof pwm) < 0) {
        fprintf(stderr, "Appending %s to pwm overflows the buffer\n", pwm_file);
        return false;
    }
    if(verbose) {
        printf("PWM level file set to %s\n", pwm);
    }
    return true;
}

bool amdgpu_fan_setup_pwm_min_file(char const *hwmon_path) {
    pwm_min_path[0] = '\0';
    if(strscat(pwm_min_path, hwmon_path, sizeof pwm_min_path) < 0) {
        fprintf(stderr, "hwmon path overflows the pwm_min buffer\n");
        return false;
    }
    if(strscat(pwm_min_path, pwm_min_file, sizeof pwm_min_path) < 0) {
        fprintf(stderr, "Appending %s to the pwm_min path overflows the buffer\n", pwm_min_file);
        return false;
    }
    if(verbose) {
        printf("PWM min file set to %s\n", pwm_min_path);
    }
    return true;
}

bool amdgpu_fan_setup_pwm_max_file(char const *hwmon_path) {
    pwm_max_path[0] = '\0';
    if(strscat(pwm_max_path, hwmon_path, sizeof pwm_max_path) < 0) {
        fprintf(stderr, "hwmon path overflows the pwm_max buffer\n");
        return false;
    }
    if(strscat(pwm_max_path, pwm_max_file, sizeof pwm_max_path) < 0) {
        fprintf(stderr, "Appending %s to the pwm_max path overflows the buffer\n", pwm_max_file);
        return false;
    }
    if(verbose) {
        printf("PWM max file set to %s\n", pwm_max_path);
    }
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

void amdgpu_fan_set_matrix(matrix m, uint8_t m_rows) {
    mtrx_rows = m_rows;
    for(size_t i = 0; i < m_rows; i++) {
        mtrx[i][0] = m[i][0];
        mtrx[i][1] = m[i][1];
    }
}

bool amdgpu_fan_set_mode(enum fanmode mode) {
    if(verbose) {
        printf("Setting fan mode: %s\n", mode == manual ? "manual" : "auto");
    }
    return write_uint8_to_file(pwm_enable, mode);
}

bool amdgpu_fan_get_percentage(uint8_t *percentage) {
    uint8_t npwm;
    if(read_uint8_from_file(pwm, &npwm)) {
        double const frac = (double)npwm / (double)(pwm_max - pwm_min);
        *percentage = (uint8_t)round((frac * 100));
        if(verbose) {
            printf("Current pwm: %u, corresponding to percentage: %u\n", npwm, *percentage);
        }
        return true;
    }

    return false;
}

bool amdgpu_fan_set_percentage(uint8_t percentage) {
    uint8_t const npwm = (uint8_t)((double)percentage / 100.0 * (double)(pwm_max - pwm_min));
    if(verbose) {
        printf("Setting pwm %u (%f%%)\n", npwm, 100.0 * (double)npwm / (double)(pwm_max - pwm_min));
    }
    return write_uint8_to_file(pwm, npwm);
}

bool amdgpu_get_temp(uint8_t *temp) {
    uint32_t value;
    if(read_uint32_from_file(temp_input, &value)) {
        /* Remove 3 trailing zeroes */
        char buffer[TEMP_BUF_SIZE] = { 0 };
        sprintf(buffer, "%u", value);
        size_t const len = strlen(buffer);
        if(len < 5u) {
            fprintf(stderr, "%s shows highly unlikely temperature\n", buffer);
            return false;
        }
        buffer[len - 3] = '\0';

        *temp = atoi(buffer);
        if(verbose) {
            printf("Temperature is %u\n", *temp);
        }
        return true;
    }

    return false;
}

bool amdgpu_fan_update_speed(void) {
    uint8_t temp;
    if(!amdgpu_get_temp(&temp)) {
        return false;
    }

    uint8_t temps[MATRIX_ROWS];
    matrix_extract_temps(temps, mtrx, mtrx_rows);

    int16_t idx = get_lower_row_idx_of_temp(temp, temps);

    uint8_t speeds[MATRIX_ROWS];
    matrix_extract_speeds(speeds, mtrx, mtrx_rows);

    if(idx < 0 || (aggressive_throttle && idx == 0)) {
        return amdgpu_fan_set_percentage(speeds[0]);
    }
    else if(idx >= mtrx_rows - 1) {
        return amdgpu_fan_set_percentage(speeds[mtrx_rows - 1u]);
    }

    uint8_t const percentage = inverse_lerp_uint8(temps[idx], temps[idx + 1], temp);
    uint8_t const npwm = lerp_uint8(speeds[idx], speeds[idx + 1], (double)percentage / 100.0);
    return amdgpu_fan_set_percentage(npwm);
}
