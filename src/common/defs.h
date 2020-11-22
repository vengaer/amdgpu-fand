#ifndef DEFS_H
#define DEFS_H

#define DAEMON_WORKING_DIR "/var/run/amdgpu-fand"
#define SERVER_SOCKET DAEMON_WORKING_DIR"/fand.sock"

enum { MAX_TEMP_THRESHOLDS = 16 };

#endif /* DEFS_H */
