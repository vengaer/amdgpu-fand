#ifndef CONFIG_H
#define CONFIG_H

#include "interpolation.h"
#include "matrix.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CONFIG_FILE "amdgpu-fand.conf"
#define CONFIG_FULL_PATH "/etc/"CONFIG_FILE
#define COMMENT_CHAR '#'

struct config_params {
    char *path, *persistent, *hwmon;
    size_t persistent_size, hwmon_size;
    uint8_t *interval, *mtrx_rows;
    bool *throttle;
    enum interpolation_method *interp;
    uint8_t (*mtrx)[];
};

bool parse_config(struct config_params *params);

bool replace_persistent_path_value(char const *restrict path, char const *restrict replacement);

void set_config_monitoring_enabled(bool monitor);
bool config_monitoring_enabled(void);
void* monitor_config(void *monitor);

#endif
