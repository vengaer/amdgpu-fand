#include "config.h"
#include "filesystem.h"
#include "hwmon.h"
#include "logger.h"
#include "strutils.h"

bool generate_hwmon_dir(char *restrict dst, char const *restrict src, size_t count, char const *restrict config) {
    if(*src) {
        LOG(VERBOSITY_LVL3, "Generating hwmon dir from %s supplied in config\n", src);
        if(!file_exists(src)) {
            fprintf(stderr, "Persistent path %s does not exist\n", src);
            return false;
        }
        if(strscpy(dst, src, count) < 0 || strscat(dst, "/", count) < 0) {
            fprintf(stderr, "Persistent path %s overflows the buffer\n", src);
            return false;
        }
    }
    else {
        LOG(VERBOSITY_LVL2, "Persistent path not set in config, following symlink %s\n", HWMON_SYMLINK);
        if(readlink_absolute(HWMON_SYMLINK, dst, count) < 0 ) {
            fprintf(stderr, "Absolue device path overflows buffer\n");
            return false;
        }
        if(strscat(dst, DRM_SUBDIR, count) < 0) {
            fprintf(stderr, "hwmon dir overflows the path buffer\n");
            return false;
        }

        LOG(VERBOSITY_LVL2, "Replacing empty persistent path in config with %s\n", dst);
        if(!replace_persistent_path_value(config, dst)) {
            LOG(VERBOSITY_LVL1, "Failed to replace empty persistent path in config\n");
        }
    }

    return true;
}

bool generate_hwmon_path(char *restrict dst, char const *restrict hwmon, size_t count) {
    if(strscat(dst, "/", count) < 0 || strscat(dst, hwmon, count) < 0) {
        fprintf(stderr, "%s would overflow the buffer when appended to %s\n", hwmon, dst);
        return false;
    }
    LOG(VERBOSITY_LVL2, "hwmon path set to %s\n", dst);
    return true;
}
