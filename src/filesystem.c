#define _DEFAULT_SOURCE

#include "filesystem.h"
#include "strutils.h"

#include <stdio.h>
#include <string.h>

#include <dirent.h>
#include <errno.h>
#include <regex.h>

bool find_dir_matching_pattern(char *restrict dst, size_t count, char const *restrict pattern, char const *restrict parent) {
    regex_t rgx;
    int reti;

    reti = regcomp(&rgx, pattern, REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile regex %s\n", pattern);
        return false;
    }

    struct dirent *entry;
    DIR *sdir = opendir(parent);
    if(!sdir) {
        int errnum = errno;
        fprintf(stderr, "Failed to open directory %s: %s\n", parent, strerror(errnum));
        return false;
    }

    while((entry = readdir(sdir))) {
        if(entry->d_type == DT_DIR && regexec(&rgx, entry->d_name, 0, NULL, 0) == 0) {
            if(strscpy(dst, entry->d_name, count) < 0) {
                fprintf(stderr, "Directory %s overflowed buffer\n", entry->d_name);
                closedir(sdir);
                return false;
            }
            closedir(sdir);
            return true;
        }
    }

    closedir(sdir);
    return false;
}

bool is_valid_hwmon_dir(char const *dir) {
    regex_t hwmon_rgx;
    int reti = regcomp(&hwmon_rgx, "^hwmon[0-9]$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile hwmon regex\n");
        return false;
    }

    return regexec(&hwmon_rgx, dir, 0, NULL, 0) == 0;
}
