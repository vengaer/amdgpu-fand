#include "format.h"
#include "regutils.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <locale.h>
#include <regex.h>

#define DEGC_ASCII "'C"
#define DEGC_UTF8  "°C"

enum { MATRIX_CELL_WIDTH = 9 };

static inline bool format_utf8_support(void) {
    regex_t utf8rgx;
    bool status = false;

    if(regcomp_info(&utf8rgx, "UTF-8$", REG_EXTENDED, "utf8")) {
        return false;
    }

    char const *locale = setlocale(LC_ALL, "");
    if(!locale) {
        goto cleanup;
    }

    if(regexec(&utf8rgx, locale, 0, 0, 0) == 0) {
        status = true;
    }

cleanup:
    regfree(&utf8rgx);
    return status;
}

void format_matrix(unsigned char nrows, unsigned char const *matrix) {
    char buffer[MATRIX_CELL_WIDTH] = { 0 };
    memset(buffer, '=', MATRIX_CELL_WIDTH - 1);

    char const *degc = format_utf8_support() ? DEGC_UTF8 : DEGC_ASCII;

    printf("*%s*%s*\n", buffer, buffer);
    puts("|  temp  | speed  |");
    printf("*%s*%s*\n", buffer, buffer);
    memset(buffer, '-', MATRIX_CELL_WIDTH - 1);
    for(unsigned i = 0; i < nrows; i++) {
        printf("| %3hhu %s |  %3hhu %% |\n" ,matrix[2 * i], degc, matrix[2*i + 1]);
        printf("*%s*%s*\n", buffer, buffer);
    }

}

void format_temp(int temp) {
    if(format_utf8_support()) {
        printf("%d" DEGC_UTF8 "\n", temp);
    }
    else {
        printf("%d" DEGC_ASCII "\n", temp);
    }
}

void format_speed(int speed) {
    printf("%d%%\n", speed);
}

int format(union unpack_result const *result, ipc_request req, ipc_response rsp) {
    if(rsp == ipc_rsp_err) {
        fprintf(stderr, "%s\n", strerror(result->error));
        return -1;
    }

    if(rsp != ipc_rsp_ok) {
        fprintf(stderr, "Invalid response %hhu\n", rsp);
        return -1;
    }

    switch(req) {
        case ipc_req_speed:
            format_speed(result->speed);
            break;
        case ipc_req_temp:
            format_temp(result->temp);
            break;
        case ipc_req_matrix:
            format_matrix(result->matrix[0], &result->matrix[1]);
            break;
        default:
            fprintf(stderr, "Invalid request %hhu\n", req);
            return -1;
            break;
    }
    return 0;
}

