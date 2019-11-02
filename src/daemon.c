#include "daemon.h"
#include "fancontroller.h"

#include <stdbool.h>

#include <unistd.h>


void amdgpu_daemon_init(char const *hwmon_path) {
    amdgpu_fan_setup_pwm_enable_file(hwmon_path);
    amdgpu_fan_setup_temp_input_file(hwmon_path);
    amdgpu_fan_setup_pwm_file(hwmon_path);
    amdgpu_fan_setup_pwm_min_file(hwmon_path);
    amdgpu_fan_setup_pwm_max_file(hwmon_path);
}

void daemon_run(uint8_t update_interval) {
    extern bool daemon_alive;

    while(daemon_alive) {
        // TODO: set fan speed
        sleep(update_interval);
    }
}
