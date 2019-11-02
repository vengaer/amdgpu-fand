#include "config.h"
#include "daemon.h"
#include "fancontroller.h"
#include "filesystem.h"
#include "strutils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <regex.h>
#include <signal.h>

bool verbose = false;
bool volatile daemon_alive = true;

void interrupt(int code) {
    if(code == SIGINT || code == SIGTERM) {
        daemon_alive = false;
    }
}

static void print_usage(void) {
    fprintf(stderr,
        "Usage: amdgpu-fan [option]...\n"
        "Options:\n"
        "    -v, --verbose                       Echo actions to stdout\n"
        "    -f FILE, --hwmon=FILE               Specify hwmon file, default is hwmon1\n"
        "    -i INTERVAL, --interval=INTERVAL    Specify the interval with which to update fan speed, in seconds. The value must be in the interval 1...255.\n"
        "                                        Default is read from %s\n"
        "    -h, --help                          Print help message\n", FANCTL_CONFIG);
}

static uint8_t handle_multi_switch(char const *switches) {
    size_t const len = strlen(switches);
    for(size_t i = 1; i < len; i++) {
        if(switches[i] == 'v') {
            verbose = true;
        }
        else if(switches[i] == 'h') {
            print_usage();
            return 2;
        }
        else {
            fprintf(stderr, "Unknown switch sequence %s\n", switches);
            return 1;
        }
    }
    return 0;
}

static bool set_interval(char const *interval, uint8_t *out_interval) {
    if(!interval) {
        fprintf(stderr, "Interval is empty\n");
        return false;
    }
    char *endptr;
    long const ivl = strtol(interval, &endptr, 10);
    if(*endptr) {
        fprintf(stderr, "%s contains non-digit characters\n", interval);
        return false;
    }
    if(ivl < 1 || ivl > 255) {
        fprintf(stderr, "%ld is not in the interval 1...255\n", ivl);
        return false;
    }
    *out_interval = ivl;
    return true;
}

int main(int argc, char** argv) {
    char hwmon_path[HWMON_PATH_LEN] = { 0 };
    char hwmon_subdir[HWMON_SUBDIR_LEN] = { 0 };
    char const *hwmon = hwmon_subdir;
    regex_t hwmon_input_rgx, hwmon_subdir_rgx, interval_input_rgx;
    int reti;
    uint8_t update_interval = 5;

    signal(SIGINT, interrupt);
    signal(SIGTERM, interrupt);

    reti = regcomp(&hwmon_input_rgx, "^--hwmon=.+", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile hwmon input regex\n");
        return 1;
    }

    reti = regcomp(&hwmon_subdir_rgx, "^hwmon[0-9]", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile hwmon file regex\n");
        return 1;
    }

    reti = regcomp(&interval_input_rgx, "^--interval=[0-9]+", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile interval input regex\n");
        return 1;
    }

    /* Find hwmon subdir */
    if(!find_dir_matching_pattern(hwmon_subdir, sizeof hwmon_subdir, "^hwmon[0-9]$", HWMON_DIR)) {
        fprintf(stderr, "No hwmon directory found in %s\n", HWMON_DIR);
        return 1;
    }

    /* Parse arguments */
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
        else if(strcmp(argv[i], "-h") == 0|| strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        }
        else if(strcmp(argv[i], "-f") == 0) {
            if(i + 1 >= argc) {
                fprintf(stderr, "No argument supplied for -f\n");
                return 1;
            }
            char const *hwmon_arg = argv[++i];
            if(regexec(&hwmon_subdir_rgx, hwmon, 0, NULL, 0)) {
                fprintf(stderr, "%s is not a valid hwmon file\n", hwmon);
                return 1;
            }
            hwmon = hwmon_arg;
        }
        else if(strcmp(argv[i], "-i") == 0) {
            if(i + 1 >= argc) {
                fprintf(stderr, "No argument supplied for -i\n");
                return 1;
            }
            if(!set_interval(argv[++i], &update_interval)) {
                return 1;
            }
        }
        else if(regexec(&hwmon_input_rgx, argv[i], 0, NULL, 0) == 0) {
            char const *hwmon_arg = strchr(argv[i], '=') + 1;
            if(regexec(&hwmon_subdir_rgx, hwmon, 0, NULL, 0)) {
                fprintf(stderr, "%s is not a valid hwmon file\n", hwmon);
                return 1;
            }
            hwmon = hwmon_arg;
        }
        else if(regexec(&interval_input_rgx, argv[i], 0, NULL, 0) == 0) {
            if(!set_interval(strchr(argv[i], '=') + 1, &update_interval)) {
                return 1;
            }
        }
        else if(strlen(argv[i]) >= 2 && argv[i][0] == '-' && argv[i][1] != '-') {
            uint8_t const rv = handle_multi_switch(argv[i]);
            switch(rv) {
                case 0:
                    break;
                default:
                    return rv % 2;
            }
        }
        else {
            fprintf(stderr, "Unknown switch %s\n", argv[i]);
            print_usage();
            return 1;
        }
    }

    if(strscat(hwmon_path, HWMON_DIR, sizeof hwmon_path) < 0) {
        fprintf(stderr, "hwmon dir overflows the path buffer\n");
        return 1;
    }
    if(strscat(hwmon_path, hwmon, sizeof hwmon_path) < 0) {
        fprintf(stderr, "%s would overflow the buffer when appended to %s\n", hwmon, hwmon_path);
        return 1;
    }

    if(!file_exists(hwmon_path)) {
        fprintf(stderr, "%s does not exist\n", hwmon_path);
        return 1;
    }

    if(!amdgpu_daemon_init(hwmon_path)) {
        fprintf(stderr, "Failed to initialize daemon\n");
        return 1;
    }

    //amdgpu_daemon_run(update_interval);

    return 0;
}
