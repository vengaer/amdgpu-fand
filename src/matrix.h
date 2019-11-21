#ifndef MATRIX_H
#define MATRIX_H

#define MATRIX_COLS 2
#define MATRIX_ROWS 16

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t matrix[MATRIX_ROWS][MATRIX_COLS];

static inline void matrix_extract_column(uint8_t *dst, matrix mtrx, uint8_t col, size_t count) {
    size_t const max = count < MATRIX_ROWS ? count : MATRIX_ROWS;
    for(size_t i = 0; i < max; i++) {
        dst[i] = mtrx[i][col];
    }
}

static inline void matrix_extract_temps(uint8_t *dst, matrix mtrx, size_t count) {
    matrix_extract_column(dst, mtrx, 0u, count);
}

static inline void matrix_extract_speeds(uint8_t *dst, matrix mtrx, size_t count) {
    matrix_extract_column(dst, mtrx, 1u, count);
}

void matrix_print(matrix mtrx, size_t rows);

#endif
