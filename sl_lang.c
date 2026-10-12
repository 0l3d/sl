#include "sl.h"
#include "stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SPECIAL_TOKENS "()+-/*%^&|=<>,{}"
#define PIPE_TOKEN "|>"
#define PIPE_PRECEDENCE 5

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

static int find_closing_brace(char **tokens, int open_brace_index,
                              int max_tokens) {
  int nested_depth = 0;

  for (int i = open_brace_index + 1; i < max_tokens; i++) {
    if (tokens[i] == NULL)
      continue;

    if (tokens[i][0] == '{') {
      nested_depth++;
    } else if (tokens[i][0] == '}') {
      if (nested_depth == 0)
        return i;
      nested_depth--;
    }
  }

  return -1;
}

int brace_block_splitter(struct SL_Code *code, int *current_token) {
  int closing_brace =
      find_closing_brace(code->code, *current_token, code->token_count);

  if (closing_brace == -1) {
    sl_throw_an_error(*code, code->code, *current_token, code->token_count,
                      "UNCLOSED '{' (opening brace)",
                      "Expected: a matching '}' (closing brace)");
  }
  return closing_brace;
}

static void compile_block_as_value(struct SL_Code *code, int first_token,
                                   int end_token) {
  size_t skip_body_jump = sl_emit_jump(code, OPCODE_JMP);
  size_t body_start_pc = sl_current_pc(code);

  struct SL_FrameScope block_scope;
  sl_begin_frame_scope(code, &block_scope);
  sl_compile_statements(code, first_token, end_token);
  sl_emit(code, OPCODE_PUSH_EMPTY);
  sl_emit(code, OPCODE_RETURN);
  int block_frame_size = sl_end_frame_scope(code, &block_scope);

  sl_patch_jump_here(code, skip_body_jump);
  sl_emit(code, OPCODE_CALL_ADDRESS);
  sl_emit(code, (uint32_t)body_start_pc);
  sl_emit(code, (uint32_t)block_frame_size);
}

struct SL_Variable brace_block_expression(struct SL_Code *code,
                                          int *current_token) {
  struct SL_Variable nothing_to_return = {0};
  int opening_brace = *current_token;
  int closing_brace = brace_block_splitter(code, current_token);

  compile_block_as_value(code, opening_brace + 1, closing_brace);

  *current_token = closing_brace;
  return nothing_to_return;
}

static int find_closing_parenthesis(char **tokens, int open_index,
                                    int end_token) {
  int nested_depth = 0;

  for (int i = open_index + 1; i < end_token; i++) {
    if (tokens[i][0] == '(') {
      nested_depth++;
    } else if (tokens[i][0] == ')') {
      if (nested_depth == 0)
        return i;
      nested_depth--;
    }
  }

  return -1;
}

static int find_argument_end(char **tokens, int first_token,
                             int closing_token) {
  int nested_depth = 0;

  for (int i = first_token; i < closing_token; i++) {
    if (tokens[i][0] == '(')
      nested_depth++;
    else if (tokens[i][0] == ')')
      nested_depth--;
    else if (tokens[i][0] == ',' && nested_depth == 0)
      return i;
  }

  return closing_token;
}

static int compile_pipe_arguments(struct SL_Code *code, int open_index,
                                  int closing_index) {
  int argument_count = 0;
  int argument_start = open_index + 1;

  while (argument_start < closing_index) {
    int argument_end =
        find_argument_end(code->code, argument_start, closing_index);
    sl_compile_expression(code, argument_start, argument_end);
    argument_count++;
    argument_start = argument_end + 1;
  }

  return argument_count;
}

static void emit_pipe_call(struct SL_Code *code, int stage_token,
                           int argument_count) {
  int function_index = sl_find_function_index(code, code->code[stage_token]);
  if (function_index != -1) {
    sl_emit_call_function(code, function_index, argument_count);
    return;
  }

  int name_index = sl_add_constant(
      code, sl_copy_variable(code->fixed_values[stage_token]));
  sl_emit(code, OPCODE_CALL_DYNAMIC_LINKED_FUNC);
  sl_emit(code, (uint32_t)name_index);
  sl_emit(code, (uint32_t)argument_count);
}

static void pipe_token_handler(struct SL_Code *code, int first_token,
                               int operator_token, int end_token) {
  int stage_token = operator_token + 1;
  if (operator_token == first_token || stage_token >= end_token) {
    sl_throw_an_error(*code, code->code, operator_token, end_token,
                      "UNEXPECTED PIPE USAGE",
                      "Expected: <value> |> <function>");
  }

  int stage_end = stage_token + 1;
  int closing_parenthesis = -1;
  if (stage_end < end_token && code->code[stage_end][0] == '(') {
    closing_parenthesis =
        find_closing_parenthesis(code->code, stage_end, end_token);
    if (closing_parenthesis == -1) {
      sl_throw_an_error(*code, code->code, stage_end, end_token,
                        "UNCLOSED '(' (opening parenthesis) AFTER PIPE",
                        "Expected: a matching ')' (closing parenthesis)");
    }
    stage_end = closing_parenthesis + 1;
  }

  if (stage_end != end_token) {
    sl_throw_an_error(*code, code->code, stage_end, end_token,
                      "UNEXPECTED TOKENS AFTER PIPE STAGE",
                      "Expected: <value> |> <function> or "
                      "<value> |> <function>(extra arguments)");
  }

  sl_compile_expression(code, first_token, operator_token);

  int argument_count = 1;
  if (closing_parenthesis != -1)
    argument_count +=
        compile_pipe_arguments(code, stage_token + 1, closing_parenthesis);

  emit_pipe_call(code, stage_token, argument_count);
}

static void register_language_extensions(struct SL_Code *sl_code) {
  sl_add_custom_token(sl_code, PIPE_TOKEN, PIPE_PRECEDENCE, pipe_token_handler);
  sl_add_custom_splitter(sl_code, "{", brace_block_splitter);
  sl_add_custom_expr(sl_code, "{", brace_block_expression);
}

static int flag_matches(const char *argument, char letter) {
  int has_prefix = argument[0] == '-' || argument[0] == '/';
  return has_prefix && argument[1] == letter && argument[2] == '\0';
}

static void print_usage(const char *program) {
  fprintf(stderr, "Usage: %s <file.sl>\n", program);
  fprintf(stderr, "       %s -c <file.sl> <output.slx>\n", program);
  fprintf(stderr, "       %s -r <file.slx> [arguments...]\n", program);
  fprintf(stderr, "       %s -s <file.slx>\n", program);
  fprintf(stderr, "Windows style flags /c, /r and /s are also accepted.\n");
}

static char **build_forwarded_argv(int argc, char **argv, int first_file_index,
                                   int *forwarded_argc) {
  *forwarded_argc = argc - first_file_index + 1;
  char **forwarded = malloc((size_t)(*forwarded_argc + 1) * sizeof(char *));
  if (forwarded == NULL)
    return NULL;

  forwarded[0] = argv[0];
  for (int i = 1; i < *forwarded_argc; i++)
    forwarded[i] = argv[first_file_index + i - 1];
  forwarded[*forwarded_argc] = NULL;
  return forwarded;
}

static int compile_file(struct SL_Code *sl_code, const char *source_name,
                        const char *output_name) {
  int status = 0;
  if (sl_open_sl_process(sl_code, source_name) != 0)
    status = 1;
  else if (sl_save_slx(sl_code, output_name) != 0)
    status = 1;

  sl_close_sl_process(sl_code);
  return status;
}

static int run_binary(struct SL_Code *sl_code, const char *binary_name) {
  int status = 0;
  if (sl_load_slx(sl_code, binary_name) != 0)
    status = 1;
  else
    sl_run_vm(sl_code);

  sl_close_sl_process(sl_code);
  return status;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  int compile_mode = flag_matches(argv[1], 'c');
  int run_mode = flag_matches(argv[1], 'r');
  int disassemble_mode = flag_matches(argv[1], 's');
  int has_flag = compile_mode || run_mode || disassemble_mode;

  if (has_flag && argc < 3) {
    fprintf(stderr, "Please specify the file name.\n");
    print_usage(argv[0]);
    return 1;
  }

  if (compile_mode && argc < 4) {
    fprintf(stderr, "Please specify the output file name.\n");
    print_usage(argv[0]);
    return 1;
  }

  int first_file_index = has_flag ? 2 : 1;
  char *file_name = argv[first_file_index];

  int forwarded_argc = 0;
  char **forwarded_argv =
      build_forwarded_argv(argc, argv, first_file_index, &forwarded_argc);
  if (forwarded_argv == NULL)
    return 1;

  struct SL_Code sl_code = sl_init_sl_process();
  sl_code.special_tokens = SPECIAL_TOKENS;
  register_language_extensions(&sl_code);
  sl_add_func(&sl_code, "not", builtin_not_fn);
  init_sl_stdlib(&sl_code, forwarded_argc, forwarded_argv);

  int exit_status = 0;
  if (disassemble_mode) {
    sl_code.disassemble = 1;
    exit_status = run_binary(&sl_code, file_name);
  } else if (run_mode)
    exit_status = run_binary(&sl_code, file_name);
  else if (compile_mode)
    exit_status = compile_file(&sl_code, file_name, argv[3]);
  else
    sl_run_process(&sl_code, file_name);

  close_sl_stdlib();
  free(forwarded_argv);
  return exit_status;
}