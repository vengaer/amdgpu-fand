#include "config.h"
#include "logger.h"
#include "strutils.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define LINE_SIZE 128
#define OPTION_BUF_SIZE 4
#define INTERP_OPTION_SIZE 8
#define TEMP_MAX INT8_MAX

static regex_t interval_rgx, hwmon_rgx, hwmon_content_rgx, hwmon_empty_rgx, empty_rgx, leading_space_rgx;
static regex_t matrix_rgx, matrix_start_rgx, matrix_end_rgx, throttle_rgx, throttle_option_rgx;
static regex_t interpolation_rgx, interpolation_option_rgx;
static bool parsing_matrix = false, regexps_compiled = false;
static uint8_t line_number = 0;

enum parse_result {
    failure = -1,
    match,
    no_match
};


static inline size_t regmatch_size(regmatch_t regm) {
    return regm.rm_eo - regm.rm_so;
}

static inline bool regmatch_to_uint8(char const *line, regmatch_t regm, uint8_t *value) {
    char buffer[OPTION_BUF_SIZE];
    buffer[0] = '\0';

    if(strsncpy(buffer, line + regm.rm_so, regmatch_size(regm), sizeof buffer) < 0) {
        fprintf(stderr, "Overflow while converting regmatch to uint\n");
        return false;
    }

    *value = atoi(buffer);
    return true;
}

static bool compile_regexps(void) {
    LOG(LOG_LVL2, "Compiling config regexps\n");
    int reti;
    reti = regcomp(&interval_rgx, "^INTERVAL=\"?([0-9]{1,3})\"?\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile interval regex\n");
        return false;
    }
    reti = regcomp(&hwmon_rgx, "^HWMON=\"(.*)\"\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile hwmon regex\n");
        return false;
    }
    reti = regcomp(&hwmon_content_rgx, "^HWMON=\"(hwmon[0-9])\"\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile hwmon content regex\n");
        return false;
    }
    reti = regcomp(&hwmon_empty_rgx, "^HWMON=\"\"\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile hwmon empty regex\n");
        return false;
    }
    reti = regcomp(&matrix_rgx, "^(MATRIX=\\()?'([0-9]{1,3})::([0-9]{1,3})'\\)?*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile matrix regex\n");
        return false;
    }
    reti = regcomp(&matrix_start_rgx, "^MATRIX=\\('[0-9]{1,3}::[0-9]{1,3}'\\)?\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile matrix start regex\n");
        return false;
    }
    reti = regcomp(&matrix_end_rgx, "\\s*(MATRIX=\\()?'[0-9]{1,3}::[0-9]{1,3}'\\)\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile matrix end regex\n");
        return false;
    }
    reti = regcomp(&empty_rgx, "^\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile empty regex\n");
        return false;
    }
    reti = regcomp(&leading_space_rgx, "^\\s*(.*)$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile leading spaces regex\n");
        return false;
    }
    reti = regcomp(&throttle_rgx, "^\\s*AGGRESSIVE_THROTTLING=\".*\"\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile throttling regex\n");
        return false;
    }
    reti = regcomp(&throttle_option_rgx, "^\\s*AGGRESSIVE_THROTTLING=\"(yes|no)\"\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile throttling option regex\n");
        return false;
    }
    reti = regcomp(&interpolation_rgx, "^\\s*INTERPOLATION=\".*\"\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile interpolation regex\n");
        return false;
    }
    reti = regcomp(&interpolation_option_rgx, "^\\s*INTERPOLATION=\"(linear|cosine)\"\\s*$", REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile interpolation option regex\n");
        return false;
    }
    regexps_compiled = true;
    return true;
}

static bool strip_comments(char *restrict dst, char const *restrict src, size_t count) {
    char *end = strchr(src, COMMENT_CHAR);
    if(end == src) {
        dst[0] = '\0';
    }
    else if(end && *(end - 1) != '\\') {
        if(strsncpy(dst, src, end - src, count) < 0) {
            fprintf(stderr, "Buffer overflow while stripping comments\n");
            return false;
        }
    }
    else {
        if(strscpy(dst, src, count) < 0) {
            fprintf(stderr, "Buffer overflow copying entire line\n");
            return false;
        }
    }
    return true;
}

static bool strip_leading_whitespace(char *restrict dst, char const *restrict src, size_t count) {
    regmatch_t pmatch[2];
    if(regexec(&leading_space_rgx, src, 2, pmatch, 0)) {
        return false;
    }
    if(strsncpy(dst, src + pmatch[1].rm_so, regmatch_size(pmatch[1]), count) < 0) {
        fprintf(stderr, "Removing whitespace causes overflow\n");
        return false;
    }
    return true;
}

static enum parse_result parse_hwmon(char const *restrict line, char *restrict hwmon, size_t count) {
    LOG(LOG_LVL2, "Matching %s against hwon...\n", line);
    regmatch_t pmatch[2];
    if(regexec(&hwmon_rgx, line, 0, NULL, 0)) {
        LOG(LOG_LVL2, "No match\n");
        return no_match;
    }
    else if(regexec(&hwmon_empty_rgx, line, 0, NULL, 0) == 0) {
        LOG(LOG_LVL1, "hwmon is empty, keeping current path\n");
        hwmon[0] = '\0';
        return match;
    }
    else if(regexec(&hwmon_content_rgx, line, 2, pmatch, 0)) {
        fprintf(stderr, "Syntax error on line %u: %s\n", line_number, line);
        return failure;
    }
    if(strsncpy(hwmon, line + pmatch[1].rm_so, regmatch_size(pmatch[1]), count) < 0) {
        fprintf(stderr, "hwmon value on line %u overflows the buffer\n", line_number);
        return failure;
    }
    LOG(LOG_LVL1, "hwmon set to %s\n", hwmon);
    return match;
}

static enum parse_result parse_interval(char const *line, uint8_t *interval) {
    LOG(LOG_LVL2, "Matching %s against interval...\n", line);
    regmatch_t pmatch[2];
    if(regexec(&interval_rgx, line, 2, pmatch, 0)) {
        LOG(LOG_LVL2, "No match\n");
        return no_match;
    }
    char buffer[OPTION_BUF_SIZE];
    if(strsncpy(buffer, line + pmatch[1].rm_so, regmatch_size(pmatch[1]), sizeof buffer) < 0) {
        fprintf(stderr, "Interval on line %u overflows the buffer\n", line_number);
        return failure;
    }

    *interval = atoi(buffer);
    LOG(LOG_LVL1, "Interval set to %u seconds\n", *interval);
    return match;
}

static enum parse_result parse_throttling(char const *line, bool *throttle) {
    LOG(LOG_LVL2, "Matching %s against throttling...\n", line);
    regmatch_t pmatch[2];
    if(regexec(&throttle_rgx, line, 0, NULL, 0)) {
        LOG(LOG_LVL2, "No match\n");
        return no_match;
    }
    if(regexec(&throttle_option_rgx, line, 2, pmatch, 0)) {
        printf("Syntax error on line %u: %s\n", line_number, line);
        return failure;
    }

    char buffer[OPTION_BUF_SIZE];
    if(strsncpy(buffer, line + pmatch[1].rm_so, regmatch_size(pmatch[1]), sizeof buffer) < 0) {
        fprintf(stderr, "Throttling option on line %u overflows the buffer\n", line_number);
        return failure;
    }

    *throttle = strcmp(buffer, "yes")  == 0;
    LOG(LOG_LVL1, "Throttling set to %s\n", *throttle ? "aggressive" : "non-aggressive");
    return match;
}

static enum parse_result parse_interpolation(char const *line, enum interpolation_method *method) {
    LOG(LOG_LVL2, "Matching %s against interpolation...\n", line);
    regmatch_t pmatch[2];

    if(regexec(&interpolation_rgx, line, 0, NULL, 0)) {
        LOG(LOG_LVL2, "No match\n");
        return no_match;
    }
    if(regexec(&interpolation_option_rgx, line, 2, pmatch, 0)) {
        fprintf(stderr, "Syntax error on line %u: %s\n", line_number, line);
        return failure;
    }

    char buffer[INTERP_OPTION_SIZE];
    if(strsncpy(buffer, line + pmatch[1].rm_so, regmatch_size(pmatch[1]), sizeof buffer) < 0) {
        fprintf(stderr, "Interpolation option on line %u overflows the buffer\n", line_number);
        return failure;
    }

    *method = strcmp(buffer, "cosine") == 0;
    LOG(LOG_LVL1, "%s interpolation set\n", *method ? "Cosine" : "Linear");
    return match;
}

static enum parse_result parse_matrix(char const *line, matrix mtrx, uint8_t *mtrx_rows) {
    LOG(LOG_LVL2, "Matching %s against matrix...\n", line);
    static int8_t current_temp = -1;
    regmatch_t pmatch[4];
    if(regexec(&matrix_rgx, line, 4, pmatch, 0)) {
        LOG(LOG_LVL2, "No match\n");
        return no_match;
    }

    if(regexec(&matrix_start_rgx, line, 0, NULL, 0) == 0) {
        parsing_matrix = true;
        current_temp = -1;
    }
    if(!parsing_matrix) {
        fprintf(stderr, "Stray matrix row %s on line %u\n", line, line_number);
        return failure;
    }
    if(*mtrx_rows >= MATRIX_ROWS - 1) {
        fprintf(stderr, "Too many rows in matrix\n");
        return failure;
     }

    if(!regmatch_to_uint8(line, pmatch[2], &mtrx[*mtrx_rows][0])) {
        fprintf(stderr, "Failed to read temperature in %s on line %u\n", line, line_number);
        return failure;
    }
    if(!regmatch_to_uint8(line, pmatch[3], &mtrx[*mtrx_rows][1])) {
        fprintf(stderr, "Failed to read fan speed in %s on line %u\n", line, line_number);
        return failure;
    }

    if(mtrx[*mtrx_rows][0] <= current_temp) {
        fprintf(stderr, "Config error on line %u: temperatures in matrix must be increasing:\n"
                        "    expected a value > %d, got %u\n", line_number, current_temp, mtrx[*mtrx_rows][0]);
        return failure;
    }
    if(mtrx[*mtrx_rows][0] > TEMP_MAX) {
        fprintf(stderr, "Config error on line %u, max temperature allowed in matrix is %d\n", line_number, TEMP_MAX);
        return failure;
    }
    current_temp = (int8_t)mtrx[*mtrx_rows][0];

    LOG(LOG_LVL1, "Set values on row %u, temp: %u, speed: %u%%\n", *mtrx_rows, mtrx[*mtrx_rows][0], mtrx[*mtrx_rows][1]);

    ++(*mtrx_rows);

    if(regexec(&matrix_end_rgx, line, 0, NULL, 0) == 0) {
        parsing_matrix = false;
    }

    return match;
}

static inline bool is_empty_line(char const *line) {
    return regexec(&empty_rgx, line, 0, NULL, 0) == 0;
}


bool parse_config(char const *restrict path, char *restrict hwmon, size_t hwmon_count, uint8_t *interval, bool *throttle, enum interpolation_method *interp, matrix mtrx, uint8_t *mtrx_rows) {
    if(!regexps_compiled && !compile_regexps()) {
        return false;
    }
    *mtrx_rows = 0;
    line_number = 0;
    parsing_matrix = false;

    FILE *fp = fopen(path, "r");
    if(!fp) {
        fprintf(stderr, "Failed to read config file\n");
        return false;
    }

    char buffer[LINE_SIZE];
    char tmp[LINE_SIZE];
    enum parse_result result;

    while(fgets(buffer, sizeof buffer, fp)) {
        replace_char(buffer, '\n', '\0');
        LOG(LOG_LVL2, "Read line '%s'\n", buffer);
        ++line_number;
        if(!strip_comments(tmp, buffer, sizeof tmp)) {
            fclose(fp);
            return false;
        }
        if(!strip_leading_whitespace(buffer, tmp, sizeof buffer)) {
            fclose(fp);
            return false;
        }

        if(is_empty_line(buffer)) {
            continue;
        }

        result = parse_hwmon(buffer, hwmon, hwmon_count);
        if(result == failure) {
            fclose(fp);
            return false;
        }
        else if(result == match) {
            continue;
        }

        result = parse_interval(buffer, interval);
        if(result == failure) {
            fclose(fp);
            return false;
        }
        else if(result == match) {
            continue;
        }

        result = parse_throttling(buffer, throttle);
        if(result == failure) {
            fclose(fp);
            return false;
        }
        else if(result == match) {
            continue;
        }

        result = parse_interpolation(buffer, interp);
        if(result == failure) {
            fclose(fp);
            return false;
        }
        else if(result == match) {
            continue;
        }

        result = parse_matrix(buffer, mtrx, mtrx_rows);
        if(result == failure) {
            fclose(fp);
            return false;
        }
        else if(result == match) {
            continue;
        }

        fprintf(stderr, "Syntax error on line %u: %s\n", line_number, buffer);
        fclose(fp);
        return false;
    }
    fclose(fp);

    if(*interval < 1) {
        fprintf(stderr, "Update intervals below 1 are not allowed, setting interval to 1\n");
        *interval = 1;
    }
    else if(*mtrx_rows < 1) {
        fprintf(stderr, "No matrix specified\n");
        return false;
    }
    else if(parsing_matrix) {
        fprintf(stderr, "Syntax error on line %u: Unterminted matrix\n", line_number);
        return false;
    }
    return true;
}

void *monitor_config(void *monitor) {
    extern bool volatile daemon_alive;
    char const *path = ((struct file_monitor*)monitor)->path;
    bool(*callback)(char const*) = ((struct file_monitor*)monitor)->callback;
    struct stat attrib;
    time_t last_read;
    time(&last_read);

    while(daemon_alive) {
        if(stat(path, &attrib) == -1) {
            fprintf(stderr, "Unable to monitor config file\n");
            break;
        }
        if(difftime(attrib.st_mtime, last_read) > 0) {
            LOG(LOG_LVL1, "Config file updated, reloading...\n");
            if(!callback(path)) {
                fprintf(stderr, "Failed to reload config\n");
            }
            else {
                printf("Config reloaded\n");
            }
            time(&last_read);
        }
        sleep(CONFIG_MONITOR_INTERVAL);
    }
    return NULL;
}
