#ifndef FANCONTROLLER_H
#define FANCONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

#define HWMON_PATH_LEN 64
#define HWMON_SUBDIR_LEN 16
#define HWMON_DIR "/sys/class/drm/card0/device/hwmon/"
#define PWM_ENABLE "pwm1_enable"
#define TEMP_INPUT "temp1_input"
#define PWM "pwm1"

enum fanmode {
    manual = 1,
    automatic
};

bool amdgpu_fan_setup_pwm_enable_file(char const *hwmon_path);
bool amdgpu_fan_setup_temp_input_file(char const *hwmon_path);
bool amdgpu_fan_setup_pwm_file(char const *hwmon_path);
bool amdgpu_fan_setup_pwm_min_file(char const *hwmon_path);
bool amdgpu_fan_setup_pwm_max_file(char const *hwmon_path);

bool amdgpu_fan_store_pwm_min(void);
bool amdgpu_fan_store_pwm_max(void);

bool amdgpu_fan_set_mode(enum fanmode mode);
bool amdgpu_fan_get_percentage(uint8_t *percentage);
bool amdgpu_fan_set_percentage(uint8_t percentage);

#endif
