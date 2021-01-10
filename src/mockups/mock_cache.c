#include "mock_cache.h"
#include "regutils.h"

#include <regex.h>

bool cache_struct_is_padded(void) {
    return true;
}

bool cache_file_exists_in_sysfs(char const *file) {
    regex_t regex;
    bool exists;

    if(regcomp_info(&regex, "^/sys/devices/pci[0-9:]+(/[a-zA-Z0-9:.]+){4}/hwmon/hwmon[0-9]+/(pwm[0-9]+(_input)?|temp[0-9]+_input)", REG_EXTENDED, "sysfs mock")) {
        return false;
    }

    exists = regexec(&regex, file, 0, 0, 0) == 0;

    regfree(&regex);
    return exists;
}
