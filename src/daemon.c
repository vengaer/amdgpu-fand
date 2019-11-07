#include "config.h"
#include "daemon.h"
#include "fancontroller.h"
#include "filesystem.h"
#include "logger.h"
#include "strutils.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pthread.h>
#include <unistd.h>

#define MAX_ADJUST_FAILURES 10

static pthread_t monitor_thread;
static struct file_monitor monitor;
static pthread_mutex_t lock;

static uint8_t update_interval;

static bool setup_hwmon(char const *hwmon_path) {
    if(!file_exists(hwmon_path)) {
        fprintf(stderr, "%s does not exist\n", hwmon_path);
        return false;
    }

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


static bool reinitialize(char const *hwmon, uint8_t interval, bool throttle, enum interpolation_method interp, matrix mtrx, uint8_t mtrx_rows) {
    if(strlen(hwmon) > 0) {
        if(!is_valid_hwmon_dir(hwmon)) {
            fprintf(stderr, "%s is not a valid hwmon dir\n", hwmon);
            return false;
        }
        char hwmon_path[HWMON_PATH_LEN] = { 0 };
        if(strscat(hwmon_path, HWMON_DIR, sizeof hwmon_path) < 0 || strscat(hwmon_path, hwmon, sizeof hwmon_path) < 0) {
            fprintf(stderr, "Updated hwmon path overflows the buffer\n");
            return false;
        }
        if(!setup_hwmon(hwmon_path)) {
            fprintf(stderr, "Failed to update hwmon\n");
            return false;
        }
    }

    amdgpu_fan_set_matrix(mtrx, mtrx_rows);
    amdgpu_fan_set_aggressive_throttle(throttle);
    amdgpu_fan_set_interpolation_method(interp);

    update_interval = interval;
    return true;
}

bool amdgpu_daemon_init(char const *restrict config, char const *restrict hwmon_path, bool aggressive_throttle, enum interpolation_method interp, matrix mtrx, uint8_t mtrx_rows) {
    bool result = setup_hwmon(hwmon_path);
    amdgpu_fan_set_matrix(mtrx, mtrx_rows);
    amdgpu_fan_set_aggressive_throttle(aggressive_throttle);
    amdgpu_fan_set_interpolation_method(interp);

    monitor.path = config;
    monitor.callback = amdgpu_daemon_restart;

    pthread_mutex_init(&lock, NULL);

    LOG(VERBOSITY_LVL2, "Spawning monitor thread\n");
    if(pthread_create(&monitor_thread, NULL, monitor_config, (void *)&monitor)) {
        fprintf(stderr, "Failed to create monitor thread, live reloading is unavailable\n");
    }

    return result;
}

bool amdgpu_daemon_restart(char const *config) {
    matrix mtrx;
    uint8_t mtrx_rows;
    uint8_t interval = update_interval;
    bool throttle = amdgpu_fan_get_aggressive_throttle();
    enum interpolation_method interp = amdgpu_fan_get_interpolation_method();
    char hwmon[HWMON_PATH_LEN];

    if(!parse_config(config, hwmon, sizeof hwmon, &interval, &throttle, &interp, mtrx, &mtrx_rows)) {
        fprintf(stderr, "Failed to reread config, keeping current values\n");
        return false;
    }

    pthread_mutex_lock(&lock);
    bool result = reinitialize(hwmon, interval, throttle, interp, mtrx, mtrx_rows);
    pthread_mutex_unlock(&lock);

    return result;
}

void amdgpu_daemon_run(uint8_t interval) {
    extern bool volatile daemon_alive;
    update_interval = interval;

    amdgpu_fan_set_mode(manual);

    while(daemon_alive) {
        pthread_mutex_lock(&lock);
        if(!amdgpu_fan_update_speed()) {
            fprintf(stderr, "Failed to adjust fan speed\n");
        }
        pthread_mutex_unlock(&lock);

        sleep(update_interval);
    }

    LOG(VERBOSITY_LVL2, "Joining monitor thread\n");
    if(pthread_join(monitor_thread, NULL)) {
        fprintf(stderr, "Failed to join monitor thread\n");
    }

    pthread_mutex_destroy(&lock);

    amdgpu_fan_set_mode(automatic);
}
