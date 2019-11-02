#include "fancontroller.h"
#include "strutils.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <regex.h>

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

uint8_t pwm_min;
uint8_t pwm_max;

static bool read_number_from_file(char const *path, uint8_t *data) {
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

    if(value < 0 || value > 0xFFFF) {
        fprintf(stderr, "%ld is not an unsigned 8-bit value\n", value);
        return false;
    }
    *data = (uint8_t)value;

    return true;
}

static bool write_number_to_file(char const *path, uint8_t data) {
    FILE *fp = fopen(path, "w");
    if(!fp) {
        fprintf(stderr, "Failed to open %s for writing\n", path);
        return false;
    }

    fprintf(fp, "%u", data);
    fclose(fp);
    return true;
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
    return read_number_from_file(pwm_min_path, &pwm_min);
}

bool amdgpu_fan_store_pwm_max(void) {
    return read_number_from_file(pwm_max_path, &pwm_max);
}

bool amdgpu_fan_set_mode(enum fanmode mode) {
    if(verbose) {
        printf("Setting fan mode: %s\n", mode == manual ? "manual" : "auto");
    }
    return write_number_to_file(pwm_enable, mode);
}

bool amdgpu_fan_get_percentage(uint8_t *percentage) {
    uint8_t npwm;
    if(read_number_from_file(pwm, &npwm)) {
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
        printf("Setting pwm %u\n", npwm);
    }
    return write_number_to_file(pwm, npwm);
}

