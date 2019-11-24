#include "procs.h"
#include "strutils.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <error.h>
#include <signal.h>
#include <unistd.h>

#define PROCFS_PATH_SIZE 64
#define COMM_SIZE 32

#define SUDO_COMM "(sudo)"

static bool construct_proc_path(char *dst, pid_t pid, size_t count) {
    dst[count - 1] = '\0';
    snprintf(dst, count, "/proc/%d/stat", pid);
    if(dst[count - 1]) {
        dst[count - 1] = '\0';
        fprintf(stderr, "Procfs path overflows the buffer\n");
        return false;
    }
    return true;
}

/* Full comm name read if comm matches the pattern (comm) */
static bool full_comm_name_read(char const *comm, size_t count) {
    int8_t parens_balance = 0;

    for(size_t i = 0; i < count && comm[i]; i++) {
        if(comm[i] == '(') {
            ++parens_balance;
        }
        else if(comm[i] == ')') {
            --parens_balance;
        }
    }

    return !parens_balance;
}

static bool get_proc_comm(char *dst, pid_t pid, size_t count) {
    char path[PROCFS_PATH_SIZE];
    char comm[COMM_SIZE];
    if(!construct_proc_path(path, pid, count)) {
        fprintf(stderr, "Failed to read proc comm value\n");
        return false;
    }
    comm[COMM_SIZE - 1] = '\0';

    FILE *fp = fopen(path, "r");
    if(!fp) {
        fprintf(stderr, "Failed to open %s\n", path);
        return false;
    }

    int status = fscanf(fp, "%*d %32s", comm);
    fclose(fp);
    if(status == EOF) {
        fprintf(stderr, "Failed to read comm of %d\n", pid);
        return false;
    }

    bool buffer_overflow = !full_comm_name_read(comm, sizeof comm);
    if(comm[COMM_SIZE - 1]) {
        fprintf(stderr, "Comm of %d overflows the buffer\n", pid);
        comm[COMM_SIZE - 1] = '\0';
        buffer_overflow = true;
    }

    if(strscpy(dst, comm, count) < 0) {
        fprintf(stderr, "%s overflows the destination buffer\n", comm);
        buffer_overflow = true;
    }
    return !buffer_overflow;
}

pid_t get_ppid_of(pid_t pid) {
    char path[PROCFS_PATH_SIZE];
    if(!construct_proc_path(path, pid, sizeof path)) {
        return -1;
    }

    FILE *fp = fopen(path, "r");
    if(!fp) {
        fprintf(stderr, "Failed to open %s\n", path);
        return -1;
    }

    pid_t ppid;
    int status = fscanf(fp, "%*d %*s %*s %d", &ppid);
    fclose(fp);
    if(status == EOF) {
        fprintf(stderr, "Failed to read ppid of %d\n", pid);
        return -1;
    }
    return ppid;
}

pid_t get_pid_of_shell(void) {
    pid_t ppid = getppid();

    /* If running in sudo, we need the parent process of sudo... */
    if(proc_running_in_sudo(getpid())) {
        ppid = get_ppid_of(ppid);
    }
    return ppid;
}

enum proc_result proc_running_in_sudo(pid_t pid) {
    char comm[COMM_SIZE];
    pid_t ppid = get_ppid_of(pid);
    if(ppid == -1) {
        fprintf(stderr, "Failed to get ppid of %d\n", pid);
        return proc_unknown;
    }
    if(!get_proc_comm(comm, ppid, sizeof comm)) {
        return proc_unknown;
    }
    return (enum proc_result)strcmp(comm, SUDO_COMM) == 0;
}

bool proc_alive(pid_t pid) {
    return kill(pid, 0) == 0 || errno != ESRCH;
}
