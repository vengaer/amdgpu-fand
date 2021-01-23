#include "cache_mock.h"
#include "mock.h"

#include <stdio.h>

static bool(*is_padded)(void) = 0;
static bool(*exists_in_sysfs)(char const *) = 0;

void mock_cache_struct_is_padded(bool(*mock)(void)) {
    mock_function(is_padded, mock);
}

void mock_cache_file_exists_in_sysfs(bool(*mock)(char const *)) {
    mock_function(exists_in_sysfs, mock);
}

bool cache_struct_is_padded(void) {
    validate_mock(cache_struct_is_padded, is_padded);
    return is_padded();
}

bool cache_file_exists_in_sysfs(char const *file) {
    validate_mock(cache_file_exists_in_sysfs, exists_in_sysfs);
    return exists_in_sysfs(file);
}
