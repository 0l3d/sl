#ifndef SL_H
#define SL_H

#include <stdint.h>
#include <stddef.h>

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
    BYTES = 10,
};

struct SL_Variable
{
    char *name;
    unsigned long hash;
    int cache_index;
    enum SL_Types type;
    int scope_lifetime;
    size_t length;
    size_t info;
    union {
        int vali;
        double valf;
        int valb;
        char valc;
        char *vals;
        intptr_t valh;
    };
};

enum TokenTypes
{
    T_UNKNOWN = 0,
    T_IF,
    T_DEF,
    T_WHILE,
    T_THEN,
    T_END,
    T_VAR,
    T_ELSE,
    T_ELIF,
    T_IMPORT,
    T_BREAK,
    T_CONTINUE,
    T_RETURN,
    T_EQU,
    T_NEQ,
    T_EQG,
    T_EQL,
    T_AND,
    T_OR,
    T_SHLEFT,
    T_SHRIGHT,
};

struct SL_Code
{
    char **code;
    enum TokenTypes *types;
    struct SL_Variable *fixed_values;
    int token_count;
    struct SL_Variable *vars;
    int total_size_v;
    int total_vars;
    struct SL_Function *funcs;
    int total_size_f;
    int total_funcs;
    int scope_depth;
    int types_set;
};

struct SL_L_Function
{
    int total_arguments;
    int *argument_indexes;
    int starting_index;
};

struct SL_Function
{
    char *name;
    unsigned long hash;
    struct SL_Variable *arguments;
    int total_arguments;
    char **code_tokens;
    enum TokenTypes *types;
    struct SL_Variable *fixed_values;
    int info;
    int code_len;
    int vaargs;
    struct SL_Variable (*funcr)(struct SL_Code *, struct SL_L_Function, struct SL_Function);
    int linked_function;
    int scope_lifetime;
};

char *sl_string_getter(char *word);
char *sl_get_assignment_var();
char *sl_bytes_copy(const char *bytes, size_t length);
void sl_free_variable(struct SL_Variable *var);
void sl_free_function(struct SL_Function *func);
/* Safe Memory Allocation Functions */
void *smalloc(size_t size);
void *scalloc(size_t how_much, size_t size);
void *srealloc(void *pptr, size_t size);
/* Safe Memory Allocation Functions */
char *sl_quote_string(const char *str);
int sl_get_scope(struct SL_Code *code);
int sl_raw_lexer(char *bufin, char ***bufout, size_t max_count, char *special_tokens, int start_size);
unsigned long sl_hash_string(const char *str);
int sl_add_raw_func(struct SL_Code *code, struct SL_Function *function);
int sl_add_func(struct SL_Code *code, char *name,
                struct SL_Variable (*funcr)(struct SL_Code *, struct SL_L_Function, struct SL_Function));
struct SL_Variable sl_copy_variable(struct SL_Variable var);
struct SL_Function sl_copy_function(struct SL_Function function);
struct SL_Variable sl_get_argument(struct SL_Code code, struct SL_L_Function func, int which_one);
int sl_add_var(struct SL_Code *code, struct SL_Variable var);
struct SL_Variable *sl_get_var(struct SL_Code *code, const char *name);
struct SL_Function *sl_get_func(struct SL_Code *code, const char *name);
int sl_init_sl_lexer(size_t malloc_size, char *file_name, char ***bufout, char *special_tokens);
struct SL_Code sl_init_sl_process();
struct SL_Variable sl_dostr_sl_process(struct SL_Code *code_s, char *code);
int sl_open_sl_process(struct SL_Code *code, char *file_name);
struct SL_Variable sl_init_sl_parser(struct SL_Code *code_s);
int sl_close_sl_process(struct SL_Code *code);
// int use_custom_sl_parser();

#endif
