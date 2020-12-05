#include <stdbool.h>

#include <argp.h>

char const *argp_program_version = "amdgpu-fanctl 4.0";
char const *argP_program_bug_address = "<vilhelm.engstrom@tuta.io>";

static char doc[] = "amdgpu-fanctl -- Command line interface for amdgpu-fand"
                    "\vThe TARGET passed to the get switch may be either 'matrix', 'speed' or\n"
                    "'temp[erature]'.";
static char args_doc[] = "";

static struct argp_option options[] = {
    {"exit", 'e', 0,        0, "Kill the daemon", 0 },
    {"get",  'g', "TARGET", 0, "Get value corresponding to TARGET (see below)", 0 },
    { 0 }
};

struct args {
    bool exit;
    char const *target;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct args *args = state->input;

    switch(key) {
        case 'e':
            args->exit = true;
            break;
        case 'g':
            args->target = arg;
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
        .exit = false,
        .target = 0
    };

    argp_parse(&argp, argc, argv, 0, 0, &args);

    /* TODO: ipc */

    return 0;
}
