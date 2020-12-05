#include "filesystem.h"

#include <errno.h>

#include <sys/stat.h>

bool fsys_dir_exists(char const *path) {
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
}
