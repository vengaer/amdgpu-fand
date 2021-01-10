#include "cache_mock.h"

#include <stdio.h>

static bool(*is_padded)(void) = 0;
static bool(*exists_in_sysfs)(char const *) = 0;

void mock_cache_struct_is_padded(bool(*mock)(void)) {
    is_padded = mock;
}

void mock_cache_file_exists_in_sysfs(bool(*mock)(char const *)) {
    exists_in_sysfs = mock;
}

bool cache_struct_is_padded(void) {
    if(!is_padded) {
        fputs("cache_struct_is_padded not mocked\n", stderr);
        return false;
    }
    return is_padded();
}

bool cache_file_exists_in_sysfs(char const *file) {
    if(!exists_in_sysfs) {
        fputs("cache_file_exists_in_sysfs not mocked\n", stderr);
        return false;
    }

    return exists_in_sysfs(file);
}
