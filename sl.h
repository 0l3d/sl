#ifndef SL_H
#define SL_H

#include <stddef.h>
#include <stdint.h>

#define INT32MAX 2147483647
#define INT32MIN (-2147483647)

#define SL_INIT 4096
#define DEFAULT_SPECIAL_TOKENS "()+-/*%^&|=<>,"
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
    int info;
    union {
        int vali;
        double valf;
        int valb;
        char valc;
        char *vals;
        intptr_t valh;
        void* valp;
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
    char* special_tokens;
    char **custom_keyword;
    char **custom_expr;
    char **custom_splitter;
    int custom_keyword_count;
    int custom_expr_count;
    int custom_expr_capacity;
    int custom_splitter_count;
    int custom_splitter_capacity;
    int custom_keyword_capacity;
    void (**custom_keywordr)(struct SL_Code *, int *current_token);
    int (**custom_splitterr)(struct SL_Code *, int *current_token);
    struct SL_Variable (**custom_exprr)(struct SL_Code *, int *current_token);
    int starting_token;
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

/* PARSING FUNCTIONS */ 
int sl_then_finder(char *tokens[], enum TokenTypes *types, int current_token,int max_tokens, int *then_pos);
void sl_clean_local_scope(struct SL_Code *code, int starting_var_index, int starting_func_index);
int sl_find_end(char **tokens, enum TokenTypes *types, int start, int max_tokens, int branch);
struct SL_Variable sl_expression_solver(struct SL_Code *code_s, char *expression[], enum TokenTypes *types, int *current_token, int max_tokens);
void sl_identifier_tokenizer(char **code, enum TokenTypes **types, struct SL_Variable **fixed_values, int token_count);
void sl_throw_an_error(struct SL_Code code, char **tokens, int current_token, int max_tokens, char *error_msg, char *expected_tip);
int sl_find_end_of_expr(struct SL_Code *code_s, char **code, enum TokenTypes *types, int starting, int max);
int sl_add_custom_expr(struct SL_Code *code, char* expr_start, struct SL_Variable (*custom_exprr)(struct SL_Code *, int *current_token));
int sl_remove_custom_expr(struct SL_Code *code, int index);
int sl_remove_custom_keyword(struct SL_Code *code, int index);
int sl_remove_custom_splitter(struct SL_Code *code, int index);
int sl_add_custom_keyword(struct SL_Code *code, char* keyword_name, void (*custom_keywordr)(struct SL_Code *, int *current_token)); 
int sl_add_custom_splitter(struct SL_Code *code, char* splitter_name, int (*custom_splitterr)(struct SL_Code *, int *current_token)); 
int sl_init_sl_lexer(size_t malloc_size, const char *restrict file_name, char ***bufout, char *special_tokens);
struct SL_Variable sl_dostr_sl_process(struct SL_Code *code_s, char *code);
int sl_raw_lexer(char *bufin, char ***bufout, size_t max_count, char *special_tokens, int start_size);
struct SL_Variable sl_init_sl_parser(struct SL_Code *code_s);
/* PARSING FUNCTIONS */

/* HIGH LEVEL IMPLEMENTATION FUNCS */ 
struct SL_Code sl_init_sl_process();
int sl_open_sl_process(struct SL_Code *code, const char *restrict file_name);
int sl_close_sl_process(struct SL_Code *code);
/* HIGH LEVEL IMPLEMENTATION FUNCS */

/* EXTRAS FOR ANYTHING */ 
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
unsigned long sl_hash_string(const char *str);
int sl_add_raw_func(struct SL_Code *code, struct SL_Function *function);
int sl_add_func(struct SL_Code *code, char *name, struct SL_Variable (*funcr)(struct SL_Code *, struct SL_L_Function, struct SL_Function));
struct SL_Variable sl_copy_variable(struct SL_Variable var);
struct SL_Function sl_copy_function(struct SL_Function function);
struct SL_Variable sl_get_argument(struct SL_Code code, struct SL_L_Function func, int which_one);
int sl_add_var(struct SL_Code *code, struct SL_Variable var);
struct SL_Variable *sl_get_var(struct SL_Code *code, const char *name);
struct SL_Function *sl_get_func(struct SL_Code *code, const char *name);
/* EXTRAS FOR ANYTHING */

#endif
