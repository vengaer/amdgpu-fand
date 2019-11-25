#ifndef DAEMON_H
#define DAEMON_H

#include "interpolation.h"
#include "matrix.h"

#include <stdbool.h>
#include <stdint.h>

bool amdgpu_daemon_init(char const *restrict config, char const *restrict hwmon_path, bool aggressive_throttle, enum interpolation_method interp, matrix mtrx, uint8_t mtrx_rows);
bool amdgpu_daemon_restart(void);
void amdgpu_daemon_run(uint8_t interval);

#endif