#include "sl.h"
#include "stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SPECIAL_TOKENS "()+-/*%^&|=<>,{}"

struct SL_Variable builtin_not_fn(struct SL_Code *code,
                                  struct SL_L_Function func,
                                  struct SL_Function rfunc) {

  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

  switch (first_arg.type) {
  case BOOLEAN:
    return_var.valb = !first_arg.valb;
    return_var.type = BOOLEAN;
    break;
  case INTEGER:
    return_var.valb = ~first_arg.vali;
    return_var.type = INTEGER;
    break;
  case LONG:
    return_var.valh = ~first_arg.valh;
    return_var.type = LONG;
    break;
  default:
    return_var.type = ERROR;
    return_var.vals = "Unexpected not() usage!";
    return return_var;
  }

  return return_var;
}

int find_end_brace(char **tokens, int start, int max_tokens) {
  int depth = 0;

  for (int i = start; i < max_tokens; i++) {
    if (tokens[i] == NULL)
      continue;

    if (tokens[i][0] == '{') {
      depth++;
    } else if (tokens[i][0] == '}') {
      if (depth == 0)
        return i;

      depth--;
    }
  }

  return -1;
}
int then_end_splitter(struct SL_Code *code, int *current_token) {

  int tokens =
      find_end_brace(code->code, (*current_token) + 1, code->token_count);
  return tokens;
}

struct SL_Variable then_end_expression(struct SL_Code *code,
                                       int *current_token) {

  struct SL_Variable return_val = {0};
  int old_tokens = code->token_count;
  int tokens =
      find_end_brace(code->code, (*current_token) + 1, code->token_count);
  (*current_token)++;
  struct SL_Code code_def = *code;
  code_def.starting_token = *current_token;
  code_def.token_count = tokens;
  int start_var_index = code->total_vars;
  int start_func_index = code->total_funcs;
  return_val = sl_init_sl_parser(&code_def);
  code->vars = code_def.vars;
  code->total_size_v = code_def.total_size_v;
  code->total_vars = code_def.total_vars;
  code->funcs = code_def.funcs;
  code->total_size_f = code_def.total_size_f;
  code->total_funcs = code_def.total_funcs;
  code_def.fixed_values = code->fixed_values;
  code->token_count = old_tokens;
  sl_clean_local_scope(code, start_var_index, start_func_index);
  (*current_token)--;
  return return_val;
}

int main(int argc, char **argv) {
  char *code = NULL;
  int console = 0;

  if (argc > 1) {
    if (strcmp(argv[1], "-c") == 0) {
      console = 1;
      code = strdup("./code.sl");
    } else {
      code = strdup(argv[1]);
      if (code == NULL) {
        fprintf(
            stderr,
            "main(): strdup() failed to allocate memory and returned NULL\n");
        return -1;
      }
    }
  } else {
    /* falls back to this file if no file is specified in the command */
    code = strdup("code.sl");
    if (code == NULL) {
      fprintf(stderr,
              "main(): strdup() failed to allocate memory and returned NULL\n");
      return -1;
    }
  }
  char buff[1024];
  char **code_array;

  struct SL_Code sl_code = sl_init_sl_process();
  sl_code.special_tokens = SPECIAL_TOKENS;
  sl_add_func(&sl_code, "not", builtin_not_fn);
  sl_add_custom_splitter(&sl_code, "{", then_end_splitter);
  sl_add_custom_expr(&sl_code, "{", then_end_expression);
  init_sl_stdlib(&sl_code, argc, argv);

  if (sl_open_sl_process(&sl_code, code) != 0) {
    free(code);
    free(sl_code.types);
    free(sl_code.funcs);
    free(sl_code.vars);
    exit(-1);
  }

  if (sl_close_sl_process(&sl_code) == -1) {
    fprintf(stderr, "close_sl_process failed.");
    return -1;
  }
  close_sl_stdlib();
  free(code);
  return 0;
}
