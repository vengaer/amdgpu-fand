#ifndef DEFS_H
#define DEFS_H

#define DAEMON_WORKING_DIR "/var/run/amdgpu-fand"
#define SERVER_SOCKET DAEMON_WORKING_DIR"/fand.sock"

enum { DEVICE_PATH_MAX_SIZE = 256 };
enum { MAX_TEMP_THRESHOLDS = 16 };
enum { FAND_FATAL_ERR = -0x20 };

#endif /* DEFS_H */
