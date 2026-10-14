#ifndef SL_H
#define SL_H

#include <stddef.h>
#include <stdint.h>

#define INT32MAX 2147483647
#define INT32MIN (-2147483647)

#define SL_INIT 4096
#define DEFAULT_SPECIAL_TOKENS "()+-/*%^&|=<>,"
#define OPERATORS "*/+-%><&|^"

#define SL_REALLOC_DEFAULT 1024
#define SL_BUILTIN_PRECEDENCE_SCALE 10

#if defined(__GNUC__) && !defined(_WIN32)
#pragma GCC visibility push(default)
#endif

enum SL_Types {
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

struct SL_Variable {
  char *name;
  unsigned long hash;
  int cache_index;
  enum SL_Types type;
  int scope_lifetime;
  size_t length;
  int info;
  union {
    long vali;
    double valf;
    long valb;
    char valc;
    char *vals;
    intmax_t valh;
    void *valp;
  };
};

/* VIRTUAL MACHINE */
enum OpcodeTypes {
  OPCODE_UNKNOWN = 0x00,
  /* OLD USAGE OPCODE_PUSH_CONST = 0x01,  */
  OPCODE_LOAD_VAR = 0x02,
  OPCODE_STORE_VAR = 0x03,
  OPCODE_IF_JUMP_FALSE = 0x13,
  OPCODE_ADD = 0x15,
  OPCODE_SUB = 0x16,
  OPCODE_MUL = 0x17,
  OPCODE_DIV = 0x18,
  OPCODE_MOD = 0x19,
  OPCODE_BAND = 0x1A,
  OPCODE_BOR = 0x1B,
  OPCODE_XOR = 0x1C,
  OPCODE_BIGGER = 0x1D,
  OPCODE_LESSER = 0x1E,
  OPCODE_EQU = 0x1F,
  OPCODE_NEQ = 0x20,
  OPCODE_EQG = 0x21,
  OPCODE_EQL = 0x22,
  OPCODE_SHR = 0x23,
  OPCODE_SHL = 0x24,
  OPCODE_LAND = 0x25,
  OPCODE_LOR = 0x26,
  OPCODE_FUNC_CALL = 0x28,
  OPCODE_CALL_LINKED_FUNC = 0x29,
  OPCODE_UNINITIALIZED = 0x2A,
  OPCODE_JMP = 0x2B,
  /* 64 BIT INDEXING (not yet)
  OPCODE_IF_JUMP_FALSE_BIG   					= 0x13,
  OPCODE_JMP_BIG             					= 0x2B,
OPCODE_PUSH_BIG_CONST      					= 0x2C,
OPCODE_STORE_BIG_VAR       					= 0x2D,
OPCODE_LOAD_BIG_VAR        					= 0x2E,
OPCODE_BIG_FUNC_CALL       					= 0x2F,
OPCODE_BIG_LINKED_FUNC_CALL					= 0x30,
     64 BIT INDEXING (not yet) */
  OPCODE_RETURN = 0x31,
  OPCODE_LOAD_LOCAL_VAR = 0x32,
  OPCODE_STORE_LOCAL_VAR = 0x33,
  OPCODE_CALL_DYNAMIC_LINKED_FUNC = 0x34,
  OPCODE_LOAD_DYNAMIC_ADDED_VAR = 0x35,
  OPCODE_STORE_DYNAMIC_ADDED_VAR = 0x36,
  OPCODE_PUSH_EMPTY = 0x37,
  OPCODE_CALL_ADDRESS = 0x38,
  OPCODE_PUSH_POOL_CONST = 0x39,
  OPCODE_SET_ASSIGNMENT = 0x3A,
  OPCODE_INC_VAR = 0x3B,
  OPCODE_DEC_VAR = 0x3C
};

struct SL_VM_Frame {
  struct SL_Variable *vars;
  int total_vars;
  int total_vars_capacity;
};

struct SL_CallFrame {
  size_t frame_base;
  size_t saved_total_vars;
};

struct SL_VM_Objects {
  struct SL_Variable *stack;
  size_t stack_size;
  size_t stack_capacity;
  size_t *call_stack;
  size_t call_stack_size;
  size_t call_stack_capacity;

  struct SL_CallFrame *call_frames;
  size_t call_frame_size;
  size_t call_frame_capacity;
};

struct SL_Code {
  uint32_t *bytecode;
  size_t bytecode_len;
  size_t bytecode_capacity;
  struct SL_VM_Objects obj;
  struct SL_VM_Frame frame;
  size_t stack_pointer;
  int stack_size;
  size_t current_frame_base;
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
  char *special_tokens;
  char **custom_keyword;
  char **custom_expr;
  char **custom_splitter;
  int custom_keyword_count;
  int custom_expr_count;
  int custom_expr_capacity;
  int custom_splitter_count;
  int custom_splitter_capacity;
  int custom_keyword_capacity;
  char **custom_token;
  int *custom_token_precedence;
  void (**custom_tokenr)(struct SL_Code *, int first_token, int operator_token,
                         int end_token);
  int custom_token_count;
  int custom_token_capacity;
  void (**custom_keywordr)(struct SL_Code *, int *current_token);
  int (**custom_splitterr)(struct SL_Code *, int *current_token);
  struct SL_Variable (**custom_exprr)(struct SL_Code *, int *current_token);
  int starting_token;
  struct SL_Variable *constant_pool;
  int constant_pool_len;
  int constant_pool_capacity;
  int disassemble;
};

/* Virtual Machine */

enum TokenTypes {
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

struct SL_Variable pop(struct SL_Code *code);
void sl_run_vm(struct SL_Code *code);
void sl_disassemble(struct SL_Code *code);

struct SL_L_Function {
  int total_arguments;
  int *argument_indexes;
  int starting_index;
};

struct SL_Function {
  char *name;
  unsigned long hash;
  int entry_pc;
  struct SL_Variable *arguments;
  int total_arguments;
  int info;
  int vaargs;
  struct SL_Variable (*funcr)(struct SL_Code *, struct SL_L_Function,
                              struct SL_Function);
  int linked_function;
  int scope_lifetime;
  int frame_size;
};

/* SL EXECUTABLE FILE FORMAT */
struct SL_X_Header {
  uint64_t magic;
  uint64_t bytecode_size;
  uint64_t file_size;
  uint64_t total_vars;
  uint64_t total_funcs;
  uint64_t total_fixeds;
  uint64_t total_constants;
  uint64_t *var_sizes;
  uint64_t *func_sizes;
  uint64_t *fixed_sizes;
  uint64_t *constant_sizes;
  uint64_t header_size;
};

struct SL_X_File {
  struct SL_Variable *vars;
  int total_vars;
  struct SL_Function *funcs;
  int total_funcs;
  struct SL_Variable *fixeds;
  int total_fixeds;
  struct SL_Variable *constants;
  int total_constants;
};
/* SL EXECUTABLE FILE FORMAT */

/* COMPILER TOOLKIT */
struct SL_FrameScope {
  struct SL_Variable *saved_outer_vars;
  int saved_outer_count;
};

size_t sl_current_pc(struct SL_Code *code);
void sl_emit(struct SL_Code *code, uint32_t value);
size_t sl_emit_jump(struct SL_Code *code, uint32_t jump_opcode);
void sl_patch_jump_to(struct SL_Code *code, size_t patch_position,
                      size_t target_pc);
void sl_patch_jump_here(struct SL_Code *code, size_t patch_position);
int sl_add_constant(struct SL_Code *code, struct SL_Variable constant);
void sl_emit_push_constant(struct SL_Code *code, struct SL_Variable constant);
void sl_emit_push_string(struct SL_Code *code, const char *text);
void sl_compile_statements(struct SL_Code *code, int first_token,
                           int end_token);
void sl_compile_expression(struct SL_Code *code, int first_token,
                           int end_token);
void sl_begin_frame_scope(struct SL_Code *code, struct SL_FrameScope *scope);
int sl_end_frame_scope(struct SL_Code *code, struct SL_FrameScope *scope);
void sl_loop_begin(struct SL_Code *code);
uint32_t sl_loop_start_pc(void);
void sl_loop_end(struct SL_Code *code);
int sl_find_function_index(struct SL_Code *code, const char *name);
void sl_emit_call_function(struct SL_Code *code, int function_index,
                           int argument_count);
struct SL_Variable sl_call_function(struct SL_Code *code, int func_idx,
                                    struct SL_Variable *args, int arg_count);
/* COMPILER TOOLKIT */

/* PARSING FUNCTIONS */
int sl_then_finder(char *tokens[], enum TokenTypes *types, int current_token,
                   int max_tokens, int *then_pos);
void sl_clean_local_scope(struct SL_Code *code, int starting_var_index,
                          int starting_func_index);
int sl_find_end(char **tokens, enum TokenTypes *types, int start,
                int max_tokens, int branch);
void sl_expression_solver(struct SL_Code *code_s, char *expression[],
                          enum TokenTypes *types, int *current_token,
                          int max_tokens);
void sl_identifier_tokenizer(struct SL_Code *code_s, char **code,
                             enum TokenTypes **types,
                             struct SL_Variable **fixed_values,
                             int token_count);
void sl_throw_an_error(struct SL_Code code, char **tokens, int current_token,
                       int max_tokens, char *error_msg, char *expected_tip);
int sl_find_end_of_expr(struct SL_Code *code_s, char **code,
                        enum TokenTypes *types, int starting, int max);
int sl_add_custom_expr(struct SL_Code *code, char *expr_start,
                       struct SL_Variable (*custom_exprr)(struct SL_Code *,
                                                          int *current_token));
int sl_remove_custom_expr(struct SL_Code *code, int index);
int sl_remove_custom_keyword(struct SL_Code *code, int index);
int sl_remove_custom_splitter(struct SL_Code *code, int index);
int sl_add_custom_keyword(struct SL_Code *code, char *keyword_name,
                          void (*custom_keywordr)(struct SL_Code *,
                                                  int *current_token));
int sl_add_custom_splitter(struct SL_Code *code, char *splitter_name,
                           int (*custom_splitterr)(struct SL_Code *,
                                                   int *current_token));
int sl_add_custom_token(struct SL_Code *code, char *token_name, int precedence,
                        void (*custom_tokenr)(struct SL_Code *, int first_token,
                                              int operator_token,
                                              int end_token));
int sl_remove_custom_token(struct SL_Code *code, int index);
int sl_find_custom_token(struct SL_Code *code, const char *token_name);
int sl_init_sl_lexer(struct SL_Code *code, size_t malloc_size,
                     const char *restrict file_name, char ***bufout,
                     char *special_tokens);
struct SL_Variable sl_dostr_sl_process(struct SL_Code *code_s, char *code);
int sl_raw_lexer(char *bufin, char ***bufout, size_t max_count,
                 char *special_tokens, int start_size);
struct SL_Variable sl_init_sl_parser(struct SL_Code *code_s);
/* PARSING FUNCTIONS */

/* HIGH LEVEL IMPLEMENTATION FUNCS */
struct SL_Code sl_init_sl_process();
int sl_open_sl_process(struct SL_Code *code, const char *restrict file_name);
void sl_run_process(struct SL_Code *code, char *file_name);
int sl_close_sl_process(struct SL_Code *code);
int sl_save_slx(struct SL_Code *code, const char *file_name);
int sl_load_slx(struct SL_Code *code, const char *file_name);
/* HIGH LEVEL IMPLEMENTATION FUNCS */

/* EXTRAS FOR ANYTHING */
char *sl_string_getter(char *word);
char *sl_get_assignment_var();
char *sl_bytes_copy(const char *bytes, size_t length);
void sl_free_variable(struct SL_Variable *var);
void sl_free_function(struct SL_Function *func);
/* Safe Memory Allocation Functions */
void *smalloc(struct SL_Variable *error, size_t size);
void *scalloc(struct SL_Variable *error,size_t how_much, size_t size);
void *srealloc(struct SL_Variable *error, void *pptr, size_t size);
char *sstrdup(struct SL_Variable *error, const char *restrict str);
/* Safe Memory Allocation Functions */
char *sl_quote_string(const char *str);
int sl_get_scope(struct SL_Code *code);
unsigned long sl_hash_string(const char *str);
int sl_add_raw_func(struct SL_Code *code, struct SL_Function *function);
int sl_add_func(struct SL_Code *code, char *name,
                struct SL_Variable (*funcr)(struct SL_Code *,
                                            struct SL_L_Function,
                                            struct SL_Function));
struct SL_Variable sl_copy_variable(struct SL_Variable var);
struct SL_Function sl_copy_function(struct SL_Function function);
struct SL_Variable sl_get_argument_p(const struct SL_Code *code,
                                     struct SL_L_Function func, int which_one);
#define sl_get_argument(code, func, which_one)                                 \
  sl_get_argument_p(&(code), (func), (which_one))
int sl_add_var(struct SL_Code *code, struct SL_Variable var);
struct SL_Variable *sl_get_var(struct SL_Code *code, const char *name);
struct SL_Function *sl_get_func(struct SL_Code *code, const char *name);
/* EXTRAS FOR ANYTHING */

#if defined(__GNUC__) && !defined(_WIN32)
#pragma GCC visibility pop
#endif

#endif
