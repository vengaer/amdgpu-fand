#include "commontype.h"
#include "filesystem.h"
#include "ipc.h"
#include "logger.h"
#include "procs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <regex.h>
#include <unistd.h>

#define FAN_PERCENTAGE_MIN 0
#define FAN_PERCENTAGE_MAX 100

char const *ipc_request_type_value[4] = { "get", "set", "reset", "invalid" };
char const *ipc_request_target_value[6] = { "temp", "speed", "matrix", "speed-iface", "pwm_path", "invalid" };

static regex_t cmd_get_rgx, cmd_set_rgx, cmd_reset_rgx, target_temp_rgx, target_speed_rgx, target_matrix_rgx, target_speed_iface_rgx;
static regex_t numeric_value_rgx, speed_iface_value_rgx, tacho_iface_rgx;

static bool compile_regexps(void) {
    int reti = 0;
    reti |= regcomp(&cmd_get_rgx, "^\\s*get\\s*$", REG_EXTENDED) |
            regcomp(&cmd_set_rgx, "^\\s*set\\s*$", REG_EXTENDED) |
            regcomp(&cmd_reset_rgx, "^\\s*reset\\s*$", REG_EXTENDED) |
            regcomp(&target_temp_rgx, "^\\s*temp(erature)?\\s*$", REG_EXTENDED) |
            regcomp(&target_speed_rgx, "^\\s*(fan)?speed\\s*$", REG_EXTENDED) |
            regcomp(&target_matrix_rgx, "^\\s*matrix\\s*$", REG_EXTENDED) |
            regcomp(&target_speed_iface_rgx, "^\\s*speed-i(nter)?face\\s*$", REG_EXTENDED) |
            regcomp(&numeric_value_rgx, "^\\s*[0-9]{1,3}%?\\s*$", REG_EXTENDED) |
            regcomp(&speed_iface_value_rgx, "^\\s*tacho(meter)?|daemon|fand\\s*$", REG_EXTENDED) |
            regcomp(&tacho_iface_rgx, "^\\s*tacho(meter)?\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile ipc regexps\n");
        return false;
    }
    return true;
}

static bool parse_command_param(char const *request_param, struct ipc_request *result) {
    if(regexec(&cmd_get_rgx, request_param, 0, NULL, 0) == 0) {
        LOG(VERBOSITY_LVL3, "Setting ipc request type ipc_get\n");
        result->type = ipc_get;
    }
    else if(regexec(&cmd_set_rgx, request_param, 0, NULL, 0) == 0) {
        LOG(VERBOSITY_LVL3, "Setting ipc request type ipc_set\n");
        result->type = ipc_set;
        if(result->ppid != DETACH_FROM_SHELL) {
            result->ppid = get_pid_of_shell();
            if(result->ppid == -1) {
                fprintf(stderr, "Failed to get pid of shell\n");
                return false;
            }
        }
    }
    else if(regexec(&cmd_reset_rgx, request_param, 0, NULL, 0) == 0) {
        LOG(VERBOSITY_LVL3, "Setting ipc request type ipc_reset\n");
        result->type = ipc_reset;
    }
    else {
        fprintf(stderr, "%s is not a valid ipc command\n", request_param);
        return false;
    }
    return true;

}

static bool parse_target_param(char const *request_param, struct ipc_request *result) {
    if(regexec(&target_temp_rgx, request_param, 0, NULL, 0) == 0) {
        /* Can't set temperature... */
        if(result->type == ipc_set || result->type == ipc_reset) {
            fprintf(stderr, "Target '%s' is not available with command 'set'\n", request_param);
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
        if(result->type == ipc_set || result->type == ipc_reset) {
            fprintf(stderr, "Target '%s' is not available with command 'set'\n", request_param);
            return false;
        }
        result->target = ipc_matrix;
        LOG(VERBOSITY_LVL3, "Setting ipc target ipc_matrix\n");
    }
    else if(regexec(&target_speed_iface_rgx, request_param, 0, NULL, 0) == 0) {
        LOG(VERBOSITY_LVL3, "Setting ipc raget ipc_speed_iface\n");
        result->target = ipc_speed_iface;
    }
    else {
        fprintf(stderr, "%s is not a valid ipc target\n", request_param);
        return false;
    }
    return true;
}

bool parse_numeric_value_param(char const *request_param, struct ipc_request *result) {
    if(regexec(&numeric_value_rgx, request_param, 0, NULL, 0) != 0) {
        fprintf(stderr, "Invalid value %s\n", request_param);
        return false;
    }
    bool is_percentage = strchr(request_param, '%');

    /* %-sign ignored by atoi */
    int value = atoi(request_param);

    if(is_percentage) {
        if(value < FAN_PERCENTAGE_MIN || value > FAN_PERCENTAGE_MAX) {
            LOG(VERBOSITY_LVL1, "%d is not in the interval [%d, %d]\n", value, FAN_PERCENTAGE_MIN, FAN_PERCENTAGE_MAX);
            return false;
        }
        value |= IPC_PERCENTAGE_BIT;
    }

    LOG(VERBOSITY_LVL3, "Setting ipc value %d, percentage bit: %d\n", (value & 0xFF), (value & IPC_PERCENTAGE_BIT) == IPC_PERCENTAGE_BIT);
    result->value = value;

    return true;
}

bool parse_speed_iface_value_param(char const *request_param, struct ipc_request *result) {
    if(regexec(&speed_iface_value_rgx, request_param, 0, NULL, 0) != 0) {
        fprintf(stderr, "Invalid value %s\n", request_param);
        return false;
    }

    if(regexec(&tacho_iface_rgx, request_param, 0, NULL, 0) == 0) {
        result->value = sifc_tacho;
    }
    else {
        result->value = sifc_daemon;
    }

    LOG(VERBOSITY_LVL3, "Setting ipc value %s\n", result->value == sifc_tacho ? "tachometer" : "daemon");
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
            return parse_command_param(request_param, result);
        case 1:
            return parse_target_param(request_param, result);
        case 2:
            if(result->type != ipc_set && result->type != ipc_reset) {
                return true;
            }
            switch(result->target) {
                case ipc_speed:
                    return parse_numeric_value_param(request_param, result);
                case ipc_speed_iface:
                    return parse_speed_iface_value_param(request_param, result);
                    return 0;
                default:
                    break;
            }
        default:
            break;
    }
    return false;
}

enum ipc_request_state get_ipc_state(struct ipc_request *request) {
    if(request->type == ipc_invalid_type) {
        return ipc_server_state;
    }
    
    if(request->type == ipc_set && request->value == IPC_EMPTY_REQUEST_FIELD) {
        return ipc_invalid_state;
    }

    return ipc_client_state;
}

bool ipc_server_running(void) {
    return file_exists(SERVER_SOCK_FILE);
}

