#include "charset.h"
#include "strutils.h"

#include <stdbool.h>
#include <stdio.h>

#include <locale.h>
#include <regex.h>

static bool locale_is_utf8(void) {
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

ssize_t degrees_celcius(char *dst, size_t count) {
    ssize_t len = -1;
    bool utf8 = locale_is_utf8();
    if(utf8) {
        len = strscpy(dst, "°C", count);
    }

    if(!utf8 || len < 0) {
        len = strscpy(dst, "'C", count);
    }

    return len;
}
