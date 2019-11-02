#ifndef DAEMON_H
#define DAEMON_H

#include <stdint.h>

void amdgpu_daemon_init(char const *hwmon_path);
void amdgpu_daemon_run(uint8_t update_interval);

#endif
