#ifndef CONFIG_H
#define CONFIG_H

#include "defs.h"

#define CONFIG_DEFAULT_PATH "/etc/amdgpu-fand.conf"

enum { DEVICE_PATH_MAX_SIZE = 256 };
enum { HWMON_PATH_MAX_SIZE = 32 };
enum { MATRIX_MAX_SIZE = 2 * MAX_TEMP_THRESHOLDS };

struct fand_config {
    unsigned char matrix_rows;
    unsigned char hysteresis;
    unsigned short interval;
    unsigned char matrix[MATRIX_MAX_SIZE];
    char hwmon[HWMON_PATH_MAX_SIZE];
    char devpath[DEVICE_PATH_MAX_SIZE];

};

int config_parse(char const *path, struct fand_config *data);

#endif /* CONFIG_H */
