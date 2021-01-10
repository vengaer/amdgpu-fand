#include "config.h"
#include "macro.h"
#include "regutils.h"
#include "strutils.h"

#include <errno.h>
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
#define CONFIG_KEY_THROTTLE "aggressive_throttle"

enum { CONFIG_BUFFER_SIZE = 256 };
enum { CONFIG_KEY_SIZE = 64 };
enum { CONFIG_NUMBUF_SIZE = 4 };

struct config_pair {
    char const *key;
    int(*handler)(struct fand_config *, char const*);
};

static int config_set_interval(struct fand_config *data, char const *value);
static int config_set_hysteresis(struct fand_config *data, char const *value);
static int config_set_matrix(struct fand_config *data, char const *value);
static int config_set_throttle(struct fand_config *data, char const *value);

static struct config_pair config_map[] = {
    { CONFIG_KEY_INTERVAL,    config_set_interval },
    { CONFIG_KEY_HYSTERESIS,  config_set_hysteresis },
    { CONFIG_KEY_MATRIX,      config_set_matrix },
    { CONFIG_KEY_THROTTLE,    config_set_throttle }
};

static inline int regmatch_length(regmatch_t *match) {
    return match->rm_eo - match->rm_so;
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

static int config_set_interval(struct fand_config *data, char const *value) {
    unsigned long ul;
    int reti = strstoul_range(value, &ul, 0ul, (unsigned long)USHRT_MAX);
    if(reti) {
        syslog(LOG_ERR, "Invalid interval %s, must be a number between 0 and %hu", value, (unsigned short)USHRT_MAX);
        return reti;
    }
    data->interval = (unsigned short)ul;
    return 0;
}

static int config_set_hysteresis(struct fand_config *data, char const *value) {
    unsigned long ul;
    int reti = strstoul_range(value, &ul, 0, UCHAR_MAX);
    if(reti) {
        syslog(LOG_ERR, "Invalid hysteresis %s, must be a number between 0 and %hhu", value, (unsigned char)UCHAR_MAX);
        return reti;
    }
    data->hysteresis = (unsigned char)ul;
    return 0;
}

static int config_set_matrix(struct fand_config *data, char const *value) {
    regex_t matv_regex;
    regmatch_t pmatch[3];
    int status = 0;
    unsigned char matrix_rows;
    char numbuf[CONFIG_NUMBUF_SIZE];
    unsigned long ul;
    unsigned char temp;

    if(regcomp_info(&matv_regex, "[;(]'([0-9]+)::([0-9]+)'.*[;)]", REG_EXTENDED, "matrix value")) {
        return -1;
    }

    for(matrix_rows = 0; matrix_rows < MAX_TEMP_THRESHOLDS && value; matrix_rows++) {
        if(regexec(&matv_regex, value, array_size(pmatch), pmatch, 0)) {
            syslog(LOG_ERR, "Error while parsing matrix, offending segment: %s", value);
            status = -1;
            goto cleanup;
        }

        if(strsncpy(numbuf, value + pmatch[1].rm_so, sizeof(numbuf), regmatch_length(&pmatch[1])) < 0) {
            syslog(LOG_ERR, "Temperature %.*s exceeds allowed limit", regmatch_length(&pmatch[1]), value + pmatch[1].rm_so);
            status = -1;
            goto cleanup;
        }

        if(strstoul_range(numbuf, &ul, 0, UCHAR_MAX)) {
            syslog(LOG_ERR, "Invalid temperature %s, must be a number between 0 and %hhu", numbuf, (unsigned char)UCHAR_MAX);
            status = -1;
            goto cleanup;
        }
        temp = (unsigned char)ul;

        if(strsncpy(numbuf, value + pmatch[2].rm_so, sizeof(numbuf), regmatch_length(&pmatch[2])) < 0) {
            syslog(LOG_ERR, "Percentage %.*s exceeds allowed limit", regmatch_length(&pmatch[2]), value + pmatch[2].rm_so);
            status = -1;
            goto cleanup;
        }
        if(strstoul_range(numbuf, &ul, 0, 100)) {
            syslog(LOG_ERR, "Invalid percentage %s, must be a number between 0 and 100", numbuf);
            status = -1;
            goto cleanup;
        }

        data->matrix[matrix_rows * 2] = temp;
        data->matrix[matrix_rows * 2 + 1] = (unsigned char)ul;

        value = strchr(value + 1, ';');
    }

    if(matrix_rows == MAX_TEMP_THRESHOLDS) {
        syslog(LOG_ERR, "Matrix may contain at most %d rows", MAX_TEMP_THRESHOLDS);
        status = -1;
        goto cleanup;
    }

    data->matrix_rows = matrix_rows;
cleanup:
    regfree(&matv_regex);
    return status;
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

    if(regcomp_info(&mpat_start, "\\s*(\\('[0-9]+::[0-9]+')\\s*$", REG_EXTENDED, "matrix start")) {
        return status;
    }
    if(regcomp_info(&mpat_mid, "\\s*('[0-9]+::[0-9]+')\\s*$", REG_EXTENDED, "matrix middle")) {
        regfree(&mpat_start);
        return status;
    }
    if(regcomp_info(&mpat_end, "\\s*('[0-9]+::[0-9]+'\\))\\s*$", REG_EXTENDED, "matrix end")) {
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

int config_parse(char const *path, struct fand_config *data) {
    int status = 0;
    unsigned lineno = 0;
    unsigned config_idx;
    regex_t valregex;
    regmatch_t pmatch[3];
    char buffer[CONFIG_BUFFER_SIZE];
    char key[CONFIG_KEY_SIZE];
    char value[CONFIG_BUFFER_SIZE];

    int reti = regcomp_info(&valregex, "^\\s*(\\S+)\\s*=\\s*\"?([^\" ]+)\"?\\s*$", REG_EXTENDED, "config value");
    if(reti) {
        return reti;
    }

    FILE *fp = fopen(path, "r");
    if(!fp) {
        syslog(LOG_ERR, "Could not open config file %s: %s", path, strerror(errno));
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
