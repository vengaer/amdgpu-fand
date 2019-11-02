#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <regex.h>


bool verbose = false;
uint16_t update_interval = 5;
char const *hwmon_file = "hwmon1";

static void print_usage(void) {
    fprintf(stderr,
        "Usage: amdgpu-fan [option]...\n"
        "Options:\n"
        "    -v, --verbose                       Echo actions to stdout\n"
        "    -f FILE, --hwmon=FILE               Specify hwmon file, default is hwmon1\n"
        "    -i INTERVAL, --interval=INTERVAL    Specify the interval with which to update fan speed, in seconds. The valud must be in the interval 1...65535."
        "                                        Default is every %u seconds\n"
        "    -h, --help                          Print help message\n", update_interval);
}

static bool validate_interval(char const *interval) {
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
    if(ivl < 1 || ivl > 65535) {
        fprintf(stderr, "%ld is not in the interval 1...65535\n", ivl);
        return false;
    }
    update_interval = ivl;
    return true;
}

int main(int argc, char** argv) {
    regex_t hwmon_input_rgx, hwmon_file_rgx, interval_input_rgx;
    int reti;

    reti = regcomp(&hwmon_input_rgx, "^--hwmon=.+", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile hwmon input regex\n");
        return 1;
    }

    reti = regcomp(&hwmon_file_rgx, "^hwmon[0-9]", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile hwmon file regex\n");
        return 1;
    }

    reti = regcomp(&interval_input_rgx, "^--interval=[0-9]+", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile interval input regex\n");
        return 1;
    }

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
            char const *hwmon = argv[++i];
            if(regexec(&hwmon_file_rgx, hwmon, 0, NULL, 0)) {
                fprintf(stderr, "%s is not a valid hwmon file\n", hwmon);
                return 1;
            }
            hwmon_file = hwmon;
        }
        else if(strcmp(argv[i], "-i") == 0) {
            if(i + 1 >= argc) {
                fprintf(stderr, "No argument supplied for -i\n");
                return 1;
            }
            if(!validate_interval(argv[++i])) {
                return 1;
            }
        }
        else if(regexec(&hwmon_input_rgx, argv[i], 0, NULL, 0) == 0) {
            char const *hwmon = strchr(argv[i], '=') + 1;
            if(regexec(&hwmon_file_rgx, hwmon, 0, NULL, 0)) {
                fprintf(stderr, "%s is not a valid hwmon file\n", hwmon);
                return 1;
            }
            hwmon_file = hwmon;
        }
        else if(regexec(&interval_input_rgx, argv[i], 0, NULL, 0) == 0) {
            if(!validate_interval(strchr(argv[i], '=') + 1)) {
                return 1;
            }
        }
        else {
            fprintf(stderr, "Unknown switch %s\n", argv[i]);
            print_usage();
            return 1;
        }
    }

    return 0;
}
