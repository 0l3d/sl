# $L

SL is a simple scripting language for C.

Wiki: [SL-BASICS](https://github.com/0l3d/sl/wiki)

## Features
- `if` support.
- `while` loop support.
- `def` function support.
- `var` dynamic variables.
- All math expressions except assignments. Check wiki if you wanna know why.

## Usage 

```bash
git clone https://github.com/0l3d/sl
cd sl/
make

# Usage: 
# ./sl (without arguments reads code.sl automatically.)
# ./sl <filename>
```
  
You can edit SL files with basic syntax highlighting without using an external editor:  
```
./sl examples/editor.sl examples/brainfuck.sl # or your path 
# Cross Platform editor for SL.
```
  
## Platform compatibility 
  
`sl.c` and `sl.h` (the main library sources) are written in Standard C.  
(No POSIX, WinAPI, or other OS specific APIs are used.)  
  
However, `stdlib.h` may change in the future, but I aim to implement it for as many operating systems as possible.  
  
The Console API and Network API are supported on both posix and windows.

### Dynamic Loading (standard library dyn api)
  
Huge thanks to everyone who worked on dyncall and made such a great library.  
  
SL's dynamic loading support follows the same platforms supported by dyncall. I chose dyncall because libffi was too heavy for this project and would have been harder to embed into the repository, while dyncall is minimal and has very little impact on the binary size. Thanks to dyncall, you can load raylib functions and make games, use OpenGL, call system libraries, and pretty much anything else you can think of. Keep in mind that dynamic loading is unsafe and requires you to deal with C's memory management. you'll need to use the free functions provided by the library you're working with.  
  
## API 
All API functions here:
```c
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
```
Check wiki for API REF.  

## License

This project is licensed under the BSD3-Clause License.

# Author 
Created by **0l3d**
