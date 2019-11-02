#ifndef CONFIG_H
#define CONFIG_H

#include "matrix.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FANCTL_CONFIG "/etc/amdgpu-fanctl.conf"
#define COMMENT_CHAR '#'

bool parse_config(char const *restrict path, char *restrict hwmon, size_t hwmon_count, uint8_t *interval, matrix m, uint8_t *matrix_rows);

#endif
