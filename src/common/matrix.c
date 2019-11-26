#include "charset.h"
#include "matrix.h"

#include <stdio.h>

#define TEMP_SUFFIX_SIZE 16

void matrix_print(matrix mtrx, size_t rows) {
    char temp_suffix[TEMP_SUFFIX_SIZE];

    if(degrees_celcius(temp_suffix, sizeof temp_suffix) < 0) {
        fprintf(stderr, "Temperature suffix overflows the buffer\n");
    }

    printf(
        "*----------------------*\n"
        "|   temp    |  speed   |\n"
        "*----------------------*\n");
    for(size_t i = 0; i < rows; i++) {
        printf(
        "|   %03u%s   |   %03u%%   |\n"
        "*----------------------*\n", mtrx[i][0], temp_suffix, mtrx[i][1]);
    }
}
