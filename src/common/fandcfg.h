#ifndef DEFS_H
#define DEFS_H

#ifndef FAND_FUZZ_CONFIG
#define DAEMON_WORKING_DIR "/var/run/amdgpu-fand"
#else
#define DAEMON_WORKING_DIR "/tmp"
#endif
#define DAEMON_SERVER_SOCKET DAEMON_WORKING_DIR"/fand.sock"


enum { HWMON_PATH_SIZE = 256 };
enum { MAX_TEMP_THRESHOLDS = 16 };
enum { FAND_FATAL_ERR = -0x20 };
enum { PWM_MIN = 0 };
enum { PWM_MAX = 255 };


#endif /* DEFS_H */
