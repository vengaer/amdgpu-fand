#ifndef DAEMON_H
#define DAEMON_H

#include "matrix.h"

#include <stdbool.h>
#include <stdint.h>

bool amdgpu_daemon_init(char const *hwmon_path, matrix mtrx, uint8_t mtrx_rows);
void amdgpu_daemon_run(uint8_t interval);

#endif
