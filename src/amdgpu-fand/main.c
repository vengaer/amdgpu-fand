#include "config.h"
#include "daemon.h"
#include "filesystem.h"
#include "hwmon.h"
#include "logger.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <argp.h>
#include <signal.h>

sig_atomic_t volatile daemon_alive = 1;

void signal_handler(int code) {
    if(code == SIGINT || code == SIGTERM) {
        LOG(VERBOSITY_LVL1, "Killing daemon...\n");
        daemon_alive = 0;
    }
}

static bool uint8_from_chars(char const *str, uint8_t *value) {
    char *endptr;
    long const v = strtol(str, &endptr, 10);
    if(*endptr) {
        fprintf(stderr, "%s contains non-digit characters\n", str);
        return false;
    }
    if(v <= 0 || v > UINT8_MAX) {
        fprintf(stderr, "%ld is not in the interval 1...%u\n", v, UINT8_MAX);
        return false;
    }
    *value = (uint8_t)v;
    return true;
}

char const *argp_program_version = "amdgpu-fand 1.0";
char const *argp_program_bug_address = "<vilhelm.engstrom@tuta.io>";

static char doc[] = "amdgpu-fand -- A daemon controlling the fan speed on AMD Radeon GPUs";
static char args_doc[] = "";

static struct argp_option options[] = {
    {"verbose",  'v', 0,          0, "Echo actions to standard out. May be repeated up to 3 times to set verbosity level", 0 },
    {"hwmon",    'm', "FILE",     0, "Specify the hardware monitor", 0 },
    {"interval", 'i', "INTERVAL", 0,
     "Specify the interval, in seconds, with which to update fan speed. The value must be in the interval 1...255", 0 },
    {"config",   'c', "FILE",     0, "Override config path", 0 },
    { 0 }
};

struct arguments {
    char *hwmon;
    char *config;
    uint8_t verbosity;
    uint8_t interval;
    bool interval_passed;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *args = state->input;

    switch(key) {
        case 'v':
            set_verbosity_level(++args->verbosity);
            break;
        case 'f':
            args->hwmon = arg;
            break;
        case 'i':
            if(!uint8_from_chars(arg, &args->interval)) {
                args->interval = 0;
            }
            args->interval_passed = true;
            break;
        case 'c':
            args->config = arg;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
        }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

int main(int argc, char** argv) {
    char hwmon_full_path[HWMON_PATH_LEN];

    /* For values specified in config */
    char persistent_path[HWMON_PATH_LEN];
    char hwmon_buf[HWMON_SUBDIR_LEN];
    uint8_t config_interval = 0;
    uint8_t mtrx_rows;
    enum interpolation_method interp = linear;
    bool aggressive_throttle = false;
    matrix mtrx;

    struct arguments args = {
        .config = CONFIG_FULL_PATH,
        .verbosity = 0,
        .interval = 5,
        .interval_passed = false
    };

    /* Handle signals */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    argp_parse(&argp, argc, argv, 0, 0, &args);

    if(!parse_config(args.config, persistent_path, sizeof persistent_path, hwmon_buf, sizeof hwmon_buf, &config_interval, &aggressive_throttle, &interp, mtrx, &mtrx_rows)) {
        return 1;
    }

    if(args.interval_passed) {
        if(!args.interval) {
            fprintf(stderr, "Update interval must be a number in 1...255\n");
            return 1;
        }
    }
    else if(config_interval) {
        args.interval = config_interval;
    }

    if(!generate_hwmon_dir(hwmon_full_path, persistent_path, sizeof hwmon_full_path, args.config)) {
        return 1;
    }

    /* Set hwmon, precedence is argument -> config -> default */
    if(!args.hwmon) {
        if(!strlen(hwmon_buf)) {
            if(!find_dir_matching_pattern(hwmon_buf, sizeof hwmon_buf, "^hwmon[0-9]$", hwmon_full_path)) {
                fprintf(stderr, "No hwmon directory found in %s\n", hwmon_full_path);
                return 1;
            }
        }
        args.hwmon = hwmon_buf;
    }

    if(!is_valid_hwmon_dir(args.hwmon)) {
        fprintf(stderr, "%s is not a valid hwmon directory\n", args.hwmon);
        return 1;
    }

    if(!generate_hwmon_path(hwmon_full_path, args.hwmon, sizeof hwmon_full_path)) {
        return 1;
    }

    if(!amdgpu_daemon_init(args.config, hwmon_full_path, aggressive_throttle, interp, mtrx, mtrx_rows)) {
        fprintf(stderr, "Failed to initialize daemon\n");
        return 1;
    }

    amdgpu_daemon_run(args.interval);

    return 0;
}
