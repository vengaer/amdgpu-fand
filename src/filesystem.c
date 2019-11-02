#define _DEFAULT_SOURCE

#include "filesystem.h"
#include "strutils.h"

#include <stdio.h>

#include <dirent.h>
#include <regex.h>

bool find_dir_matching_pattern(char *restrict dst, size_t count, char const *restrict pattern, char const *restrict parent) {
    regex_t rgx;
    int reti;

    reti = regcomp(&rgx, pattern, REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile regex %s\n", pattern);
        return false;
    }

    DIR *sdir = opendir(parent);
    struct dirent *entry;

    while((entry = readdir(sdir))) {
        if(entry->d_type == DT_DIR && regexec(&rgx, entry->d_name, 0, NULL, 0) == 0) {
            if(strscpy(dst, entry->d_name, count) < 0) {
                fprintf(stderr, "Directory %s overflowed buffer\n", entry->d_name);
                return false;
            }
            return true;
        }
    }

    return false;
}
