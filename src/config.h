#ifndef CONFIG_H
#define CONFIG_H

#include "interpolation.h"
#include "matrix.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CONFIG_MONITOR_INTERVAL 5
#define FANCTL_CONFIG "/etc/amdgpu-fanctl.conf"
#define COMMENT_CHAR '#'

struct file_monitor {
    char const *path;
    bool(*callback)(char const*);
};

bool parse_config(char const *restrict path, char *restrict persistent, size_t persistent_count, char *restrict hwmon, size_t hwmon_count,
                  uint8_t *interval, bool *throttle, enum interpolation_method *interp, matrix m, uint8_t *matrix_rows);

void* monitor_config(void *monitor);

#endif
