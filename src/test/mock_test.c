#define FAND_MOCK_TEST
#include "fanctrl.h"
#include "fanctrl_mock.h"
#include "mock.h"
#include "mock_test.h"
#include "mock.h"
#include "test.h"

#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include <unistd.h>

static sig_atomic_t abort_caught = 0;
static jmp_buf jmpbuf;

static fpos_t pos;
static int fd;

void sigabrt_handler(int signal) {
    (void)signal;
    abort_caught = 1;
    longjmp(jmpbuf, 1);
}

static inline bool does_abort(int(*function)(void)) {
    if(!setjmp(jmpbuf)) {
        signal(SIGABRT, sigabrt_handler);
        function();
        signal(SIGABRT, SIG_DFL);
    }
    bool result = abort_caught;
    abort_caught = 0;
    return result;
}

static int speed(void) {
    return 0;
}

static void redirect_stderr(char const *restrict path) {
    fflush(stderr);
    fgetpos(stderr, &pos);
    fd = dup(STDERR_FILENO);
    freopen(path, "w", stderr);
}

static void reset_stderr(void) {
    fflush(stderr);
    dup2(fd, STDERR_FILENO);
    close(fd);
    clearerr(stderr);
    fsetpos(stderr, &pos);
}

void test_validate_mock(void) {
    redirect_stderr("/dev/null");

    fand_assert(does_abort(fanctrl_get_speed));
    mock_guard {
        mock_fanctrl_get_speed(speed);
        fand_assert(!does_abort(fanctrl_get_speed));
    }

    reset_stderr();
}
