#ifndef PROCS_H
#define PROCS_H

#include <stdbool.h>

#include <sys/types.h>

enum proc_result {
    proc_unknown = -1,
    proc_false,
    proc_true
};

pid_t get_ppid_of(pid_t pid);
pid_t get_pid_of_shell(void);
enum proc_result proc_running_in_sudo(pid_t pid);

#endif
