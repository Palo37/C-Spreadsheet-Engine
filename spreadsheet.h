
#ifndef SPREADSHEET_H
#define SPREADSHEET_H

typedef enum {           // To identify what kind of task is to be done
    CMD_INVALID,
    CMD_QUIT,
    CMD_SCROLL,
    CMD_HELPER,
    CMD_FORMULA
} CommandType;

typedef enum {            // To identify if the value is constant or is a cell
    VAL_CONST,
    VAL_CELL
} ValueType;

typedef enum {             // To check whether the given expression is whether a single value, arithmetic or a function such as sum,avg,etc
    EXPR_VALUE,
    EXPR_ARITH,
    EXPR_FUNC
} ExprType;

typedef struct {            // to check whether the type of value is whether a constanct and hence value or if it is a cell and hence row and column 
    ValueType type;
    int value;         
    int row, col;      
} Value;

typedef struct {            // for defining operations and segregating left and right value
    char op;           
    Value left;
    Value right;
} Arithmetic;

typedef struct {            
    char name[10];           // for char name of length 10 it takes care of stdev and other function names and 10 is to have a safety net
    char range[32];          // for range as in when range is used as SUM(ZZZ999:ZZZ999) so theoretically 13 is enough but we have safety net to handle error incase worst input is kept
    int is_range;            // to check whether argument is a range or a single value (SLEEP)
    Value single_val;        // used when function takes single value like SLEEP(2) or SLEEP(A1)         
} Function;
    

typedef struct {
    ExprType type;            // To check the expression type whether it is value, arithmetic, function this encapsulates the above data structures that have been created
    union {                   // Using union as only one of them will be applicable at a time 
        Value val;
        Arithmetic arith;
        Function func;
    };
} Expression;

typedef struct {
    int target_row;         // This struct defines where to store the value and what expression to use for the same
    int target_col;
    Expression expr;
} Formula;

typedef struct {            // To represent a single cell its value and err if division by 0 occurs(etc)
    int value;
    int is_err;
} Cell;

typedef struct {            // represents entire spreadsheet total number of rows/colums and starting indicies for them for current visible sheet section
    int rows, cols;
    int start_row, start_col;
    Cell **grid;
} Spreadsheet;

/*Declaring functions so that the different .c files can interact with each other*/ 

/* parser */
CommandType parse_command(const char *input, Formula *f);
int parse_formula(const char *input, Formula *f);
int parse_range(const char *range, int *r1, int *c1, int *r2, int *c2);

/* interface */
void print_col_label(int n);
void display_window(Spreadsheet* sheet);
void parse_cell(const char *s, int *row, int *col);

/* evaluator */
void evaluate_formula(Spreadsheet *sheet, Formula f);

/* graph */
void init_graph(int rows, int cols);
void free_graph(int rows, int cols);
void clear_dependencies(int r, int c, int rows, int cols);
void extract_dependencies(Expression expr, int tr, int tc);
int detect_cycle(Spreadsheet *sheet);
void propagate(Spreadsheet *sheet, int r, int c);
void update_with_dependencies(Spreadsheet *sheet, Formula f);

#endif



