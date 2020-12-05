#include "filesystem.h"
#include "strutils.h"

#include <errno.h>
#include <string.h>

#include <sys/stat.h>

bool fsys_dir_exists(char const *path) {
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
}

bool fsys_file_exists(char const *path) {
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISREG(sb.st_mode);
}

ssize_t fsys_append(char *stem, char const *app, size_t bufsize) {

    ssize_t pos = strlen(stem);
    pos += strscpy(stem + pos, "/", bufsize - pos);
    if(pos < 0) {
        return pos;
    }
    pos += strscpy(stem + pos, app, bufsize - pos);
    if(pos < 0) {
        return pos;
    }

    return pos;
}
