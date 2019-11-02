#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FANCTL_CONFIG "/etc/amdgpu-fanctl.conf"

#define MATRIX_DIMS 16

typedef uint8_t matrix[MATRIX_DIMS][MATRIX_DIMS];

bool parse_config(char *hwmon, size_t hwmon_count, uint8_t *interval, matrix *m);

#endif
