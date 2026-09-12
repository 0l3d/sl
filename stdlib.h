/*
 * SL Standart Library
 */

#include "sl.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define CHAR WIN32_CHAR
#define LONG WIN32_LONG
#define BOOLEAN WIN32_BOOLEAN
#define DOUBLE WIN32_DOUBLE

#include <winsock2.h>
#include <ws2tcpip.h>

#undef CHAR
#undef LONG
#undef BOOLEAN
#undef DOUBLE
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

struct SL_Code *use_code = NULL;

struct SL_FD_List {
  fd_set *fd;
  int is_set;
};

struct SL_FD_List *fd_list = NULL;
int fd_list_size = 0;
int fd_list_capacity = 0;
/* Example Function for function definition ref. */
struct SL_Variable example_fn(struct SL_Code *code, struct SL_L_Function func,
                              struct SL_Function rfunc) {

  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

  return return_var;
}
/* Example Function for function definition ref. */

/* Global Arguments */
char **arguments = NULL;
int argcN;
/* Global Arguments */

/* Lists struct */
struct SL_List {
  int capacity;
  struct SL_Variable *vars;
  int size;
  int fixed;
  int current;
};

/* Single Collection Struct */
struct SL_Collection {
  char *name;
  char **attrs;
  struct SL_Function *functions;
  int total_attrs;
  int total_funcs;
};

/* Collections struct  */
struct SL_Collections {
  struct SL_Collection *collections;
  int size;
  int capacity;
};

/* LISTS */
int LISTS_count = 0;
int LISTS_capacity = SL_INIT;
struct SL_List *LISTS = {0};

/* COLLECTIONS */
struct SL_Collections collections = {0};

int sl_add_fixed_int(struct SL_Code *code, char *name, int value) {
  struct SL_Variable var = {0};
  var.name = name;
  var.hash = sl_hash_string(name);
  var.scope_lifetime = code->scope_depth;
  var.vali = value;
  var.type = INTEGER;
  sl_add_var(code, var);
  return 1;
}

/* LIST FUNCTIONS */
int create_new_list(int capacity, int fixed) {
  if (LISTS_count >= LISTS_capacity) {
    LISTS_capacity *= 2;
    void *tmp = realloc(LISTS, capacity * sizeof(struct SL_List));
    if (!tmp)
      perror("Realloc failed.");

    LISTS = tmp;
  }

  LISTS[LISTS_count].vars = calloc(capacity, sizeof(struct SL_Variable));
  LISTS[LISTS_count].capacity = capacity;
  if (fixed == 1) {
    LISTS[LISTS_count].fixed = 1;
    LISTS[LISTS_count].size = capacity;
    for (int i = 0; i < capacity; i++) {
      LISTS[LISTS_count].vars[i].type = INTEGER;
      LISTS[LISTS_count].vars[i].vali = 0;
    }
  } else {
    LISTS[LISTS_count].size = 0;
  }
  int index = LISTS_count;
  LISTS_count++;
  return index;
}

int list_push(struct SL_List *list, struct SL_Variable value) {
  if (list->size >= list->capacity) {
    list->capacity = (list->capacity == 0) ? 8 : list->capacity * 2;

    list->vars =
        realloc(list->vars, list->capacity * sizeof(struct SL_Variable));
  }

  list->vars[list->size++] = sl_copy_variable(value);

  return 1;
}

struct SL_Variable list_pop(struct SL_List *list) {
  struct SL_Variable ret = sl_copy_variable(list->vars[list->size - 1]);

  if (list->vars[list->size - 1].name != NULL) {
    free(list->vars[list->size - 1].name);
    list->vars[list->size - 1].name = NULL;
  }

  if ((list->vars[list->size - 1].type == STRING ||
       list->vars[list->size - 1].type == RETURN) &&
      list->vars[list->size - 1].vals != NULL) {
    free(list->vars[list->size - 1].vals);
    list->vars[list->size - 1].vals = NULL;
  }

  list->size--;

  return ret;
}

int list_set(struct SL_List *list, int index, struct SL_Variable value) {
  if (index < 0 || index >= list->size)
    return 0;

  if ((list->vars[index].type == STRING || list->vars[index].type == RETURN) &&
      list->vars[index].vals != NULL) {
    free(list->vars[index].vals);
    list->vars[index].vals = NULL;
  }

  if (list->vars[index].name != NULL) {
    free(list->vars[index].name);
    list->vars[index].name = NULL;
  }

  list->vars[index] = sl_copy_variable(value);

  return 1;
}

int list_remove(struct SL_List *list, int index) {
  if (index < 0 || index >= list->size)
    return 0;

  if ((list->vars[index].type == STRING || list->vars[index].type == RETURN) &&
      list->vars[index].vals != NULL) {
    free(list->vars[index].vals);
    list->vars[index].vals = NULL;
  }

  if (list->vars[index].name != NULL) {
    free(list->vars[index].name);
    list->vars[index].name = NULL;
  }

  for (int i = index; i < list->size - 1; i++) {
    list->vars[i] = list->vars[i + 1];
  }

  list->size--;
  return 1;
}

/* LIST FUNCTIONS */

/* Input/Output for stdout/stdin*/
struct SL_Variable print_fn(struct SL_Code *code, struct SL_L_Function func,
                            struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  for (int i = 0; i < func.total_arguments; i++) {
    return_var = sl_get_argument(*code, func, i);
    switch (return_var.type) {
    case INTEGER:
      printf("%d", return_var.vali);
      break;
    case DOUBLE:
      printf("%f", return_var.valf);
      break;
    case STRING: {
      char *string = sl_string_getter(return_var.vals);
      printf("%s", string);
      free(string);
    } break;
    case BOOLEAN:
      if (return_var.valb == 1)
        printf("true");
      else if (return_var.valb == 0)
        printf("false");
      break;
    case CHAR:
      printf("%c", return_var.valc);
      break;
    case LONG:
      printf("%" PRIdPTR, return_var.valh);
      break;
    default:
      break;
    }
  }
  return return_var;
}

struct SL_Variable input_fn(struct SL_Code *code, struct SL_L_Function func,
                            struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  for (int i = 0; i < func.total_arguments; i++) {
    return_var = sl_get_argument(*code, func, i);
    switch (return_var.type) {
    case INTEGER:
      printf("%d", return_var.vali);
      break;
    case DOUBLE:
      printf("%f", return_var.valf);
      break;
    case STRING: {
      char *string = sl_string_getter(return_var.vals);
      printf("%s", string);
      free(string);
    } break;
    case BOOLEAN:
      if (return_var.valb == 1)
        printf("true");
      else if (return_var.valb == 0)
        printf("false");
      break;
    case CHAR:
      printf("%c", return_var.valc);
      break;
    case LONG:
      printf("%" PRIdPTR, return_var.valh);
      break;
    default:
      break;
    }
  }
  char string[1024];
  fgets(string, sizeof(string), stdin);
  string[strcspn(string, "\n")] = '\0';

  return_var.type = STRING;
  return_var.vals = malloc(1024);
  strncpy(return_var.vals, string, 1024);

  return return_var;
}

struct SL_Variable io_getchar_fn(struct SL_Code *code,
                                 struct SL_L_Function func,
                                 struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  return_var.valc = getchar();
  return_var.type = CHAR;
  return return_var;
}

struct SL_Variable io_fflush_fn(struct SL_Code *code, struct SL_L_Function func,
                                struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  fflush(stdout);
  return return_var;
}

/* Input/Output for stdout/stdin */

/* Input/Output for file/dir */
struct SL_Variable file_read_to_str_fn(struct SL_Code *code,
                                       struct SL_L_Function func,
                                       struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at file.read_to_str! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  char *file_name = sl_string_getter(first_arg.vals);

  FILE *file_open = fopen(file_name, "r");
  if (file_open == NULL) {
    return_var.type = ERROR;
    return_var.vals = "File not found!";
    return return_var;
  }

  fseek(file_open, 0, SEEK_END);
  long int size = ftell(file_open);
  rewind(file_open);

  char *buffer = malloc(size + 1);
  fread(buffer, 1, size, file_open);
  buffer[size] = '\0';
  fclose(file_open);

  return_var.type = STRING;
  return_var.vals = strdup(buffer);
  free(buffer);
  free(file_name);
  return return_var;
}

struct SL_Variable file_write_from_str_fn(struct SL_Code *code,
                                          struct SL_L_Function func,
                                          struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at file.write_from_str! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  char *file_name = sl_string_getter(first_arg.vals);

  FILE *file_open = fopen(file_name, "w");
  if (file_open == NULL) {
    return_var.type = ERROR;
    return_var.vals = "File not found!";
    return return_var;
  }
  size_t len = strlen(second_arg.vals);
  char *text = sl_string_getter(second_arg.vals);
  if (fwrite(text, 1, len, file_open) != len) {
    fclose(file_open);

    return_var.type = ERROR;
    return_var.vals = "Could not write to file!";
    return return_var;
  }
  free(text);

  fclose(file_open);

  return_var.type = STRING;
  return_var.vals = strdup(second_arg.vals);
  free(file_name);
  return return_var;
}

struct SL_Variable file_append_from_str_fn(struct SL_Code *code,
                                           struct SL_L_Function func,
                                           struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at file.append_from_str! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  char *file_name = sl_string_getter(first_arg.vals);

  FILE *file_open = fopen(file_name, "a");
  if (file_open == NULL) {
    return_var.type = ERROR;
    return_var.vals = "File not found!";
    return return_var;
  }
  size_t len = strlen(second_arg.vals);
  char *text = sl_string_getter(second_arg.vals);
  if (fwrite(text, 1, len, file_open) != len) {
    fclose(file_open);

    return_var.type = ERROR;
    return_var.vals = "Could not write to file!";
    return return_var;
  }
  free(text);

  fclose(file_open);

  return_var.type = STRING;
  return_var.vals = strdup(second_arg.vals);
  free(file_name);
  return return_var;
}
/* Input/Output for file/dir */

/* Extra C Standart Library Functions */
struct SL_Variable random_fn(struct SL_Code *code, struct SL_L_Function func,
                             struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at rand.random! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  int random = rand() % (second_arg.vali - first_arg.vali + 1) + first_arg.vali;
  struct SL_Variable return_var = {0};
  return_var.type = INTEGER;
  return_var.vali = random;
  return return_var;
}

/* Types for type checking/converting/more */
struct SL_Variable str_to_int_fn(struct SL_Code *code,
                                 struct SL_L_Function func,
                                 struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.str_to_int! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};
  return_var.type = INTEGER;
  return_var.vali = atoi(first_arg.vals);
  return return_var;
}

struct SL_Variable int_to_char_fn(struct SL_Code *code,
                                  struct SL_L_Function func,
                                  struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.int_to_char! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};
  if (first_arg.vali > 255) {
    return_var.type = ERROR;
    return_var.vals = "Char overflow!";
    return return_var;
  }
  return_var.valc = first_arg.vali;
  return_var.type = CHAR;
  return return_var;
}

struct SL_Variable char_to_int_fn(struct SL_Code *code,
                                  struct SL_L_Function func,
                                  struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.char_to_int! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};
  return_var.vali = first_arg.valc;
  return_var.type = INTEGER;
  return return_var;
}

struct SL_Variable char_to_str_fn(struct SL_Code *code,
                                  struct SL_L_Function func,
                                  struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.char_to_str! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};
  return_var.vals = malloc(2);
  return_var.vals[0] = first_arg.valc;
  return_var.vals[1] = '\0';
  return_var.type = STRING;
  return return_var;
}

struct SL_Variable int_to_str_fn(struct SL_Code *code,
                                 struct SL_L_Function func,
                                 struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.char_to_str! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};
  int digits = 0;
  int temp = first_arg.vali;
  if (temp == 0)
    digits = 1;
  else
    while (temp != 0) {
      digits++;
      temp /= 10;
    }

  return_var.vals = malloc(digits + 1);
  snprintf(return_var.vals, digits + 1, "%d", first_arg.vali);
  return_var.type = STRING;
  return return_var;
}

struct SL_Variable typeof_fn(struct SL_Code *code, struct SL_L_Function func,
                             struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.typeof! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  return_var.type = INTEGER;
  return_var.vali = first_arg.type;
  return return_var;
}

struct SL_Variable is_int_fn(struct SL_Code *code, struct SL_L_Function func,
                             struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.is_int! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  if (first_arg.type == INTEGER)
    return_var.valb = 1;
  return_var.type = BOOLEAN;
  return return_var;
}

struct SL_Variable is_char_fn(struct SL_Code *code, struct SL_L_Function func,
                              struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.is_char! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  if (first_arg.type == CHAR)
    return_var.valb = 1;
  return_var.type = BOOLEAN;
  return return_var;
}

struct SL_Variable is_bool_fn(struct SL_Code *code, struct SL_L_Function func,
                              struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.is_bool! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  if (first_arg.type == BOOLEAN)
    return_var.valb = 1;
  return_var.type = BOOLEAN;
  return return_var;
}

struct SL_Variable is_string_fn(struct SL_Code *code, struct SL_L_Function func,
                                struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.is_string! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  if (first_arg.type == STRING)
    return_var.valb = 1;
  return_var.type = BOOLEAN;
  return return_var;
}

struct SL_Variable is_double_fn(struct SL_Code *code, struct SL_L_Function func,
                                struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.is_string! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  if (first_arg.type == DOUBLE)
    return_var.valb = 1;
  return_var.type = BOOLEAN;
  return return_var;
}

struct SL_Variable is_not_initialized_fn(struct SL_Code *code,
                                         struct SL_L_Function func,
                                         struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.is_string! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  if (first_arg.type == INIT)
    return_var.valb = 1;
  return_var.type = BOOLEAN;
  return return_var;
}

struct SL_Variable is_digit_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.str_to_int! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  char *str = sl_string_getter(first_arg.vals);

  struct SL_Variable return_var = {0};
  return_var.type = BOOLEAN;
  return_var.valb = 0;

  if (first_arg.type == STRING) {
    char *str = sl_string_getter(first_arg.vals);
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
      if (!isdigit(str[i])) {
        free(str);
        return return_var;
      }
    }
    free(str);
  } else if (first_arg.type == CHAR)
    if (!isdigit(first_arg.valc))
      return return_var;

  return_var.valb = 1;
  return return_var;
}

struct SL_Variable is_space_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.str_to_int! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

  struct SL_Variable return_var = {0};
  return_var.type = BOOLEAN;
  return_var.valb = 0;
  if (first_arg.type == STRING) {
    char *str = sl_string_getter(first_arg.vals);
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
      if (!isspace(str[i])) {
        free(str);
        return return_var;
      }
    }
    free(str);
  } else if (first_arg.type == CHAR)
    if (!isspace(first_arg.valc))
      return return_var;

  return_var.valb = 1;
  return return_var;
}

/* String Helper functions */
struct SL_Variable string_charat_fn(struct SL_Code *code,
                                    struct SL_L_Function func,
                                    struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at string.char_at! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  if (first_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected string as the first argument to string.char_at.";
    return return_var;
  }

  if (second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected integer as the second argument to string.char_at.";
    return return_var;
  }

  int len = strlen(first_arg.vals);

  if (second_arg.vali >= len) {
    return_var.type = ERROR;
    return_var.vals = "Buffer overflow!";
    return return_var;
  }

  if (second_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "Buffer underflow!";
    return return_var;
  }
  char *plain = sl_string_getter(first_arg.vals);
  return_var.valc = plain[second_arg.vali];
  return_var.type = CHAR;
  free(plain);
  return return_var;
}

struct SL_Variable string_setcharat_fn(struct SL_Code *code,
                                       struct SL_L_Function func,
                                       struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at string.char_at! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable third_arg = sl_get_argument(*code, func, 2);
  if (first_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected string as the first argument to string.set_char_at.";
    return return_var;
  }

  if (second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected integer as the second argument to string.set_char_at.";
    return return_var;
  }

  if (third_arg.type != CHAR) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected char as the second argument to string.set_char_at.";
    return return_var;
  }

  int len = strlen(first_arg.vals);

  if (second_arg.vali >= len) {
    return_var.type = ERROR;
    return_var.vals = "Buffer overflow!";
    return return_var;
  }

  if (second_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "Buffer underflow!";
    return return_var;
  }
  struct SL_Variable *ref_var = sl_get_var(code, first_arg.name);
  char *string = sl_string_getter(ref_var->vals);
  free(ref_var->vals);
  string[second_arg.vali] = third_arg.valc;
  ref_var->vals = strdup(string);
  return_var.type = BOOLEAN;
  return_var.valb = 1;
  free(string);
  return return_var;
}

struct SL_Variable string_len_fn(struct SL_Code *code,
                                 struct SL_L_Function func,
                                 struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at string.len! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

  if (first_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected string as the first argument to string.len.";
    return return_var;
  }

  int len = strlen(first_arg.vals);
  return_var.vali = len;
  return_var.type = INTEGER;
  return return_var;
}

struct SL_Variable string_split_fn(struct SL_Code *code,
                                   struct SL_L_Function func,
                                   struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at string.split! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  if (first_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected string as the first argument to string.split.";
    return return_var;
  }
  if (second_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected string as the second argument to string.split.";
    return return_var;
  }
  char *splt_string = sl_string_getter(first_arg.vals);

  char *splt_token = sl_string_getter(second_arg.vals);

  int listind = create_new_list(256, 0);

  char *tokenize = strtok(splt_string, splt_token);
  while (tokenize != NULL) {
    struct SL_Variable push_val = {0};
    push_val.vals = strdup(tokenize);
    push_val.type = STRING;
    list_push(&LISTS[listind], push_val);
    tokenize = strtok(NULL, splt_token);
  }

  free(splt_string);
  free(splt_token);
  return_var.vali = listind;
  return_var.type = INTEGER;
  return return_var;
}

struct SL_Variable string_contains_fn(struct SL_Code *code,
                                   struct SL_L_Function func,
                                   struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at string.contains! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  if (first_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected string as the first argument to string.contains.";
    return return_var;
  }
  if (second_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected string as the second argument to string.split.";
    return return_var;
  }

  char *raw_string = sl_string_getter(first_arg.vals);
  
  char *searchingstr = sl_string_getter(second_arg.vals);
  return_var.valb = strstr(raw_string, searchingstr) != NULL;
  return_var.type = BOOLEAN;
  free(raw_string);
  free(searchingstr);
  return return_var;
}



struct SL_Variable string_slice_fn(struct SL_Code *code,
                                   struct SL_L_Function func,
                                   struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at string.slice! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable third_arg = sl_get_argument(*code, func, 2);
  if (first_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected string as the first argument to string.slice.";
    return return_var;
  }
  if (second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected integer as the second argument to string.slice.";
    return return_var;
  }

  if (third_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals = "Expected integer as the third argument to string.slice.";
    return return_var;
  }
  char *raw_str = sl_string_getter(first_arg.vals);
  int len = strlen(raw_str);
  if (len >= second_arg.vali || len > third_arg.vali || len < 0) {
    return_var.type = ERROR;
    return_var.vals = "Buffer over/underflow!";
  }
  char *result = malloc(len + 1);
  strncpy(result, raw_str + second_arg.vali, third_arg.vali - second_arg.vali);

  free(raw_str);
  return_var.type = STRING;
  return_var.vals = strdup(result);
  free(result);
  return return_var;
}

struct SL_Variable string_trim_fn(struct SL_Code *code,
                                  struct SL_L_Function func,
                                  struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at string.trim! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

  if (first_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected string as the first argument to string.trim.";
    return return_var;
  }

  char *raw_str = sl_string_getter(first_arg.vals);

  int start = 0;
  int end = strlen(raw_str);

  while (start < end && isspace((unsigned char)raw_str[start])) {
    start++;
  }

  while (end > start && isspace((unsigned char)raw_str[end - 1])) {
    end--;
  }

  int len = end - start;

  char *result = malloc(len + 1);
  memcpy(result, raw_str + start, len);
  result[len] = '\0';

  return_var.type = STRING;
  return_var.vals = strdup(result);

  free(result);
  free(raw_str);

  return return_var;
}

/* Error handling */
struct SL_Variable errors_string_fn(struct SL_Code *code,
                                    struct SL_L_Function func,
                                    struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at errors.string! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};
  if (first_arg.type == ERROR) {
    printf("%s\n", first_arg.vals);
  }
  return sl_copy_variable(first_arg);
}

struct SL_Variable errors_bool_fn(struct SL_Code *code,
                                  struct SL_L_Function func,
                                  struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at errors.bool! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};
  if (first_arg.type == ERROR) {
    return_var.valb = 1;
    return_var.type = BOOLEAN;
    return return_var;
  }
  return_var.valb = 0;
  return_var.type = BOOLEAN;
  return return_var;
}

struct SL_Variable errors_panic_fn(struct SL_Code *code,
                                   struct SL_L_Function func,
                                   struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at errors.string! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};
  if (first_arg.type == ERROR) {
    printf("Program panicked with error: %s\n", first_arg.vals);
    exit(-1);
  }
  return sl_copy_variable(first_arg);
}

/* SYS Library for more specific functions */
struct SL_Variable sys_exit_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at sys.exit! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};
  if (first_arg.type == INTEGER) {
    exit(first_arg.vali);
  }
  return return_var;
}

struct SL_Variable sys_get_arg_fn(struct SL_Code *code,
                                  struct SL_L_Function func,
                                  struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at sys.get_arg! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};
  if (first_arg.type == INTEGER && first_arg.vali < argcN) {
    return_var.type = STRING;
    return_var.vals = strdup(arguments[first_arg.vali]);
  } else {
    return_var.type = ERROR;
    return_var.vals = "Argument not found!";
  }
  return return_var;
}

/* Dynamic/Static Array system for SL. */
struct SL_Variable List_new_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  struct SL_Variable first_arg;
  int fixed = 0;
  if (func.total_arguments > 0) {
    first_arg = sl_get_argument(*code, func, 0);
    fixed = 1;
  } else {
    first_arg.vali = 128;
  }

  if (func.total_arguments > 0 && first_arg.type != INTEGER) {
    struct SL_Variable err;
    err.type = ERROR;
    err.vals = "Expected integer as the first argument to List.new.";
    return err;
  }

  struct SL_Variable return_var = {0};
  int capacity = first_arg.vali > 0 ? first_arg.vali : 1;

  return_var.type = INTEGER;
  return_var.vali = create_new_list(capacity, fixed);

  return return_var;
}

struct SL_Variable List_push_fn(struct SL_Code *code, struct SL_L_Function func,
                                struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at List.push! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected list_variable as the first argument to List.push.";
    return return_var;
  }

  if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer overflow!";
    return return_var;
  }

  struct SL_List *list = &LISTS[first_arg.vali];

  if (list->fixed == 1) {
    return_var.type = ERROR;
    return_var.vals = "List is fixed list!";
    return return_var;
  }
  for (int i = 1; i < func.total_arguments; i++) {
    struct SL_Variable list_item = sl_get_argument(*code, func, i);
    return_var.type = INTEGER;
    return_var.vali = list_push(list, list_item);
  }

  return return_var;
}

struct SL_Variable List_pop_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at List.pop! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

  struct SL_Variable return_var = {0};

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected list_variable as the first argument to List.pop.";
    return return_var;
  }

  if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer overflow!";
    return return_var;
  }

  struct SL_List *list = &LISTS[first_arg.vali];

  if (list->size <= 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer underflow";
    return return_var;
  }

  if (list->fixed == 1) {
    return_var.type = ERROR;
    return_var.vals = "List is fixed list!";
    return return_var;
  }

  return list_pop(list);
}

struct SL_Variable List_peek_fn(struct SL_Code *code, struct SL_L_Function func,
                                struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at List.peek! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected list_variable as the first argument to List.peek.";
    return return_var;
  }

  if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer overflow!";
    return return_var;
  }

  struct SL_List *list = &LISTS[first_arg.vali];

  if (list->size <= 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer underflow";
    return return_var;
  }

  if (list->fixed == 1) {
    return_var.type = ERROR;
    return_var.vals = "List is fixed list!";
    return return_var;
  }
  return sl_copy_variable(list->vars[list->size - 1]);
}

struct SL_Variable List_set_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  if (func.total_arguments < 3) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at List.set! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable third_arg = sl_get_argument(*code, func, 2);

  struct SL_Variable return_var = {0};

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected list_variable as the first argument to List.set.";
    return return_var;
  }

  if (second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals = "Expected integer as the second argument to List.set.";
    return return_var;
  }

  if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer overflow!";
    return return_var;
  }

  struct SL_List *list = &LISTS[first_arg.vali];

  if (second_arg.vali < 0 || second_arg.vali >= list->size) {
    return_var.type = ERROR;
    return_var.vals = "Buffer overflow on List element.";
    return return_var;
  }

  if (list_set(list, second_arg.vali, third_arg) == 0) {
    return_var.type = ERROR;
    return_var.vals = "Buffer overflow on List element.";
    return return_var;
  }

  return_var.type = INTEGER;
  return_var.vali = 1;

  return return_var;
}

struct SL_Variable List_get_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at List.get! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable return_var = {0};

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected list_variable as the first argument to List.get.";
    return return_var;
  }

  if (second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals = "Expected integer as the second argument to List.get.";
    return return_var;
  }

  if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer overflow!";
    return return_var;
  }

  if (second_arg.vali >= LISTS[first_arg.vali].size || second_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "Buffer overflow on List element.";
    return return_var;
  }

  return sl_copy_variable(LISTS[first_arg.vali].vars[second_arg.vali]);
}

struct SL_Variable List_remove_fn(struct SL_Code *code,
                                  struct SL_L_Function func,
                                  struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at List.remove! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected list_variable as the first argument to List.remove.";
    return return_var;
  }

  if (second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected integer index as the second argument to List.remove.";
    return return_var;
  }

  if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer overflow!";
    return return_var;
  }

  struct SL_List *list = &LISTS[first_arg.vali];

  if (list->fixed == 1) {
    return_var.type = ERROR;
    return_var.vals = "List is fixed list!";
    return return_var;
  }

  if (second_arg.vali < 0 || second_arg.vali >= list->size) {
    return_var.type = ERROR;
    return_var.vals = "Index out of bounds for List.remove!";
    return return_var;
  }

  int success = list_remove(list, second_arg.vali);

  return_var.type = BOOLEAN;
  return_var.valb = success;
  return return_var;
}

struct SL_Variable List_find_fn(struct SL_Code *code, struct SL_L_Function func,
                                struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at List.remove! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected list_variable as the first argument to List.remove.";
    return return_var;
  }

  if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer overflow!";
    return return_var;
  }

  struct SL_List *list = &LISTS[first_arg.vali];

  int found = 0;

  for (int i = 0; i < list->size; i++) {
    if (list->vars[i].type == second_arg.type) {
      switch (second_arg.type) {
      case INTEGER:
        if (second_arg.vali == list->vars[i].vali) {
          found = 1;
        }
        break;
      case STRING:
        if (strcmp(second_arg.vals, list->vars[i].vals) == 0) {
          found = 1;
        }
        break;
      case DOUBLE:
        if (second_arg.valf == list->vars[i].valf) {
          found = 1;
        }
        break;
      case CHAR:
        if (second_arg.valc == list->vars[i].valc) {
          found = 1;
        }
        break;
      case BOOLEAN:
        if (second_arg.valb == list->vars[i].valb) {
          found = 1;
        }
        break;
      case LONG:
        if (second_arg.valh == list->vars[i].valh) {
          found = 1;
        }
        break;
      default:
        break;
      }

      if (found == 1) {
        return_var.vali = i;
        return_var.type = INTEGER;
        return return_var;
      }
    }
  }

  return_var.type = ERROR;
  return_var.vals = "Item not found!";
  return return_var;
}

struct SL_Variable List_iter_fn(struct SL_Code *code, struct SL_L_Function func,
                                struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at List.iter! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected list_variable as the first argument to List.iter.";
    return return_var;
  }

  if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer overflow!";
    return return_var;
  }

  if (LISTS[first_arg.vali].current >= LISTS[first_arg.vali].size) {
    LISTS[first_arg.vali].current = 0;
    return_var.valb = 0;
    return_var.type = BOOLEAN;
  } else {
    return_var.valb = 1;
    return_var.type = BOOLEAN;
  }

  return return_var;
}

struct SL_Variable List_next_fn(struct SL_Code *code, struct SL_L_Function func,
                                struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at List.next! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected list_variable as the first argument to List.next.";
    return return_var;
  }

  if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer overflow!";
    return return_var;
  }

  if (LISTS[first_arg.vali].current >= LISTS[first_arg.vali].size) {
    LISTS[first_arg.vali].current = 0;
  }

  return sl_copy_variable(
      LISTS[first_arg.vali].vars[LISTS[first_arg.vali].current++]);
}

struct SL_Variable List_relative_fn(struct SL_Code *code,
                                    struct SL_L_Function func,
                                    struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at List.relative! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable return_var = {0};

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected list_variable as the first argument to List.relative.";
    return return_var;
  }

  if (second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected integer as the second argument to List.relative.";
    return return_var;
  }

  if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer overflow!";
    return return_var;
  }

  if (LISTS[first_arg.vali].current >= LISTS[first_arg.vali].size) {
    LISTS[first_arg.vali].current = 0;
  }

  if (LISTS[first_arg.vali].current > 0 &&
      LISTS[first_arg.vali].current + second_arg.vali > 0 &&
      LISTS[first_arg.vali].current + second_arg.vali <
          LISTS[first_arg.vali].size) {
    return sl_copy_variable(
        LISTS[first_arg.vali]
            .vars[LISTS[first_arg.vali].current + second_arg.vali]);
  }
  return_var.type = ERROR;
  return_var.vals = "List buffer overflow/underflow";
  return return_var;
}

struct SL_Variable List_len_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at List.len! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected list_variable as the first argument to List.len.";
    return return_var;
  }

  if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer overflow!";
    return return_var;
  }

  return_var.type = INTEGER;
  return_var.vali = LISTS[first_arg.vali].size;
  return return_var;
}

/* Simple Database system for SL */
struct SL_Variable db_from_lists_fn(struct SL_Code *code,
                                    struct SL_L_Function func,
                                    struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at db.from_lists! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable return_var = {0};

  size_t body_cap = 1024;
  size_t body_len = 0;
  char *body = malloc(body_cap);
  body[0] = '\0';

  for (int i = 0; i < func.total_arguments; i++) {
    struct SL_Variable arg = sl_get_argument(*code, func, i);
    if (arg.type != INTEGER) {
      free(body);
      return_var.type = ERROR;
      return_var.vals =
          "Expected integer list index as argument to db.from_lists.";
      return return_var;
    }
    if (arg.vali >= LISTS_count || arg.vali < 0) {
      free(body);
      return_var.type = ERROR;
      return_var.vals = "List buffer overflow in db.from_lists!";
      return return_var;
    }

    struct SL_List *list = &LISTS[arg.vali];
    const char *name = (arg.name != NULL) ? arg.name : "unnamed_list";

    size_t n_len = strlen(name);
    if (body_len + n_len + 2 >= body_cap) {
      body_cap = body_cap * 2 + n_len + 2;
      body = realloc(body, body_cap);
    }
    strcpy(body + body_len, name);
    body_len += n_len;

    for (int j = 0; j < list->size; j++) {
      struct SL_Variable item = list->vars[j];
      char item_str[512] = "";
      switch (item.type) {
      case INTEGER:
        snprintf(item_str, sizeof(item_str), "%d", item.vali);
        break;
      case DOUBLE:
        snprintf(item_str, sizeof(item_str), "%f", item.valf);
        break;
      case STRING: {
        char *s = sl_string_getter(item.vals);
        snprintf(item_str, sizeof(item_str), "%s", s);
        free(s);
      } break;
      case BOOLEAN:
        snprintf(item_str, sizeof(item_str), "%s",
                 item.valb ? "true" : "false");
        break;
      case CHAR:
        snprintf(item_str, sizeof(item_str), "%c", item.valc);
        break;
      case LONG:
        snprintf(item_str, sizeof(item_str), "%" PRIdPTR, item.valh);
        break;
      default:
        break;
      }

      size_t is_len = strlen(item_str);
      if (body_len + is_len + 2 >= body_cap) {
        body_cap = body_cap * 2 + is_len + 2;
        body = realloc(body, body_cap);
      }
      body[body_len++] = '/';
      strcpy(body + body_len, item_str);
      body_len += is_len;
    }

    if (i < func.total_arguments - 1) {
      if (body_len + 2 >= body_cap) {
        body_cap *= 2;
        body = realloc(body, body_cap);
      }
      body[body_len++] = '\\';
      body[body_len] = '\0';
    }
  }

  return_var.type = STRING;
  return_var.vals = body;
  return return_var;
}

struct SL_Variable db_to_lists_fn(struct SL_Code *code,
                                  struct SL_L_Function func,
                                  struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at db.to_lists! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

  if (first_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected string as argument to db.to_lists.";
    return return_var;
  }

  int master_list_idx = create_new_list(8, 0);
  struct SL_List *master_list = &LISTS[master_list_idx];

  char *src = first_arg.vals;
  if (!src) {
    return_var.type = INTEGER;
    return_var.vali = master_list_idx;
    return return_var;
  }

  char *p = src;
  int current_list_idx = -1;
  int is_first_token = 1;

  size_t tok_cap = 256;
  size_t tok_len = 0;
  char *tok = malloc(tok_cap);

  while (*p != '\0') {
    if (*p == '\\') {
      if (current_list_idx != -1 && !is_first_token) {
        tok[tok_len] = '\0';
        struct SL_Variable item = {0};
        item.type = STRING;
        item.vals = strdup(tok);
        list_push(&LISTS[current_list_idx], item);
      }
      if (current_list_idx != -1) {
        struct SL_Variable list_ref = {0};
        list_ref.type = INTEGER;
        list_ref.vali = current_list_idx;
        list_push(master_list, list_ref);
      }
      current_list_idx = -1;
      is_first_token = 1;
      tok_len = 0;
      p++;
    } else if (*p == '/') {
      tok[tok_len] = '\0';
      if (is_first_token) {
        current_list_idx = create_new_list(8, 0);
        is_first_token = 0;
      } else {
        struct SL_Variable item = {0};
        item.type = STRING;
        item.vals = strdup(tok);
        list_push(&LISTS[current_list_idx], item);
      }
      tok_len = 0;
      p++;
    } else {
      if (tok_len + 1 >= tok_cap) {
        tok_cap *= 2;
        tok = realloc(tok, tok_cap);
      }
      tok[tok_len++] = *p;
      p++;
    }
  }

  if (current_list_idx != -1 && !is_first_token) {
    tok[tok_len] = '\0';
    struct SL_Variable item = {0};
    item.type = STRING;
    item.vals = strdup(tok);
    list_push(&LISTS[current_list_idx], item);

    struct SL_Variable list_ref = {0};
    list_ref.type = INTEGER;
    list_ref.vali = current_list_idx;
    list_push(master_list, list_ref);
  }

  free(tok);

  return_var.type = INTEGER;
  return_var.vali = master_list_idx;
  return return_var;
}

/* Object Like system for SL */
struct SL_Variable collections_new_collection_fn(struct SL_Code *code,
                                                 struct SL_L_Function func,
                                                 struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  int collec_index = 0;
  int len = strlen(rfunc.name);
  for (int i = 0; i < collections.size; i++) {
    if (strncmp(collections.collections[i].name, rfunc.name, len - 4) == 0) {
      collec_index = i;
    }
  }

  int scope = sl_get_scope(code);
  if (scope > 1)
    scope -= 1;

  char *assigned_var = sl_get_assignment_var();
  int assigned_len = strlen(assigned_var);
  for (int i = 0; i < collections.collections[collec_index].total_attrs; i++) {
    char *raw_attr = collections.collections[collec_index].attrs[i];
    int assigned_var_len = strlen(assigned_var);
    int attr_len = strlen(raw_attr);
    int total_len = assigned_var_len + attr_len;
    char *full_attr_name = malloc(total_len + 2);
    snprintf(full_attr_name, total_len + 2, "%s.%s", assigned_var, raw_attr);
    struct SL_Variable var = {0};
    var.name = full_attr_name;
    var.type = INTEGER;
    var.hash = sl_hash_string(full_attr_name);
    var.scope_lifetime = scope;
    sl_add_var(code, var);
    free(full_attr_name);
  }
  for (int i = 0; i < collections.collections[collec_index].total_funcs; i++) {
    struct SL_Function raw_func =
        collections.collections[collec_index].functions[i];
    int assigned_var_len = strlen(assigned_var);
    int funcn_len = strlen(raw_func.name);
    int total_len = assigned_var_len + funcn_len;
    char *full_func_name = malloc(total_len + 2);
    snprintf(full_func_name, total_len + 2, "%s:%s", assigned_var,
             raw_func.name);
    struct SL_Function func = {0};
    func = raw_func;
    func.code_tokens[3] = malloc(assigned_len + 3);
    snprintf(func.code_tokens[3], assigned_len + 3, "\"%s\"", assigned_var);
    func.name = full_func_name;
    func.hash = sl_hash_string(full_func_name);
    func.scope_lifetime = scope;
    sl_add_raw_func(code, &func);
    free(full_func_name);
  }
  return_var.vals = strdup(collections.collections[collec_index].name);
  return_var.type = STRING;
  return return_var;
}

struct SL_Variable collections_set_attr_fn(struct SL_Code *code,
                                           struct SL_L_Function func,
                                           struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  struct SL_Variable self = sl_get_argument(*code, func, 0);
  struct SL_Variable attr_name = sl_get_argument(*code, func, 1);
  struct SL_Variable attr_val = sl_get_argument(*code, func, 2);

  int scope = sl_get_scope(code);
  if (scope > 1)
    scope -= 1;

  char *self_n = sl_string_getter(self.vals);
  char *attr_name_r = sl_string_getter(attr_name.vals);

  int total_size = strlen(attr_name_r) + strlen(self_n);
  char *full_var_name = malloc(total_size + 2);
  snprintf(full_var_name, total_size + 2, "%s.%s", self_n, attr_name_r);
  struct SL_Variable var = {0};
  var = attr_val;
  var.name = full_var_name;
  var.hash = sl_hash_string(full_var_name);
  var.scope_lifetime = scope;
  sl_add_var(code, var);
  free(full_var_name);
  free(self_n);
  free(attr_name_r);
  return return_var;
}

struct SL_Variable collections_get_attr_fn(struct SL_Code *code,
                                           struct SL_L_Function func,
                                           struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  struct SL_Variable self = sl_get_argument(*code, func, 0);
  struct SL_Variable attr_name = sl_get_argument(*code, func, 1);

  int scope = sl_get_scope(code);
  if (scope > 1)
    scope -= 1;

  char *self_n = sl_string_getter(self.vals);
  char *attr_name_r = sl_string_getter(attr_name.vals);

  int total_size = strlen(attr_name_r) + strlen(self_n);
  char *full_var_name = malloc(total_size + 2);
  snprintf(full_var_name, total_size + 2, "%s.%s", self_n, attr_name_r);
  struct SL_Variable *ref_var = sl_get_var(code, full_var_name);
  return_var = sl_copy_variable(*ref_var);
  if (return_var.name != NULL) {
    free(return_var.name);
    return_var.name = NULL;
  }

  free(full_var_name);
  free(self_n);
  free(attr_name_r);
  return return_var;
}

struct SL_Variable collections_create_collection_fn(struct SL_Code *code,
                                                    struct SL_L_Function func,
                                                    struct SL_Function rfunc) {
  if (func.total_arguments < 0) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at Collections.create_collection! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable return_var = {0};
  char *assigned_var = sl_get_assignment_var();

  int scope = sl_get_scope(code);
  if (scope > 1)
    scope -= 1;

  if (collections.size >= collections.capacity) {
    collections.capacity *= 2;
    void *tmp = realloc(collections.collections,
                        collections.capacity * sizeof(struct SL_Collection));
    if (!tmp)
      perror("realloc failed on collections");
    collections.collections = tmp;
  }

  int index = collections.size;
  struct SL_Variable name_item = sl_get_argument(*code, func, 0);
  collections.collections[index].attrs = calloc(SL_INIT, sizeof(char *));
  collections.collections[index].functions =
      calloc(SL_INIT, sizeof(struct SL_Function));

  char *raw_name = sl_string_getter(name_item.vals);
  collections.collections[index].name = strdup(raw_name);
  for (int i = 1; i < func.total_arguments; i++) {
    struct SL_Variable item = sl_get_argument(*code, func, i);
    if (item.type != STRING) {
      return_var.vals = "All items must be typed as string! on "
                        "Collections.create_collection.";
      return_var.type = ERROR;
      return return_var;
    }
    char *raw_str = sl_string_getter(item.vals);
    if (raw_str[0] == 'v' && raw_str[1] == ':') {
      raw_str = raw_str + 2; /* v:name + 2 = name */
      collections.collections[index]
          .attrs[collections.collections[index].total_attrs++] =
          strdup(raw_str);
    } else if (raw_str[0] == 'f' && raw_str[1] == ':') {
      raw_str = raw_str + 2; /* f:name:link + 2 = name:link */
      char *token = strtok(raw_str, ":");
      char *actual_name = NULL;
      char *link_name = NULL;
      actual_name = strdup(token);
      token = strtok(NULL, ":");
      if (token != NULL)
        link_name = strdup(token);
      else
        link_name = actual_name;

      struct SL_Function *link_func_p = sl_get_func(code, actual_name);
      struct SL_Function link_func = sl_copy_function(*link_func_p);
      /* [var] [self] [=] ["attr_name"] 4 more tokens */
      link_func.code_tokens = realloc(
          link_func.code_tokens, (link_func.code_len + 4) * sizeof(char *));
      link_func.types = realloc(link_func.types, (link_func.code_len + 4) *
                                                     sizeof(enum TokenTypes));
      memmove(link_func.code_tokens + 4, link_func.code_tokens,
              link_func.code_len * sizeof(char *));
      link_func.code_tokens[0] = strdup("var");
      link_func.code_tokens[1] = strdup("self");
      link_func.code_tokens[2] = strdup("=");
      link_func.code_len += 4;
      if (link_func.name != NULL)
        free(link_func.name);

      link_func.name = strdup(link_name);
      collections.collections[index]
          .functions[collections.collections[index].total_funcs++] = link_func;
      if (link_name != actual_name) {
        free(link_name);
      }
      free(actual_name);
    }
    free(raw_str - 2);
  }
  int total_len = strlen(raw_name) + strlen(":new");
  char *collection_new_name = malloc(total_len + 1);
  snprintf(collection_new_name, total_len + 1, "%s:new", raw_name);
  sl_add_func(code, collection_new_name, collections_new_collection_fn);
  free(raw_name);
  free(collection_new_name);
  collections.size++;
  return return_var;
}

/* ENUMS */
struct SL_Variable enums_create_enum_fn(struct SL_Code *code,
                                        struct SL_L_Function func,
                                        struct SL_Function rfunc) {
  if (func.total_arguments < 0) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at Collections.create_collection! Not enough arguments.";
    return return_var;
  }
  int scope = sl_get_scope(code);
  if (scope > 1)
    scope -= 1;

  struct SL_Variable return_var = {0};

  for (int i = 0; i < func.total_arguments; i++) {
    struct SL_Variable item = sl_get_argument(*code, func, i);
    char *raw_name = sl_string_getter(item.vals);
    struct SL_Variable var = {0};
    int len = strlen(raw_name) + 2;
    char *name = malloc(len);
    snprintf(name, len, "%s;", raw_name);
    var.name = name;
    var.hash = sl_hash_string(name);
    var.vali = rand();
    var.type = INTEGER;
    var.scope_lifetime = scope;
    sl_add_var(code, var);
    free(name);
    free(raw_name);
  }
  return return_var;
}

/* NET */
#ifdef _WIN32
struct SL_Variable net_new_socket_win_fn(struct SL_Code *code,
                                         struct SL_L_Function func,
                                         struct SL_Function rfunc) {
  if (func.total_arguments < 3) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.new_socket! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable third_arg = sl_get_argument(*code, func, 2);
  struct SL_Variable return_var = {0};
  if (first_arg.type != INTEGER || second_arg.type != INTEGER ||
      third_arg.type != INTEGER) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "All items must be typed as integer! on net.new_socket.";
    return return_var;
  }

  SOCKET sock = socket(first_arg.vali, second_arg.vali, third_arg.vali);
  if (sock == INVALID_SOCKET) {
    return_var.type = ERROR;
    return_var.vals = "Socket creation failed.";
    return return_var;
  }
  return_var.type = INTEGER;
  return_var.vali = (int)sock;
  return return_var;
}

struct SL_Variable net_bind_win_fn(struct SL_Code *code,
                                   struct SL_L_Function func,
                                   struct SL_Function rfunc) {
  if (func.total_arguments < 3) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.bind! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable third_arg = sl_get_argument(*code, func, 2);
  int complex_mode = 0;
  if (func.total_arguments > 3) {
    complex_mode = 1;
  }
  struct SL_Variable return_var = {0};
  return_var.type = INTEGER;
  return_var.vali = 1;

  if (complex_mode == 0) {
    if (first_arg.type != INTEGER || second_arg.type != STRING ||
        third_arg.type != INTEGER) {
      struct SL_Variable return_var = {0};
      return_var.type = ERROR;
      return_var.vals = "All items must be typed as socket_fd,string,integer "
                        "on basic usage net.bind";
      return return_var;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((u_short)third_arg.vali);
    char *raw_ip = sl_string_getter(second_arg.vals);

    if (inet_pton(AF_INET, raw_ip, &server_addr.sin_addr) <= 0) {
      return_var.type = ERROR;
      return_var.vals = "Undefined ip address or format.";
      free(raw_ip);
      return return_var;
    }

    free(raw_ip);

    int bd = bind((SOCKET)first_arg.vali, (struct sockaddr *)&server_addr,
                  sizeof(server_addr));

    if (bd == SOCKET_ERROR) {
      return_var.type = ERROR;
      return_var.vals = "Socket failed to connect port.";
      return return_var;
    }

  } else {
    struct SL_Variable fourth_arg = sl_get_argument(*code, func, 3);
    if (first_arg.type != INTEGER || second_arg.type != INTEGER ||
        third_arg.type != INTEGER || fourth_arg.type != INTEGER) {
      struct SL_Variable return_var = {0};
      return_var.type = ERROR;
      return_var.vals = "All items must be typed as "
                        "socket_fd,integer,integer,integer on complex net.bind";
      return return_var;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = (short)second_arg.vali;
    server_addr.sin_port = htons((u_short)fourth_arg.vali);
    server_addr.sin_addr.s_addr = third_arg.vali;

    int bd = bind((SOCKET)first_arg.vali, (struct sockaddr *)&server_addr,
                  sizeof(server_addr));
    if (bd == SOCKET_ERROR) {
      return_var.type = ERROR;
      return_var.vals = "Socket failed to connect port.";
      return return_var;
    }
  }
  return return_var;
}

struct SL_Variable net_listen_win_fn(struct SL_Code *code,
                                     struct SL_L_Function func,
                                     struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.listen! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable return_var = {0};
  if (first_arg.type != INTEGER || second_arg.type != INTEGER) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals =
        "All items must be typed as socket_fd and integer! on net.listen.";
    return return_var;
  }

  if (listen((SOCKET)first_arg.vali, second_arg.vali) == SOCKET_ERROR) {
    return_var.type = ERROR;
    return_var.vals = "Socket failed to listen.";
  }
  return return_var;
}

struct SL_Variable net_accept_win_fn(struct SL_Code *code,
                                     struct SL_L_Function func,
                                     struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.accept! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};
  struct sockaddr_in client_addr;
  if (first_arg.type != INTEGER) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals =
        "First argument must be typed as socket_fd! on net.accept.";
    return return_var;
  }

  int client_len = sizeof(client_addr);
  SOCKET client_socket = accept((SOCKET)first_arg.vali,
                                (struct sockaddr *)&client_addr, &client_len);

  if (client_socket == INVALID_SOCKET) {
    return_var.type = ERROR;
    return_var.vals = "Socket failed to accept client.";
    return return_var;
  }

  return_var.type = INTEGER;
  return_var.vali = (int)client_socket;
  return return_var;
}

struct SL_Variable net_recv_win_fn(struct SL_Code *code,
                                   struct SL_L_Function func,
                                   struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.recv! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = {0};
  struct SL_Variable return_var = {0};
  int buffer_size = 1024;
  int gotflag = 0;
  struct SL_Variable third_arg = {0};
  if (first_arg.type != INTEGER) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "First argument must be socket_fd on net.recv!";
    return return_var;
  }

  if (func.total_arguments > 1) {
    second_arg = sl_get_argument(*code, func, 1);
    if (second_arg.type != INTEGER) {
      struct SL_Variable return_var = {0};
      return_var.type = ERROR;
      return_var.vals = "Buffer size argument must be integer on net.recv!";
      return return_var;
    }
    buffer_size = second_arg.vali;
    if (func.total_arguments > 2) {
      gotflag = 1;
      third_arg = sl_get_argument(*code, func, 2);
      if (third_arg.type != INTEGER) {
        struct SL_Variable return_var = {0};
        return_var.type = ERROR;
        return_var.vals = "Flag argument must be integer on net.recv!";
        return return_var;
      }
    }
  }

  char *buffer = malloc(buffer_size);
  int read_size = 0;

  if (gotflag == 1)
    read_size =
        recv((SOCKET)first_arg.vali, buffer, buffer_size, third_arg.vali);
  else
    read_size = recv((SOCKET)first_arg.vali, buffer, buffer_size, 0);

  if (read_size == SOCKET_ERROR || read_size < 0) {
    return_var.type = ERROR;
    return_var.vals = "Nothing received from client.";
    free(buffer);
    return return_var;
  }

  return_var.type = STRING;
  buffer[read_size < buffer_size ? read_size : buffer_size - 1] = '\0';
  return_var.vals = strdup(buffer);
  free(buffer);
  return return_var;
}

struct SL_Variable net_send_win_fn(struct SL_Code *code,
                                   struct SL_L_Function func,
                                   struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.send! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  int gotflag = 0;
  struct SL_Variable third_arg = {0};

  if (func.total_arguments > 2) {
    gotflag = 1;
    third_arg = sl_get_argument(*code, func, 2);
    if (third_arg.type != INTEGER) {
      struct SL_Variable return_var = {0};
      return_var.type = ERROR;
      return_var.vals = "Flag argument must be integer on net.send!";
      return return_var;
    }
  }
  struct SL_Variable return_var = {0};
  if (first_arg.type != INTEGER || second_arg.type != STRING) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals =
        "All items must be typed as socket_fd and string on net.send!";
    return return_var;
  }

  char *buffer = sl_string_getter(second_arg.vals);
  if (gotflag == 1)
    send((SOCKET)first_arg.vali, buffer, (int)strlen(buffer), third_arg.vali);
  else
    send((SOCKET)first_arg.vali, buffer, (int)strlen(buffer), 0);

  free(buffer);
  return return_var;
}

struct SL_Variable net_new_fd_win_fn(struct SL_Code *code,
                                     struct SL_L_Function func,
                                     struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.new_fd! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  if (fd_list_size >= fd_list_capacity) {
    fd_list_capacity = (fd_list_capacity == 0) ? 4 : fd_list_capacity * 2;
    void *tmp = realloc(fd_list, fd_list_capacity * sizeof(struct SL_FD_List));
    if (tmp == NULL)
      perror("Realloc failed.");
    fd_list = tmp;
  }
  int id = fd_list_size++;
  fd_list[id].fd = malloc(sizeof(fd_set));
  FD_ZERO((fd_set *)fd_list[id].fd);
  fd_list[id].is_set = 1;
  return_var.type = INTEGER;
  return_var.vali = id;
  return return_var;
}

struct SL_Variable net_zero_fd_win_fn(struct SL_Code *code,
                                      struct SL_L_Function func,
                                      struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.zero_fd! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};
  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "First and second argument must be typed as fd! on net.zero_fd.";
    return return_var;
  }
  if (first_arg.vali >= fd_list_size || first_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "FD Not found!";
    return return_var;
  }

  FD_ZERO((fd_set *)fd_list[first_arg.vali].fd);
  return return_var;
}

struct SL_Variable net_add_fd_win_fn(struct SL_Code *code,
                                     struct SL_L_Function func,
                                     struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.add_fd! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable return_var = {0};
  if (first_arg.type != INTEGER || second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "All arguments must be typed as socket_fd and fd! on net.add_fd.";
    return return_var;
  }
  if (second_arg.vali >= fd_list_size || second_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "FD Not found!";
    return return_var;
  }

  FD_SET((SOCKET)first_arg.vali, (fd_set *)fd_list[second_arg.vali].fd);
  return return_var;
}

struct SL_Variable net_check_fd_win_fn(struct SL_Code *code,
                                       struct SL_L_Function func,
                                       struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.check_fd! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable return_var = {0};
  if (first_arg.type != INTEGER || second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "All arguments must be typed as socket_fd and fd! on net.check_fd.";
    return return_var;
  }
  if (second_arg.vali >= fd_list_size || second_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "FD Not found!";
    return return_var;
  }

  int out =
      FD_ISSET((SOCKET)first_arg.vali, (fd_set *)fd_list[second_arg.vali].fd);
  return_var.type = BOOLEAN;
  return_var.valb = (out != 0) ? 1 : 0;
  return return_var;
}

struct SL_Variable net_remove_fd_win_fn(struct SL_Code *code,
                                        struct SL_L_Function func,
                                        struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.remove_fd! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable return_var = {0};
  if (first_arg.type != INTEGER || second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "All arguments must be typed as socket_fd and fd! on net.remove_fd.";
    return return_var;
  }
  if (second_arg.vali >= fd_list_size || second_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "FD Not found!";
    return return_var;
  }

  FD_CLR((SOCKET)first_arg.vali, (fd_set *)fd_list[second_arg.vali].fd);
  return return_var;
}

struct SL_Variable net_resolve_win_fn(struct SL_Code *code,
                                      struct SL_L_Function func,
                                      struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.resolve! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  if (first_arg.type != STRING) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Argument must be typed as string on net.resolve.";
    return return_var;
  }

  struct SL_Variable return_var = {0};
  char *hostname = sl_string_getter(first_arg.vals);

  struct hostent *he = gethostbyname(hostname);
  free(hostname);

  if (he == NULL || he->h_addr_list[0] == NULL) {
    return_var.type = ERROR;
    return_var.vals = "Could not resolve hostname.";
    return return_var;
  }

  struct in_addr **addr_list = (struct in_addr **)he->h_addr_list;

  return_var.type = STRING;
  return_var.vals = strdup(inet_ntoa(*addr_list[0]));
  return return_var;
}

struct SL_Variable net_set_nonblocking_win_fn(struct SL_Code *code,
                                              struct SL_L_Function func,
                                              struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at net.set_nonblocking! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals = "Argument must be typed as socket_fd (integer).";
    return return_var;
  }

  u_long mode = 1;
  if (ioctlsocket((SOCKET)first_arg.vali, FIONBIO, &mode) != 0) {
    return_var.type = ERROR;
    return_var.vals = "Failed to set socket to non-blocking.";
    return return_var;
  }

  return return_var;
}

struct SL_Variable net_setsockopt_win_fn(struct SL_Code *code,
                                         struct SL_L_Function func,
                                         struct SL_Function rfunc) {
  if (func.total_arguments < 4) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at net.setsockopt! Needs fd, level, optname, optval.";
    return return_var;
  }

  struct SL_Variable arg_fd = sl_get_argument(*code, func, 0);
  struct SL_Variable arg_level = sl_get_argument(*code, func, 1);
  struct SL_Variable arg_opt = sl_get_argument(*code, func, 2);
  struct SL_Variable arg_val = sl_get_argument(*code, func, 3);

  struct SL_Variable return_var = {0};

  if (arg_fd.type != INTEGER || arg_level.type != INTEGER ||
      arg_opt.type != INTEGER || arg_val.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals = "All arguments must be integers for net.setsockopt.";
    return return_var;
  }

  int optval = arg_val.vali;
  if (setsockopt((SOCKET)arg_fd.vali, arg_level.vali, arg_opt.vali,
                 (const char *)&optval, sizeof(optval)) == SOCKET_ERROR) {
    return_var.type = ERROR;
    return_var.vals = "setsockopt failed.";
    perror("net.setsockopt failed!");
    return return_var;
  }

  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable net_connect_win_fn(struct SL_Code *code,
                                      struct SL_L_Function func,
                                      struct SL_Function rfunc) {
  if (func.total_arguments < 3) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.connect! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable third_arg = sl_get_argument(*code, func, 2);
  int complex_mode = 0;
  if (func.total_arguments > 3) {
    complex_mode = 1;
  }
  struct SL_Variable return_var = {0};

  if (complex_mode == 0) {
    if (first_arg.type != INTEGER || second_arg.type != STRING ||
        third_arg.type != INTEGER) {
      struct SL_Variable return_var = {0};
      return_var.type = ERROR;
      return_var.vals = "All items must be typed as integer,string,integer on "
                        "basic usage net.connect";
      return return_var;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((u_short)third_arg.vali);
    char *raw_ip = sl_string_getter(second_arg.vals);

    if (inet_pton(AF_INET, raw_ip, &server_addr.sin_addr) <= 0) {
      return_var.type = ERROR;
      return_var.vals = "Undefined ip address or format.";
      free(raw_ip);
      return return_var;
    }

    free(raw_ip);

    int bd = connect((SOCKET)first_arg.vali, (struct sockaddr *)&server_addr,
                     sizeof(server_addr));

    if (bd == SOCKET_ERROR) {
      return_var.type = ERROR;
      return_var.vals = "Socket failed to connect port.";
      return return_var;
    }

  } else {
    struct SL_Variable fourth_arg = sl_get_argument(*code, func, 3);
    if (first_arg.type != INTEGER || second_arg.type != INTEGER ||
        third_arg.type != INTEGER || fourth_arg.type != INTEGER) {
      struct SL_Variable return_var = {0};
      return_var.type = ERROR;
      return_var.vals =
          "All items must be typed as socket_fd,integer,integer,integer on "
          "complex net.connect";
      return return_var;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = (short)second_arg.vali;
    server_addr.sin_port = htons((u_short)fourth_arg.vali);
    server_addr.sin_addr.s_addr = third_arg.vali;

    int bd = connect((SOCKET)first_arg.vali, (struct sockaddr *)&server_addr,
                     sizeof(server_addr));
    if (bd == SOCKET_ERROR) {
      return_var.type = ERROR;
      return_var.vals = "Socket failed to connect port.";
      return return_var;
    }
  }
  return return_var;
}

struct SL_Variable net_close_win_fn(struct SL_Code *code,
                                    struct SL_L_Function func,
                                    struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.close! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  closesocket((SOCKET)first_arg.vali);
  return return_var;
}

#else
struct SL_Variable net_new_socket_posix_fn(struct SL_Code *code,
                                           struct SL_L_Function func,
                                           struct SL_Function rfunc) {
  if (func.total_arguments < 3) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.new_socket! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable third_arg = sl_get_argument(*code, func, 2);
  struct SL_Variable return_var = {0};
  if (first_arg.type != INTEGER || second_arg.type != INTEGER ||
      third_arg.type != INTEGER) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "All items must be typed as integer! on net.new_socket.";
    return return_var;
  }

  int sock = socket(first_arg.vali, second_arg.vali, third_arg.vali);
  if (sock < 0) {
    return_var.type = ERROR;
    return_var.vals = "Socket creation failed.";
    perror("net.socket failed!");
    return return_var;
  }
  return_var.type = INTEGER;
  return_var.vali = sock;
  return return_var;
}

struct SL_Variable net_bind_posix_fn(struct SL_Code *code,
                                     struct SL_L_Function func,
                                     struct SL_Function rfunc) {
  if (func.total_arguments < 3) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.bind! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable third_arg = sl_get_argument(*code, func, 2);
  int complex_mode = 0;
  if (func.total_arguments > 3) {
    complex_mode = 1;
  }
  struct SL_Variable return_var = {0};

  if (complex_mode == 0) {
    if (first_arg.type != INTEGER || second_arg.type != STRING ||
        third_arg.type != INTEGER) {
      struct SL_Variable return_var = {0};
      return_var.type = ERROR;
      return_var.vals = "All items must be typed as socket_fd,string,integer "
                        "on basic usage net.bind";
      return return_var;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(third_arg.vali);
    char *raw_ip = sl_string_getter(second_arg.vals);
    if (inet_pton(AF_INET, raw_ip, &server_addr.sin_addr) <= 0) {
      return_var.type = ERROR;
      return_var.vals = "Undefined ip address or format.";
      return return_var;
    }

    free(raw_ip);

    int bd = bind(first_arg.vali, (struct sockaddr *)&server_addr,
                  sizeof(server_addr));

    if (bd < 0) {
      return_var.type = ERROR;
      return_var.vals = "Socket failed to connect port.";
      perror("net.bind failed!");
      return return_var;
    }

  } else {
    struct SL_Variable fourth_arg = sl_get_argument(*code, func, 3);
    if (first_arg.type != INTEGER || second_arg.type != INTEGER ||
        third_arg.type != INTEGER || fourth_arg.type != INTEGER) {
      struct SL_Variable return_var = {0};
      return_var.type = ERROR;
      return_var.vals = "All items must be typed as "
                        "socket_fd,integer,integer,integer on complex net.bind";
      return return_var;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = second_arg.vali;
    server_addr.sin_port = htons(fourth_arg.vali);
    server_addr.sin_addr.s_addr = third_arg.vali;
    int bd = bind(first_arg.vali, (struct sockaddr *)&server_addr,
                  sizeof(server_addr));
    if (bd < 0) {
      return_var.type = ERROR;
      return_var.vals = "Socket failed to connect port.";
      perror("net.bind failed!");
      return return_var;
    }
  }
  return return_var;
}

struct SL_Variable net_listen_posix_fn(struct SL_Code *code,
                                       struct SL_L_Function func,
                                       struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.listen! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable return_var = {0};
  if (first_arg.type != INTEGER || second_arg.type != INTEGER) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals =
        "All items must be typed as socket_fd and integer! on net.listen.";
    return return_var;
  }

  if (listen(first_arg.vali, second_arg.vali) < 0) {
    return_var.type = ERROR;
    return_var.vals = "Socket failed to listen.";
  }
  return return_var;
}

struct SL_Variable net_accept_posix_fn(struct SL_Code *code,
                                       struct SL_L_Function func,
                                       struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.accept! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  int client_socket =
      accept(first_arg.vali, (struct sockaddr *)&client_addr, &client_len);
  if (client_socket < 0) {
    return_var.type = ERROR;
    return_var.vals = "Socket failed to accept client.";
    return return_var;
  }
  return_var.type = INTEGER;
  return_var.vali = client_socket;
  return return_var;
}

struct SL_Variable net_recv_posix_fn(struct SL_Code *code,
                                     struct SL_L_Function func,
                                     struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.recv! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  if (first_arg.type != INTEGER) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "First argument must be typed as socket_fd! on net.recv.";
    return return_var;
  }

  struct SL_Variable second_arg = {0};
  struct SL_Variable return_var = {0};
  int buffer_size = 1024;
  int gotflag = 0;
  struct SL_Variable third_arg = {0};
  if (func.total_arguments > 1) {
    second_arg = sl_get_argument(*code, func, 1);
    if (second_arg.type != INTEGER) {
      struct SL_Variable return_var = {0};
      return_var.type = ERROR;
      return_var.vals =
          "Second argument must be typed as integer! on net.recv.";
      return return_var;
    }
    buffer_size = second_arg.vali;
    if (func.total_arguments > 2) {
      gotflag = 1;
      if (second_arg.type != INTEGER) {
        struct SL_Variable return_var = {0};
        return_var.type = ERROR;
        return_var.vals = "Flag argument must be typed as int! on net.recv.";
        return return_var;
      }
      third_arg = sl_get_argument(*code, func, 2);
    }
  }
  char *buffer = malloc(buffer_size);
  int read_size = 0;
  if (gotflag == 1)
    read_size = recv(first_arg.vali, buffer, buffer_size, third_arg.vali);
  else
    read_size = recv(first_arg.vali, buffer, buffer_size, 0);
  if (read_size < 0) {
    return_var.type = ERROR;
    return_var.vals = "Nothing received from client.";
    return return_var;
  }
  return_var.type = STRING;
  return_var.vals = strdup(buffer);
  free(buffer);
  return return_var;
}

struct SL_Variable net_new_fd_posix_fn(struct SL_Code *code,
                                       struct SL_L_Function func,
                                       struct SL_Function rfunc) {
  if (func.total_arguments < 0) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.new_fd! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  if (fd_list_size >= fd_list_capacity) {
    fd_list_capacity *= 2;
    void *tmp = realloc(fd_list, fd_list_capacity * sizeof(struct SL_FD_List));
    if (tmp == NULL)
      perror("Realloc failed.");
    fd_list = tmp;
  }
  int id = fd_list_size++;
  fd_list[id].fd = malloc(sizeof(fd_set));
  FD_ZERO(fd_list[id].fd);
  fd_list[id].is_set = 1;
  return_var.type = INTEGER;
  return_var.vali = id;
  return return_var;
}

struct SL_Variable net_zero_fd_posix_fn(struct SL_Code *code,
                                        struct SL_L_Function func,
                                        struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.zero_fd! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};
  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "First and second argument must be typed as fd! on net.zero_fd.";
    return return_var;
  }
  if (first_arg.vali > fd_list_size) {
    return_var.type = ERROR;
    return_var.vals = "FD Not found!";
    return return_var;
  }

  FD_ZERO(fd_list[first_arg.vali].fd);
  return return_var;
}

struct SL_Variable net_add_fd_posix_fn(struct SL_Code *code,
                                       struct SL_L_Function func,
                                       struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.add_fd! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable return_var = {0};
  if (first_arg.type != INTEGER || second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "All arguments must be typed as socket_fd and fd! on net.add_fd.";
    return return_var;
  }
  if (second_arg.vali < 0 || second_arg.vali >= fd_list_size) {
    return_var.type = ERROR;
    return_var.vals = "FD Not found!";
    return return_var;
  }

  FD_SET(first_arg.vali, fd_list[second_arg.vali].fd);
  return return_var;
}

struct SL_Variable net_check_fd_posix_fn(struct SL_Code *code,
                                         struct SL_L_Function func,
                                         struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.check_fd! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable return_var = {0};
  if (first_arg.type != INTEGER || second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "All arguments must be typed as socket_fd and fd! on net.check_fd.";
    return return_var;
  }
  if (second_arg.vali < 0 || second_arg.vali >= fd_list_size) {
    return_var.type = ERROR;
    return_var.vals = "FD Not found!";
    return return_var;
  }

  int out = FD_ISSET(first_arg.vali, fd_list[second_arg.vali].fd);
  return_var.type = BOOLEAN;
  return_var.valb = (out != 0) ? 1 : 0;
  return return_var;
}

struct SL_Variable net_remove_fd_posix_fn(struct SL_Code *code,
                                          struct SL_L_Function func,
                                          struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.remove_fd! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable return_var = {0};
  if (first_arg.type != INTEGER || second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "All arguments must be typed as socket_fd and fd! on net.remove_fd.";
    return return_var;
  }
  if (second_arg.vali < 0 || second_arg.vali >= fd_list_size) {
    return_var.type = ERROR;
    return_var.vals = "FD Not found!";
    return return_var;
  }

  FD_CLR(first_arg.vali, fd_list[second_arg.vali].fd);
  return return_var;
}

struct SL_Variable net_resolve_posix_fn(struct SL_Code *code,
                                        struct SL_L_Function func,
                                        struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.resolve! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  if (first_arg.type != STRING) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Argument must be typed as string on net.resolve.";
    return return_var;
  }

  struct SL_Variable return_var = {0};
  char *hostname = sl_string_getter(first_arg.vals);

  struct hostent *he = gethostbyname(hostname);
  free(hostname);

  if (he == NULL || he->h_addr_list[0] == NULL) {
    return_var.type = ERROR;
    return_var.vals = "Could not resolve hostname.";
    return return_var;
  }

  struct in_addr **addr_list = (struct in_addr **)he->h_addr_list;

  return_var.type = STRING;
  return_var.vals = strdup(inet_ntoa(*addr_list[0]));
  return return_var;
}

struct SL_Variable net_set_nonblocking_posix_fn(struct SL_Code *code,
                                                struct SL_L_Function func,
                                                struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at net.set_nonblocking! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals = "Argument must be typed as socket_fd (integer).";
    return return_var;
  }

  int flags = fcntl(first_arg.vali, F_GETFL, 0);
  if (flags == -1) {
    return_var.type = ERROR;
    return_var.vals = "Failed to get socket flags.";
    return return_var;
  }

  if (fcntl(first_arg.vali, F_SETFL, flags | O_NONBLOCK) == -1) {
    return_var.type = ERROR;
    return_var.vals = "Failed to set socket to non-blocking.";
    return return_var;
  }

  return return_var;
}

struct SL_Variable net_setsockopt_posix_fn(struct SL_Code *code,
                                           struct SL_L_Function func,
                                           struct SL_Function rfunc) {
  if (func.total_arguments < 4) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at net.setsockopt! Needs fd, level, optname, optval.";
    return return_var;
  }

  struct SL_Variable arg_fd = sl_get_argument(*code, func, 0);
  struct SL_Variable arg_level = sl_get_argument(*code, func, 1);
  struct SL_Variable arg_opt = sl_get_argument(*code, func, 2);
  struct SL_Variable arg_val = sl_get_argument(*code, func, 3);

  struct SL_Variable return_var = {0};

  if (arg_fd.type != INTEGER || arg_level.type != INTEGER ||
      arg_opt.type != INTEGER || arg_val.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals = "All arguments must be integers for net.setsockopt.";
    return return_var;
  }

  int optval = arg_val.vali;
  if (setsockopt(arg_fd.vali, arg_level.vali, arg_opt.vali, &optval,
                 sizeof(optval)) < 0) {
    return_var.type = ERROR;
    return_var.vals = "setsockopt failed.";
    perror("net.setsockopt failed!");
    return return_var;
  }

  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable net_send_posix_fn(struct SL_Code *code,
                                     struct SL_L_Function func,
                                     struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.send! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  if (first_arg.type != INTEGER || second_arg.type != STRING) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "First and second argument must be typed as socket_fd "
                      "and integer! on net.send.";
    return return_var;
  }
  int gotflag = 0;
  struct SL_Variable third_arg = {0};
  if (func.total_arguments > 2) {
    gotflag = 1;
    third_arg = sl_get_argument(*code, func, 2);
    if (third_arg.type != INTEGER) {
      struct SL_Variable return_var = {0};
      return_var.type = ERROR;
      return_var.vals = "Flag argument must be typed as integer! on net.send.";
      return return_var;
    }
  }
  struct SL_Variable return_var = {0};

  char *buffer = sl_string_getter(second_arg.vals);
  if (gotflag == 1)
    send(first_arg.vali, buffer, strlen(buffer), third_arg.vali);
  else
    send(first_arg.vali, buffer, strlen(buffer), 0);
  free(buffer);
  return return_var;
}

struct SL_Variable net_connect_posix_fn(struct SL_Code *code,
                                        struct SL_L_Function func,
                                        struct SL_Function rfunc) {
  if (func.total_arguments < 3) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.connect! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable third_arg = sl_get_argument(*code, func, 2);
  int complex_mode = 0;
  if (func.total_arguments > 3) {
    complex_mode = 1;
  }
  struct SL_Variable return_var = {0};

  if (complex_mode == 0) {
    if (first_arg.type != INTEGER || second_arg.type != STRING ||
        third_arg.type != INTEGER) {
      struct SL_Variable return_var = {0};
      return_var.type = ERROR;
      return_var.vals = "All items must be typed as integer,string,integer on "
                        "basic usage net.connect";
      return return_var;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(third_arg.vali);
    char *raw_ip = sl_string_getter(second_arg.vals);
    if (inet_pton(AF_INET, raw_ip, &server_addr.sin_addr) <= 0) {
      return_var.type = ERROR;
      return_var.vals = "Undefined ip address or format.";
      return return_var;
    }

    free(raw_ip);

    int bd = connect(first_arg.vali, (struct sockaddr *)&server_addr,
                     sizeof(server_addr));

    if (bd < 0) {
      return_var.type = ERROR;
      return_var.vals = "Socket failed to connect port.";
      perror("net.connect failed!");
      return return_var;
    }

  } else {
    struct SL_Variable fourth_arg = sl_get_argument(*code, func, 3);
    if (first_arg.type != INTEGER || second_arg.type != INTEGER ||
        third_arg.type != INTEGER || fourth_arg.type != INTEGER) {
      struct SL_Variable return_var = {0};
      return_var.type = ERROR;
      return_var.vals =
          "All items must be typed as integer,integer,integer,integer on "
          "complex net.connect";
      return return_var;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = second_arg.vali;
    server_addr.sin_port = htons(fourth_arg.vali);
    server_addr.sin_addr.s_addr = third_arg.vali;
    int bd = connect(first_arg.vali, (struct sockaddr *)&server_addr,
                     sizeof(server_addr));
    if (bd < 0) {
      return_var.type = ERROR;
      return_var.vals = "Socket failed to connect port.";
      perror("net.bind failed!");
      return return_var;
    }
  }
  return return_var;
}

struct SL_Variable net_close_posix_fn(struct SL_Code *code,
                                      struct SL_L_Function func,
                                      struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.close! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  close(first_arg.vali);
  return return_var;
}

#endif

int used_io = 0;
int used_file = 0;
int used_types = 0;
int used_sys = 0;
int used_string = 0;
int used_errors = 0;
int used_list = 0;
int used_extra = 0;
int used_db = 0;
int used_collections = 0;
int used_enums = 0;
int used_net = 0;

struct SL_Variable use_fn(struct SL_Code *code, struct SL_L_Function func,
                          struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at use! Not enough arguments.";
    return return_var;
  }
  for (int i = 0; i < func.total_arguments; i++) {
    struct SL_Variable lib = sl_get_argument(*code, func, i);
    char *libstr = sl_string_getter(lib.vals);

    if (strcmp(libstr, "io") == 0 && used_io == 0) {
      used_io = 1;
      sl_add_func(code, "io.print", print_fn);
      sl_add_func(code, "io.input", input_fn);
      sl_add_func(code, "io.getchar", io_getchar_fn);
      sl_add_func(code, "io.fflush", io_fflush_fn);
    } else if (strcmp(libstr, "file") == 0 && used_file == 0) {
      used_file = 1;
      sl_add_func(code, "file.read_to_str", file_read_to_str_fn);
      sl_add_func(code, "file.write_from_str", file_write_from_str_fn);
      sl_add_func(code, "file.append_from_str", file_append_from_str_fn);
    } else if (strcmp(libstr, "types") == 0 && used_types == 0) {
      used_types = 1;
      /* CONVERT */
      sl_add_func(code, "types.str_to_int", str_to_int_fn);
      sl_add_func(code, "types.int_to_char", int_to_char_fn);
      sl_add_func(code, "types.char_to_int", char_to_int_fn);
      sl_add_func(code, "types.char_to_str", char_to_str_fn);
      sl_add_func(code, "types.int_to_str", int_to_str_fn);

      /* TYPE CHECK */
      sl_add_func(code, "types.is_int", is_int_fn);
      sl_add_func(code, "types.is_char", is_char_fn);
      sl_add_func(code, "types.is_string", is_string_fn);
      sl_add_func(code, "types.is_double", is_double_fn);
      sl_add_func(code, "types.is_not_initialized", is_not_initialized_fn);
      sl_add_func(code, "types.typeof", typeof_fn);

      /* STRING TYPE CHECK */
      sl_add_func(code, "types.is_digit", is_digit_fn);
      sl_add_func(code, "types.is_space", is_space_fn);

    } else if (strcmp(libstr, "sys") == 0 && used_sys == 0) {
      used_sys = 1;
      sl_add_func(code, "sys.get_arg", sys_get_arg_fn);
      sl_add_func(code, "sys.exit", sys_exit_fn);
    } else if (strcmp(libstr, "errors") == 0 && used_errors == 0) {
      used_errors = 1;
      sl_add_func(code, "errors.string", errors_string_fn);
      sl_add_func(code, "errors.bool", errors_bool_fn);
      sl_add_func(code, "errors.panic", errors_panic_fn);
    } else if (strcmp(libstr, "collections") == 0 && used_collections == 0) {
      used_collections = 1;
      collections.collections = calloc(SL_INIT, sizeof(struct SL_Collection));
      collections.size = 0;
      collections.capacity = SL_INIT;

      sl_add_func(code, "Collections.create_collection",
                  collections_create_collection_fn);
      sl_add_func(code, "Collections.set_attr", collections_set_attr_fn);
      sl_add_func(code, "Collections.get_attr", collections_get_attr_fn);
    } else if (strcmp(libstr, "enums") == 0 && used_enums == 0) {
      used_enums = 1;
      sl_add_func(code, "Enums.create_enum", enums_create_enum_fn);
    } else if (strcmp(libstr, "net") == 0 && used_net == 0) {
      used_net = 1;
      fd_list_capacity = SL_INIT;
      fd_list = calloc(fd_list_capacity, sizeof(struct SL_FD_List));
      fd_list_size = 0;
      /* FIXEDS */
#ifdef AF_UNSPEC
      sl_add_fixed_int(code, "AF_UNSPEC", AF_UNSPEC);
#endif

#ifdef AF_UNIX
      sl_add_fixed_int(code, "AF_UNIX", AF_UNIX);
#endif

#ifdef AF_INET
      sl_add_fixed_int(code, "AF_INET", AF_INET);
#endif

#ifdef AF_AX25
      sl_add_fixed_int(code, "AF_AX25", AF_AX25);
#endif

#ifdef AF_IPX
      sl_add_fixed_int(code, "AF_IPX", AF_IPX);
#endif

#ifdef AF_APPLETALK
      sl_add_fixed_int(code, "AF_APPLETALK", AF_APPLETALK);
#endif

#ifdef AF_NETROM
      sl_add_fixed_int(code, "AF_NETROM", AF_NETROM);
#endif

#ifdef AF_BRIDGE
      sl_add_fixed_int(code, "AF_BRIDGE", AF_BRIDGE);
#endif

#ifdef AF_ATMPVC
      sl_add_fixed_int(code, "AF_ATMPVC", AF_ATMPVC);
#endif

#ifdef AF_X25
      sl_add_fixed_int(code, "AF_X25", AF_X25);
#endif

#ifdef AF_INET6
      sl_add_fixed_int(code, "AF_INET6", AF_INET6);
#endif

#ifdef AF_ROSE
      sl_add_fixed_int(code, "AF_ROSE", AF_ROSE);
#endif

#ifdef AF_DECnet
      sl_add_fixed_int(code, "AF_DECnet", AF_DECnet);
#endif

#ifdef AF_NETBEUI
      sl_add_fixed_int(code, "AF_NETBEUI", AF_NETBEUI);
#endif

#ifdef AF_SECURITY
      sl_add_fixed_int(code, "AF_SECURITY", AF_SECURITY);
#endif

#ifdef AF_KEY
      sl_add_fixed_int(code, "AF_KEY", AF_KEY);
#endif

#ifdef AF_NETLINK
      sl_add_fixed_int(code, "AF_NETLINK", AF_NETLINK);
#endif

#ifdef AF_ROUTE
      sl_add_fixed_int(code, "AF_ROUTE", AF_ROUTE);
#endif

#ifdef AF_PACKET
      sl_add_fixed_int(code, "AF_PACKET", AF_PACKET);
#endif

#ifdef AF_CAN
      sl_add_fixed_int(code, "AF_CAN", AF_CAN);
#endif

#ifdef AF_BLUETOOTH
      sl_add_fixed_int(code, "AF_BLUETOOTH", AF_BLUETOOTH);
#endif

#ifdef AF_ALG
      sl_add_fixed_int(code, "AF_ALG", AF_ALG);
#endif

#ifdef AF_VSOCK
      sl_add_fixed_int(code, "AF_VSOCK", AF_VSOCK);
#endif

#ifdef AF_RDS
      sl_add_fixed_int(code, "AF_RDS", AF_RDS);
#endif

#ifdef AF_PPPOX
      sl_add_fixed_int(code, "AF_PPPOX", AF_PPPOX);
#endif

#ifdef AF_LLC
      sl_add_fixed_int(code, "AF_LLC", AF_LLC);
#endif

#ifdef AF_IB
      sl_add_fixed_int(code, "AF_IB", AF_IB);
#endif

#ifdef AF_MPLS
      sl_add_fixed_int(code, "AF_MPLS", AF_MPLS);
#endif

#ifdef AF_NFC
      sl_add_fixed_int(code, "AF_NFC", AF_NFC);
#endif

#ifdef AF_TIPC
      sl_add_fixed_int(code, "AF_TIPC", AF_TIPC);
#endif

#ifdef AF_RXRPC
      sl_add_fixed_int(code, "AF_RXRPC", AF_RXRPC);
#endif

#ifdef AF_ISDN
      sl_add_fixed_int(code, "AF_ISDN", AF_ISDN);
#endif

#ifdef AF_PHONET
      sl_add_fixed_int(code, "AF_PHONET", AF_PHONET);
#endif

#ifdef AF_IEEE802154
      sl_add_fixed_int(code, "AF_IEEE802154", AF_IEEE802154);
#endif

#ifdef AF_CAIF
      sl_add_fixed_int(code, "AF_CAIF", AF_CAIF);
#endif

#ifdef AF_NATIONAL
      sl_add_fixed_int(code, "AF_NATIONAL", AF_NATIONAL);
#endif

#ifdef AF_SNA
      sl_add_fixed_int(code, "AF_SNA", AF_SNA);
#endif

#ifdef AF_IRDA
      sl_add_fixed_int(code, "AF_IRDA", AF_IRDA);
#endif

#ifdef AF_BTH
      sl_add_fixed_int(code, "AF_BTH", AF_BTH);
#endif

#ifdef SOCK_STREAM
      sl_add_fixed_int(code, "SOCK_STREAM", SOCK_STREAM);
#endif

#ifdef SOCK_DGRAM
      sl_add_fixed_int(code, "SOCK_DGRAM", SOCK_DGRAM);
#endif

#ifdef SOCK_RAW
      sl_add_fixed_int(code, "SOCK_RAW", SOCK_RAW);
#endif

#ifdef SOCK_RDM
      sl_add_fixed_int(code, "SOCK_RDM", SOCK_RDM);
#endif

#ifdef SOCK_SEQPACKET
      sl_add_fixed_int(code, "SOCK_SEQPACKET", SOCK_SEQPACKET);
#endif

#ifdef SOCK_NONBLOCK
      sl_add_fixed_int(code, "SOCK_NONBLOCK", SOCK_NONBLOCK);
#endif

#ifdef SOCK_CLOEXEC
      sl_add_fixed_int(code, "SOCK_CLOEXEC", SOCK_CLOEXEC);
#endif
#ifdef IPPROTO_IP
      sl_add_fixed_int(code, "IPPROTO_IP", IPPROTO_IP);
#endif

#ifdef IPPROTO_ICMP
      sl_add_fixed_int(code, "IPPROTO_ICMP", IPPROTO_ICMP);
#endif

#ifdef IPPROTO_IGMP
      sl_add_fixed_int(code, "IPPROTO_IGMP", IPPROTO_IGMP);
#endif

#ifdef IPPROTO_GGP
      sl_add_fixed_int(code, "IPPROTO_GGP", IPPROTO_GGP);
#endif

#ifdef IPPROTO_TCP
      sl_add_fixed_int(code, "IPPROTO_TCP", IPPROTO_TCP);
#endif

#ifdef IPPROTO_EGP
      sl_add_fixed_int(code, "IPPROTO_EGP", IPPROTO_EGP);
#endif

#ifdef IPPROTO_PUP
      sl_add_fixed_int(code, "IPPROTO_PUP", IPPROTO_PUP);
#endif

#ifdef IPPROTO_UDP
      sl_add_fixed_int(code, "IPPROTO_UDP", IPPROTO_UDP);
#endif

#ifdef IPPROTO_IDP
      sl_add_fixed_int(code, "IPPROTO_IDP", IPPROTO_IDP);
#endif

#ifdef IPPROTO_TP
      sl_add_fixed_int(code, "IPPROTO_TP", IPPROTO_TP);
#endif

#ifdef IPPROTO_DCCP
      sl_add_fixed_int(code, "IPPROTO_DCCP", IPPROTO_DCCP);
#endif

#ifdef IPPROTO_IPV6
      sl_add_fixed_int(code, "IPPROTO_IPV6", IPPROTO_IPV6);
#endif

#ifdef IPPROTO_RSVP
      sl_add_fixed_int(code, "IPPROTO_RSVP", IPPROTO_RSVP);
#endif

#ifdef IPPROTO_GRE
      sl_add_fixed_int(code, "IPPROTO_GRE", IPPROTO_GRE);
#endif

#ifdef IPPROTO_ESP
      sl_add_fixed_int(code, "IPPROTO_ESP", IPPROTO_ESP);
#endif

#ifdef IPPROTO_AH
      sl_add_fixed_int(code, "IPPROTO_AH", IPPROTO_AH);
#endif

#ifdef IPPROTO_ICMPV6
      sl_add_fixed_int(code, "IPPROTO_ICMPV6", IPPROTO_ICMPV6);
#endif

#ifdef IPPROTO_NONE
      sl_add_fixed_int(code, "IPPROTO_NONE", IPPROTO_NONE);
#endif

#ifdef IPPROTO_RAW
      sl_add_fixed_int(code, "IPPROTO_RAW", IPPROTO_RAW);
#endif
#ifdef INADDR_ANY
      sl_add_fixed_int(code, "INADDR_ANY", INADDR_ANY);
#endif

#ifdef INADDR_LOOPBACK
      sl_add_fixed_int(code, "INADDR_LOOPBACK", INADDR_LOOPBACK);
#endif

#ifdef INADDR_BROADCAST
      sl_add_fixed_int(code, "INADDR_BROADCAST", INADDR_BROADCAST);
#endif

#ifdef INADDR_NONE
      sl_add_fixed_int(code, "INADDR_NONE", INADDR_NONE);
#endif
#ifdef SOMAXCONN
      sl_add_fixed_int(code, "SOMAXCONN", SOMAXCONN);
#endif

#ifdef SOL_SOCKET
      sl_add_fixed_int(code, "SOL_SOCKET", SOL_SOCKET);
#endif

#ifdef SO_DEBUG
      sl_add_fixed_int(code, "SO_DEBUG", SO_DEBUG);
#endif

#ifdef SO_ACCEPTCONN
      sl_add_fixed_int(code, "SO_ACCEPTCONN", SO_ACCEPTCONN);
#endif

#ifdef SO_REUSEADDR
      sl_add_fixed_int(code, "SO_REUSEADDR", SO_REUSEADDR);
#endif

#ifdef SO_REUSEPORT
      sl_add_fixed_int(code, "SO_REUSEPORT", SO_REUSEPORT);
#endif

#ifdef SO_REUSE_UNICASTPORT
      sl_add_fixed_int(code, "SO_REUSE_UNICASTPORT", SO_REUSE_UNICASTPORT);
#endif

#ifdef SO_EXCLUSIVEADDRUSE
      sl_add_fixed_int(code, "SO_EXCLUSIVEADDRUSE", SO_EXCLUSIVEADDRUSE);
#endif

#ifdef SO_KEEPALIVE
      sl_add_fixed_int(code, "SO_KEEPALIVE", SO_KEEPALIVE);
#endif

#ifdef SO_DONTROUTE
      sl_add_fixed_int(code, "SO_DONTROUTE", SO_DONTROUTE);
#endif

#ifdef SO_BROADCAST
      sl_add_fixed_int(code, "SO_BROADCAST", SO_BROADCAST);
#endif

#ifdef SO_LINGER
      sl_add_fixed_int(code, "SO_LINGER", SO_LINGER);
#endif

#ifdef SO_DONTLINGER
      sl_add_fixed_int(code, "SO_DONTLINGER", SO_DONTLINGER);
#endif

#ifdef SO_OOBINLINE
      sl_add_fixed_int(code, "SO_OOBINLINE", SO_OOBINLINE);
#endif

#ifdef SO_SNDBUF
      sl_add_fixed_int(code, "SO_SNDBUF", SO_SNDBUF);
#endif

#ifdef SO_RCVBUF
      sl_add_fixed_int(code, "SO_RCVBUF", SO_RCVBUF);
#endif

#ifdef SO_SNDLOWAT
      sl_add_fixed_int(code, "SO_SNDLOWAT", SO_SNDLOWAT);
#endif

#ifdef SO_RCVLOWAT
      sl_add_fixed_int(code, "SO_RCVLOWAT", SO_RCVLOWAT);
#endif

#ifdef SO_SNDTIMEO
      sl_add_fixed_int(code, "SO_SNDTIMEO", SO_SNDTIMEO);
#endif

#ifdef SO_RCVTIMEO
      sl_add_fixed_int(code, "SO_RCVTIMEO", SO_RCVTIMEO);
#endif

#ifdef SO_ERROR
      sl_add_fixed_int(code, "SO_ERROR", SO_ERROR);
#endif

#ifdef SO_TYPE
      sl_add_fixed_int(code, "SO_TYPE", SO_TYPE);
#endif

#ifdef SO_MAX_MSG_SIZE
      sl_add_fixed_int(code, "SO_MAX_MSG_SIZE", SO_MAX_MSG_SIZE);
#endif

#ifdef SO_MAXDG
      sl_add_fixed_int(code, "SO_MAXDG", SO_MAXDG);
#endif

#ifdef SO_MAXPATHDG
      sl_add_fixed_int(code, "SO_MAXPATHDG", SO_MAXPATHDG);
#endif

#ifdef SO_OPENTYPE
      sl_add_fixed_int(code, "SO_OPENTYPE", SO_OPENTYPE);
#endif

#ifdef SO_PROTOCOL_INFOW
      sl_add_fixed_int(code, "SO_PROTOCOL_INFOW", SO_PROTOCOL_INFOW);
#endif

#ifdef SO_GROUP_ID
      sl_add_fixed_int(code, "SO_GROUP_ID", SO_GROUP_ID);
#endif

#ifdef SO_GROUP_PRIORITY
      sl_add_fixed_int(code, "SO_GROUP_PRIORITY", SO_GROUP_PRIORITY);
#endif

#ifdef SO_RANDOMIZE_PORT
      sl_add_fixed_int(code, "SO_RANDOMIZE_PORT", SO_RANDOMIZE_PORT);
#endif
#ifdef IP_OPTIONS
      sl_add_fixed_int(code, "IP_OPTIONS", IP_OPTIONS);
#endif

#ifdef IP_HDRINCL
      sl_add_fixed_int(code, "IP_HDRINCL", IP_HDRINCL);
#endif

#ifdef IP_TOS
      sl_add_fixed_int(code, "IP_TOS", IP_TOS);
#endif

#ifdef IP_TTL
      sl_add_fixed_int(code, "IP_TTL", IP_TTL);
#endif

#ifdef IP_MULTICAST_IF
      sl_add_fixed_int(code, "IP_MULTICAST_IF", IP_MULTICAST_IF);
#endif

#ifdef IP_MULTICAST_TTL
      sl_add_fixed_int(code, "IP_MULTICAST_TTL", IP_MULTICAST_TTL);
#endif

#ifdef IP_MULTICAST_LOOP
      sl_add_fixed_int(code, "IP_MULTICAST_LOOP", IP_MULTICAST_LOOP);
#endif

#ifdef IP_ADD_MEMBERSHIP
      sl_add_fixed_int(code, "IP_ADD_MEMBERSHIP", IP_ADD_MEMBERSHIP);
#endif

#ifdef IP_DROP_MEMBERSHIP
      sl_add_fixed_int(code, "IP_DROP_MEMBERSHIP", IP_DROP_MEMBERSHIP);
#endif

#ifdef IP_ADD_SOURCE_MEMBERSHIP
      sl_add_fixed_int(code, "IP_ADD_SOURCE_MEMBERSHIP",
                       IP_ADD_SOURCE_MEMBERSHIP);
#endif

#ifdef IP_DROP_SOURCE_MEMBERSHIP
      sl_add_fixed_int(code, "IP_DROP_SOURCE_MEMBERSHIP",
                       IP_DROP_SOURCE_MEMBERSHIP);
#endif

#ifdef IP_BLOCK_SOURCE
      sl_add_fixed_int(code, "IP_BLOCK_SOURCE", IP_BLOCK_SOURCE);
#endif

#ifdef IP_UNBLOCK_SOURCE
      sl_add_fixed_int(code, "IP_UNBLOCK_SOURCE", IP_UNBLOCK_SOURCE);
#endif

#ifdef IP_DONTFRAGMENT
      sl_add_fixed_int(code, "IP_DONTFRAGMENT", IP_DONTFRAGMENT);
#endif

#ifdef IP_PKTINFO
      sl_add_fixed_int(code, "IP_PKTINFO", IP_PKTINFO);
#endif

#ifdef IP_RECEIVE_BROADCAST
      sl_add_fixed_int(code, "IP_RECEIVE_BROADCAST", IP_RECEIVE_BROADCAST);
#endif

#ifdef IP_RECVIF
      sl_add_fixed_int(code, "IP_RECVIF", IP_RECVIF);
#endif

#ifdef IP_RECVTTL
      sl_add_fixed_int(code, "IP_RECVTTL", IP_RECVTTL);
#endif

#ifdef IP_UNICAST_IF
      sl_add_fixed_int(code, "IP_UNICAST_IF", IP_UNICAST_IF);
#endif

#ifdef IP_MTU
      sl_add_fixed_int(code, "IP_MTU", IP_MTU);
#endif

#ifdef IP_MTU_DISCOVER
      sl_add_fixed_int(code, "IP_MTU_DISCOVER", IP_MTU_DISCOVER);
#endif

#ifdef IP_IFLIST
      sl_add_fixed_int(code, "IP_IFLIST", IP_IFLIST);
#endif

#ifdef IP_ADD_IFLIST
      sl_add_fixed_int(code, "IP_ADD_IFLIST", IP_ADD_IFLIST);
#endif

#ifdef IP_DEL_IFLIST
      sl_add_fixed_int(code, "IP_DEL_IFLIST", IP_DEL_IFLIST);
#endif

#ifdef IP_GET_IFLIST
      sl_add_fixed_int(code, "IP_GET_IFLIST", IP_GET_IFLIST);
#endif

#ifdef IP_ORIGINAL_ARRIVAL_IF
      sl_add_fixed_int(code, "IP_ORIGINAL_ARRIVAL_IF", IP_ORIGINAL_ARRIVAL_IF);
#endif

#ifdef IP_USER_MTU
      sl_add_fixed_int(code, "IP_USER_MTU", IP_USER_MTU);
#endif

#ifdef IP_WFP_REDIRECT_CONTEXT
      sl_add_fixed_int(code, "IP_WFP_REDIRECT_CONTEXT",
                       IP_WFP_REDIRECT_CONTEXT);
#endif

#ifdef IP_WFP_REDIRECT_RECORDS
      sl_add_fixed_int(code, "IP_WFP_REDIRECT_RECORDS",
                       IP_WFP_REDIRECT_RECORDS);
#endif
#ifdef IPV6_V6ONLY
      sl_add_fixed_int(code, "IPV6_V6ONLY", IPV6_V6ONLY);
#endif

#ifdef IPV6_UNICAST_HOPS
      sl_add_fixed_int(code, "IPV6_UNICAST_HOPS", IPV6_UNICAST_HOPS);
#endif

#ifdef IPV6_UNICAST_IF
      sl_add_fixed_int(code, "IPV6_UNICAST_IF", IPV6_UNICAST_IF);
#endif

#ifdef IPV6_MULTICAST_IF
      sl_add_fixed_int(code, "IPV6_MULTICAST_IF", IPV6_MULTICAST_IF);
#endif

#ifdef IPV6_MULTICAST_HOPS
      sl_add_fixed_int(code, "IPV6_MULTICAST_HOPS", IPV6_MULTICAST_HOPS);
#endif

#ifdef IPV6_MULTICAST_LOOP
      sl_add_fixed_int(code, "IPV6_MULTICAST_LOOP", IPV6_MULTICAST_LOOP);
#endif

#ifdef IPV6_ADD_MEMBERSHIP
      sl_add_fixed_int(code, "IPV6_ADD_MEMBERSHIP", IPV6_ADD_MEMBERSHIP);
#endif

#ifdef IPV6_DROP_MEMBERSHIP
      sl_add_fixed_int(code, "IPV6_DROP_MEMBERSHIP", IPV6_DROP_MEMBERSHIP);
#endif

#ifdef IPV6_JOIN_GROUP
      sl_add_fixed_int(code, "IPV6_JOIN_GROUP", IPV6_JOIN_GROUP);
#endif

#ifdef IPV6_LEAVE_GROUP
      sl_add_fixed_int(code, "IPV6_LEAVE_GROUP", IPV6_LEAVE_GROUP);
#endif

#ifdef IPV6_PKTINFO
      sl_add_fixed_int(code, "IPV6_PKTINFO", IPV6_PKTINFO);
#endif

#ifdef IPV6_HOPLIMIT
      sl_add_fixed_int(code, "IPV6_HOPLIMIT", IPV6_HOPLIMIT);
#endif

#ifdef IPV6_RECVIF
      sl_add_fixed_int(code, "IPV6_RECVIF", IPV6_RECVIF);
#endif

#ifdef IPV6_HDRINCL
      sl_add_fixed_int(code, "IPV6_HDRINCL", IPV6_HDRINCL);
#endif

#ifdef IPV6_MTU
      sl_add_fixed_int(code, "IPV6_MTU", IPV6_MTU);
#endif

#ifdef IPV6_MTU_DISCOVER
      sl_add_fixed_int(code, "IPV6_MTU_DISCOVER", IPV6_MTU_DISCOVER);
#endif

#ifdef IPV6_PROTECTION_LEVEL
      sl_add_fixed_int(code, "IPV6_PROTECTION_LEVEL", IPV6_PROTECTION_LEVEL);
#endif

#ifdef IPV6_IFLIST
      sl_add_fixed_int(code, "IPV6_IFLIST", IPV6_IFLIST);
#endif

#ifdef IPV6_ADD_IFLIST
      sl_add_fixed_int(code, "IPV6_ADD_IFLIST", IPV6_ADD_IFLIST);
#endif

#ifdef IPV6_DEL_IFLIST
      sl_add_fixed_int(code, "IPV6_DEL_IFLIST", IPV6_DEL_IFLIST);
#endif

#ifdef IPV6_GET_IFLIST
      sl_add_fixed_int(code, "IPV6_GET_IFLIST", IPV6_GET_IFLIST);
#endif

#ifdef IPV6_ORIGINAL_ARRIVAL_IF
      sl_add_fixed_int(code, "IPV6_ORIGINAL_ARRIVAL_IF",
                       IPV6_ORIGINAL_ARRIVAL_IF);
#endif

#ifdef IPV6_USER_MTU
      sl_add_fixed_int(code, "IPV6_USER_MTU", IPV6_USER_MTU);
#endif
#ifdef TCP_NODELAY
      sl_add_fixed_int(code, "TCP_NODELAY", TCP_NODELAY);
#endif

#ifdef TCP_MAXSEG
      sl_add_fixed_int(code, "TCP_MAXSEG", TCP_MAXSEG);
#endif

#ifdef TCP_KEEPALIVE
      sl_add_fixed_int(code, "TCP_KEEPALIVE", TCP_KEEPALIVE);
#endif

#ifdef TCP_KEEPIDLE
      sl_add_fixed_int(code, "TCP_KEEPIDLE", TCP_KEEPIDLE);
#endif

#ifdef TCP_KEEPINTVL
      sl_add_fixed_int(code, "TCP_KEEPINTVL", TCP_KEEPINTVL);
#endif

#ifdef TCP_KEEPCNT
      sl_add_fixed_int(code, "TCP_KEEPCNT", TCP_KEEPCNT);
#endif

#ifdef TCP_FASTOPEN
      sl_add_fixed_int(code, "TCP_FASTOPEN", TCP_FASTOPEN);
#endif

#ifdef TCP_MAXRT
      sl_add_fixed_int(code, "TCP_MAXRT", TCP_MAXRT);
#endif

#ifdef TCP_TIMESTAMPS
      sl_add_fixed_int(code, "TCP_TIMESTAMPS", TCP_TIMESTAMPS);
#endif

#ifdef TCP_BSDURGENT
      sl_add_fixed_int(code, "TCP_BSDURGENT", TCP_BSDURGENT);
#endif

#ifdef TCP_EXPEDITED_1122
      sl_add_fixed_int(code, "TCP_EXPEDITED_1122", TCP_EXPEDITED_1122);
#endif
#ifdef MSG_OOB
      sl_add_fixed_int(code, "MSG_OOB", MSG_OOB);
#endif

#ifdef MSG_PEEK
      sl_add_fixed_int(code, "MSG_PEEK", MSG_PEEK);
#endif

#ifdef MSG_DONTROUTE
      sl_add_fixed_int(code, "MSG_DONTROUTE", MSG_DONTROUTE);
#endif

#ifdef MSG_DONTWAIT
      sl_add_fixed_int(code, "MSG_DONTWAIT", MSG_DONTWAIT);
#endif

#ifdef MSG_WAITALL
      sl_add_fixed_int(code, "MSG_WAITALL", MSG_WAITALL);
#endif

#ifdef MSG_EOR
      sl_add_fixed_int(code, "MSG_EOR", MSG_EOR);
#endif

#ifdef MSG_TRUNC
      sl_add_fixed_int(code, "MSG_TRUNC", MSG_TRUNC);
#endif

#ifdef MSG_CTRUNC
      sl_add_fixed_int(code, "MSG_CTRUNC", MSG_CTRUNC);
#endif

#ifdef MSG_ERRQUEUE
      sl_add_fixed_int(code, "MSG_ERRQUEUE", MSG_ERRQUEUE);
#endif

#ifdef MSG_NOSIGNAL
      sl_add_fixed_int(code, "MSG_NOSIGNAL", MSG_NOSIGNAL);
#endif

#ifdef MSG_MORE
      sl_add_fixed_int(code, "MSG_MORE", MSG_MORE);
#endif

#ifdef MSG_PARTIAL
      sl_add_fixed_int(code, "MSG_PARTIAL", MSG_PARTIAL);
#endif
#ifdef _WIN32
      WSADATA wsaData;
      WSAStartup(MAKEWORD(2, 2), &wsaData);
      sl_add_func(code, "net.new_socket", net_new_socket_win_fn);
      sl_add_func(code, "net.bind", net_bind_win_fn);
      sl_add_func(code, "net.listen", net_listen_win_fn);
      sl_add_func(code, "net.accept", net_accept_win_fn);
      sl_add_func(code, "net.recv", net_recv_win_fn);
      sl_add_func(code, "net.send", net_send_win_fn);
      sl_add_func(code, "net.connect", net_connect_win_fn);
      sl_add_func(code, "net.new_fd", net_new_fd_win_fn);
      sl_add_func(code, "net.zero_fd", net_zero_fd_win_fn);
      sl_add_func(code, "net.add_fd", net_add_fd_win_fn);
      sl_add_func(code, "net.remove_fd", net_remove_fd_win_fn);
      sl_add_func(code, "net.check_fd", net_check_fd_win_fn);
      sl_add_func(code, "net.resolve", net_resolve_win_fn);
      sl_add_func(code, "net.setsockopt", net_setsockopt_win_fn);
      sl_add_func(code, "net.set_nonblocking", net_set_nonblocking_win_fn);
      sl_add_func(code, "net.close", net_close_win_fn);
#else
      sl_add_func(code, "net.new_socket", net_new_socket_posix_fn);
      sl_add_func(code, "net.bind", net_bind_posix_fn);
      sl_add_func(code, "net.listen", net_listen_posix_fn);
      sl_add_func(code, "net.accept", net_accept_posix_fn);
      sl_add_func(code, "net.recv", net_recv_posix_fn);
      sl_add_func(code, "net.send", net_send_posix_fn);
      sl_add_func(code, "net.connect", net_connect_posix_fn);
      sl_add_func(code, "net.new_fd", net_new_fd_posix_fn);
      sl_add_func(code, "net.zero_fd", net_zero_fd_posix_fn);
      sl_add_func(code, "net.add_fd", net_add_fd_posix_fn);
      sl_add_func(code, "net.remove_fd", net_remove_fd_posix_fn);
      sl_add_func(code, "net.check_fd", net_check_fd_posix_fn);
      sl_add_func(code, "net.resolve", net_resolve_posix_fn);
      sl_add_func(code, "net.setsockopt", net_setsockopt_posix_fn);
      sl_add_func(code, "net.set_nonblocking", net_set_nonblocking_posix_fn);
      sl_add_func(code, "net.close", net_close_posix_fn);
#endif
    } else if (strcmp(libstr, "string") == 0 && used_string == 0) {
      used_string = 1;
      sl_add_func(code, "string.char_at", string_charat_fn);
      sl_add_func(code, "string.split", string_split_fn);
      sl_add_func(code, "string.slice", string_slice_fn);
      sl_add_func(code, "string.trim", string_trim_fn);
      sl_add_func(code, "string.set_char_at", string_setcharat_fn);
      sl_add_func(code, "string.contains", string_contains_fn);
      sl_add_func(code, "string.len", string_len_fn);
    } else if (strcmp(libstr, "list") == 0 && used_list == 0) {
      used_list = 1;
      LISTS = calloc(SL_INIT, sizeof(struct SL_List));
      sl_add_func(code, "List.new", List_new_fn);
      sl_add_func(code, "List.push", List_push_fn);
      sl_add_func(code, "List.pop", List_pop_fn);
      sl_add_func(code, "List.peek", List_peek_fn);
      sl_add_func(code, "List.set", List_set_fn);
      sl_add_func(code, "List.get", List_get_fn);
      sl_add_func(code, "List.find", List_find_fn);
      sl_add_func(code, "List.next", List_next_fn);
      sl_add_func(code, "List.iter", List_iter_fn);
      sl_add_func(code, "List.relative", List_relative_fn);
      sl_add_func(code, "List.remove", List_remove_fn);
      sl_add_func(code, "List.len", List_len_fn);
    } else if (strcmp(libstr, "extra") == 0 && used_extra == 0) {
      used_extra = 1;
      sl_add_func(code, "rand.random", random_fn);
    } else if (strcmp(libstr, "db") == 0 && used_db == 0) {
      used_db = 1;
      sl_add_func(code, "db.from_lists", db_from_lists_fn);
      sl_add_func(code, "db.to_lists", db_to_lists_fn);
    }
    free(libstr);
  }

  return return_var;
}

void init_sl_stdlib(struct SL_Code *sl_code, int argc, char **argv) {
  srand(time(NULL));
  arguments = malloc(argc * sizeof(char *));
  for (int i = 0; i < argc; i++) {
    arguments[i] = strdup(argv[i]);
  }
  argcN = argc;
  use_code = sl_code;
  sl_add_func(sl_code, "use", use_fn);
}

void close_sl_stdlib() {
  for (int i = 0; i < argcN; i++) {
    free(arguments[i]);
  }
  if (arguments != NULL)
    free(arguments);

  if (used_list == 1) {
    for (int i = 0; i < LISTS_count; i++) {
      for (int size = 0; size < LISTS[i].size; size++) {

        if (LISTS[i].vars[size].name != NULL) {
          free(LISTS[i].vars[size].name);
          LISTS[i].vars[size].name = NULL;
        }

        if ((LISTS[i].vars[size].type == STRING ||
             LISTS[i].vars[size].type == RETURN) &&
            LISTS[i].vars[size].vals != NULL) {
          free(LISTS[i].vars[size].vals);
          LISTS[i].vars[size].vals = NULL;
        }
      }

      if (LISTS[i].vars != NULL) {
        free(LISTS[i].vars);
        LISTS[i].vars = NULL;
      }
    }

    free(LISTS);
  }

  if (used_collections == 1) {
    for (int i = 0; i < collections.size; i++) {

      if (collections.collections[i].name != NULL) {
        free(collections.collections[i].name);
        collections.collections[i].name = NULL;
      }

      if (collections.collections[i].attrs != NULL) {
        for (int j = 0; j < collections.collections[i].total_attrs; j++) {
          if (collections.collections[i].attrs[j] != NULL) {
            free(collections.collections[i].attrs[j]);
            collections.collections[i].attrs[j] = NULL;
          }
        }

        free(collections.collections[i].attrs);
        collections.collections[i].attrs = NULL;
      }

      if (collections.collections[i].functions != NULL) {
        for (int j = 0; j < collections.collections[i].total_funcs; j++) {

          struct SL_Function *func = &collections.collections[i].functions[j];

          if (func->name != NULL) {
            free(func->name);
            func->name = NULL;
          }

          if (func->code_tokens != NULL) {
            for (int k = 0; k < func->code_len; k++) {
              if (func->code_tokens[k] != NULL) {
                free(func->code_tokens[k]);
                func->code_tokens[k] = NULL;
              }
            }

            free(func->code_tokens);
            func->code_tokens = NULL;
          }

          if (func->types != NULL) {
            free(func->types);
            func->types = NULL;
          }
          if (func->arguments != NULL) {
            for (int arg_idx = 0; arg_idx < func->total_arguments; arg_idx++) {
              if (func->arguments[arg_idx].name != NULL) {
                free(func->arguments[arg_idx].name);
                func->arguments[arg_idx].name = NULL;
              }

              if ((func->arguments[arg_idx].type == STRING ||
                   func->arguments[arg_idx].type == RETURN) &&
                  func->arguments[arg_idx].vals != NULL) {

                free(func->arguments[arg_idx].vals);
                func->arguments[arg_idx].vals = NULL;
              }
            }

            free(func->arguments);
            func->arguments = NULL;
          }
        }

        free(collections.collections[i].functions);
        collections.collections[i].functions = NULL;
      }

      collections.collections[i].total_attrs = 0;
      collections.collections[i].total_funcs = 0;
    }

    if (collections.collections != NULL) {
      free(collections.collections);
      collections.collections = NULL;
    }

    collections.size = 0;
    collections.capacity = 0;
  }

  if (used_net == 1) {
    for (int i = 0; i < fd_list_size; i++) {
      if (fd_list[i].is_set == 1) {
        free(fd_list[i].fd);
      }
    }
    free(fd_list);
  }

#ifdef _WIN32
  if (used_net == 1) {
    WSACleanup();
  }
#endif
}
