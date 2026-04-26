#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "spreadsheet.h"

Spreadsheet* create_sheet(int rows, int cols) {
    Spreadsheet *sheet = (Spreadsheet*)malloc(sizeof(Spreadsheet));

    sheet->rows = rows;         // Assigning total num of rows and columns
    sheet->cols = cols;
    sheet->start_row = 0;          // Indice of the initial displayed cell
    sheet->start_col = 0;

    sheet->grid = (Cell**)malloc(rows * sizeof(Cell*));         // Allocating memory to accomodate each cell 

    for (int i = 0; i < rows; i++) {                            // Initialising each cell (memory)
        sheet->grid[i] = (Cell*)malloc(cols * sizeof(Cell));

        for (int j = 0; j < cols; j++) {
            sheet->grid[i][j].value = 0;
            sheet->grid[i][j].is_err = 0;
        }
    }

    return sheet;
}

void free_sheet(Spreadsheet *sheet) {               
    for (int i = 0; i < sheet->rows; i++) {
        free(sheet->grid[i]);                   // releasing memory of each cell and freeing the memory
    }
    free(sheet->grid);              // frees the array that was holding pointers
    free(sheet);                    // finally free the sheet free => disallocating memory
}

void handle_scroll(Spreadsheet *sheet, char cmd) {          // to scroll 10 rows/colmns in any direction
    int step = 10;

    if (cmd == 'w' && sheet->start_row - step >= 0)
        sheet->start_row -= step;

    else if (cmd == 's' && sheet->start_row + step < sheet->rows)
        sheet->start_row += step;

    else if (cmd == 'a' && sheet->start_col - step >= 0)
        sheet->start_col -= step;

    else if (cmd == 'd' && sheet->start_col + step < sheet->cols)
        sheet->start_col += step;
}

void scroll_to_cell(Spreadsheet *sheet, const char *cell) {         // determine the first cell(top left element) to br printed as per command
    int r, c;
    parse_cell(cell, &r, &c);

    sheet->start_row = r;
    sheet->start_col = c;
}

int main(int argc, char *argv[]) {              // Accept the initialising of sheet as taking 3 arguments ex: sheet R C is the only valid way to execute

    if (argc != 3) {
        printf("Invalid arguments\n");
        return 1;
    }

    int rows = atoi(argv[1]);           // Converting the argument value for row and column to int
    int cols = atoi(argv[2]);

    if (rows < 1 || rows > 999 || cols < 1 || cols > 18278) {
        printf("Invalid sheet size\n");
        return 1;
    }

    Spreadsheet *sheet = create_sheet(rows, cols);      // Initialising of sheet

    init_graph(rows, cols);     // initialise dependency tracking structures for formulas and updates
    char input[256];            // buffer to take inputs with 255 character 
    int output_enabled = 1;

    CommandType cmd;
    Formula f;

    double last_time = 0.0;
    char status[64] = "ok";

    while (1) {                 // Display of sheet

        if (output_enabled)
            display_window(sheet);

        printf("[%.1f] (%s) > ", last_time, status);        // required input taking visual

        if (!fgets(input, sizeof(input), stdin))            // read input line break if EOF(something like ctrl + D or W) or error
            break;

        input[strcspn(input, "\n")] = '\0';         // remove trailing newline from input

        clock_t start = clock();                // Initialised For calculating the time taken to run the command so it can be printed for next input 

        cmd = parse_command(input, &f);         // Parsing commands
        strcpy(status, "ok");

        if (cmd == CMD_QUIT) {
            break;
        }

        else if (cmd == CMD_SCROLL) {
            handle_scroll(sheet, input[0]);
        }

        else if (cmd == CMD_HELPER) {

            if (strcmp(input, "disable_output") == 0) {
                output_enabled = 0;
            }

            else if (strcmp(input, "enable_output") == 0) {
                output_enabled = 1;
            }

            else if (strncmp(input, "scroll_to", 9) == 0) {
                char cell[32];

                if (sscanf(input, "scroll_to %s", cell) == 1) {
                    scroll_to_cell(sheet, cell);
                    if (output_enabled)
                        display_window(sheet);
                } 
                else {
                    strcpy(status, "Invalid scroll_to");
                }
            }
        }

        else if (cmd == CMD_FORMULA) {
            update_with_dependencies(sheet, f);
        }

        else {
            strcpy(status, "unrecognized cmd");
        }

        clock_t end = clock();
        last_time = (double)(end - start) / CLOCKS_PER_SEC;
    }
    
    free_graph(rows, cols);
    free_sheet(sheet);
    return 0;
}