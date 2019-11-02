#include "daemon.h"
#include "fancontroller.h"
#include "filesystem.h"

#include <stdint.h>
#include <stdio.h>

#include <unistd.h>


bool amdgpu_daemon_init(char const *hwmon_path, matrix mtrx, uint8_t mtrx_rows) {
    if(!file_exists(hwmon_path)) {
        fprintf(stderr, "%s does not exist\n", hwmon_path);
        return 1;
    }

    uint8_t result = 1;
    result &= amdgpu_fan_setup_pwm_enable_file(hwmon_path);
    result &= amdgpu_fan_setup_temp_input_file(hwmon_path);
    result &= amdgpu_fan_setup_pwm_file(hwmon_path);
    result &= amdgpu_fan_setup_pwm_min_file(hwmon_path);
    result &= amdgpu_fan_setup_pwm_max_file(hwmon_path);

    result &= amdgpu_fan_store_pwm_min();
    result &= amdgpu_fan_store_pwm_max();

    amdgpu_fan_set_matrix(mtrx, mtrx_rows);

    return result;
}

void amdgpu_daemon_run(uint8_t interval) {
    extern bool daemon_alive;

    amdgpu_fan_set_mode(manual);
    while(daemon_alive) {
        amdgpu_fan_update_speed();
        sleep(interval);
    }
    amdgpu_fan_set_mode(automatic);
}
