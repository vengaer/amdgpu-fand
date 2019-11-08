#include "config.h"
#include "daemon.h"
#include "fancontroller.h"
#include "filesystem.h"
#include "hwmon.h"
#include "logger.h"
#include "strutils.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pthread.h>
#include <unistd.h>

static pthread_t monitor_thread;
static struct file_monitor file_monitor;
static pthread_mutex_t lock;
static bool monitor_thread_alive = false;

static uint8_t update_interval;

static void spawn_monitor_thread() {
    LOG(VERBOSITY_LVL2, "Spawning monitor thread\n");
    set_config_monitoring_enabled(true);
    if(pthread_create(&monitor_thread, NULL, monitor_config, (void *)&file_monitor)) {
        fprintf(stderr, "Failed to create monitor thread, live reloading is unavailable\n");
    }
    monitor_thread_alive = true;
}

static inline void join_monitor_thread_safely(void) {
    if(!monitor_thread_alive) {
        return;
    }
    LOG(VERBOSITY_LVL2, "Joining monitor thread\n");
    if(pthread_join(monitor_thread, NULL)) {
        fprintf(stderr, "Failed to join monitor thread\n");
    }
    monitor_thread_alive = false;
}

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


static bool reinitialize(char const *restrict persistent, char const *restrict hwmon, uint8_t interval, bool throttle, enum interpolation_method interp, matrix mtrx, uint8_t mtrx_rows) {
    char hwmon_full_path[HWMON_PATH_LEN] = { 0 };
    char hwmon_dir[HWMON_SUBDIR_LEN];

    if(!generate_hwmon_dir(hwmon_full_path, persistent, sizeof hwmon_full_path)) {
        return false;
    }
    if(!strlen(hwmon)) {
        if(!find_dir_matching_pattern(hwmon_dir, sizeof hwmon_dir, "^hwmon[0-9]$", hwmon_full_path)) {
            fprintf(stderr, "No hwmon directory found in %s\n", hwmon_full_path);
            return false;
        }
        hwmon = hwmon_dir;
    }

    if(!is_valid_hwmon_dir(hwmon)) {
        fprintf(stderr, "%s is not a valid hwmon dir\n", hwmon);
        return false;
    }
    if(!generate_hwmon_path(hwmon_full_path, hwmon, sizeof hwmon_full_path)) {
        return false;
    }

    if(!setup_hwmon(hwmon_full_path)) {
        fprintf(stderr, "Failed to update hwmon\n");
        return false;
    }

    amdgpu_fan_set_matrix(mtrx, mtrx_rows);
    amdgpu_fan_set_aggressive_throttle(throttle);
    amdgpu_fan_set_interpolation_method(interp);

    update_interval = interval;
    return true;
}

bool amdgpu_daemon_init(char const *restrict config, char const *restrict hwmon_path, bool aggressive_throttle, bool monitor, enum interpolation_method interp, matrix mtrx, uint8_t mtrx_rows) {
    bool result = setup_hwmon(hwmon_path);
    amdgpu_fan_set_matrix(mtrx, mtrx_rows);
    amdgpu_fan_set_aggressive_throttle(aggressive_throttle);
    amdgpu_fan_set_interpolation_method(interp);

    pthread_mutex_init(&lock, NULL);

    file_monitor.path = config;
    file_monitor.callback = amdgpu_daemon_restart;

    if(monitor) {
        spawn_monitor_thread();
    }

    return result;
}

bool amdgpu_daemon_restart(char const *config) {
    matrix mtrx;
    uint8_t mtrx_rows;
    uint8_t interval = update_interval;
    bool throttle = amdgpu_fan_get_aggressive_throttle();
    bool monitor = true;
    enum interpolation_method interp = amdgpu_fan_get_interpolation_method();
    char hwmon[HWMON_SUBDIR_LEN];
    char persistent[HWMON_PATH_LEN];

    if(!parse_config(config, persistent, sizeof persistent, hwmon, sizeof hwmon, &interval, &throttle, &monitor, &interp, mtrx, &mtrx_rows)) {
        fprintf(stderr, "Failed to reread config, keeping current values\n");
        return false;
    }

    if(!monitor) {
        set_config_monitoring_enabled(false);
        LOG(VERBOSITY_LVL1, "Disabling config monitoring\n");
        return true;
    }

    pthread_mutex_lock(&lock);
    bool result = reinitialize(persistent, hwmon, interval, throttle, interp, mtrx, mtrx_rows);
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

        if(!config_monitoring_enabled()) {
            join_monitor_thread_safely();
        }

        sleep(update_interval);
    }

    join_monitor_thread_safely();
    pthread_mutex_destroy(&lock);

    amdgpu_fan_set_mode(automatic);
}
