#include "charset.h"
#include "strutils.h"

#include <stdbool.h>
#include <stdio.h>

#include <locale.h>
#include <regex.h>

static char const *deg_c_ascii = "'C";
static char const *deg_c_utf8 = "°C";

static bool utf8_available(void) {
    static regex_t rgx;
    static bool rgx_compiled = false;
    if(!rgx_compiled) {
        if(regcomp(&rgx, "UTF-8$", REG_EXTENDED)) {
            fprintf(stderr, "Failed to compile utf regex\n");
            return false;
        }
        rgx_compiled = true;
    }

    char const *locale = setlocale(LC_ALL, "");
    return regexec(&rgx, locale, 0, NULL, 0) == 0;
}

char const *deg_celcius_str(void) {
    return utf8_available() ? deg_c_utf8 : deg_c_ascii;
}
