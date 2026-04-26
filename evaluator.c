#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "spreadsheet.h"

int get_cell_value(Spreadsheet *sheet, int r, int c, int *is_err) {
    if (r < 0 || r >= sheet->rows || c < 0 || c >= sheet->cols) {           //Checking whether the cell is valid
        *is_err = 1;
        return 0;
    }

    if (sheet->grid[r][c].is_err) {             // checking whether the content is valid or not
        *is_err = 1;
        return 0;
    }

    *is_err = 0;
    return sheet->grid[r][c].value;
}

int evaluate_value(Spreadsheet *sheet, Value v, int *is_err) {      // check whether the term is a value or a cell
    if (v.type == VAL_CONST) {
        *is_err = 0;
        return v.value;
    }

    if (v.type == VAL_CELL) {
        return get_cell_value(sheet, v.row, v.col, is_err);
    }

    *is_err = 1;
    return 0;
}

/*Defining arithmetic functions*/

int eval_add(int a, int b) { return a + b; }
int eval_sub(int a, int b) { return a - b; }
int eval_mul(int a, int b) { return a * b; }

int eval_div(int a, int b, int *is_err) {
    if (b == 0) {
        *is_err = 1;
        return 0;
    }
    return a / b;
}

int evaluate_arithmetic(Spreadsheet *sheet, Arithmetic arith, int *is_err) {
    int err1 = 0, err2 = 0;

    int left = evaluate_value(sheet, arith.left, &err1);            // left and right of arithmetic defined in parser that is utilized
    int right = evaluate_value(sheet, arith.right, &err2);          // we use &err1/2 to pass address so function can update error flag

    if (err1 || err2) {
        *is_err = 1;
        return 0;
    }

    switch (arith.op) {
        case '+': return eval_add(left, right);
        case '-': return eval_sub(left, right);
        case '*': return eval_mul(left, right);
        case '/': return eval_div(left, right, is_err);
    }

    *is_err = 1;
    return 0;
}

int iterate_range(Spreadsheet *sheet, int r1, int c1, int r2, int c2,           // Taking the whole function on rhs as input i.e SUM(A1:B3)
                  int (*func)(int, int), int initial, int *is_err) {

    int result = initial;

    for (int i = r1; i <= r2; i++) {                                  // Iterating over rows and columns to evaluate a 2D grid if neccessary
        for (int j = c1; j <= c2; j++) {

            int err = 0;
            int val = get_cell_value(sheet, i, j, &err);

            if (err) {
                *is_err = 1;
                return 0;
            }

            result = func(result, val);
        }
    }

    return result;
}

int add_func(int a, int b) { return a + b; }
int min_func(int a, int b) { return (a < b) ? a : b; }              // The ? operator as an if else where if condition is true we take 1st arg or 2nd
int max_func(int a, int b) { return (a > b) ? a : b; }

int eval_sum(Spreadsheet *sheet, int r1, int c1, int r2, int c2, int *is_err) {
    return iterate_range(sheet, r1, c1, r2, c2, add_func, 0, is_err);
}

int eval_min(Spreadsheet *sheet, int r1, int c1, int r2, int c2, int *is_err) {
    int err = 0;
    int init = get_cell_value(sheet, r1, c1, &err);

    if (err) {
        *is_err = 1;
        return 0;
    }

    return iterate_range(sheet, r1, c1, r2, c2, min_func, init, is_err);
}

int eval_max(Spreadsheet *sheet, int r1, int c1, int r2, int c2, int *is_err) {
    int err = 0;
    int init = get_cell_value(sheet, r1, c1, &err);

    if (err) {
        *is_err = 1;
        return 0;
    }

    return iterate_range(sheet, r1, c1, r2, c2, max_func, init, is_err);
}

int eval_avg(Spreadsheet *sheet, int r1, int c1, int r2, int c2, int *is_err) {
    int sum = 0, count = 0;

    for (int i = r1; i <= r2; i++) {
        for (int j = c1; j <= c2; j++) {

            int err = 0;
            int val = get_cell_value(sheet, i, j, &err);

            if (err) {
                *is_err = 1;
                return 0;
            }

            sum += val;
            count++;
        }
    }

    return (count == 0) ? 0 : (sum / count);
}

int eval_stdev(Spreadsheet *sheet, int r1, int c1, int r2, int c2, int *is_err) {
    int sum = 0, count = 0;

    for (int i = r1; i <= r2; i++) {                    // calculating mean
        for (int j = c1; j <= c2; j++) {

            int err = 0;
            int val = get_cell_value(sheet, i, j, &err);

            if (err) {
                *is_err = 1;
                return 0;
            }

            sum += val;
            count++;
        }
    }

    if (count == 0) return 0;

    double mean = (double)sum / count;
    double variance = 0;

    for (int i = r1; i <= r2; i++) {                            // calculating the difference from mean for each term and squaring it
        for (int j = c1; j <= c2; j++) {

            int err = 0;
            int val = get_cell_value(sheet, i, j, &err);

            if (err) {
                *is_err = 1;
                return 0;
            }

            variance += (val - mean) * (val - mean);
        }
    }

    variance /= count;          // variance = variance/count

    return (int)sqrt(variance);
}

int eval_sleep(Spreadsheet *sheet, Value v, int *is_err) {
    int val = evaluate_value(sheet, v, is_err);

    if (*is_err) return 0;

    sleep(val);   // val = seconds
    return val;
}

int evaluate_function(Spreadsheet *sheet, Function func, int *is_err) {             // Actual evaluation of functions

    if (strcmp(func.name, "SLEEP") == 0) {
        return eval_sleep(sheet, func.single_val, is_err);
    }

    int r1, c1, r2, c2;

    if (!parse_range(func.range, &r1, &c1, &r2, &c2)) {             // checking whether the range input is valid or not
        *is_err = 1;
        return 0;
    }

    if (strcmp(func.name, "SUM") == 0)
        return eval_sum(sheet, r1, c1, r2, c2, is_err);

    if (strcmp(func.name, "MIN") == 0)
        return eval_min(sheet, r1, c1, r2, c2, is_err);

    if (strcmp(func.name, "MAX") == 0)
        return eval_max(sheet, r1, c1, r2, c2, is_err);

    if (strcmp(func.name, "AVG") == 0)
        return eval_avg(sheet, r1, c1, r2, c2, is_err);

    if (strcmp(func.name, "STDEV") == 0)
        return eval_stdev(sheet, r1, c1, r2, c2, is_err);

    *is_err = 1;
    return 0;
}

int evaluate_expression(Spreadsheet *sheet, Expression expr, int *is_err) {         // Evaluation of expression

    if (expr.type == EXPR_VALUE)
        return evaluate_value(sheet, expr.val, is_err);

    if (expr.type == EXPR_ARITH)
        return evaluate_arithmetic(sheet, expr.arith, is_err);

    if (expr.type == EXPR_FUNC)
        return evaluate_function(sheet, expr.func, is_err);

    *is_err = 1;
    return 0;
}

void evaluate_formula(Spreadsheet *sheet, Formula f) {

    int is_err = 0;

    int result = evaluate_expression(sheet, f.expr, &is_err);

    if (is_err) {
        sheet->grid[f.target_row][f.target_col].is_err = 1;
        sheet->grid[f.target_row][f.target_col].value = 0;
    } else {
        sheet->grid[f.target_row][f.target_col].is_err = 0;
        sheet->grid[f.target_row][f.target_col].value = result;
    }
}