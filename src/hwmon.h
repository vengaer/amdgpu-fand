#ifndef HWMON_H
#define HWMON_H

#define HWMON_SYMLINK "/sys/class/drm/card0"
#define DRM_SUBDIR "/device/hwmon"

#define HWMON_PATH_LEN 128
#define HWMON_SUBDIR_LEN 16

#include <stdbool.h>
#include <stddef.h>

bool generate_hwmon_dir(char *restrict dst, char const *restrict src, size_t count, char const *restrict config);
bool generate_hwmon_path(char *restrict dst, char const *restrict src, size_t count);
bool is_valid_hwmon_dir(char const *dir);

#endif
