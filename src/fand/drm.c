#include "drm.h"
#include "fandcfg.h"
#include "macro.h"
#include "regutils.h"
#include "strutils.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <libdrm/amdgpu_drm.h>
#include <regex.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#define DRI_DEV_DIR "/dev/dri/"

static int drm_fd = -1;

enum { DRM_BUF_SIZE = 128 };
enum { DRM_CARD_IDX_OFFSET = 128 };

int drm_open(unsigned card_idx) {
    char buffer[DRM_BUF_SIZE];
    regex_t devregex;
    regmatch_t pmatch[2];

    ssize_t pos = strscpy(buffer, DRI_DEV_DIR, sizeof(buffer));

    if(pos < 0) {
        return pos;
    }

    if(regcomp_info(&devregex, "^renderD([0-9]+)", REG_EXTENDED, "dri")) {
        return -1;
    }

    DIR *dir = opendir(buffer);
    struct dirent *dp;

    unsigned index;

    while((dp = readdir(dir))) {
        if(regexec(&devregex, dp->d_name, array_size(pmatch), pmatch, 0)) {
            continue;
        }

        index = atoi(dp->d_name + pmatch[1].rm_so);

        if(index == DRM_CARD_IDX_OFFSET + card_idx) {
            pos = strscpy(buffer + pos, dp->d_name, sizeof(buffer) - pos);
            if(pos < 0) {
                goto cleanup;
            }
            break;
        }
    }

    /* Found matching entry */
    if(dp) {
        drm_fd = open(buffer, O_RDONLY);
    }

    if(drm_fd == -1) {
        syslog(LOG_ERR, "Error when opening dri device: %s\n", strerror(errno));
    }

cleanup:
    regfree(&devregex);
    closedir(dir);

    return drm_fd;
}

int drm_close(void) {
    int status = close(drm_fd);
    if(status == -1) {
        syslog(LOG_WARNING, "Could not close drm file desriptor: %s", strerror(errno));
    }
    return status;
}

int drm_get_temp(void) {
    int temp;
    struct drm_amdgpu_info hwinfo = {
        .return_pointer = (__u64)&temp,
        .return_size = sizeof(temp),
        .query = AMDGPU_INFO_SENSOR,
        .sensor_info.type = AMDGPU_INFO_SENSOR_GPU_TEMP
    };

    if(ioctl(drm_fd, DRM_IOCTL_AMDGPU_INFO, &hwinfo)) {
        syslog(LOG_WARNING, "Could not read temperature sensor: %s", strerror(errno));
        return -1;
    }

    return temp;
}
