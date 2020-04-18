#include "config.h"
#include "filesystem.h"
#include "hwmon.h"
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
#define SPEED_IFACE_SIZE 16
#define TEMP_MAX INT8_MAX

#define TMP_FILE_NAME ".amdgpu-fanctl.tmp"

#define HANDLE_PARSE_RESULT(result)     \
    if(result == parse_result_failure) {             \
        fclose(fp);                     \
        return false;                   \
    }                                   \
    else if(result == parse_result_match) {          \
        continue;                       \
    }

static regex_t interval_rgx, hwmon_rgx, hwmon_content_rgx, empty_value_rgx, persistent_rgx, persistent_content_rgx;
static regex_t empty_rgx, leading_space_rgx, matrix_rgx, matrix_start_rgx, matrix_end_rgx;
static regex_t throttle_rgx, throttle_option_rgx;
static regex_t interpolation_rgx, interpolation_option_rgx;
static regex_t speed_iface_rgx, speed_iface_option_rgx, speed_iface_tacho_rgx;
static regex_t hysteresis_rgx, hysteresis_content_rgx;
static bool parsing_matrix = false, regexps_compiled = false;
static uint8_t line_number = 0;

enum parse_result {
    parse_result_failure = -1,
    parse_result_match,
    parse_result_no_match
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
    LOG(VERBOSITY_LVL3, "Compiling config regexps\n");
    int reti;
    reti = regcomp(&interval_rgx, "^\\s*INTERVAL\\s*=\\s*\"?([0-9]{1,3})\"?\\s*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile interval regex\n", stderr);
        return false;
    }
    reti = regcomp(&hwmon_rgx, "^\\s*HWMON\\s*=\\s*\"(.*)\"\\s*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile hwmon regex\n", stderr);
        return false;
    }
    reti = regcomp(&hwmon_content_rgx, "^\\s*HWMON\\s*=\\s*\"(hwmon[0-9])\"\\s*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile hwmon content regex\n", stderr);
        return false;
    }
    reti = regcomp(&empty_value_rgx, "\"\\s*\"\\s*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile hwmon empty regex\n", stderr);
        return false;
    }
    reti = regcomp(&persistent_rgx, "^\\s*PERSISTENT_PATH\\s*=\\s*\".*\"\\s*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile persistent path regex\n", stderr);
        return false;
    }
    reti = regcomp(&persistent_content_rgx, "^\\s*PERSISTENT_PATH\\s*=\\s*\"(.*)\"\\s*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile persistent path content regex\n", stderr);
        return false;
    }
    reti = regcomp(&matrix_rgx, "^(MATRIX\\s*=\\s*\\()?'([0-9]{1,3})::([0-9]{1,3})'\\)?*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile matrix regex\n", stderr);
        return false;
    }
    reti = regcomp(&matrix_start_rgx, "^MATRIX\\s*=\\s*\\('[0-9]{1,3}::[0-9]{1,3}'\\)?\\s*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile matrix start regex\n", stderr);
        return false;
    }
    reti = regcomp(&matrix_end_rgx, "\\s*(MATRIX\\s*=\\s*\\()?'[0-9]{1,3}::[0-9]{1,3}'\\)\\s*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile matrix end regex\n", stderr);
        return false;
    }
    reti = regcomp(&empty_rgx, "^\\s*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile empty regex\n", stderr);
        return false;
    }
    reti = regcomp(&leading_space_rgx, "^\\s*(.*)$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile leading spaces regex\n", stderr);
        return false;
    }
    reti = regcomp(&throttle_rgx, "^\\s*AGGRESSIVE_THROTTLING\\s*=\\s*\".*\"\\s*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile throttling regex\n", stderr);
        return false;
    }
    reti = regcomp(&throttle_option_rgx, "^\\s*AGGRESSIVE_THROTTLING\\s*=\\s*\"(yes|no)\"\\s*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile throttling option regex\n", stderr);
        return false;
    }
    reti = regcomp(&interpolation_rgx, "^\\s*INTERPOLATION\\s*=\\s*\".*\"\\s*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile interpolation regex\n", stderr);
        return false;
    }
    reti = regcomp(&interpolation_option_rgx, "^\\s*INTERPOLATION\\s*=\\s*\"(linear|cosine)\"\\s*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile interpolation option regex\n", stderr);
        return false;
    }
    reti = regcomp(&speed_iface_rgx, "^\\s*SPEED_INTERFACE\\s*=\\s*\".*\"\\s*", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile speed interface regex\n", stderr);
        return false;
    }
    reti = regcomp(&speed_iface_option_rgx, "^\\s*SPEED_INTERFACE\\s*=\\s*\"(tacho(meter)?|daemon|fand)\"\\s*", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile speed interface option regex\n", stderr);
        return false;
    }
    reti = regcomp(&speed_iface_tacho_rgx, "^tacho(meter)?$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile speed interface tacho regex\n", stderr);
        return false;
    }
    reti = regcomp(&hysteresis_rgx, "^\\s*HYSTERESIS\\s*=\\s*\"?.*\"?\\s*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile hysteresis regex\n", stderr);
        return false;
    }
    reti = regcomp(&hysteresis_content_rgx, "^\\s*HYSTERESIS\\s*=\\s*\"?([0-9]{1,3})\"?\\s*$", REG_EXTENDED);
    if(reti) {
        fputs("Failed to compile hysteresis content regex\n", stderr);
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
    LOG(VERBOSITY_LVL3, "Stripped comments: \"%s\" -> \"%s\"\n", src, dst);
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
    LOG(VERBOSITY_LVL3, "Stripped whitespace: \"%s\" ->\"%s\"\n", src, dst);
    return true;
}

static enum parse_result parse_persistent(char const *restrict line, char *restrict path, size_t count) {
    LOG(VERBOSITY_LVL3, "Matching %s against persistent path...\n", line);
    regmatch_t pmatch[2];
    if(regexec(&persistent_rgx, line, 0, NULL, 0)) {
        LOG(VERBOSITY_LVL3, "No match\n");
        return parse_result_no_match;
    }
    else if(regexec(&empty_value_rgx, line, 0, NULL, 0) == 0) {
        LOG(VERBOSITY_LVL1, "Persistent path is empty, using default\n");
        path[0] = '\0';
        return parse_result_match;
    }
    else if(regexec(&persistent_content_rgx, line, 2, pmatch, 0)) {
        fprintf(stderr, "Syntax error on line %u: %s\n", line_number, line);
        return parse_result_failure;
    }
    if(strsncpy(path, line + pmatch[1].rm_so, regmatch_size(pmatch[1]), count) < 0) {
        fprintf(stderr, "Persistent path on line %u overflows the buffer\n", line_number);
        return parse_result_failure;
    }
    LOG(VERBOSITY_LVL1, "Persistent path set to %s\n", path);
    return parse_result_match;
}

static enum parse_result parse_hwmon(char const *restrict line, char *restrict hwmon, size_t count) {
    LOG(VERBOSITY_LVL3, "Matching %s against hwmon...\n", line);
    regmatch_t pmatch[2];
    if(regexec(&hwmon_rgx, line, 0, NULL, 0)) {
        LOG(VERBOSITY_LVL3, "No match\n");
        return parse_result_no_match;
    }
    else if(regexec(&empty_value_rgx, line, 0, NULL, 0) == 0) {
        LOG(VERBOSITY_LVL1, "hwmon is empty, keeping current path\n");
        hwmon[0] = '\0';
        return parse_result_match;
    }
    else if(regexec(&hwmon_content_rgx, line, 2, pmatch, 0)) {
        fprintf(stderr, "Syntax error on line %u: %s\n", line_number, line);
        return parse_result_failure;
    }
    if(strsncpy(hwmon, line + pmatch[1].rm_so, regmatch_size(pmatch[1]), count) < 0) {
        fprintf(stderr, "hwmon value on line %u overflows the buffer\n", line_number);
        return parse_result_failure;
    }
    LOG(VERBOSITY_LVL1, "hwmon set to %s\n", hwmon);
    return parse_result_match;
}

static enum parse_result parse_interval(char const *line, uint8_t *interval) {
    LOG(VERBOSITY_LVL3, "Matching %s against interval...\n", line);
    regmatch_t pmatch[2];
    if(regexec(&interval_rgx, line, 2, pmatch, 0)) {
        LOG(VERBOSITY_LVL3, "No match\n");
        return parse_result_no_match;
    }
    char buffer[OPTION_BUF_SIZE];
    if(strsncpy(buffer, line + pmatch[1].rm_so, regmatch_size(pmatch[1]), sizeof buffer) < 0) {
        fprintf(stderr, "Interval on line %u overflows the buffer\n", line_number);
        return parse_result_failure;
    }

    *interval = atoi(buffer);
    LOG(VERBOSITY_LVL1, "Interval set to %u seconds\n", *interval);
    return parse_result_match;
}

static enum parse_result parse_throttling(char const *line, bool *throttle) {
    LOG(VERBOSITY_LVL3, "Matching %s against throttling...\n", line);
    regmatch_t pmatch[2];
    if(regexec(&throttle_rgx, line, 0, NULL, 0)) {
        LOG(VERBOSITY_LVL3, "No match\n");
        return parse_result_no_match;
    }
    if(regexec(&throttle_option_rgx, line, 2, pmatch, 0)) {
        fprintf(stderr, "Syntax error on line %u: %s\n", line_number, line);
        return parse_result_failure;
    }

    char buffer[OPTION_BUF_SIZE];
    if(strsncpy(buffer, line + pmatch[1].rm_so, regmatch_size(pmatch[1]), sizeof buffer) < 0) {
        fprintf(stderr, "Throttling option on line %u overflows the buffer\n", line_number);
        return parse_result_failure;
    }

    *throttle = strcmp(buffer, "yes")  == 0;
    LOG(VERBOSITY_LVL1, "Throttling set to %s\n", *throttle ? "aggressive" : "non-aggressive");
    return parse_result_match;
}

static enum parse_result parse_interpolation(char const *line, enum interpolation_method *method) {
    LOG(VERBOSITY_LVL3, "Matching %s against interpolation...\n", line);
    regmatch_t pmatch[2];

    if(regexec(&interpolation_rgx, line, 0, NULL, 0)) {
        LOG(VERBOSITY_LVL3, "No match\n");
        return parse_result_no_match;
    }
    if(regexec(&interpolation_option_rgx, line, 2, pmatch, 0)) {
        fprintf(stderr, "Syntax error on line %u: %s\n", line_number, line);
        return parse_result_failure;
    }

    char buffer[INTERP_OPTION_SIZE];
    if(strsncpy(buffer, line + pmatch[1].rm_so, regmatch_size(pmatch[1]), sizeof buffer) < 0) {
        fprintf(stderr, "Interpolation option on line %u overflows the buffer\n", line_number);
        return parse_result_failure;
    }

    *method = strcmp(buffer, "cosine") == 0;
    LOG(VERBOSITY_LVL1, "%s interpolation set\n", *method ? "Cosine" : "Linear");
    return parse_result_match;
}

static enum parse_result parse_speed_interface(char const *line, enum speed_interface *iface) {
    LOG(VERBOSITY_LVL3, "Matching %s against speed interface...\n", line);
    regmatch_t pmatch[2];

    if(regexec(&speed_iface_rgx, line, 0, NULL, 0)) {
        LOG(VERBOSITY_LVL3, "No match\n");
        return parse_result_no_match;
    }
    if(regexec(&speed_iface_option_rgx, line, 2, pmatch, 0)) {
        fprintf(stderr, "Syntax error on line %u: %s\n", line_number, line);
        return parse_result_failure;
    }

    char buffer[SPEED_IFACE_SIZE];
    if(strsncpy(buffer, line + pmatch[1].rm_so, regmatch_size(pmatch[1]), sizeof buffer) < 0) {
        fprintf(stderr, "Speed interface option on line %u overtflows the buffer\n", line_number);
        return parse_result_failure;
    }

    if(regexec(&speed_iface_tacho_rgx, buffer, 0, NULL, 0) == 0) {
        *iface = sifc_tacho;
    }
    else {
        *iface = sifc_daemon;
    }
    LOG(VERBOSITY_LVL1, "Speed interface set to %s\n", *iface ? "daemon" : "tacho");
    return parse_result_match;
}

static enum parse_result parse_hysteresis(char const *line, uint8_t *hysteresis) {
    LOG(VERBOSITY_LVL3, "Matching %s against hysteresis...\n", line);
    regmatch_t pmatch[2];

    if(regexec(&hysteresis_rgx, line, 0, NULL, 0)) {
        LOG(VERBOSITY_LVL3, "No match\n");
        return parse_result_no_match;
    }
    if(regexec(&hysteresis_content_rgx, line, 2, pmatch, 0)) {
        fprintf(stderr, "Syntax error on line %u: %s\n", line_number, line);
        return parse_result_failure;
    }

    char buffer[OPTION_BUF_SIZE];
    if(strsncpy(buffer, line + pmatch[1].rm_so, regmatch_size(pmatch[1]), sizeof buffer) < 0) {
        fprintf(stderr, "Hysteresis on line %u overflows the buffer\n", line_number);
        return parse_result_failure;
    }

    *hysteresis = atoi(buffer);
    LOG(VERBOSITY_LVL1, "Hysteresis set to %u degrees\n", *hysteresis);
    return parse_result_match;
}

static enum parse_result parse_matrix(char const *line, matrix mtrx, uint8_t *mtrx_rows) {
    LOG(VERBOSITY_LVL3, "Matching %s against matrix...\n", line);
    static int8_t current_temp = -1;
    regmatch_t pmatch[4];
    if(regexec(&matrix_rgx, line, 4, pmatch, 0)) {
        LOG(VERBOSITY_LVL3, "No match\n");
        return parse_result_no_match;
    }

    if(regexec(&matrix_start_rgx, line, 0, NULL, 0) == 0) {
        parsing_matrix = true;
        current_temp = -1;
    }
    if(!parsing_matrix) {
        fprintf(stderr, "Stray matrix row %s on line %u\n", line, line_number);
        return parse_result_failure;
    }
    if(*mtrx_rows >= MATRIX_ROWS - 1) {
        fprintf(stderr, "Too many rows in matrix\n");
        return parse_result_failure;
     }

    if(!regmatch_to_uint8(line, pmatch[2], &mtrx[*mtrx_rows][0])) {
        fprintf(stderr, "Failed to read temperature in %s on line %u\n", line, line_number);
        return parse_result_failure;
    }
    if(!regmatch_to_uint8(line, pmatch[3], &mtrx[*mtrx_rows][1])) {
        fprintf(stderr, "Failed to read fan speed in %s on line %u\n", line, line_number);
        return parse_result_failure;
    }

    if(mtrx[*mtrx_rows][0] <= current_temp) {
        fprintf(stderr, "Config error on line %u: expected a value > %d, got %u (temperature must be strictly increasing)\n", line_number, current_temp, mtrx[*mtrx_rows][0]);
        return parse_result_failure;
    }
    if(mtrx[*mtrx_rows][0] > TEMP_MAX) {
        fprintf(stderr, "Config error on line %u, max temperature allowed in matrix is %d\n", line_number, TEMP_MAX);
        return parse_result_failure;
    }
    current_temp = (int8_t)mtrx[*mtrx_rows][0];

    LOG(VERBOSITY_LVL1, "Set values on row %u, temp: %u, speed: %u%%\n", *mtrx_rows, mtrx[*mtrx_rows][0], mtrx[*mtrx_rows][1]);

    ++(*mtrx_rows);

    if(regexec(&matrix_end_rgx, line, 0, NULL, 0) == 0) {
        parsing_matrix = false;
    }

    return parse_result_match;
}

static inline bool is_empty_line(char const *line) {
    return regexec(&empty_rgx, line, 0, NULL, 0) == 0;
}

static bool replace_line_matching_pattern(char const *restrict path, char const *restrict pattern, char const *restrict replacement) {
    char tmp_file[LINE_SIZE];
    char buffer[LINE_SIZE];
    regex_t rgx;
    int reti = regcomp(&rgx, pattern, REG_EXTENDED);
    if(reti) {
        fprintf(stderr, "Failed to compile replacement regex\n");
        return false;
    }

    if(parent_dir(tmp_file, path, sizeof tmp_file) < 0) {
        fprintf(stderr, "Parent path of %s overflows the buffer\n", path);
        return false;
    }
    if((*tmp_file && strscat(tmp_file, "/", sizeof tmp_file) < 0) || strscat(tmp_file, TMP_FILE_NAME, sizeof tmp_file) < 0) {
        fprintf(stderr, "Temp file overflows the buffer\n");
        return false;
    }

    FILE *src = fopen(path, "r");
    if(!src) {
        fprintf(stderr, "Failed to open %s for reading\n", path);
        return false;
    }
    FILE *dst = fopen(tmp_file, "w");
    if(!dst) {
        fprintf(stderr, "Failed to open %s for writing\n", buffer);
        fclose(src);
        return false;
    }

    while(fgets(buffer, sizeof buffer, src)) {
        if(regexec(&rgx, buffer, 0, NULL, 0) == 0) {
            fprintf(dst, "%s\n", replacement);
        }
        else {
            fprintf(dst, "%s", buffer);
        }
    }
    fclose(src);
    fclose(dst);
    rename(tmp_file, path);
    return true;
}


bool parse_config(struct config_params *params) {
    if(!regexps_compiled && !compile_regexps()) {
        return false;
    }
    *params->mtrx_rows = 0;
    line_number = 0;
    parsing_matrix = false;

    FILE *fp = fopen(params->path, "r");
    if(!fp) {
        fprintf(stderr, "Failed to read config file %s\n", params->path);
        return false;
    }

    char buffer[LINE_SIZE];
    char tmp[LINE_SIZE];
    enum parse_result result;

    while(fgets(buffer, sizeof buffer, fp)) {
        replace_char(buffer, '\n', '\0');
        LOG(VERBOSITY_LVL3, "Read line '%s'\n", buffer);
        ++line_number;
        if(!strip_comments(tmp, buffer, sizeof tmp) || !strip_leading_whitespace(buffer, tmp, sizeof buffer)) {
            fclose(fp);
            return false;
        }

        if(is_empty_line(buffer)) {
            continue;
        }

        result = parse_hwmon(buffer, params->hwmon, params->hwmon_size);
        HANDLE_PARSE_RESULT(result);

        result = parse_interval(buffer, params->interval);
        HANDLE_PARSE_RESULT(result);

        result = parse_throttling(buffer, params->aggressive_throttle);
        HANDLE_PARSE_RESULT(result);

        result = parse_interpolation(buffer, params->interp_method);
        HANDLE_PARSE_RESULT(result);

        result = parse_speed_interface(buffer, params->speed_iface);
        HANDLE_PARSE_RESULT(result);

        result = parse_persistent(buffer, params->persistent, params->persistent_size);
        HANDLE_PARSE_RESULT(result);

        result = parse_hysteresis(buffer, params->hysteresis);
        HANDLE_PARSE_RESULT(result);

        result = parse_matrix(buffer, params->mtrx, params->mtrx_rows);
        HANDLE_PARSE_RESULT(result);

        fprintf(stderr, "Syntax error on line %u: %s\n", line_number, buffer);
        fclose(fp);
        return false;
    }
    fclose(fp);

    if(*params->interval < 1) {
        fprintf(stderr, "Update intervals below 1 are not allowed, setting interval to 1\n");
        *params->interval = 1;
    }
    else if(*params->mtrx_rows < 1) {
        fprintf(stderr, "No matrix specified\n");
        return false;
    }
    else if(parsing_matrix) {
        fprintf(stderr, "Syntax error on line %u: Unterminated matrix\n", line_number);
        return false;
    }
    return true;
}

bool replace_persistent_path_value(char const *restrict path, char const *restrict replacement) {
    char buffer[HWMON_PATH_LEN];
    if(strscpy(buffer, "PERSISTENT_PATH=\"", sizeof buffer) < 0 ||
       strscat(buffer, replacement, sizeof buffer) < 0          ||
       strscat(buffer, "\"", sizeof buffer) < 0)
    {
        fprintf(stderr, "Overflow while attempting to replace persistent path\n");
        return false;
    }
    return replace_line_matching_pattern(path, "^\\s*PERSISTENT_PATH=\"\\s*\"\\s*$", buffer);
}
