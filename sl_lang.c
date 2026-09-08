#include "sl.h"
#include "stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    return_var.vals = "Unexpected return usage!";
    return return_var;
  }

  return return_var;
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
    }
  } else {
    code = strdup("./code.sl");
  }
  char buff[1024];
  char **code_array;

  struct SL_Code sl_code = sl_init_sl_process();
  sl_add_func(&sl_code, "not", builtin_not_fn);
  init_sl_stdlib(&sl_code, argc, argv);

  if (sl_open_sl_process(&sl_code, code) != 0) {
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
