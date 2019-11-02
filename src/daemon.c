#include "daemon.h"
#include "fancontroller.h"

#include <stdint.h>

#include <unistd.h>


bool amdgpu_daemon_init(char const *hwmon_path) {
    uint8_t result = 1;
    result &= amdgpu_fan_setup_pwm_enable_file(hwmon_path);
    result &= amdgpu_fan_setup_temp_input_file(hwmon_path);
    result &= amdgpu_fan_setup_pwm_file(hwmon_path);
    result &= amdgpu_fan_setup_pwm_min_file(hwmon_path);
    result &= amdgpu_fan_setup_pwm_max_file(hwmon_path);

    result &= amdgpu_fan_store_pwm_min();
    result &= amdgpu_fan_store_pwm_max();

    return result;
}

void daemon_run(uint8_t update_interval) {
    extern bool daemon_alive;

    amdgpu_fan_set_mode(manual);
    while(daemon_alive) {
        // TODO: set fan speed
        sleep(update_interval);
    }
    amdgpu_fan_set_mode(automatic);
}
