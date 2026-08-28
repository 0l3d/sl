#ifndef SL_H
#define SL_H

#define SL_INIT 4096
#define GENERAL_MALLOC_SIZE 131072
#define MAX_CODE_SIZE 1048576
#define SPECIAL_TOKENS "()+-/*%^&|=<>,"
#define OPERATORS "*/+-%><&|^"


enum SL_Types
{
    INIT = 0,
    INTEGER = 1,
    DOUBLE = 2,
    CHAR = 3,
    STRING = 4,
    BOOLEAN = 5,
    RETURN = 6,
    LONG = 7,
    ERROR = 8,
    POINTER = 9,
};

struct SL_Variable
{
    char *name;
    enum SL_Types type;
    union
    {
        int vali;
        double valf;
        int valb;
        char valc;
        char *vals;
        long valh;
    };
};

struct SL_Code
{
    char **code;
    int token_count;
    struct SL_Variable *vars;
    int total_size_v;
    int total_vars;
    struct SL_Function *funcs;
    int total_size_f;
    int total_funcs;
};

struct SL_L_Function {
    int total_arguments;
    int *argument_indexes;
    int starting_index;
};

struct SL_Function
{
    char *name;
    struct SL_Variable *arguments;
    int total_arguments;
    char** code_tokens;
    int code_len;
    int vaargs;
    struct SL_Variable (*funcr)(struct SL_Code*, struct SL_L_Function);
    int linked_function;
};

char * sl_string_getter(char *word);
int sl_add_func(struct SL_Code *code, char* name, struct SL_Variable (*funcr)(struct SL_Code*, struct SL_L_Function));
struct SL_Variable sl_copy_variable(struct SL_Variable var);
struct SL_Variable sl_get_argument(struct SL_Code code, struct SL_L_Function func, int which_one);
int sl_add_var(struct SL_Code *code, struct SL_Variable var);
struct SL_Variable sl_get_var(struct SL_Code code, const char *name);
struct SL_Function sl_get_func(struct SL_Code code, const char *name);
int sl_init_sl_lexer(int malloc_size, char* file_name, char ***bufout, char *special_tokens);
struct SL_Code sl_init_sl_process(); 
int sl_dostr_sl_process(struct SL_Code *code_s, char *code);
int sl_open_sl_process(struct SL_Code *code, char* file_name);
struct SL_Variable sl_init_sl_parser(struct SL_Code *code_s);
int sl_close_sl_process(struct SL_Code *code);
// int use_custom_sl_parser();

#endif
