#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FANCTL_CONFIG "/etc/amdgpu-fanctl.conf"
#define COMMENT_CHAR '#'

#define MATRIX_COLS 2
#define MATRIX_ROWS 16

typedef uint8_t matrix[MATRIX_ROWS][MATRIX_COLS];

bool parse_config(char const *restrict path, char *restrict hwmon, size_t hwmon_count, uint8_t *interval, matrix m, uint8_t *matrix_rows);

#endif
