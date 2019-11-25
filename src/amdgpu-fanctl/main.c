#include "ipc.h"
#include "ipc_client.h"
#include "logger.h"

#include <stdint.h>

#include <argp.h>

uint8_t verbosity_level = 0;

static inline void set_verbosity_level(uint8_t verbosity) {
    if(verbosity > MAX_VERBOSITY) {
        fprintf(stderr, "Warning, max log level is %d\n", MAX_VERBOSITY);
        verbosity = MAX_VERBOSITY;
    }
    verbosity_level = verbosity;
}

char const *argp_program_version = "amdgpu-fanctl 2.0";
char const *argp_program_bug_address = "<vilhelm.engstrom@tuta.io>";

static char doc[] = "amdgpu-fanctl -- Command line interface for amdgpu-fand\
                    \vCommands:\n"
                    "  get TARGET [VALUE]         Get target value, TARGET may be [fan]speed,\n"
                    "                             temp[erature] or matrix. Any potential VALUE is\n"
                    "                             silently discarded\n"
                    "  set TARGET VALUE           Set value of target TARGET to VALUE. Changes\n"
                    "                             are kept as long as the process invoking the\n"
                    "                             ipc command remains alive. Valid targets are\n"
                    "                             [fan]speed. VALUE should be givan as a percentage\n";
static char args_doc[] = "[COMMAND TARGET [VALUE]]";

static struct argp_option options[] = {
    {"verbose",  'v', 0,          0, "Echo actions to standard out. May be repeated up to 3 times to set verbosity level", 0 },
    {"detach",  'd', 0,          0, "Detach from current shell instance", 0 },
    { 0 }
};

struct arguments {
    struct ipc_request request;
    uint8_t verbosity;
    bool detach;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *args = state->input;

    switch(key) {
        case 'v':
            set_verbosity_level(++args->verbosity);
            break;
        case 'd':
            args->detach = true;
            break;
        case ARGP_KEY_ARG:
            if(state->arg_num >= 3 || !parse_ipc_param(arg, state->arg_num, &args->request)) {
                argp_usage(state);
            }
            break;
        default:
            return ARGP_ERR_UNKNOWN;
        }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

int main(int argc, char** argv) {

    struct arguments args = {
        .request = { .type = ipc_invalid_type, .target = ipc_invalid_target, .value = -1 },
        .verbosity = 0,
        .detach = false
    };

    argp_parse(&argp, argc, argv, 0, 0, &args);

    enum ipc_request_state const ipc_state = get_ipc_state(&args.request);

    switch(ipc_state) {
        default:
        case ipc_invalid_state:
            fprintf(stderr, "Invalid ipc request\n");
            return 1;
        case ipc_client_state:
            return !ipc_client_handle_request(&args.request);
        case ipc_server_state:
            if(ipc_server_running()) {
                fprintf(stderr, "Server already running\n");
                return 1;
            }
            break;
    }

    return 0;
}
