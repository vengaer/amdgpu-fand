#include "fancontroller.h"
#include "strutils.h"

#include <stdbool.h>
#include <stdio.h>

#include <regex.h>

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
