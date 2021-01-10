#ifndef MOCK_CACHE_H
#define MOCK_CACHE_H

#include <stdbool.h>

void mock_cache_struct_is_padded(bool (*mock)(void));
void mock_cache_file_exists_in_sysfs(bool (*mock)(char const *));

bool cache_struct_is_padded(void);
bool cache_file_exists_in_sysfs(char const *file);

#endif /* MOCK_CACHE_H */
