#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "spreadsheet.h"


int **dep_count;          // number of dependent cells for each cell

int ***dep_r;             // stores dependent row indices
int ***dep_c;             // stores dependent column indices

Formula **stored_formula;
int **has_formula;

int **visited;
int **stack;

void init_graph(int rows, int cols) {           // Intialising dependencies 

    dep_count = (int**)malloc(rows * sizeof(int*));     // allocate row pointers for dependency count matrix
    dep_r = (int***)malloc(rows * sizeof(int**));       // allocate row pointers for dependent row indices (3D)
    dep_c = (int***)malloc(rows * sizeof(int**));       // allocate row pointers for dependent column indices (3D)
    stored_formula = (Formula**)malloc(rows * sizeof(Formula*));       // allocate row pointers for storing formulas
    has_formula = (int**)malloc(rows * sizeof(int*));       // allocate row pointers to mark cells with formulas
    visited = (int**)malloc(rows * sizeof(int*));       // allocate row pointers for visited array (cycle detection)
    stack = (int**)malloc(rows * sizeof(int*));     // allocate row pointers for recursion stack (cycle detection)

    for (int i = 0; i < rows; i++) {

        dep_count[i] = (int*)calloc(cols, sizeof(int));     
        dep_r[i] = (int**)malloc(cols * sizeof(int*));      
        dep_c[i] = (int**)malloc(cols * sizeof(int*));      
        stored_formula[i] = (Formula*)malloc(cols * sizeof(Formula));
        has_formula[i] = (int*)calloc(cols, sizeof(int));   
        visited[i] = (int*)calloc(cols, sizeof(int));       
        stack[i] = (int*)calloc(cols, sizeof(int));         

        for (int j = 0; j < cols; j++) {
            dep_r[i][j] = NULL;      // no dependency initially
            dep_c[i][j] = NULL;      
        }
    }
}

void clear_dependencies(int r, int c, int rows, int cols) {
    for (int i = 0; i < rows; i++) { 
        for (int j = 0; j < cols; j++) {

            int k = 0;                          // index to iterate dependency list

            while (k < dep_count[i][j]) {       // traverse all dependents of cell (i,j)

                if (dep_r[i][j][k] == r && dep_c[i][j][k] == c) {   // if current dependency matches target cell

                    for (int x = k; x < dep_count[i][j] - 1; x++) {
                        dep_r[i][j][x] = dep_r[i][j][x + 1];   // shift remaining elements left
                        dep_c[i][j][x] = dep_c[i][j][x + 1];
                    }

                    dep_count[i][j]--;         // reduce dependency count after removal
                } else {
                    k++;                       // move to next dependency
                }
            }
        }
    }   
}

void add_dependency(int sr, int sc, int dr, int dc) {

    int k = dep_count[sr][sc];     // current number of dependents for source cell

    dep_r[sr][sc] = (int*)realloc(dep_r[sr][sc], (k + 1) * sizeof(int)); // expand row index list
    dep_c[sr][sc] = (int*)realloc(dep_c[sr][sc], (k + 1) * sizeof(int)); // expand column index list

    dep_r[sr][sc][k] = dr;        // store dependent cell row
    dep_c[sr][sc][k] = dc;        // store dependent cell column

    dep_count[sr][sc]++;          // increment dependency count
}

void extract_dependencies(Expression expr, int tr, int tc) {

    if (expr.type == EXPR_VALUE) {                  // to check whether dependency is with a cell directly

        if (expr.val.type == VAL_CELL) {
            add_dependency(expr.val.row, expr.val.col, tr, tc);     
        }
    }

    else if (expr.type == EXPR_ARITH) {             // to check whether dependency is an arithmetic

        if (expr.arith.left.type == VAL_CELL) {
            add_dependency(expr.arith.left.row, expr.arith.left.col, tr, tc);
        }

        if (expr.arith.right.type == VAL_CELL) {
            add_dependency(expr.arith.right.row, expr.arith.right.col, tr, tc);
        }
    }

    else if (expr.type == EXPR_FUNC) {          // to check whether dependency is functional

        if (!expr.func.is_range) {

            if (expr.func.single_val.type == VAL_CELL) {
                add_dependency(expr.func.single_val.row,
                               expr.func.single_val.col,
                               tr, tc);
            }
        }

        else {
            int r1, c1, r2, c2;

            if (parse_range(expr.func.range, &r1, &c1, &r2, &c2)) {

                for (int i = r1; i <= r2; i++) {
                    for (int j = c1; j <= c2; j++) {

                        if (i == tr && j == tc)
                            continue;

                        add_dependency(i, j, tr, tc);
                    }
                }
            }
        }
    }
}

int dfs_cycle(int r, int c) {

    if (stack[r][c]) return 1;     // cycle found
    if (visited[r][c]) return 0;   // skip if already processed

    visited[r][c] = 1;
    stack[r][c] = 1;

    for (int k = 0; k < dep_count[r][c]; k++) {
        /*check for next dependent cell */
        int nr = dep_r[r][c][k];        
        int nc = dep_c[r][c][k];

        if (dfs_cycle(nr, nc))      // recursively checing for cycle
            return 1;
    }

    stack[r][c] = 0;            // remove from stack after processing
    return 0;
}

int detect_cycle(Spreadsheet *sheet) {          // checking entire sheet for cycles

    for (int i = 0; i < sheet->rows; i++) {
        memset(visited[i], 0, sheet->cols * sizeof(int));  // reset visited array
        memset(stack[i], 0, sheet->cols * sizeof(int));     // reset recursion stack
    }

    for (int i = 0; i < sheet->rows; i++) {
        for (int j = 0; j < sheet->cols; j++) {

            if (dfs_cycle(i, j))
                return 1;
        }
    }

    return 0;
}

void propagate(Spreadsheet *sheet, int r, int c) {

    for (int k = 0; k < dep_count[r][c]; k++) {
        /*Dependent cell*/
        int nr = dep_r[r][c][k];        
        int nc = dep_c[r][c][k];

        if (has_formula[nr][nc]) {                  // check if update is needed

            evaluate_formula(sheet, stored_formula[nr][nc]);
            propagate(sheet, nr, nc);
        }
    }
}

void update_with_dependencies(Spreadsheet *sheet, Formula f) {

    if (f.target_row < 0 || f.target_row >= sheet->rows ||
        f.target_col < 0 || f.target_col >= sheet->cols) {

        printf("Error: Invalid cell\n");
        return;
    }

    int r = f.target_row;
    int c = f.target_col;

    clear_dependencies(r, c, sheet->rows, sheet->cols);   // remove old links

    stored_formula[r][c] = f;
    has_formula[r][c] = 1;

    extract_dependencies(f.expr, r, c);   // build dependency graph

    if (detect_cycle(sheet)) {
        printf("Error: Circular dependency\n");
        has_formula[r][c] = 0;
        return;
    }

    evaluate_formula(sheet, f);   // evaluate current cell

    propagate(sheet, r, c);       // update dependent cells
}

void free_graph(int rows, int cols) {
    for (int i = 0; i < rows; i++) {

        for (int j = 0; j < cols; j++) {
            free(dep_r[i][j]);   // free each dependency list
            free(dep_c[i][j]);
        }

        free(dep_r[i]);
        free(dep_c[i]);
        free(dep_count[i]);
        free(stored_formula[i]);
        free(has_formula[i]);
        free(visited[i]);
        free(stack[i]);
    }

    free(dep_r);
    free(dep_c);
    free(dep_count);
    free(stored_formula);
    free(has_formula);
    free(visited);
    free(stack);
}

