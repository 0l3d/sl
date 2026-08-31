# SL

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

## Platform compatibility 
  
`sl.c` and `sl.h` (the main library sources) are written in Standard C.  
(No POSIX, WinAPI, or other OS specific APIs are used.)  
  
However, `stdlib.h` may change in the future, but I aim to implement it for as many operating systems as possible.  
  
## API 
All API functions here:
```c
char * sl_string_getter(char *word);
int sl_add_func(struct SL_Code *code, char* name, struct SL_Variable (*funcr)(struct SL_Code*, struct SL_L_Function));
struct SL_Variable sl_copy_variable(struct SL_Variable var);
struct SL_Variable sl_get_argument(struct SL_Code code, struct SL_L_Function func, int which_one);
int sl_add_var(struct SL_Code *code, struct SL_Variable var);
struct SL_Variable sl_get_var(struct SL_Code code, const char *name);
struct SL_Function sl_get_func(struct SL_Code code, const char *name);
int sl_init_sl_lexer(int malloc_size, char* file_name, char ***bufout, char *special_tokens);
struct SL_Code sl_init_sl_process(); 
struct SL_Variable sl_dostr_sl_process(struct SL_Code *code_s, char *code);
int sl_open_sl_process(struct SL_Code *code, char* file_name);
struct SL_Variable sl_init_sl_parser(struct SL_Code *code_s);
int sl_close_sl_process(struct SL_Code *code);
```
Check wiki or sl_example.c for example.  

## License

This project is licensed under the BSD3-Clause License.

# Author 
Created by **0l3d**
