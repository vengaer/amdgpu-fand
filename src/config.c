#include "config.h"
#include "strutils.h"

#include <stdio.h>
#include <string.h>

#include <regex.h>

#define LINE_SIZE 128

static regex_t interval_rgx, hwmon_rgx, matrix_rgx, matrix_start_rgx;

static bool compile_regexps(void) {
    int reti;
    reti = regcomp(&interval_rgx, "^INTERVAL=\"[0-9]+\"$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile interval regex\n");
        return false;
    }
    reti = regcomp(&hwmon_rgx, "^HWMON=\".*\"$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile hwmon regex\n");
        return false;
    }
    reti = regcomp(&matrix_rgx, "'[0-9]{1,3}::[0-9]{1,3}'", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile matrix regex\n");
        return false;
    }
    reti = regcomp(&matrix_start_rgx, "^MATRIX=('[0-9]{1,3}::[0-9]{1,3}'\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile matrix start regex\n");
        return false;
    }
    return true;
}

static bool strip_comments(char *restrict dst, char const *restrict src, size_t count) {
    char *end = strchr(src, '#');
    if(end == src) {
        if(strscpy(dst, '\0', count) < 0) {
            fprintf(stderr, "dst has size 0\n");
            return false;
        }
    }
        else if(end && *(end - 1) != '\\') {
        if(strsncpy(dst, src, end - src, count) < 0) {
            fprintf(stderr, "Buffer overflow while stripping comments\n");
            return false;
        }
    }
    else {
        if(strscpy(dst, src, count) < 0) {
            fprintf(stderr, "Buffer overflow copying entire line\n");
            return false;
        }
    }
    return true;
}


bool parse_config(char *hwmon, size_t hwmon_count, uint8_t *interval, matrix *m) {
    compile_regexps();

    FILE *fp = fopen(FANCTL_CONFIG, "r");
    if(!fp) {
        fprintf(stderr, "Failed to read config file\n");
        return false;
    }

    char buffer[LINE_SIZE];

    while(fgets(buffer, sizeof buffer, fp)) {

    }
}
