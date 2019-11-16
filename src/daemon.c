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

#include <fcntl.h>
#include <sys/inotify.h>
#include <unistd.h>

#define INOTIFY_BUF_LEN 16 * sizeof(struct inotify_event)
#define CONF_DIR_LEN 64

static int inotify_fd, inotify_wd;
static uint8_t update_interval;
static char config_file[CONF_DIR_LEN];

static bool setup_inotify(void) {
    char dir[CONF_DIR_LEN];
    if(parent_dir(dir, config_file, sizeof dir) < 0) {
        fprintf(stderr, "Config dir overflows the inotify buffer\n");
        return false;
    }
    inotify_fd = inotify_init();
    if(inotify_fd == -1) {
        fprintf(stderr, "Failed to initialize inotify\n");
        return false;
    }
    inotify_wd = inotify_add_watch(inotify_fd, dir, IN_CLOSE_WRITE);
    if(inotify_wd == -1) {
        fprintf(stderr, "Failed to add %s to watchl list\n", config_file);
        close(inotify_fd);
        return false;
    }
    if(fcntl(inotify_fd, F_SETFL, fcntl(inotify_fd, F_GETFL) | O_NONBLOCK) == -1) {
        fprintf(stderr, "Failed to make inotify non-blocking\n");
        inotify_rm_watch(inotify_fd, inotify_wd);
        close(inotify_fd);
        return false;
    }
    LOG(VERBOSITY_LVL2, "Watching %s\n", dir);

    return true;
}

static inline void cleanup_inotify(void) {
    inotify_rm_watch(inotify_fd, inotify_wd);
    close(inotify_fd);
}

static void handle_inotify_events(void) {
    char buf[INOTIFY_BUF_LEN];
    ssize_t nbytes;

    nbytes = read(inotify_fd, buf, sizeof buf);

    if(nbytes == -1) {
        extern bool volatile daemon_alive;
        if(daemon_alive) {
            E_LOG(VERBOSITY_LVL3, "No inotify events read\n");
        }
        return;
    }
    else {
        LOG(VERBOSITY_LVL3, "Read %ld bytes from inotify descriptor\n", nbytes);
    }

    char *p = buf;
    struct inotify_event *e;

    while(p < buf + nbytes) {
        e = (struct inotify_event *)p;
        if((e->mask & IN_CLOSE_WRITE) && e->len && strcmp(e->name, CONFIG_FILE) == 0) {
            LOG(VERBOSITY_LVL1, "Reloading %s\n", config_file);
            amdgpu_daemon_restart();
        }
        p += sizeof(struct inotify_event) + e->len;
    }
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

    if(!generate_hwmon_dir(hwmon_full_path, persistent, sizeof hwmon_full_path, config_file)) {
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

bool amdgpu_daemon_init(char const *restrict config, char const *restrict hwmon_path, bool aggressive_throttle, enum interpolation_method interp, matrix mtrx, uint8_t mtrx_rows) {
    bool result = setup_hwmon(hwmon_path);
    amdgpu_fan_set_matrix(mtrx, mtrx_rows);
    amdgpu_fan_set_aggressive_throttle(aggressive_throttle);
    amdgpu_fan_set_interpolation_method(interp);

    if(strscpy(config_file, config, sizeof config_file) < 0) {
        fprintf(stderr, "Config path %s overflows the buffer\n", config_file);
        return false;
    }

    if(!setup_inotify()) {
        return false;
    }
    return result;
}

bool amdgpu_daemon_restart(void) {
    matrix mtrx;
    uint8_t mtrx_rows;
    uint8_t interval = update_interval;
    bool throttle = amdgpu_fan_get_aggressive_throttle();
    enum interpolation_method interp = amdgpu_fan_get_interpolation_method();
    char hwmon[HWMON_SUBDIR_LEN];
    char persistent[HWMON_PATH_LEN];

    if(!parse_config(config_file, persistent, sizeof persistent, hwmon, sizeof hwmon, &interval, &throttle, &interp, mtrx, &mtrx_rows)) {
        fprintf(stderr, "Failed to reread config, keeping current values\n");
        return false;
    }
    return reinitialize(persistent, hwmon, interval, throttle, interp, mtrx, mtrx_rows);
}

void amdgpu_daemon_run(uint8_t interval) {
    extern bool volatile daemon_alive;
    update_interval = interval;

    amdgpu_fan_set_mode(manual);

    while(daemon_alive) {
        if(!amdgpu_fan_update_speed()) {
            fprintf(stderr, "Failed to adjust fan speed\n");
        }
        handle_inotify_events();

        sleep(update_interval);
    }

    cleanup_inotify();
    amdgpu_fan_set_mode(automatic);
}
