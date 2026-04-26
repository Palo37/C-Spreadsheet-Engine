#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "spreadsheet.h"

int is_number(const char *s) {
    for (int i = 0; s[i]; i++) {            // convert the number to string and loop over the number 
        if (!isdigit(s[i])) return 0;
    }
    return 1;
}

int is_cell(const char *s) {
    int i = 0;
    if (!isalpha(s[i])) return 0;         // Checking whether the first term used is an alphabet or not as an invalid cell example would be 1A 

    while (isalpha(s[i])) i++;
    if (!isdigit(s[i])) return 0;

    while (isdigit(s[i])) i++;
    return s[i] == '\0';
}

int is_valid_function(const char *name) {
    return strcmp(name, "MIN") == 0 ||
           strcmp(name, "MAX") == 0 ||
           strcmp(name, "AVG") == 0 ||
           strcmp(name, "SUM") == 0 ||
           strcmp(name, "STDEV") == 0 ||
           strcmp(name, "SLEEP") == 0;
}

void parse_cell(const char *s, int *row, int *col) {            // Converting the cell value into its row and column value
    int i = 0;  
    *col = 0;
    while (isalpha(s[i])) {
        *col = (*col) * 26 + (toupper(s[i]) - 'A' + 1);         // Converting to uppercase so even lowercase if typed unintentionally for cell it should work, we are calculating column value by converting each alphabets to its ASCII value and subtracting A from it.
        i++;
    }

    *row = atoi(&s[i]);     // converting string to integer
    *col -= 1;  // zero indexing
    *row -= 1;
}

int parse_range(const char *range, int *r1, int *c1, int *r2, int *c2) {
    char left[16], right[16];

    if (sscanf(range, "%[^:]:%s", left, right) != 2)        // Searching till ':' and then splitting
        return 0;

    if (!is_cell(left) || !is_cell(right))
        return 0;

    parse_cell(left, r1, c1);
    parse_cell(right, r2, c2);

    if (*r1 > *r2 || *c1 > *c2)                             // Checking whether LHS is Smaller than RHS
        return 0;

    return 1;
}

Value parse_value(const char *token) {
    Value v;

    if (is_number(token)) {
        v.type = VAL_CONST;
        v.value = atoi(token);              // converting string to int for further processing
    } else if (is_cell(token)) {
        v.type = VAL_CELL;
        parse_cell(token, &v.row, &v.col);
    } else {
        v.type = VAL_CONST;
        v.value = 0; // fallback
    }

    return v;
}

Expression parse_expression(const char *expr_str) {
    Expression expr;        // storing final result

    if (strchr(expr_str, '(')) {         // to check whether a function is used such SUM'(', AVG'(', etc checks for first occurence to classify as a funciton   
        expr.type = EXPR_FUNC;

        sscanf(expr_str, "%[^ (](%[^)])", expr.func.name, expr.func.range);     // Start reading when it sees '(' and reads til instance of ')'
        if (!is_valid_function(expr.func.name)) {
            expr.type = EXPR_VALUE;
            expr.val.value = 0;
            return expr;
        }

        if (strcmp(expr.func.name, "SLEEP") == 0) {
            expr.func.is_range = 0;
            expr.func.single_val = parse_value(expr.func.range);
        } else {
            expr.func.is_range = 1;

            int r1, c1, r2, c2;
            if (!parse_range(expr.func.range, &r1, &c1, &r2, &c2)) {
                expr.type = EXPR_VALUE;
                expr.val.value = 0;
                return expr;
            }
        }
        return expr; 
    }

    char ops[] = "+-*/";
    for (int i = 0; i < 4; i++) {
        char *pos = strchr(expr_str, ops[i]);       // check if given operation is arithmetic
        if (pos) {
            expr.type = EXPR_ARITH;

            char left[32], right[32];

            strncpy(left, expr_str, pos - expr_str);    // right now POS points to A1 for A1+B1 so subtracting expr_str we get A1 
            left[pos - expr_str] = '\0';
            strcpy(right, pos + 1);             // To obtain Right side of operand

            expr.arith.op = ops[i];
            expr.arith.left = parse_value(left);
            expr.arith.right = parse_value(right);

            return expr;
        }
    }

    expr.type = EXPR_VALUE;
    expr.val = parse_value(expr_str);

    return expr;
}

int parse_formula(const char *input, Formula *f) {      // Helps split formulas as LHS and RHS as in A1=B1+5 seprates left A1 and right B1+5
    char lhs[32], rhs[128];         // buffer

    if (sscanf(input, "%[^=]=%s", lhs, rhs) != 2)       // splitting based on =, if it is not equal to 2 splitting failed
        return 0;

    if (!is_cell(lhs))                                  // left must be a cell 
        return 0;

    parse_cell(lhs, &f->target_row, &f->target_col);    // Obtaining the target cell
    f->expr = parse_expression(rhs);                    // evaluating the expression on rhs

    return 1;
}

CommandType parse_command(const char *input, Formula *f) {

    char clean[256];                              // removing blank space                          
    int j = 0;
    for (int i = 0; input[i]; i++) {              // looping till input[i] whis is '\0' as that is where each string ends         
        if (input[i] != ' ')
            clean[j++] = input[i];               // copying non space characters
    }   
    clean[j] = '\0';                             // adding a null terminator

    if (strcmp(clean, "q") == 0)
        return CMD_QUIT;

    if (strlen(clean) == 1 &&
        (clean[0] == 'w' || clean[0] == 'a' || clean[0] == 's' || clean[0] == 'd'))
        return CMD_SCROLL;

    if (strncmp(clean, "scroll_to", 9) == 0) {
        char cell[32];
        if (sscanf(clean, "scroll_to%s", cell) == 1 && is_cell(cell)) {
            return CMD_HELPER;
        }
        return CMD_INVALID;
    }

    if (strcmp(clean, "disable_output") == 0 || strcmp(clean, "enable_output") == 0)
        return CMD_HELPER;

    if (strchr(clean, '=')) {                   // checking for formula
        if (parse_formula(clean, f))    
            return CMD_FORMULA;
        else
            return CMD_INVALID;
    }

    return CMD_INVALID;
}