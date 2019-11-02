#ifndef FANCONTROLLER_H
#define FANCONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

#define HWMON_PATH_LEN 64
#define HWMON_DIR "/sys/class/drm/card0/device/hwmon"
#define PWM_ENABLE "pwm1_enable"
#define TEMP_INPUT "temp1_input"
#define PWM "pwm1"

bool amdgpu_fan_setup_pwm_enable_file(char const *hwmon_path);
bool amdgpu_fan_setup_temp_input_file(char const *hwmon_path);
bool amdgpu_fan_setup_pwm_file(char const *hwmon_path);

#endif
