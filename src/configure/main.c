#include "libc.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if(argc < 2) {
        fputs("No output file supplied\n", stderr);
        return 1;
    }

    enum libc_vendor vendor = libc_get_vendor();

    FILE *fp = fopen(argv[1], "w");

    if(!fp) {
        fprintf(stderr, "Could not open %s: %s", argv[1], strerror(errno));
        return 1;
    }

    fprintf(fp, "libc := %s\n", libc_vendor_strings[vendor]);

    if(fclose(fp)) {
        perror("Error on fclose");
    }

    /* Exit with 0 status if vendor is detected */
    return vendor == libc_vendor_unknown;
}
