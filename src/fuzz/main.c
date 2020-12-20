#include "config.h"
#include "server.h"
#include "sigutil.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SOCKFILE "/tmp/amdgpu-fuzzd"

static void sighandler(int signal) {
    int childstatus;

    switch(signal) {
        case SIGPIPE:
            fputs("Broken pipe\n", stderr);
            break;
        case SIGCHLD:
            while(waitpid(-1, &childstatus, WNOHANG) > 0);
            sigbits = WEXITSTATUS(childstatus);
            if(sigutil_exit()) {
                abort();
            }
            break;
    }
}

int write_data(int fd, uint8_t const *data, size_t size) {
    char *buffer = malloc(size + 1);
    int status = 0;
    if(!buffer) {
        fputs("Memory allocation failure\n", stderr);
        return -1;
    }

    memcpy(buffer, data, size);
    buffer[size] = '\0';

    if(write(fd, buffer, size + 1) == -1) {
        fprintf(stderr, "Could not write data to makeshift socket: %s\n", strerror(errno));
        status = -1;
    }

    free(buffer);
    return status;
}

int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
    if(!size) {
        return 0;
    }
    struct fand_config config = {
        .throttle = false,
        .matrix_rows = 1,
        .hysteresis = 3,
        .interval = 2,
        .matrix = { 0 }
    };
    if(sigutil_sethandler(SIGPIPE, SA_RESTART, sighandler) < 0 || sigutil_sethandler(SIGCHLD, SA_RESTART, sighandler) < 0) {
        return 0;
    }

    if(server_init()) {
        fputs("Error starting server\n", stderr);
        return 0;
    }

    int fd = open(SOCKFILE, O_RDWR | O_CREAT);

    if(fd == -1) {
        fprintf(stderr, "Could not open makeshift socket: %s\n", strerror(errno));
        goto cleanup;
    }

    if(write_data(fd, data, size) < 0) {
        goto cleanup;
    }

    server_recv_and_respond(fd, &config);

cleanup:
    if(fd != -1) {
        close(fd);
    }

    if(unlink(SOCKFILE) == -1) {
        fprintf(stderr, "Could not unlink makeshift socket: %s\n", strerror(errno));
    }

    if(server_kill()) {
        fputs("Error stopping server\n", stderr);
    }

    return 0;
}
