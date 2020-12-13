#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if(argc < 2) {
        fputs("No output file supplied\n", stderr);
        return 1;
    }

    char const *file = argv[1];

    bool little_endian = *(unsigned char *)&(unsigned short){ 1 };

    FILE *fp = fopen(file, "w");
    if(!fp) {
        fprintf(stderr, "Failed to open %s for writing: %s", file, strerror(errno));
        return 1;
    }

    fprintf(fp, "little_endian := %c", little_endian ? 'y' : 'n');

    fclose(fp);

    return 0;
}
