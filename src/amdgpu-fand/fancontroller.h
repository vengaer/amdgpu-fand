#ifndef FANCONTROLLER_H
#define FANCONTROLLER_H
#include "commontype.h"
#include "interpolation.h"
#include "matrix.h"

#include <stdbool.h>
#include <stdint.h>

#include <sys/types.h>

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

void amdgpu_fan_set_aggressive_throttle(bool throttle);
void amdgpu_fan_set_interpolation_method(enum interpolation_method method);
void amdgpu_fan_set_speed_interface(enum speed_interface iface);
void amdgpu_fan_set_matrix(matrix mtrx, uint8_t mtrx_rows);
void amdgpu_fan_set_hysteresis(uint8_t hysteresis_val);

ssize_t amdgpu_fan_get_pwm_path(char *buffer, size_t count);
bool amdgpu_fan_get_aggressive_throttle(void);
enum interpolation_method amdgpu_fan_get_interpolation_method(void);
void amdgpu_fan_get_matrix(matrix mtrx, uint8_t *mtrx_rows);

bool amdgpu_fan_set_mode(enum fanmode mode);
bool amdgpu_fan_get_percentage(uint8_t *percentage);
bool amdgpu_fan_set_percentage(uint8_t percentage);
bool amdgpu_fan_get_speed(uint8_t *speed);

bool amdgpu_get_temp(uint8_t *temp);

void amdgpu_fan_set_override_speed(int16_t speed, pid_t ppid);
void amdgpu_fan_set_override_percentage(uint8_t percentage, pid_t ppid);
void amdgpu_fan_reset_override_speed(void);
void amdgpu_fan_set_speed_interface_override(enum speed_interface iface);
void amdgpu_fan_reset_speed_interface_override(void);
bool amdgpu_fan_update_speed(void);

#endif
