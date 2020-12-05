#include "config.h"
#include "filesystem.h"
#include "macro.h"
#include "strutils.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <regex.h>
#include <syslog.h>

#define CONFIG_KEY_INTERVAL "interval"
#define CONFIG_KEY_HYSTERESIS "hysteresis"
#define CONFIG_KEY_MATRIX "matrix"
#define CONFIG_KEY_DEVPATH "device_path"
#define CONFIG_KEY_HWMON "hardware_monitor"
#define CONFIG_KEY_CTRL_MODE "ctrl_mode"
#define CONFIG_KEY_PWM_MIN "pwm_min"
#define CONFIG_KEY_PWM_MAX "pwm_max"
#define CONFIG_KEY_PWM_CTRL "pwm_controller"
#define CONFIG_KEY_TEMP_SENSOR "temp_sensor"
#define CONFIG_KEY_THROTTLE "aggressive_throttle"

#define CONFIG_DEFAULT_HWMON "hwmon2"
#define CONFIG_DEFAULT_CTRL_MODE "fan1_enable"
#define CONFIG_DEFAULT_PWM_MIN "pwm1_min"
#define CONFIG_DEFAULT_PWM_MAX "pwm1_max"
#define CONFIG_DEFAULT_PWM_CTRL "pwm1"
#define CONFIG_DEFAULT_TEMP_SENSOR "temp1_input"

enum { CONFIG_BUFFER_SIZE = DEVICE_PATH_MAX_SIZE };
enum { CONFIG_KEY_SIZE = 64 };
enum { CONFIG_NUMBUF_SIZE = 4 };
enum { REGEX_ERRBUF_SIZE = 64 };

struct config_pair {
    char const *key;
    int(*handler)(struct fand_config *, char const*);
};

static int config_set_interval(struct fand_config *data, char const *value);
static int config_set_hysteresis(struct fand_config *data, char const *value);
static int config_set_matrix(struct fand_config *data, char const *value);
static int config_set_devpath(struct fand_config *data, char const *value);
static int config_set_hwmon(struct fand_config *data, char const *value);
static int config_set_ctrl_mode(struct fand_config *data, char const *value);
static int config_set_pwm_min(struct fand_config *data, char const *value);
static int config_set_pwm_max(struct fand_config *data, char const *value);
static int config_set_pwm_ctrl(struct fand_config *data, char const *value);
static int config_set_temp_sensor(struct fand_config *data, char const *value);
static int config_set_throttle(struct fand_config *data, char const *value);

static struct config_pair config_map[] = {
    { CONFIG_KEY_INTERVAL,    config_set_interval },
    { CONFIG_KEY_HYSTERESIS,  config_set_hysteresis },
    { CONFIG_KEY_MATRIX,      config_set_matrix },
    { CONFIG_KEY_DEVPATH,     config_set_devpath },
    { CONFIG_KEY_HWMON,       config_set_hwmon },
    { CONFIG_KEY_CTRL_MODE,   config_set_ctrl_mode },
    { CONFIG_KEY_PWM_MIN,     config_set_pwm_min },
    { CONFIG_KEY_PWM_MAX,     config_set_pwm_max },
    { CONFIG_KEY_PWM_CTRL,    config_set_pwm_ctrl },
    { CONFIG_KEY_TEMP_SENSOR, config_set_temp_sensor },
    { CONFIG_KEY_THROTTLE,    config_set_throttle }
};

static inline int regmatch_length(regmatch_t *match) {
    return match->rm_eo - match->rm_so;
}

static inline int config_regcomp(regex_t *regex, char const *pat, int cflags, char const *desc) {
    char errbuf[REGEX_ERRBUF_SIZE];
    int reti = regcomp(regex, pat, cflags);

    if(reti) {
        regerror(reti, regex, errbuf, sizeof(errbuf));
        syslog(LOG_EMERG, "Failed to compile %s regex: %s", desc, errbuf);
        return -1;
    }

    return 0;
}

static inline void config_replace_char(char *buffer, char from, char to) {
    char *cpos = strchr(buffer, from);
    if(cpos) {
        *cpos = to;
    }
}

static inline void config_strip_comments(char *buffer) {
    config_replace_char(buffer, '#', '\0');
}

static inline bool config_line_empty(char *buffer) {
    for(; *buffer; ++buffer) {
        switch(*buffer) {
            case ' ':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                /* NOP */
                break;
            default:
                return false;
        }
    }

    return true;
}

static inline int config_set_simple_string(char *dst, char const *value, size_t dstsize, char const *desc) {
    if(strscpy(dst, value, dstsize) < 0) {
        syslog(LOG_ERR, "%s %s overflows the internal buffer", desc, value);
        return -1;
    }
    return 0;
}

static int config_set_interval(struct fand_config *data, char const *value) {
    unsigned long ul;
    int reti = strstoul_range(value, &ul, 0, USHRT_MAX);
    if(reti) {
        syslog(LOG_ERR, "Invalid interval %s, must be a number between 0 and %hu", value, USHRT_MAX);
        return reti;
    }
    data->interval = (unsigned short)ul;
    return 0;
}

static int config_set_hysteresis(struct fand_config *data, char const *value) {
    unsigned long ul;
    int reti = strstoul_range(value, &ul, 0, UCHAR_MAX);
    if(reti) {
        syslog(LOG_ERR, "Invalid hysteresis %s, must be a number between 0 and %hhu", value, UCHAR_MAX);
        return reti;
    }
    data->hysteresis = (unsigned char)ul;
    return 0;
}

static int config_set_matrix(struct fand_config *data, char const *value) {
    regex_t matv_regex;
    regmatch_t pmatch[3];
    int status = -1;
    unsigned char matrix_rows = 0;
    char numbuf[CONFIG_NUMBUF_SIZE];
    unsigned long ul;
    unsigned char temp;

    if(config_regcomp(&matv_regex, "[;(]'([0-9]+)::([0-9]+)'.*[;)]", REG_EXTENDED, "matrix value")) {
        return status;
    }

    while(value) {
        if(regexec(&matv_regex, value, array_size(pmatch), pmatch, 0)) {
            syslog(LOG_ERR, "Error while parsing matrix, offending segment: %s", value);
            goto cleanup;
        }

        if(strsncpy(numbuf, value + pmatch[1].rm_so, sizeof(numbuf), regmatch_length(&pmatch[1])) < 0) {
            syslog(LOG_ERR, "Temperature %.*s exceeds allowed limit", regmatch_length(&pmatch[1]), value + pmatch[1].rm_so);
            goto cleanup;
        }

        if(strstoul_range(numbuf, &ul, 0, UCHAR_MAX)) {
            syslog(LOG_ERR, "Invalid temperature %s, must be a number between 0 and %hhu", numbuf, UCHAR_MAX);
            goto cleanup;
        }
        temp = (unsigned char)ul;

        if(strsncpy(numbuf, value + pmatch[2].rm_so, sizeof(numbuf), regmatch_length(&pmatch[2])) < 0) {
            syslog(LOG_ERR, "Percentage %.*s exceeds allowed limit", regmatch_length(&pmatch[2]), value + pmatch[2].rm_so);
            goto cleanup;
        }
        if(strstoul_range(numbuf, &ul, 0, 100)) {
            syslog(LOG_ERR, "Invalid percentage %s, must be a number between 0 and 100", numbuf);
            goto cleanup;
        }

        data->matrix[matrix_rows * 2] = temp;
        data->matrix[matrix_rows * 2 + 1] = (unsigned char)ul;
        ++matrix_rows;

        value = strchr(value + 1, ';');
    }

    data->matrix_rows = matrix_rows;
cleanup:
    regfree(&matv_regex);
    return 0;
}

static int config_set_devpath(struct fand_config *data, char const *value) {
    if(!fsys_dir_exists(value)) {
        syslog(LOG_ERR, "%s is not a directory", value);
        return -1;
    }

    if(strscpy(data->devpath, value, sizeof(data->devpath)) < 0) {
        syslog(LOG_ERR, "Device path %s overflows the internal buffer", value);
        return -1;
    }
    return 0;
}

static int config_set_hwmon(struct fand_config *data, char const *value) {
    return config_set_simple_string(data->hwmon, value, sizeof(data->hwmon), "Hardware monitor");
}

static int config_set_ctrl_mode(struct fand_config *data, char const *value) {
    return config_set_simple_string(data->ctrl_mode, value, sizeof(data->ctrl_mode), "Control mode");
}

static int config_set_pwm_min(struct fand_config *data, char const *value) {
    return config_set_simple_string(data->pwm_min, value, sizeof(data->pwm_min), "Pwm min");
}

static int config_set_pwm_max(struct fand_config *data, char const *value) {
    return config_set_simple_string(data->pwm_max, value, sizeof(data->pwm_max), "Pwm max");
}

static int config_set_pwm_ctrl(struct fand_config *data, char const *value) {
    return config_set_simple_string(data->pwm_ctrl, value, sizeof(data->pwm_ctrl), "Pwm control");
}

static int config_set_temp_sensor(struct fand_config *data, char const *value) {
    return config_set_simple_string(data->temp_sensor, value, sizeof(data->temp_sensor), "Temperature sensor");
}

static int config_set_throttle(struct fand_config *data, char const *value) {
    if(strcmp(value, "true") == 0) {
        data->throttle = true;
    }
    else if(strcmp(value, "false") == 0) {
        data->throttle = false;
    }
    else {
        syslog(LOG_WARNING, "Unknown value %s for aggressive_throttle, valid options are 'true' or 'false'", value);
        return -1;
    }
    return 0;
}

static int config_append_matrix_rows(char *value, size_t valsize, FILE *fp , unsigned *lineno) {
    regex_t mpat_start, mpat_mid, mpat_end;
    regmatch_t pmatch[2];
    int status = -1;
    ssize_t pos;
    char buffer[CONFIG_BUFFER_SIZE];
    int mid_reti, end_reti;
    unsigned matrix_rows = 0;

    if(config_regcomp(&mpat_start, "\\s*(\\('[0-9]+::[0-9]+')\\s*$", REG_EXTENDED, "matrix start")) {
        return status;
    }
    if(config_regcomp(&mpat_mid, "\\s*('[0-9]+::[0-9]+')\\s*$", REG_EXTENDED, "matrix middle")) {
        regfree(&mpat_start);
        return status;
    }
    if(config_regcomp(&mpat_end, "\\s*('[0-9]+::[0-9]+'\\))\\s*$", REG_EXTENDED, "matrix end")) {
        regfree(&mpat_start);
        regfree(&mpat_mid);
        return status;
    }

    if(regexec(&mpat_start, value, array_size(pmatch), pmatch, 0)) {
        syslog(LOG_ERR, "Syntax error on line %u: Invalid matrix %s", *lineno, value);
        goto cleanup;
    }

    if(strsncpy(buffer, value + pmatch[1].rm_so, sizeof(buffer), regmatch_length(&pmatch[1])) < 0) {
        syslog(LOG_ERR, "Matrix %.*s overflows the internal buffer", regmatch_length(&pmatch[1]), value + pmatch[1].rm_so);
        goto cleanup;
    }

    pos = strscpy(value, buffer, valsize);
    if(pos < 0) {
        syslog(LOG_ERR, "Overflow while writing stripped matrix value back to buffer. This should not be possible");
        goto cleanup;
    }
    while(fgets(buffer, sizeof(buffer), fp)) {
        mid_reti = 1;
        end_reti = 1;
        ++*lineno;
        ++matrix_rows;
        if(matrix_rows > MATRIX_MAX_SIZE / 2) {
            syslog(LOG_ERR, "Matrix may contain at most %u rows", MATRIX_MAX_SIZE / 2);
            goto cleanup;
        }

        pos += strscpy(value + pos, ";", valsize - pos);
        if(pos < 0) {
            syslog(LOG_ERR, "Line %u: Matrix overflows the internal buffer", *lineno);
            goto cleanup;
        }

        mid_reti = regexec(&mpat_mid, buffer, array_size(pmatch), pmatch, 0);
        if(mid_reti) {
            end_reti = regexec(&mpat_end, buffer, array_size(pmatch), pmatch, 0);
        }

        if(!mid_reti || !end_reti) {
            pos += strsncpy(value + pos, buffer + pmatch[1].rm_so, valsize - pos, regmatch_length(&pmatch[1]));
            if(pos < 0) {
                syslog(LOG_ERR, "Line %u: Matrix overflows the internal buffer", *lineno);
                goto cleanup;
            }
        }

        if(!end_reti) {
            status = 0;
            break;
        }
    }

    if(status) {
        syslog(LOG_ERR, "Syntax error on line %u: Unterminated matrix", *lineno);
    }

cleanup:
    regfree(&mpat_start);
    regfree(&mpat_mid);
    regfree(&mpat_end);
    return status;
}

static int config_set_defaults(struct fand_config *data) {
    memset(data, 0, sizeof(*data));

    data->interval = 5;
    int status = (strscpy(data->hwmon,       CONFIG_DEFAULT_HWMON,       sizeof(data->hwmon))       < 0) |
                 (strscpy(data->ctrl_mode,   CONFIG_DEFAULT_CTRL_MODE,   sizeof(data->ctrl_mode))   < 0) |
                 (strscpy(data->pwm_min,     CONFIG_DEFAULT_PWM_MIN,     sizeof(data->pwm_min))     < 0) |
                 (strscpy(data->pwm_max,     CONFIG_DEFAULT_PWM_MAX,     sizeof(data->pwm_max))     < 0) |
                 (strscpy(data->temp_sensor, CONFIG_DEFAULT_TEMP_SENSOR, sizeof(data->temp_sensor)) < 0);

    return -status;
}

int config_parse(char const *path, struct fand_config *data) {
    int status = 0;
    unsigned lineno = 0;
    unsigned config_idx;
    regex_t valregex;
    regmatch_t pmatch[3];
    char buffer[CONFIG_BUFFER_SIZE];
    char key[CONFIG_KEY_SIZE];
    char value[CONFIG_BUFFER_SIZE];

    if(config_set_defaults(data)) {
        return -1;
    }

    int reti = config_regcomp(&valregex, "^\\s*(\\S+)\\s*=\\s*\"?([^\" ]+)\"?\\s*$", REG_EXTENDED, "config value");
    if(reti) {
        return reti;
    }

    FILE *fp = fopen(path, "r");
    if(!fp) {
        status = -1;
        goto cleanup;
    }

    while(fgets(buffer, sizeof(buffer), fp)) {
        ++lineno;
        config_replace_char(buffer, '\n', '\0');
        config_strip_comments(buffer);
        if(config_line_empty(buffer)) {
            continue;
        }

        if(regexec(&valregex, buffer, array_size(pmatch), pmatch, 0)) {
            syslog(LOG_ERR, "Syntax error on line %u: %s", lineno, buffer);
            status = -1;
            goto cleanup;
        }

        if(strsncpy(key, buffer + pmatch[1].rm_so, sizeof(key), regmatch_length(&pmatch[1])) < 0) {
            syslog(LOG_ERR, "Key %.*s overflows the internal buffer", regmatch_length(&pmatch[1]), buffer + pmatch[1].rm_so);
            status = -1;
            goto cleanup;
        }
        strtolower(key);

        if(strsncpy(value, buffer + pmatch[2].rm_so, sizeof(value), regmatch_length(&pmatch[2])) < 0) {
            syslog(LOG_ERR, "Value %.*s overflows the internal buffer", regmatch_length(&pmatch[2]), buffer + pmatch[2].rm_so);
            status = -1;
            goto cleanup;
        }

        if(strcmp(key, CONFIG_KEY_MATRIX) == 0) {
            reti = config_append_matrix_rows(value, sizeof(value), fp, &lineno);
            if(reti) {
                status = reti;
                goto cleanup;
            }
        }

        for(config_idx = 0; config_idx < array_size(config_map); config_idx++) {
            if(strcmp(key, config_map[config_idx].key) == 0) {
                reti = config_map[config_idx].handler(data, value);
                if(reti) {
                    status = reti;
                    goto cleanup;
                }
                break;
            }
        }

        if(config_idx == array_size(config_map)) {
            syslog(LOG_ERR, "Syntax error on line %u: unknown key %s", lineno, key);
            status = -1;
            goto cleanup;
        }
    }

    if(data->matrix_rows == 0) {
        syslog(LOG_ERR, "No matrix found");
        status = -1;
    }
cleanup:
    if(fp) {
        fclose(fp);
    }
    regfree(&valregex);
    return status;
}
