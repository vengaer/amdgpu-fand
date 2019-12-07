#include "ipc.h"
#include "ipc_client.h"
#include "logger.h"
#include "procs.h"

#include <stdint.h>

#include <argp.h>

char const *argp_program_version = "amdgpu-fanctl 2.0";
char const *argp_program_bug_address = "<vilhelm.engstrom@tuta.io>";

static char doc[] = "amdgpu-fanctl -- Command line interface for amdgpu-fand\
                    \vCommands:\n"
                    "  get TARGET [VALUE]         Get target value, TARGET may be [fan]speed,\n"
                    "                             temp[erature] or matrix. Any potential VALUE\n"
                    "                             is silently discarded\n"
                    "  set TARGET VALUE           Set value of target TARGET to VALUE. Changes\n"
                    "                             are kept as long as the process invoking the\n"
                    "                             ipc command remains alive. This behavior can be\n"
                    "                             overridden by passing the --detach switch.\n"
                    "                             Valid targets are [fan]speed. VALUE should be \n"
                    "                             given as a percentage\n"
                    "  reset [TARGET [VALUE]]     Undo any and all set commands for TARGET and fall\n"
                    "                             back on value specified in the config file. If no\n" 
                    "                             TARGET is specified, all values are reset. Valid\n"
                    "                             targets are [fan]speed. Any potential VALUE is\n"
                    "                             silently discarded\n";
static char args_doc[] = "COMMAND [TARGET [VALUE]]";

static struct argp_option options[] = {
    {"verbose", 'v', 0, 0, "Echo actions to standard out. May be repeated up to 3 times to set verbosity level", 0 },
    {"detach",  'd', 0, 0, "Detach from current shell instance", 0 },
    { 0 }
};

struct arguments {
    struct ipc_request request;
    uint8_t verbosity;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *args = state->input;

    switch(key) {
        case 'v':
            set_verbosity_level(++args->verbosity);
            break;
        case 'd':
            args->request.ppid = DETACH_FROM_SHELL;
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
        .request = { .type = ipc_invalid_type, .target = ipc_invalid_target, .value = -1, .ppid = -1 },
        .verbosity = 0,
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
