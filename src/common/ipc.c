#include "filesystem.h"
#include "ipc.h"
#include "logger.h"
#include "procs.h"

#include <stdio.h>
#include <stdlib.h>

#include <regex.h>
#include <unistd.h>

#define FAN_SPEED_MIN 0
#define FAN_SPEED_MAX 100

char const *ipc_request_type_value[3] = { "get", "set", "invalid" };
char const *ipc_request_target_value[4] = { "temp", "speed", "matrix", "invalid" };

static regex_t cmd_get_rgx, cmd_set_rgx, target_temp_rgx, target_speed_rgx, target_matrix_rgx, value_rgx;

static bool compile_regexps(void) {
    int reti = 0;
    reti |= regcomp(&cmd_get_rgx, "^\\s*get\\s*$", REG_EXTENDED) |
            regcomp(&cmd_set_rgx, "^\\s*set\\s*$", REG_EXTENDED) |
            regcomp(&target_temp_rgx, "^\\s*temp(erature)?\\s*$", REG_EXTENDED) |
            regcomp(&target_speed_rgx, "^\\s*(fan)?speed\\s*$", REG_EXTENDED) |
            regcomp(&target_matrix_rgx, "^\\s*matrix\\s*$", REG_EXTENDED) |
            regcomp(&value_rgx, "^\\s*[0-9]{1,3}\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile ipc regexps\n");
        return false;
    }
    return true;
}


bool parse_ipc_param(char const *request_param, size_t param_idx, struct ipc_request *result) {
    static bool regexps_compiled = false;
    LOG(VERBOSITY_LVL3, "Parsing ipc request param '%s'\n", request_param);
    if(!regexps_compiled) {
        compile_regexps();
        regexps_compiled = true;
    }

    switch(param_idx) {
        case 0:
            if(regexec(&cmd_get_rgx, request_param, 0, NULL, 0) == 0) {
                LOG(VERBOSITY_LVL3, "Setting ipc request type ipc_get\n");
                result->type = ipc_get;
            }
            else if(regexec(&cmd_set_rgx, request_param, 0, NULL, 0) == 0) {
                LOG(VERBOSITY_LVL3, "Setting ipc request type ipc_set\n");
                result->type = ipc_set;
                result->ppid = get_pid_of_shell();
                if(result->ppid == -1) {
                    fprintf(stderr, "Failed to get pid of shell\n");
                    return false;
                }
            }
            else {
                LOG(VERBOSITY_LVL1, "%s is not a valid ipc command\n", request_param);
                return false;
            }
            break;
        case 1:
            if(regexec(&target_temp_rgx, request_param, 0, NULL, 0) == 0) {
                /* Can't set temperature... */
                if(result->type == ipc_set) {
                    LOG(VERBOSITY_LVL1, "%s is not available with command 'set'\n", request_param);
                    return false;
                }
                result->target = ipc_temp;
                LOG(VERBOSITY_LVL3, "Setting ipc target ipc_temp\n");
            }
            else if(regexec(&target_speed_rgx, request_param, 0, NULL, 0) == 0) {
                LOG(VERBOSITY_LVL3, "Setting ipc target ipc_speed\n");
                result->target = ipc_speed;
            }
            else if(regexec(&target_matrix_rgx, request_param, 0, NULL, 0) == 0) {
                /* Can't set matrix... */
                if(result->type == ipc_set) {
                    LOG(VERBOSITY_LVL1, "%s is not available with command 'set'\n", request_param);
                    return false;
                }
                result->target = ipc_matrix;
                LOG(VERBOSITY_LVL3, "Setting ipc target ipc_matrix\n");
            }
            else {
                LOG(VERBOSITY_LVL1, "%s is not a valid ipc target\n", request_param);
                return false;
            }
            break;
        case 2:
            if(regexec(&value_rgx, request_param, 0, NULL, 0) == 0) {
                int value = atoi(request_param);
                if(value < FAN_SPEED_MIN || value > FAN_SPEED_MAX) {
                    LOG(VERBOSITY_LVL1, "%d is not in the interval (%d, %d)\n", value, FAN_SPEED_MIN, FAN_SPEED_MAX);
                    return false;
                }
                LOG(VERBOSITY_LVL3, "Setting ipc value %d\n", value);
                result->value = value;

                break;
            }
            /* fall through */
        default:
            return false;
    }

    return true;
}

enum ipc_request_state get_ipc_state(struct ipc_request *request) {
    if(request->type == ipc_invalid_type) {
        return ipc_server_state;
    }
    
    if(request->type == ipc_set && request->value == -1) {
        return ipc_invalid_state;
    }

    return ipc_client_state;
}

bool ipc_server_running(void) {
    return file_exists(SERVER_SOCK_FILE);
}

