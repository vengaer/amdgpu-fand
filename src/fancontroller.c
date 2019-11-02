#include "fancontroller.h"
#include "strutils.h"

#include <stdbool.h>
#include <stdio.h>

#include <regex.h>

static char const *pwm_enable_file = "/pwm1_enable";
static char const *temp_input_file = "/temp1_input";
static char const *pwm_file = "/pwm1";

extern bool verbose;

static char pwm_enable[HWMON_PATH_LEN];
static char temp_input[HWMON_PATH_LEN];
static char pwm[HWMON_PATH_LEN];

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
    return true;
}
