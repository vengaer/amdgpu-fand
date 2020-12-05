#ifndef CONFIG_H
#define CONFIG_H

#include "defs.h"

#include <stdbool.h>

#define CONFIG_DEFAULT_PATH "/etc/amdgpu-fand.conf"

enum { DIRENT_MAX_SIZE = 32 };
enum { MATRIX_MAX_SIZE = 2 * MAX_TEMP_THRESHOLDS };

struct fand_config {
    bool throttle;
    unsigned char matrix_rows;
    unsigned char hysteresis;
    unsigned short interval;
    unsigned char matrix[MATRIX_MAX_SIZE];
    char hwmon[DIRENT_MAX_SIZE];
    char ctrl_mode[DIRENT_MAX_SIZE];
    char pwm_min[DIRENT_MAX_SIZE];
    char pwm_max[DIRENT_MAX_SIZE];
    char pwm_ctrl[DIRENT_MAX_SIZE];
    char temp_sensor[DIRENT_MAX_SIZE];
    char devpath[DEVICE_PATH_MAX_SIZE];
};

int config_parse(char const *path, struct fand_config *data);

#endif /* CONFIG_H */