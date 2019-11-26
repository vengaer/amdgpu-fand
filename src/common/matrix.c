#include "charset.h"
#include "matrix.h"

#include <stdio.h>

void matrix_print(matrix mtrx, size_t rows) {
    printf(
        "*----------------------*\n"
        "|   temp    |  speed   |\n"
        "*----------------------*\n");
    for(size_t i = 0; i < rows; i++) {
        printf(
        "|   %03u%s   |   %03u%%   |\n"
        "*----------------------*\n", mtrx[i][0], deg_celcius_str(), mtrx[i][1]);
    }
}
