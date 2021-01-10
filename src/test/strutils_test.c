#include "strutils.h"
#include "strutils_test.h"
#include "test.h"

#include <string.h>

void test_strscpy_result(void) {
    char const *longstr = "String too long to fit in dst";
    char const *shortstr = "String";

    char dst[8];

    strscpy(dst, shortstr, sizeof dst);

    fand_assert(strcmp(dst, shortstr) == 0);

    strscpy(dst, longstr, sizeof dst);

    fand_assert(strcmp(dst, "String ") == 0);
}

void test_strscpy_return_value(void) {
    char const *longstr = "String too long to fit in dst";
    char const *shortstr = "String";

    char pfit[8];
    memset(pfit, 'a', sizeof pfit - 1);
    pfit[7] = '\0';

    char dst[8];

    fand_assert(strscpy(dst, pfit, sizeof dst) == (ssize_t) strlen(pfit));
    fand_assert(strscpy(dst, shortstr, sizeof dst) == (ssize_t) strlen(shortstr));
    fand_assert(strscpy(dst, longstr, sizeof dst) == -E2BIG);
}

void test_strscpy_null_termination(void) {
    char const *longstr = "String too long to fit in dst";
    char const *shortstr = "String";

    char dst[8];

    dst[7] = 1;

    strscpy(dst, shortstr, sizeof dst);

    /* Ensure excessive null bytes aren't written */
    fand_assert(dst[sizeof dst - 1] == 1);

    fand_assert(dst[strlen(shortstr)] == '\0');

    strscpy(dst, longstr, sizeof dst);

    fand_assert(dst[sizeof dst - 1] == '\0');
}

void test_strsncpy_result(void) {
    char const *longstr = "String too long to fit in dst";
    char const *shortstr = "String";

    char dst[8];

    strsncpy(dst, shortstr, sizeof dst, 3);
    fand_assert(strcmp(dst, "Str") == 0);

    strsncpy(dst, shortstr, sizeof dst, 32);
    fand_assert(strcmp(dst, shortstr) == 0);

    strsncpy(dst, longstr, sizeof dst, 32);
    fand_assert(strcmp(dst, "String ") == 0);
}

void test_strsncpy_return_value(void) {
    char const *longstr = "String too long to fit in dst";
    char const *shortstr = "String";

    char pfit[8];
    memset(pfit, 'a', sizeof pfit - 1);
    pfit[7] = '\0';

    char dst[8];

    fand_assert(strsncpy(dst, pfit, sizeof dst, sizeof dst - 1) == (ssize_t) strlen(pfit));
    fand_assert(strsncpy(dst, shortstr, sizeof dst, strlen(shortstr)) == (ssize_t) strlen(shortstr));
    fand_assert(strsncpy(dst, longstr, sizeof dst, sizeof dst) == -E2BIG);
    fand_assert(strsncpy(dst, pfit, sizeof dst, 3) == 3);
    fand_assert(strsncpy(dst, pfit, sizeof dst, sizeof pfit) == -E2BIG);
    fand_assert(strsncpy(dst, longstr, sizeof dst, sizeof dst + 1) == -E2BIG);
}

void test_strsncpy_null_termination(void) {
    char const *longstr = "String too long to fit in dst";
    char const *shortstr = "String";

    char dst[8];

    dst[7] = 1;

    strsncpy(dst, shortstr, sizeof dst, strlen(shortstr));

    /* Ensure excessive null bytes aren't written */
    fand_assert(dst[sizeof dst - 1] == 1);
    fand_assert(dst[strlen(shortstr)] == '\0');

    strsncpy(dst, longstr, sizeof dst, 4);
    fand_assert(dst[4] == '\0');

    strsncpy(dst, longstr, sizeof dst, strlen(longstr));
    fand_assert(dst[sizeof dst - 1] == '\0');

    strsncpy(dst, longstr, sizeof dst, sizeof dst);
    fand_assert(dst[sizeof dst - 1] == '\0');
}
