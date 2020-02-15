#ifndef DAEMON_H
#define DAEMON_H

#include "config.h"
#include "interpolation.h"
#include "matrix.h"

#include <stdbool.h>
#include <stdint.h>

struct daemon_ctrl_opts {
    bool aggressive_throttle;
    uint8_t mtrx_rows;
    enum interpolation_method interp_method;
    enum speed_interface speed_iface;
    uint8_t (*mtrx)[];
};

bool amdgpu_daemon_init(char const *restrict config, char const *restrict hwmon_path, struct daemon_ctrl_opts const *ctrl_opts);
bool amdgpu_daemon_restart(void);
void amdgpu_daemon_run(uint8_t interval);

#endif
