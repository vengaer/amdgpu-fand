#include "config.h"
#include "daemon.h"

#include <stdbool.h>

#include <argp.h>

char const *argp_program_version = "amdgpu_fand 3.0";
char const *argp_program_bug_address = "<vilhelm.engstrom@tuta.io>";

static char doc[] = "amdgpu-fand -- A daemon controlling the fan speed of AMD Radeon GPUs";
static char args_doc[] = "";

static struct argp_option options[] = {
    {"no-fork", 'F', 0,      0, "Do not fork into background", 0},
    {"config",  'c', "FILE", 0, "Specify config path", 0 },
    { 0 }
};

struct args {
    bool fork;
    char const *config;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct args *args = state->input;

    switch(key) {
        case 'F':
            args->fork = false;
            break;
        case 'c':
            args->config = arg;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

int main(int argc, char **argv) {
    struct argp argp = {
        options,
        parse_opt,
        args_doc,
        doc,
        0,
        0,
        0
    };

    struct args args = {
        .fork = true,
        .config = CONFIG_DEFAULT_PATH
    };

    argp_parse(&argp, argc, argv, 0, 0, &args);

    return daemon_main(args.fork, args.config);
}
