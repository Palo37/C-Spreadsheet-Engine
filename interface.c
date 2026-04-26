#include "spreadsheet.h"
#include <stdio.h>

void print_col_label(int n) {              // Recursively prints A, B... Z, AA, AB...
    if (n < 0) return;
    print_col_label((n / 26) - 1);
    printf("%c", (n % 26) + 'A');
}

void display_window(Spreadsheet* sheet) {

    printf("\033[H\033[J");                // Clear screen 

    printf("\n    ");                       // Spacing to seperate columns and rows and create graph like view


    for (int j = 0; j < 10 && (sheet->start_col + j) < sheet->cols; j++) {              // printing columns names
        int col_idx = sheet->start_col + j;

        printf("%4s", "");              // spacing between each column 
        print_col_label(col_idx);
    }
    printf("\n");

    for (int i = 0; i < 10 && (sheet->start_row + i) < sheet->rows; i++) {          // Printing Rows 10

        printf("%3d ", sheet->start_row + i + 1);

        for (int j = 0; j < 10 && (sheet->start_col + j) < sheet->cols; j++) {          // printing columns 10

            Cell c = sheet->grid[sheet->start_row + i][sheet->start_col + j];           // starting row and column to be displayed

            if (c.is_err)                       // Applying value to the cell
                printf("%5s", "ERR");
            else
                printf("%5d", c.value);
        }

        printf("\n");
    }

    printf("\n");
    fflush(stdout);
}