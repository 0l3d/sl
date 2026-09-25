/*
 * SL Standart Library
 */
#include "sl.h"
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define CHAR WIN32_CHAR
#define LONG WIN32_LONG
#define BOOLEAN WIN32_BOOLEAN
#define DOUBLE WIN32_DOUBLE
#ifdef ENABLE_NET
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <windows.h>
#undef CHAR
#undef LONG
#undef BOOLEAN
#undef DOUBLE
#else
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#ifdef ENABLE_NET
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif
#endif

/* CONSOLE API */
#define SL_UNDEFINED_EVENT -1
#define SL_KEY_EVENT 0
#define SL_MOUSE_EVENT 1
#define SL_WINDOW_RESIZE_EVENT 1

#define SL_KEY_UNKNOWN 0
#define SL_KEY_CHAR 1

#define SL_KEY_ENTER 2
#define SL_KEY_ESCAPE 3
#define SL_KEY_BACKSPACE 4
#define SL_KEY_TAB 5

#define SL_MOUSE_NONE 0
#define SL_MOUSE_LEFT_PRESSED 1
#define SL_MOUSE_RIGHT_PRESSED 2
#define SL_MOUSE_MIDDLE_PRESSED 3
#define SL_MOUSE_MOVED 4
#define SL_MOUSE_DOUBLE_CLICK 5
#define SL_MOUSE_WHEEL_UP 6
#define SL_MOUSE_WHEEL_DOWN 7

#define SL_KEY_UP 10
#define SL_KEY_DOWN 11
#define SL_KEY_LEFT 12
#define SL_KEY_RIGHT 13

#define SL_KEY_HOME 20
#define SL_KEY_END 21
#define SL_KEY_INSERT 22
#define SL_KEY_DELETE 23

#define SL_KEY_PAGE_UP 24
#define SL_KEY_PAGE_DOWN 25

#define SL_KEY_F1 30
#define SL_KEY_F2 31
#define SL_KEY_F3 32
#define SL_KEY_F4 33
#define SL_KEY_F5 34
#define SL_KEY_F6 35
#define SL_KEY_F7 36
#define SL_KEY_F8 37
#define SL_KEY_F9 38
#define SL_KEY_F10 39
#define SL_KEY_F11 40
#define SL_KEY_F12 41

#define SL_MOD_NONE 0
#define SL_MOD_SHIFT (1 << 0)
#define SL_MOD_CTRL (1 << 1)
#define SL_MOD_ALT (1 << 2)
#define SL_MOD_SUPER (1 << 3)
#define SL_COLOR_DEFAULT 0

#define SL_COLOR_BLACK 1
#define SL_COLOR_RED 2
#define SL_COLOR_GREEN 3
#define SL_COLOR_YELLOW 4
#define SL_COLOR_BLUE 5
#define SL_COLOR_MAGENTA 6
#define SL_COLOR_CYAN 7
#define SL_COLOR_WHITE 8

#define SL_COLOR_BRIGHT_BLACK 9
#define SL_COLOR_BRIGHT_RED 10
#define SL_COLOR_BRIGHT_GREEN 11
#define SL_COLOR_BRIGHT_YELLOW 12
#define SL_COLOR_BRIGHT_BLUE 13
#define SL_COLOR_BRIGHT_MAGENTA 14
#define SL_COLOR_BRIGHT_CYAN 15
#define SL_COLOR_BRIGHT_WHITE 16
static int sl_console_write(const void *data, size_t size) {
  if (data == NULL || size == 0) {
    return 1;
  }

  return fwrite(data, 1, size, stdout) == size;
}

static int sl_console_write_cstr(const char *text) {
  if (text == NULL) {
    return 0;
  }

  return sl_console_write(text, strlen(text));
}

static int sl_console_flush(void) {
  return fflush(stdout) == 0;
}
/* CONSOLE API */

#include <math.h>

static int sl_get_double(struct SL_Variable var, double *out) {
  if (var.type == DOUBLE) {
    *out = var.valf;
    return 1;
  }
  if (var.type == INTEGER) {
    *out = (double)var.vali;
    return 1;
  }
  return 0;
}

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
  if (LISTS == NULL) {
    LISTS_capacity = SL_INIT;
    LISTS = calloc(LISTS_capacity, sizeof(struct SL_List));

    if (LISTS == NULL) {
      return -1;
    }
  }

  if (LISTS_count >= LISTS_capacity) {
    int new_capacity = LISTS_capacity * 2;

    struct SL_List *tmp = srealloc(LISTS, new_capacity * sizeof(struct SL_List));

    memset(tmp + LISTS_capacity, 0,
           (new_capacity - LISTS_capacity) * sizeof(struct SL_List));

    LISTS = tmp;
    LISTS_capacity = new_capacity;
  }

  LISTS[LISTS_count].vars =
      calloc((size_t)capacity, sizeof(struct SL_Variable));

  if (LISTS[LISTS_count].vars == NULL) {
    return -1;
  }

  LISTS[LISTS_count].capacity = capacity;
  LISTS[LISTS_count].current = 0;
  LISTS[LISTS_count].fixed = fixed;

  if (fixed) {
    LISTS[LISTS_count].size = capacity;

    for (int i = 0; i < capacity; i++) {
      LISTS[LISTS_count].vars[i].type = INTEGER;
      LISTS[LISTS_count].vars[i].vali = 0;
    }
  } else {
    LISTS[LISTS_count].size = 0;
  }

  return LISTS_count++;
}

int list_push(struct SL_List *list, struct SL_Variable value) {
  if (list == NULL) {
    return 0;
  }

  if (list->size >= list->capacity) {
    int new_capacity = (list->capacity == 0) ? 8 : list->capacity * 2;

    list->vars =
        srealloc(list->vars, (size_t)new_capacity * sizeof(struct SL_Variable));


    list->capacity = new_capacity;
  }

  list->vars[list->size] = sl_copy_variable(value);

  if (list->vars[list->size].name != NULL) {
    free(list->vars[list->size].name);
    list->vars[list->size].name = NULL;
  }

  list->size++;
  return 1;
}

int list_free(int index) {
  if (index < 0 || index >= LISTS_count) {
    return 0;
  }

  struct SL_List *list = &LISTS[index];

  if (list->vars != NULL) {
    for (int i = 0; i < list->size; i++) {
      sl_free_variable(&list->vars[i]);
    }

    free(list->vars);
  }

  list->vars = NULL;
  list->capacity = 0;
  list->size = 0;
  list->current = 0;
  list->fixed = 0;

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
struct SL_Variable print_fn(struct SL_Code *code,
                            struct SL_L_Function func,
                            struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  for (int i = 0; i < func.total_arguments; i++) {
    return_var = sl_get_argument(*code, func, i);

    char buffer[64];
    int length = 0;

    switch (return_var.type) {
    case INTEGER:
      length = snprintf(buffer, sizeof(buffer), "%d", return_var.vali);
      if (length > 0)
        sl_console_write(buffer, (size_t)length);
      break;

    case DOUBLE:
      length = snprintf(buffer, sizeof(buffer), "%f", return_var.valf);
      if (length > 0)
        sl_console_write(buffer, (size_t)length);
      break;

    case STRING: {
      char *string = sl_string_getter(return_var.vals);

      if (string != NULL) {
        sl_console_write_cstr(string);
        free(string);
      }
      break;
    }

    case BOOLEAN:
      if (return_var.valb) {
        sl_console_write_cstr("true");
      } else {
        sl_console_write_cstr("false");
      }
      break;

    case CHAR:
      sl_console_write(&return_var.valc, 1);
      break;

    case LONG:
      length = snprintf(buffer, sizeof(buffer), "%" PRIdPTR,
                        return_var.valh);
      if (length > 0)
        sl_console_write(buffer, (size_t)length);
      break;

    default:
      break;
    }
  }

  return return_var;
}

struct SL_Variable print_raw_fn(struct SL_Code *code,
                                struct SL_L_Function func,
                                struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  for (int i = 0; i < func.total_arguments; i++) {
    struct SL_Variable value = sl_get_argument(*code, func, i);

    char buffer[64];
    int length = 0;

    switch (value.type) {
    case STRING:
      if (value.vals != NULL)
        sl_console_write_cstr(value.vals);
      break;

    case CHAR:
      sl_console_write(&value.valc, 1);
      break;

    case INTEGER:
      length = snprintf(buffer, sizeof(buffer), "%d", value.vali);
      if (length > 0)
        sl_console_write(buffer, (size_t)length);
      break;

    case DOUBLE:
      length = snprintf(buffer, sizeof(buffer), "%f", value.valf);
      if (length > 0)
        sl_console_write(buffer, (size_t)length);
      break;

    case BOOLEAN:
      sl_console_write_cstr(value.valb ? "true" : "false");
      break;

    case LONG:
      length = snprintf(buffer, sizeof(buffer), "%" PRIdPTR,
                        value.valh);
      if (length > 0)
        sl_console_write(buffer, (size_t)length);
      break;

    default:
      break;
    }
  }

  return return_var;
}

struct SL_Variable input_fn(struct SL_Code *code,
                            struct SL_L_Function func,
                            struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  for (int i = 0; i < func.total_arguments; i++) {
    return_var = sl_get_argument(*code, func, i);

    char buffer[64];
    int length = 0;

    switch (return_var.type) {
    case INTEGER:
      length = snprintf(buffer, sizeof(buffer), "%d", return_var.vali);
      if (length > 0)
        sl_console_write(buffer, (size_t)length);
      break;

    case DOUBLE:
      length = snprintf(buffer, sizeof(buffer), "%f", return_var.valf);
      if (length > 0)
        sl_console_write(buffer, (size_t)length);
      break;

    case STRING: {
      char *string = sl_string_getter(return_var.vals);

      if (string != NULL) {
        sl_console_write_cstr(string);
        free(string);
      }
      break;
    }

    case BOOLEAN:
      sl_console_write_cstr(return_var.valb ? "true" : "false");
      break;

    case CHAR:
      sl_console_write(&return_var.valc, 1);
      break;

    case LONG:
      length = snprintf(buffer, sizeof(buffer), "%" PRIdPTR,
                        return_var.valh);
      if (length > 0)
        sl_console_write(buffer, (size_t)length);
      break;

    default:
      break;
    }
  }
  sl_console_flush();

  char string[1024];

  if (fgets(string, sizeof(string), stdin) == NULL) {
    return_var.type = ERROR;
    return_var.vals = "Failed to read input.";
    return return_var;
  }

  string[strcspn(string, "\n")] = '\0';

  return_var.type = STRING;
  return_var.vals = smalloc(1024);

  strncpy(return_var.vals, string, 1023);
  return_var.vals[1023] = '\0';

  return return_var;
}

struct SL_Variable io_getchar_fn(struct SL_Code *code,
                                 struct SL_L_Function func,
                                 struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  sl_console_flush();

  return_var.valc = getchar();
  return_var.type = CHAR;

  return return_var;
}

struct SL_Variable io_fflush_fn(struct SL_Code *code,
                                struct SL_L_Function func,
                                struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  if (sl_console_flush() != 0) {
    return_var.type = BOOLEAN;
    return_var.valb = 1;
  } else {
    return_var.type = ERROR;
    return_var.vals = "Failed to flush stdout.";
  }

  return return_var;
}

/* Input/Output for stdout/stdin */

/* Input/Output for file/dir */

/* STRING */
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

  if (fseek(file_open, 0, SEEK_END) != 0) {
    fclose(file_open);
    free(file_name);

    return_var.type = ERROR;
    return_var.vals = "Could not seek file.";
    return return_var;
  }

  long int size = ftell(file_open);

  if (size < 0) {
    fclose(file_open);
    free(file_name);

    return_var.type = ERROR;
    return_var.vals = "Could not determine file size.";
    return return_var;
  }

  rewind(file_open);

  char *buffer = malloc((size_t)size + 1);

  if (buffer == NULL) {
    fclose(file_open);
    free(file_name);

    return_var.type = ERROR;
    return_var.vals = "Could not allocate file buffer.";
    return return_var;
  }

  size_t read_size = fread(buffer, 1, (size_t)size, file_open);

  if (read_size != (size_t)size && ferror(file_open)) {
    free(buffer);
    fclose(file_open);
    free(file_name);

    return_var.type = ERROR;
    return_var.vals = "Could not read file.";
    return return_var;
  }

  buffer[read_size] = '\0';
  fclose(file_open);

  return_var.type = STRING;
  return_var.vals = strdup(buffer);
  free(buffer);
  free(file_name);
  return return_var;
}

struct SL_Variable file_remove_fn(struct SL_Code *code,
                                  struct SL_L_Function func,
                                  struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at file.remove! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  char *file_name = sl_string_getter(first_arg.vals);

  if (file_name == NULL) {
    return_var.type = ERROR;
    return_var.vals = "Could not get file name.";
    return return_var;
  }

  if (remove(file_name) != 0) {
    free(file_name);

    return_var.type = ERROR;
    return_var.vals = "Could not remove file.";
    return return_var;
  }

  free(file_name);

  return_var.type = STRING;
  return_var.vals = strdup("File removed successfully.");

  return return_var;
}


struct SL_Variable file_write_from_str_fn(struct SL_Code *code,
                                          struct SL_L_Function func,
                                          struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
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
  char *text = sl_string_getter(second_arg.vals);

  if (text == NULL) {
    fclose(file_open);
    free(file_name);

    return_var.type = ERROR;
    return_var.vals = "Could not decode string.";
    return return_var;
  }

  size_t len = strlen(text);
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
  if (func.total_arguments < 2) {
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
  char *text = sl_string_getter(second_arg.vals);

  if (text == NULL) {
    fclose(file_open);
    free(file_name);

    return_var.type = ERROR;
    return_var.vals = "Could not decode string.";
    return return_var;
  }

  size_t len = strlen(text);
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

/* BINARY */
struct SL_Variable file_read_fn(struct SL_Code *code,
                                struct SL_L_Function func,
                                struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at file.read! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable return_var = {0};

  struct SL_Variable first_arg =
      sl_get_argument(*code, func, 0);

  char *file_name = sl_string_getter(first_arg.vals);

  if (file_name == NULL) {
    return_var.type = ERROR;
    return_var.vals = "Could not get file name.";
    return return_var;
  }

  FILE *file_open = fopen(file_name, "rb");

  if (file_open == NULL) {
    free(file_name);

    return_var.type = ERROR;
    return_var.vals = "File not found.";
    return return_var;
  }

  if (fseek(file_open, 0, SEEK_END) != 0) {
    fclose(file_open);
    free(file_name);

    return_var.type = ERROR;
    return_var.vals = "Could not seek file.";
    return return_var;
  }

  long int file_size = ftell(file_open);

  if (file_size < 0) {
    fclose(file_open);
    free(file_name);

    return_var.type = ERROR;
    return_var.vals = "Could not determine file size.";
    return return_var;
  }

  rewind(file_open);

  size_t size = (size_t)file_size;

  char *buffer = NULL;

  if (size != 0) {
    buffer = malloc(size);

    if (buffer == NULL) {
      fclose(file_open);
      free(file_name);

      return_var.type = ERROR;
      return_var.vals = "Could not allocate file buffer.";
      return return_var;
    }

    size_t read_size = fread(buffer, 1, size, file_open);

    if (read_size != size) {
      free(buffer);
      fclose(file_open);
      free(file_name);

      return_var.type = ERROR;
      return_var.vals = "Could not read file.";
      return return_var;
    }
  }

  fclose(file_open);
  free(file_name);

  return_var.type = BYTES;
  return_var.vals = sl_bytes_copy(buffer, size);
  return_var.length = size;

  if (return_var.vals == NULL && size != 0) {
    free(buffer);

    return_var.type = ERROR;
    return_var.vals = "Could not copy file buffer.";
    return_var.length = 0;

    return return_var;
  }

  free(buffer);

  return return_var;
}

struct SL_Variable file_write_fn(struct SL_Code *code,
                                 struct SL_L_Function func,
                                 struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at file.write! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable return_var = {0};

  struct SL_Variable first_arg =
      sl_get_argument(*code, func, 0);

  struct SL_Variable second_arg =
      sl_get_argument(*code, func, 1);

  if (second_arg.type != BYTES) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected bytes as the second argument to file.write.";
    return return_var;
  }

  char *file_name = sl_string_getter(first_arg.vals);

  if (file_name == NULL) {
    return_var.type = ERROR;
    return_var.vals = "Could not get file name.";
    return return_var;
  }

  FILE *file_open = fopen(file_name, "wb");

  if (file_open == NULL) {
    free(file_name);

    return_var.type = ERROR;
    return_var.vals = "Could not open file.";
    return return_var;
  }

  if (second_arg.length != 0) {
    size_t written =
        fwrite(second_arg.vals,
               1,
               second_arg.length,
               file_open);

    if (written != second_arg.length) {
      fclose(file_open);
      free(file_name);

      return_var.type = ERROR;
      return_var.vals = "Could not write to file.";
      return return_var;
    }
  }

  fclose(file_open);
  free(file_name);

  return return_var;
}

struct SL_Variable file_append_fn(struct SL_Code *code,
                                  struct SL_L_Function func,
                                  struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at file.append! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable return_var = {0};

  struct SL_Variable first_arg =
      sl_get_argument(*code, func, 0);

  struct SL_Variable second_arg =
      sl_get_argument(*code, func, 1);

  if (second_arg.type != BYTES) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected bytes as the second argument to file.append.";
    return return_var;
  }

  char *file_name = sl_string_getter(first_arg.vals);

  if (file_name == NULL) {
    return_var.type = ERROR;
    return_var.vals = "Could not get file name.";
    return return_var;
  }

  FILE *file_open = fopen(file_name, "ab");

  if (file_open == NULL) {
    free(file_name);

    return_var.type = ERROR;
    return_var.vals = "Could not open file.";
    return return_var;
  }

  if (second_arg.length != 0) {
    size_t written =
        fwrite(second_arg.vals,
               1,
               second_arg.length,
               file_open);

    if (written != second_arg.length) {
      fclose(file_open);
      free(file_name);

      return_var.type = ERROR;
      return_var.vals = "Could not append to file.";
      return return_var;
    }
  }

  fclose(file_open);
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

/* Time */

struct SL_Variable time_now_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  return_var.type = INTEGER;
  return_var.vali = (int)time(NULL);
  return return_var;
}

struct SL_Variable time_string_fn(struct SL_Code *code, struct SL_L_Function func,
                                  struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  const char *format = "%Y-%m-%d %H:%M:%S";

  if (func.total_arguments > 0) {
    struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
    if (first_arg.type != STRING || first_arg.vals == NULL) {
      return_var.type = ERROR;
      return_var.vals = "Expected STRING as the first argument to time.string.";
      return return_var;
    }
    format = first_arg.vals;
  }

  time_t t = time(NULL);
  struct tm *tm_info = localtime(&t);
  char buffer[128];

  strftime(buffer, sizeof(buffer), format, tm_info);

  return_var.type = STRING;
  return_var.vals = strdup(buffer);
  return return_var;
}

struct SL_Variable time_clock_fn(struct SL_Code *code, struct SL_L_Function func,
                                 struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  return_var.type = DOUBLE;
  return_var.valf = (double)clock() / CLOCKS_PER_SEC;
  return return_var;
}

struct SL_Variable time_sleep_fn(struct SL_Code *code, struct SL_L_Function func,
                                 struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at time.sleep! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  if (first_arg.type != INTEGER && first_arg.type != DOUBLE) {
    return_var.type = ERROR;
    return_var.vals = "Expected INTEGER or DOUBLE as the first argument to time.sleep.";
    return return_var;
  }

  int ms = (first_arg.type == DOUBLE) ? (int)first_arg.valf : first_arg.vali;

  #ifdef _WIN32
  Sleep(ms);
  #else
  usleep(ms * 1000);
  #endif

  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable time_hour_fn(struct SL_Code *code, struct SL_L_Function func,
                                struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  time_t t = time(NULL);
  struct tm *tm_info = localtime(&t);

  return_var.type = INTEGER;
  return_var.vali = tm_info->tm_hour;
  return return_var;
}

struct SL_Variable time_minute_fn(struct SL_Code *code, struct SL_L_Function func,
                                  struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  time_t t = time(NULL);
  struct tm *tm_info = localtime(&t);

  return_var.type = INTEGER;
  return_var.vali = tm_info->tm_min;
  return return_var;
}

struct SL_Variable time_second_fn(struct SL_Code *code, struct SL_L_Function func,
                                  struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  time_t t = time(NULL);
  struct tm *tm_info = localtime(&t);

  return_var.type = INTEGER;
  return_var.vali = tm_info->tm_sec;
  return return_var;
}

struct SL_Variable time_diff_fn(struct SL_Code *code, struct SL_L_Function func,
                                struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at time.diff! Not enough arguments!.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  if (first_arg.type != INTEGER && first_arg.type != DOUBLE) {
    return_var.type = ERROR;
    return_var.vals = "Expected INTEGER or DOUBLE as the first argument to time.diff.";
    return return_var;
  }

  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  if (second_arg.type != INTEGER && second_arg.type != DOUBLE) {
    return_var.type = ERROR;
    return_var.vals = "Expected INTEGER or DOUBLE as the second argument to time.diff.";
    return return_var;
  }

  double val1 = (first_arg.type == DOUBLE) ? first_arg.valf : (double)first_arg.vali;
  double val2 = (second_arg.type == DOUBLE) ? second_arg.valf : (double)second_arg.vali;

  return_var.type = DOUBLE;
  return_var.valf = difftime((time_t)val1, (time_t)val2);
  return return_var;
}

struct SL_Variable time_parse_fn(struct SL_Code *code, struct SL_L_Function func,
                                 struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at time.parse! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  if (first_arg.type != STRING || first_arg.vals == NULL) {
    return_var.type = ERROR;
    return_var.vals = "Expected STRING as the first argument to time.parse.";
    return return_var;
  }

  struct tm tm_info = {0};
  if (sscanf(first_arg.vals, "%d-%d-%d %d:%d:%d",
             &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday,
             &tm_info.tm_hour, &tm_info.tm_min, &tm_info.tm_sec) < 6) {
    return_var.type = ERROR;
    return_var.vals = "Invalid date format! Expected 'YYYY-MM-DD HH:MM:SS'.";
    return return_var;
  }

  tm_info.tm_year -= 1900;
  tm_info.tm_mon -= 1;

  time_t parsed_time = mktime(&tm_info);

  return_var.type = INTEGER;
  return_var.vali = (int)parsed_time;
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
  return_var.vali = atoi(sl_string_getter(first_arg.vals));
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
  char* tmp = smalloc(2);
  tmp[0] = first_arg.valc;
  tmp[1] = '\0';
  return_var.vals = sl_quote_string(tmp);
  free(tmp);
  return_var.type = STRING;
  return return_var;
}

struct SL_Variable int_to_str_fn(struct SL_Code *code,
                                 struct SL_L_Function func,
                                 struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.int_to_str! Not enough arguments.";
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

  char* tmp = smalloc(digits + 1);
  snprintf(tmp, digits + 1, "%d", first_arg.vali);
  return_var.vals = sl_quote_string(tmp);
  free(tmp);
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
    return_var.vals = "Error usage at types.is_double! Not enough arguments.";
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
    return_var.vals = "Error usage at types.is_not_initialized! Not enough arguments.";
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
    return_var.vals = "Error usage at types.is_digit! Not enough arguments.";
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
    return_var.vals = "Error usage at types.is_space! Not enough arguments.";
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

struct SL_Variable is_alpha_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.is_alpha! Not enough arguments.";
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
      if (!isalpha((unsigned char)str[i])) {
        free(str);
        return return_var;
      }
    }
    free(str);
  } else if (first_arg.type == CHAR) {
    if (!isalpha((unsigned char)first_arg.valc))
      return return_var;
  }

  return_var.valb = 1;
  return return_var;
}

struct SL_Variable is_alnum_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.is_alnum! Not enough arguments.";
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
      if (!isalnum((unsigned char)str[i])) {
        free(str);
        return return_var;
      }
    }
    free(str);
  } else if (first_arg.type == CHAR) {
    if (!isalnum((unsigned char)first_arg.valc))
      return return_var;
  }

  return_var.valb = 1;
  return return_var;
}

struct SL_Variable is_upper_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.is_upper! Not enough arguments.";
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
      if (!isupper((unsigned char)str[i])) {
        free(str);
        return return_var;
      }
    }
    free(str);
  } else if (first_arg.type == CHAR) {
    if (!isupper((unsigned char)first_arg.valc))
      return return_var;
  }

  return_var.valb = 1;
  return return_var;
}

struct SL_Variable is_lower_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.is_lower! Not enough arguments.";
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
      if (!islower((unsigned char)str[i])) {
        free(str);
        return return_var;
      }
    }
    free(str);
  } else if (first_arg.type == CHAR) {
    if (!islower((unsigned char)first_arg.valc))
      return return_var;
  }

  return_var.valb = 1;
  return return_var;
}

struct SL_Variable is_punct_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.is_punct! Not enough arguments.";
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
      if (!ispunct((unsigned char)str[i])) {
        free(str);
        return return_var;
      }
    }
    free(str);
  } else if (first_arg.type == CHAR) {
    if (!ispunct((unsigned char)first_arg.valc))
      return return_var;
  }

  return_var.valb = 1;
  return return_var;
}

struct SL_Variable is_xdigit_fn(struct SL_Code *code, struct SL_L_Function func,
                                struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.is_xdigit! Not enough arguments.";
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
      if (!isxdigit((unsigned char)str[i])) {
        free(str);
        return return_var;
      }
    }
    free(str);
  } else if (first_arg.type == CHAR) {
    if (!isxdigit((unsigned char)first_arg.valc))
      return return_var;
  }

  return_var.valb = 1;
  return return_var;
}

struct SL_Variable to_lower_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at types.to_lower! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  if (first_arg.type == STRING) {
    char *str = sl_string_getter(first_arg.vals);
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
      str[i] = (char)tolower((unsigned char)str[i]);
    }
    return_var.type = STRING;
    return_var.vals = sl_quote_string(str);
free(str);  
  } else if (first_arg.type == CHAR) {
    return_var.type = CHAR;
    return_var.valc = (char)tolower((unsigned char)first_arg.valc);
  } else {
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at to_lower! Expected STRING or CHAR argument.";
  }

  return return_var;
}

struct SL_Variable types_sizeof_fn(struct SL_Code *code,
                                 struct SL_L_Function func,
                                 struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  size_t total_size = 0;

  for (int i = 0; i < func.total_arguments; i++) {
    struct SL_Variable arg = sl_get_argument(*code, func, i);

    switch (arg.type) {
    case INTEGER:
      total_size += sizeof(arg.vali);
      break;

    case DOUBLE:
      total_size += sizeof(arg.valf);
      break;

    case LONG:
      total_size += sizeof(arg.valh);
      break;

    case BOOLEAN:
      total_size += sizeof(arg.valb);
      break;

    case CHAR:
      total_size += sizeof(arg.valc);
      break;

    case STRING: {
      char *string = sl_string_getter(arg.vals);

      if (string == NULL) {
        return_var.type = ERROR;
        return_var.vals = "Could not get string argument.";
        return return_var;
      }

      total_size += strlen(string);

      free(string);
      break;
    }

    default:
      return_var.type = ERROR;
      return_var.vals = "Unsupported type in types.pack.";
      return return_var;
    }
  }

    
  return_var.type = INTEGER;
  return_var.vali = total_size;

  return return_var;
}



struct SL_Variable types_pack_fn(struct SL_Code *code,
                                 struct SL_L_Function func,
                                 struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  size_t total_size = 0;

  for (int i = 0; i < func.total_arguments; i++) {
    struct SL_Variable arg = sl_get_argument(*code, func, i);

    switch (arg.type) {
    case INTEGER:
      total_size += sizeof(arg.vali);
      break;

    case DOUBLE:
      total_size += sizeof(arg.valf);
      break;

    case LONG:
      total_size += sizeof(arg.valh);
      break;

    case BOOLEAN:
      total_size += sizeof(arg.valb);
      break;

    case CHAR:
      total_size += sizeof(arg.valc);
      break;

    case STRING: {
      char *string = sl_string_getter(arg.vals);

      if (string == NULL) {
        return_var.type = ERROR;
        return_var.vals = "Could not get string argument.";
        return return_var;
      }

      total_size += strlen(string);

      free(string);
      break;
    }

    default:
      return_var.type = ERROR;
      return_var.vals = "Unsupported type in types.pack.";
      return return_var;
    }
  }

  char *buffer = smalloc(total_size);

  size_t offset = 0;

  for (int i = 0; i < func.total_arguments; i++) {
    struct SL_Variable arg = sl_get_argument(*code, func, i);

    switch (arg.type) {
    case INTEGER:
      memcpy(buffer + offset,
             &arg.vali,
             sizeof(arg.vali));

      offset += sizeof(arg.vali);
      break;
    case DOUBLE:
      memcpy(buffer + offset,
             &arg.valf,
             sizeof(arg.valf));

      offset += sizeof(arg.valf);
      break;

    case LONG:
      memcpy(buffer + offset,
             &arg.valh,
             sizeof(arg.valh));

      offset += sizeof(arg.valh);
      break;

    case BOOLEAN:
      memcpy(buffer + offset,
             &arg.valb,
             sizeof(arg.valb));

      offset += sizeof(arg.valb);
      break;

    case CHAR:
      memcpy(buffer + offset,
             &arg.valc,
             sizeof(arg.valc));

      offset += sizeof(arg.valc);
      break;

    case STRING: {
      char *string = sl_string_getter(arg.vals);

      if (string == NULL) {
        free(buffer);

        return_var.type = ERROR;
        return_var.vals = "Could not get string argument.";
        return return_var;
      }

      size_t string_size = strlen(string);

      memcpy(buffer + offset,
             string,
             string_size);

      offset += string_size;

      free(string);
      break;
    }

    default:
      free(buffer);

      return_var.type = ERROR;
      return_var.vals = "Unsupported type in types.pack.";
      return return_var;
    }
  }

  return_var.type = BYTES;
  return_var.vals = sl_bytes_copy(buffer, total_size);
  return_var.length = total_size;
  free(buffer);

  return return_var;
}


struct SL_Variable to_upper_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at to_upper! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  if (first_arg.type == STRING) {
    char *str = sl_string_getter(first_arg.vals);
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
      str[i] = (char)toupper((unsigned char)str[i]);
    }
    return_var.type = STRING;
    return_var.vals = sl_quote_string(str);
free(str);  
  } else if (first_arg.type == CHAR) {
    return_var.type = CHAR;
    return_var.valc = (char)toupper((unsigned char)first_arg.valc);
  } else {
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at to_upper! Expected STRING or CHAR argument.";
  }

  return return_var;
}


/* BYTES */ 

struct SL_Variable byte_get_fn(struct SL_Code *code,
                                    struct SL_L_Function func,
                                    struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at byte.get! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  if (first_arg.type != BYTES) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected bytes as the first argument to byte.get.";
    return return_var;
  }

  if (second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected integer as the second argument to byte.get.";
    return return_var;
  }


  if (second_arg.vali >= first_arg.length) {
    return_var.type = ERROR;
    return_var.vals = "Buffer overflow!";
    return return_var;
  }

  if (second_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "Buffer underflow!";
    return return_var;
  }
  return_var.valc = first_arg.vals[second_arg.vali];
  return_var.type = CHAR;
  return return_var;
}

struct SL_Variable byte_set_fn(struct SL_Code *code,
                                       struct SL_L_Function func,
                                       struct SL_Function rfunc) {
  if (func.total_arguments < 3) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at byte.set! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable third_arg = sl_get_argument(*code, func, 2);
  if (first_arg.type != BYTES) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected bytes as the first argument to byte.set.";
    return return_var;
  }

  if (second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected integer as the second argument to byte.set.";
    return return_var;
  }

  if (third_arg.type != CHAR) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected char as the second argument to byte.set.";
    return return_var;
  }


  if (second_arg.vali >= first_arg.length) {
    return_var.type = ERROR;
    return_var.vals = "Buffer overflow!";
    return return_var;
  }

  if (second_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "Buffer underflow!";
    return return_var;
  }
  char *bytes = sl_bytes_copy(first_arg.vals, first_arg.length);
  bytes[second_arg.vali] = third_arg.valc;
  return_var.type = BYTES;
  return_var.vals = sl_bytes_copy(bytes, first_arg.length);
  return_var.length = first_arg.length;
  free(bytes);
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
  if (func.total_arguments < 3) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at string.set_char_at! Not enough arguments.";
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
  char *string = sl_string_getter(first_arg.vals);
  string[second_arg.vali] = third_arg.valc;
  return_var.type = STRING;
  return_var.vals = sl_quote_string(string);
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
  char *raw_text = sl_string_getter(first_arg.vals);
  int len = strlen(raw_text);
  return_var.vali = len;
  return_var.type = INTEGER;
  free(raw_text);
  return return_var;
}

struct SL_Variable string_replace_fn(struct SL_Code *code,
                                     struct SL_L_Function func,
                                     struct SL_Function rfunc) {
  if (func.total_arguments < 3) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at string.replace! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable third_arg = sl_get_argument(*code, func, 2);

  if (first_arg.type != STRING || second_arg.type != STRING ||
      third_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected string arguments for string.replace.";
    return return_var;
  }

  char *str = sl_string_getter(first_arg.vals);
  char *sub = sl_string_getter(second_arg.vals);
  char *replace = sl_string_getter(third_arg.vals);

  size_t str_len = strlen(str);
  size_t sub_len = strlen(sub);
  size_t replace_len = strlen(replace);

  if (sub_len == 0) {
    return_var.type = STRING;
    return_var.vals = strdup(str);

    free(str);
    free(sub);
    free(replace);

    return return_var;
  }

  size_t count = 0;
  char *tmp = str;

  while ((tmp = strstr(tmp, sub)) != NULL) {
    count++;
    tmp += sub_len;
  }

  size_t result_len;

  if (replace_len >= sub_len) {
    result_len = str_len + count * (replace_len - sub_len) + 1;
  } else {
    result_len = str_len - count * (sub_len - replace_len) + 1;
  }

  char *result = malloc(result_len);

  if (result == NULL) {
    return_var.type = ERROR;
    return_var.vals = "Memory allocation failed.";

    free(str);
    free(sub);
    free(replace);

    return return_var;
  }

  char *current = str;
  char *dest = result;

  while ((tmp = strstr(current, sub)) != NULL) {
    size_t before_len = (size_t)(tmp - current);

    memcpy(dest, current, before_len);
    dest += before_len;

    memcpy(dest, replace, replace_len);
    dest += replace_len;

    current = tmp + sub_len;
  }

  strcpy(dest, current);

  return_var.type = STRING;
  return_var.vals = sl_quote_string(result);

  free(result);
  free(str);
  free(sub);
  free(replace);

  return return_var;
}

struct SL_Variable string_startswith_fn(struct SL_Code *code,
                                        struct SL_L_Function func,
                                        struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at string.startswith! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);

  if (first_arg.type != STRING || second_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected string arguments for string.startswith.";
    return return_var;
  }

  char *raw_str = sl_string_getter(first_arg.vals);
  char *prefix = sl_string_getter(second_arg.vals);

  size_t prefix_len = strlen(prefix);

  return_var.type = BOOLEAN;
  return_var.valb = (strncmp(raw_str, prefix, prefix_len) == 0);

  free(raw_str);
  free(prefix);

  return return_var;
}

struct SL_Variable string_endswith_fn(struct SL_Code *code,
                                      struct SL_L_Function func,
                                      struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at string.endswith! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);

  if (first_arg.type != STRING || second_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected string arguments for string.endswith.";
    return return_var;
  }

  char *raw_str = sl_string_getter(first_arg.vals);
  char *suffix = sl_string_getter(second_arg.vals);

  size_t str_len = strlen(raw_str);
  size_t suffix_len = strlen(suffix);

  return_var.type = BOOLEAN;

  if (suffix_len > str_len) {
    return_var.valb = 0;
  } else {
    return_var.valb = (strcmp(raw_str + str_len - suffix_len, suffix) == 0);
  }

  free(raw_str);
  free(suffix);

  return return_var;
}

struct SL_Variable string_remove_at_fn(struct SL_Code *code,
                                       struct SL_L_Function func,
                                       struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at string.remove_at! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);

  if (first_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected string as first argument to string.remove_at.";
    return return_var;
  }

  if (second_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected integer as second argument to string.remove_at.";
    return return_var;
  }

  char *raw_str = sl_string_getter(first_arg.vals);

  size_t len = strlen(raw_str);
  int idx = second_arg.vali;

  if (idx < 0 || (size_t)idx >= len) {
    return_var.type = ERROR;
    return_var.vals = "Buffer over/underflow!";

    free(raw_str);

    return return_var;
  }

  char *result = malloc(len);

  if (result == NULL) {
    return_var.type = ERROR;
    return_var.vals = "Memory allocation failed.";

    free(raw_str);

    return return_var;
  }

  memcpy(result, raw_str, idx);
  strcpy(result + idx, raw_str + idx + 1);

  return_var.type = STRING;
  return_var.vals = sl_quote_string(result);

  free(result);
  free(raw_str);

  return return_var;
}

struct SL_Variable string_index_of_fn(struct SL_Code *code,
                                      struct SL_L_Function func,
                                      struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at string.index_of! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable return_var = {0};
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);

  if (first_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected string as first argument to string.index_of.";
    return return_var;
  }

  char *raw_str = sl_string_getter(first_arg.vals);

  int found_idx = -1;

  if (second_arg.type == CHAR) {
    char *ptr = strchr(raw_str, second_arg.valc);

    if (ptr != NULL) {
      found_idx = (int)(ptr - raw_str);
    }
  } else if (second_arg.type == STRING) {
    char *search_str = sl_string_getter(second_arg.vals);

    char *ptr = strstr(raw_str, search_str);

    if (ptr != NULL) {
      found_idx = (int)(ptr - raw_str);
    }

    free(search_str);
  } else {
    return_var.type = ERROR;
    return_var.vals =
        "Expected char or string as second argument to string.index_of.";

    free(raw_str);

    return return_var;
  }

  return_var.type = INTEGER;
  return_var.vali = found_idx;

  free(raw_str);

  return return_var;
}

struct SL_Variable string_split_fn(struct SL_Code *code,
                                   struct SL_L_Function func,
                                   struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  if (func.total_arguments < 2) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at string.split! Not enough arguments.";
    return return_var;
  }

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

  if (splt_string == NULL || splt_token == NULL) {
    free(splt_string);
    free(splt_token);

    return_var.type = ERROR;
    return_var.vals = "Could not allocate string buffer.";
    return return_var;
  }

  if (strlen(splt_token) == 0) {
    free(splt_string);
    free(splt_token);

    return_var.type = ERROR;
    return_var.vals = "Expected non-empty delimiter for string.split.";
    return return_var;
  }

  int listind = create_new_list(256, 0);

  if (listind < 0) {
    free(splt_string);
    free(splt_token);

    return_var.type = ERROR;
    return_var.vals = "Could not allocate split list.";
    return return_var;
  }

  char *current = splt_string;
  char *tokenize = NULL;
  size_t delimiter_len = strlen(splt_token);

  while ((tokenize = strstr(current, splt_token)) != NULL) {
    size_t token_len = (size_t)(tokenize - current);

    char *part = malloc(token_len + 1);

    if (part == NULL) {
      free(splt_string);
      free(splt_token);

      return_var.type = ERROR;
      return_var.vals = "Memory allocation failed.";
      return return_var;
    }

    memcpy(part, current, token_len);
    part[token_len] = '\0';

    struct SL_Variable push_val = {0};
    push_val.type = STRING;
    push_val.vals = sl_quote_string(part);

    if (!list_push(&LISTS[listind], push_val)) {
      free(part);
      free(push_val.vals);
      free(splt_string);
      free(splt_token);

      return_var.type = ERROR;
      return_var.vals = "Could not push split item.";
      return return_var;
    }

    free(part);
    free(push_val.vals);

    current = tokenize + delimiter_len;
  }

  struct SL_Variable push_val = {0};
  push_val.type = STRING;
  push_val.vals = sl_quote_string(current);

  if (!list_push(&LISTS[listind], push_val)) {
    free(push_val.vals);
    free(splt_string);
    free(splt_token);

    return_var.type = ERROR;
    return_var.vals = "Could not push final split item.";
    return return_var;
  }

  free(splt_string);
  free(splt_token);
  free(push_val.vals);

  return_var.type = INTEGER;
  return_var.vali = listind;


  return return_var;
}

struct SL_Variable string_contains_fn(struct SL_Code *code,
                                      struct SL_L_Function func,
                                      struct SL_Function rfunc) {
  if (func.total_arguments < 2) {
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
    return_var.vals =
        "Expected string as the first argument to string.contains.";
    return return_var;
  }

  if (second_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals =
        "Expected string as the second argument to string.contains.";
    return return_var;
  }

  char *raw_string = sl_string_getter(first_arg.vals);
  char *searchingstr = sl_string_getter(second_arg.vals);

  return_var.valb = (strstr(raw_string, searchingstr) != NULL);

  return_var.type = BOOLEAN;

  free(raw_string);
  free(searchingstr);

  return return_var;
}

struct SL_Variable string_slice_fn(struct SL_Code *code,
                                   struct SL_L_Function func,
                                   struct SL_Function rfunc) {
  if (func.total_arguments < 3) {
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

  size_t len = strlen(raw_str);

  int start = second_arg.vali;
  int end = third_arg.vali;

  if (start < 0 || end < 0 || (size_t)start > len || (size_t)end > len ||
      start > end) {
    return_var.type = ERROR;
    return_var.vals = "Buffer over/underflow!";

    free(raw_str);

    return return_var;
  }

  size_t slice_len = (size_t)(end - start);

  char *result = malloc(slice_len + 1);

  if (result == NULL) {
    return_var.type = ERROR;
    return_var.vals = "Memory allocation failed.";

    free(raw_str);

    return return_var;
  }

  memcpy(result, raw_str + start, slice_len);
  result[slice_len] = '\0';

  return_var.type = STRING;
  return_var.vals = sl_quote_string(result);

  free(result);
  free(raw_str);

  return return_var;
}

struct SL_Variable string_reverse_fn(struct SL_Code *code,
                                     struct SL_L_Function func,
                                     struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at string.reverse! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  if (first_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected string as the first argument to string.reverse.";
    return return_var;
  }

  char *raw_str = sl_string_getter(first_arg.vals);
  size_t len = strlen(raw_str);

  for (size_t i = 0; i < len / 2; i++) {
    char tmp = raw_str[i];
    raw_str[i] = raw_str[len - 1 - i];
    raw_str[len - 1 - i] = tmp;
  }

  return_var.type = STRING;
  return_var.vals = sl_quote_string(raw_str);
  
  free(raw_str);

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

  size_t start = 0;
  size_t end = strlen(raw_str);

  while (start < end && isspace((unsigned char)raw_str[start])) {
    start++;
  }

  while (end > start && isspace((unsigned char)raw_str[end - 1])) {
    end--;
  }

  size_t len = end - start;

  char *result = malloc(len + 1);

  if (result == NULL) {
    return_var.type = ERROR;
    return_var.vals = "Memory allocation failed.";

    free(raw_str);

    return return_var;
  }

  memcpy(result, raw_str + start, len);
  result[len] = '\0';

  return_var.type = STRING;
  return_var.vals = sl_quote_string(result);

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
  return return_var;
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

struct SL_Variable sys_get_env_fn(struct SL_Code *code,
                                  struct SL_L_Function func,
                                  struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  struct SL_Variable arg = sl_get_argument(*code, func, 0);

  if (arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected string argument for env.get";
    return return_var;
  }

  char *var_name = sl_string_getter(arg.vals);
  char *env_val = getenv(var_name);
  free(var_name);

  if (env_val == NULL) {
    return_var.type = STRING;
    return_var.vals = strdup("");
  } else {
    return_var.type = STRING;
    return_var.vals = sl_quote_string(env_val);
  }

  return return_var;
}

struct SL_Variable sys_popen_fn(struct SL_Code *code, struct SL_L_Function func,
                                struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at sys.popen! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};
  if (first_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals = "Expected a plain-text shell command.";
  }

  char *command = sl_string_getter(first_arg.vals);
  FILE *cmd_out = popen(command, "r");
  if (!cmd_out) {
    free(command);
    return_var.vals = "Popen failed to open process.";
    return_var.type = ERROR;
  }

  size_t capacity = 4096;
  size_t length = 0;
  char *buffer = malloc(capacity);

  if (buffer == NULL) {
    pclose(cmd_out);
    free(command);
    return_var.vals = "buffer failed to allocate memory";
    return_var.type = ERROR;
  }

  int c;
  while ((c = fgetc(cmd_out)) != EOF) {
    if (length + 1 >= capacity) {
      capacity *= 2;

      char *tmp = realloc(buffer, capacity);
      if (!tmp) {
        free(buffer);
        pclose(cmd_out);
        free(command);
        return_var.vals = "realloc failed to allocate memory.";
        return_var.type = ERROR;
      }

      buffer = tmp;
    }

    buffer[length++] = (char)c;
  }

  buffer[length] = '\0';

  int status = pclose(cmd_out);
  free(command);

  return_var.type = STRING;
  return_var.vals = strdup(buffer);
  free(buffer);

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

struct SL_Variable List_free_fn(struct SL_Code *code, struct SL_L_Function func,
                                struct SL_Function rfunc) {
  struct SL_Variable result = {0};

  if (func.total_arguments < 1) {
    result.type = ERROR;
    result.vals = "Error usage at List.free! Not enough arguments.";
    return result;
  }

  struct SL_Variable arg = sl_get_argument(*code, func, 0);

  if (arg.type != INTEGER) {
    result.type = ERROR;
    result.vals = "List.free expects a list variable.";
    return result;
  }

  if (!list_free(arg.vali)) {
    result.type = ERROR;
    result.vals = "Invalid list variable.";
    return result;
  }

  result.type = BOOLEAN;
  result.valb = 1;
  return result;
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
  if (LISTS[first_arg.vali].size <= 0) {
    return_var.type = ERROR;
    return_var.vals = "List is empty.";
    return return_var;
  }
  if (LISTS[first_arg.vali].current >= LISTS[first_arg.vali].size) {
    LISTS[first_arg.vali].current = 0;
  }

  return sl_copy_variable(
      LISTS[first_arg.vali].vars[LISTS[first_arg.vali].current++]);
}

struct SL_Variable List_reverse_fn(struct SL_Code *code,
                                   struct SL_L_Function func,
                                   struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at List.reverse! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals = "Expected list_variable as the first argument to List.reverse.";
    return return_var;
  }

  if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer overflow!";
    return return_var;
  }

  struct SL_List *list = &LISTS[first_arg.vali];

  for (int i = 0; i < list->size / 2; i++) {
    struct SL_Variable temp = list->vars[i];
    list->vars[i] = list->vars[list->size - 1 - i];
    list->vars[list->size - 1 - i] = temp;
  }

  return_var.type = BOOLEAN;
  return_var.valb = 1;

  return return_var;
}


static int sl_compare_vars(const void *a, const void *b) {
  const struct SL_Variable *va = (const struct SL_Variable *)a;
  const struct SL_Variable *vb = (const struct SL_Variable *)b;

  if (va->type != vb->type) {
    return (va->type - vb->type);
  }

  switch (va->type) {
    case INTEGER: 
      return (va->vali - vb->vali);
    case DOUBLE:  
      return (va->valf > vb->valf) - (va->valf < vb->valf);
    case CHAR:    
      return (va->valc - vb->valc);
    case LONG:    
      return (va->valh > vb->valh) - (va->valh < vb->valh);
    case BOOLEAN: 
      return (va->valb - vb->valb);
    case STRING: {
      if (va->vals != NULL && vb->vals != NULL) {
        return strcmp(va->vals, vb->vals);
      }
      return 0;
    }
    default: 
      return 0;
  }
}

struct SL_Variable List_sort_fn(struct SL_Code *code,
                                struct SL_L_Function func,
                                struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at List.sort! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals = "Expected list_variable as the first argument to List.sort.";
    return return_var;
  }

  if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer overflow!";
    return return_var;
  }

  struct SL_List *list = &LISTS[first_arg.vali];

  if (list->size > 1) {
    qsort(list->vars, list->size, sizeof(struct SL_Variable), sl_compare_vars);
  }

  return_var.type = BOOLEAN;
  return_var.valb = 1;

  return return_var;
}

struct SL_Variable List_copy_fn(struct SL_Code *code,
                                struct SL_L_Function func,
                                struct SL_Function rfunc) {
  if (func.total_arguments < 1) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at List.copy! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable return_var = {0};

  if (first_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals = "Expected list_variable as the first argument to List.copy.";
    return return_var;
  }

  if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals = "List buffer overflow or invalid list index!";
    return return_var;
  }

  struct SL_List *src_list = &LISTS[first_arg.vali];

  int new_list_index = LISTS_count++;
  struct SL_List *dest_list = &LISTS[new_list_index];

  dest_list->size = src_list->size;

  if (src_list->size > 0) {
    dest_list->vars = (struct SL_Variable *)smalloc(src_list->size * sizeof(struct SL_Variable));

    for (int i = 0; i < src_list->size; i++) {
      dest_list->vars[i] = sl_copy_variable(src_list->vars[i]);
    }
  } else {
    dest_list->vars = NULL; 
  }

  return_var.type = INTEGER;
  return_var.vali = new_list_index;

  return return_var;
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
  char *body = smalloc(body_cap);
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
      body = srealloc(body, body_cap);
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
        body = srealloc(body, body_cap);
      }
      body[body_len++] = '/';
      strcpy(body + body_len, item_str);
      body_len += is_len;
    }

    if (i < func.total_arguments - 1) {
      if (body_len + 2 >= body_cap) {
        body_cap *= 2;
        body = srealloc(body, body_cap);
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
  char *tok = smalloc(tok_cap);

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
        tok = srealloc(tok, tok_cap);
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
    char *full_attr_name = smalloc(total_len + 2);
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
    char *full_func_name = smalloc(total_len + 2);
    snprintf(full_func_name, total_len + 2, "%s:%s", assigned_var,
             raw_func.name);
    struct SL_Function func = {0};
    func = raw_func;
    func.code_tokens[3] = smalloc(assigned_len + 3);
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
  char *full_var_name = smalloc(total_size + 2);
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
  char *full_var_name = smalloc(total_size + 2);
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
    collections.collections = srealloc(collections.collections,
                        collections.capacity * sizeof(struct SL_Collection));
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
      link_func.code_tokens = srealloc(
          link_func.code_tokens, (link_func.code_len + 4) * sizeof(char *));
      link_func.types = srealloc(link_func.types, (link_func.code_len + 4) *
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
  char *collection_new_name = smalloc(total_len + 1);
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
    char *name = smalloc(len);
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
#ifdef ENABLE_NET
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

  char *buffer = smalloc(buffer_size);
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

struct SL_Variable net_select_win_fn(struct SL_Code *code,
                                     struct SL_L_Function func,
                                     struct SL_Function rfunc) {
  if (func.total_arguments < 4) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.select! Needs nfds, readfds, "
                      "writefds, exceptfds (and optional timeout_ms).";
    return return_var;
  }

  struct SL_Variable arg_nfds = sl_get_argument(*code, func, 0);
  struct SL_Variable arg_read = sl_get_argument(*code, func, 1);
  struct SL_Variable arg_write = sl_get_argument(*code, func, 2);
  struct SL_Variable arg_except = sl_get_argument(*code, func, 3);
  struct SL_Variable return_var = {0};

  if (arg_nfds.type != INTEGER || arg_read.type != INTEGER ||
      arg_write.type != INTEGER || arg_except.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals = "First 4 arguments must be integers on net.select.";
    return return_var;
  }

  fd_set *readfds = NULL;
  fd_set *writefds = NULL;
  fd_set *exceptfds = NULL;

  if (arg_read.vali >= 0) {
    if (arg_read.vali >= fd_list_size) {
      return_var.type = ERROR;
      return_var.vals = "Read FD Not found!";
      return return_var;
    }
    readfds = (fd_set *)fd_list[arg_read.vali].fd;
  }
  if (arg_write.vali >= 0) {
    if (arg_write.vali >= fd_list_size) {
      return_var.type = ERROR;
      return_var.vals = "Write FD Not found!";
      return return_var;
    }
    writefds = (fd_set *)fd_list[arg_write.vali].fd;
  }
  if (arg_except.vali >= 0) {
    if (arg_except.vali >= fd_list_size) {
      return_var.type = ERROR;
      return_var.vals = "Except FD Not found!";
      return return_var;
    }
    exceptfds = (fd_set *)fd_list[arg_except.vali].fd;
  }

  struct timeval tv;
  struct timeval *tv_ptr = NULL;

  if (func.total_arguments > 4) {
    struct SL_Variable arg_timeout = sl_get_argument(*code, func, 4);
    if (arg_timeout.type == INTEGER && arg_timeout.vali >= 0) {
      tv.tv_sec = arg_timeout.vali / 1000;
      tv.tv_usec = (arg_timeout.vali % 1000) * 1000;
      tv_ptr = &tv;
    } else if (arg_timeout.type != INTEGER) {
      return_var.type = ERROR;
      return_var.vals = "Timeout argument must be an integer (ms).";
      return return_var;
    }
  }

  int res = select(arg_nfds.vali, readfds, writefds, exceptfds, tv_ptr);

  if (res == SOCKET_ERROR) {
    return_var.type = ERROR;
    return_var.vals = "select() failed.";
    perror("net.select failed!");
    return return_var;
  }

  return_var.type = INTEGER;
  return_var.vali = res;
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
    fd_list = srealloc(fd_list, fd_list_capacity * sizeof(struct SL_FD_List));
  }
  int id = fd_list_size++;
  fd_list[id].fd = smalloc(sizeof(fd_set));
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
  char *buffer = smalloc(buffer_size);
  int read_size = 0;
  if (gotflag == 1)
    read_size = recv(first_arg.vali, buffer, buffer_size, third_arg.vali);
  else
    read_size = recv(first_arg.vali, buffer, buffer_size, 0);
  if (read_size <= 0) {
    return_var.type = ERROR;
    return_var.vals = "Nothing received from client.";
    return return_var;
  }
  buffer[read_size] = '\0';
  return_var.type = STRING;
  return_var.vals = strdup(buffer);
  free(buffer);
  return return_var;
}

struct SL_Variable net_select_posix_fn(struct SL_Code *code,
                                       struct SL_L_Function func,
                                       struct SL_Function rfunc) {
  if (func.total_arguments < 4) {
    struct SL_Variable return_var = {0};
    return_var.type = ERROR;
    return_var.vals = "Error usage at net.select! Needs nfds, readfds, "
                      "writefds, exceptfds (and optional timeout_ms).";
    return return_var;
  }

  struct SL_Variable arg_nfds = sl_get_argument(*code, func, 0);
  struct SL_Variable arg_read = sl_get_argument(*code, func, 1);
  struct SL_Variable arg_write = sl_get_argument(*code, func, 2);
  struct SL_Variable arg_except = sl_get_argument(*code, func, 3);
  struct SL_Variable return_var = {0};

  if (arg_nfds.type != INTEGER || arg_read.type != INTEGER ||
      arg_write.type != INTEGER || arg_except.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals = "First 4 arguments must be integers on net.select.";
    return return_var;
  }

  fd_set *readfds = NULL;
  fd_set *writefds = NULL;
  fd_set *exceptfds = NULL;

  if (arg_read.vali >= 0) {
    if (arg_read.vali >= fd_list_size) {
      return_var.type = ERROR;
      return_var.vals = "Read FD Not found!";
      return return_var;
    }
    readfds = (fd_set *)fd_list[arg_read.vali].fd;
  }
  if (arg_write.vali >= 0) {
    if (arg_write.vali >= fd_list_size) {
      return_var.type = ERROR;
      return_var.vals = "Write FD Not found!";
      return return_var;
    }
    writefds = (fd_set *)fd_list[arg_write.vali].fd;
  }
  if (arg_except.vali >= 0) {
    if (arg_except.vali >= fd_list_size) {
      return_var.type = ERROR;
      return_var.vals = "Except FD Not found!";
      return return_var;
    }
    exceptfds = (fd_set *)fd_list[arg_except.vali].fd;
  }

  struct timeval tv;
  struct timeval *tv_ptr = NULL;

  if (func.total_arguments > 4) {
    struct SL_Variable arg_timeout = sl_get_argument(*code, func, 4);
    if (arg_timeout.type == INTEGER && arg_timeout.vali >= 0) {
      tv.tv_sec = arg_timeout.vali / 1000;
      tv.tv_usec = (arg_timeout.vali % 1000) * 1000;
      tv_ptr = &tv;
    } else if (arg_timeout.type != INTEGER) {
      return_var.type = ERROR;
      return_var.vals = "Timeout argument must be an integer (ms).";
      return return_var;
    }
  }

  int res = select(arg_nfds.vali, readfds, writefds, exceptfds, tv_ptr);

  if (res < 0) {
    return_var.type = ERROR;
    return_var.vals = "select() failed.";
    perror("net.select failed!");
    return return_var;
  }

  return_var.type = INTEGER;
  return_var.vali = res;
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
    fd_list = srealloc(fd_list, fd_list_capacity * sizeof(struct SL_FD_List));
  }
  int id = fd_list_size++;
  fd_list[id].fd = smalloc(sizeof(fd_set));
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
#endif
/* NET */

/* CONSOLE */
#ifdef _WIN32
HANDLE hStdIn, hStdOut;
DWORD oldSettings;
WORD OldColorAttrs;
DWORD OldConsoleMode;
INPUT_RECORD irInBuf[1];
HANDLE hOriginalOut;
HANDLE hAlternateOut;
DWORD originalOutMode;
DWORD originalInMode;

struct SL_Variable console_clear_win_fn(struct SL_Code *code,
                                        struct SL_L_Function func,
                                        struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  DWORD mode = 0;

  if (!GetConsoleMode(hStdOut, &mode)) {
    return_var.type = ERROR;
    return_var.vals = "Cannot get console mode.";
    return return_var;
  }

  const DWORD original_mode = mode;
  mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

  if (!SetConsoleMode(hStdOut, mode)) {
    return_var.type = ERROR;
    return_var.vals = "Cannot set console mode.";
    return return_var;
  }

  const char *clear_seq = "\x1b[2J\x1b[3J\x1b[H";

  if (!sl_console_write_cstr(clear_seq)) {
    SetConsoleMode(hStdOut, original_mode);
    return_var.type = ERROR;
    return_var.vals = "Cannot clear console.";
    return return_var;
  }

  SetConsoleMode(hStdOut, original_mode);

  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable console_fgcolor_win_fn(struct SL_Code *code,
                                          struct SL_L_Function func,
                                          struct SL_Function rfunc) {

  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at console.color! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

  if (first_arg.type != INTEGER) {
    return_var.vals = "All items must be typed as COLOR(integer fixed) on "
                      "console.foreground_color";
  }

  const char *color_seq = "\x1b[39m";

  switch (first_arg.vali) {
  case SL_COLOR_BLACK:
    color_seq = "\x1b[30m";
    break;
  case SL_COLOR_RED:
    color_seq = "\x1b[31m";
    break;
  case SL_COLOR_GREEN:
    color_seq = "\x1b[32m";
    break;
  case SL_COLOR_YELLOW:
    color_seq = "\x1b[33m";
    break;
  case SL_COLOR_BLUE:
    color_seq = "\x1b[34m";
    break;
  case SL_COLOR_MAGENTA:
    color_seq = "\x1b[35m";
    break;
  case SL_COLOR_CYAN:
    color_seq = "\x1b[36m";
    break;
  case SL_COLOR_WHITE:
    color_seq = "\x1b[37m";
    break;

  case SL_COLOR_BRIGHT_BLACK:
    color_seq = "\x1b[90m";
    break;
  case SL_COLOR_BRIGHT_RED:
    color_seq = "\x1b[91m";
    break;
  case SL_COLOR_BRIGHT_GREEN:
    color_seq = "\x1b[92m";
    break;
  case SL_COLOR_BRIGHT_YELLOW:
    color_seq = "\x1b[93m";
    break;
  case SL_COLOR_BRIGHT_BLUE:
    color_seq = "\x1b[94m";
    break;
  case SL_COLOR_BRIGHT_MAGENTA:
    color_seq = "\x1b[95m";
    break;
  case SL_COLOR_BRIGHT_CYAN:
    color_seq = "\x1b[96m";
    break;
  case SL_COLOR_BRIGHT_WHITE:
    color_seq = "\x1b[97m";
    break;

  case SL_COLOR_DEFAULT:
  default:
    color_seq = "\x1b[39m";
    break;
  }

if (!sl_console_write_cstr(color_seq)) {
  return_var.type = ERROR;
  return_var.vals = "Failed to set console text attribute.";
  return return_var;
}
return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable console_bgcolor_win_fn(struct SL_Code *code,
                                          struct SL_L_Function func,
                                          struct SL_Function rfunc) {

  struct SL_Variable return_var = {0};

  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at console.background_color! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

  if (first_arg.type != INTEGER) {
    return_var.vals = "All items must be typed as COLOR(integer fixed) on "
                      "console.background_color";
  }

  const char *color_seq = "\x1b[49m";

  switch (first_arg.vali) {
  case SL_COLOR_BLACK:
    color_seq = "\x1b[40m";
    break;
  case SL_COLOR_RED:
    color_seq = "\x1b[41m";
    break;
  case SL_COLOR_GREEN:
    color_seq = "\x1b[42m";
    break;
  case SL_COLOR_YELLOW:
    color_seq = "\x1b[43m";
    break;
  case SL_COLOR_BLUE:
    color_seq = "\x1b[44m";
    break;
  case SL_COLOR_MAGENTA:
    color_seq = "\x1b[45m";
    break;
  case SL_COLOR_CYAN:
    color_seq = "\x1b[46m";
    break;
  case SL_COLOR_WHITE:
    color_seq = "\x1b[47m";
    break;

  case SL_COLOR_BRIGHT_BLACK:
    color_seq = "\x1b[100m";
    break;
  case SL_COLOR_BRIGHT_RED:
    color_seq = "\x1b[101m";
    break;
  case SL_COLOR_BRIGHT_GREEN:
    color_seq = "\x1b[102m";
    break;
  case SL_COLOR_BRIGHT_YELLOW:
    color_seq = "\x1b[103m";
    break;
  case SL_COLOR_BRIGHT_BLUE:
    color_seq = "\x1b[104m";
    break;
  case SL_COLOR_BRIGHT_MAGENTA:
    color_seq = "\x1b[105m";
    break;
  case SL_COLOR_BRIGHT_CYAN:
    color_seq = "\x1b[106m";
    break;
  case SL_COLOR_BRIGHT_WHITE:
    color_seq = "\x1b[107m";
    break;

  case SL_COLOR_DEFAULT:
  default:
    color_seq = "\x1b[49m";
    break;
  }

if (!sl_console_write_cstr(color_seq)) {
  return_var.type = ERROR;
  return_var.vals = "Failed to set console text attribute.";
  return return_var;
}
return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable console_reset_color_win_fn(struct SL_Code *code,
                                              struct SL_L_Function func,
                                              struct SL_Function rfunc) {

  struct SL_Variable return_var = {0};

  const char *reset_seq = "\x1b[0m";

if (!sl_console_write_cstr(reset_seq)) {
  return_var.type = ERROR;
  return_var.vals = "Failed to set console text attribute.";
  return return_var;
}  
  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable console_cursor_position_win_fn(struct SL_Code *code,
                                                  struct SL_L_Function func,
                                                  struct SL_Function rfunc) {

  struct SL_Variable return_var = {0};
  if (func.total_arguments < 2) {
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at console.cursor_position! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);

  if (first_arg.type != INTEGER || second_arg.type != INTEGER) {
    return_var.vals =
        "All items must be typed as integer on console.cursor_position";
    return_var.type = ERROR;
    return return_var;
  }

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", second_arg.vali + 1,
           first_arg.vali + 1);

if (!sl_console_write_cstr(buf)) {
  return_var.type = ERROR;
  return_var.vals = "Failed to set cursor position.";
  return return_var;
}
return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable console_cursor_visibility_win_fn(struct SL_Code *code,
                                                    struct SL_L_Function func,
                                                    struct SL_Function rfunc) {

  struct SL_Variable return_var = {0};

  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at console.cursor_visibility! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

  if (first_arg.type != BOOLEAN) {
    return_var.vals =
        "All items must be typed as bool on console.cursor_visibility";
  }

  const char *visibility_seq;

  if (first_arg.valb == 1) {
    visibility_seq = "\x1b[?25h";
  } else {
    visibility_seq = "\x1b[?25l";
  }

if (!sl_console_write_cstr(visibility_seq)) {
  return_var.type = ERROR;
  return_var.vals = "Failed to set cursor visibility.";
  return return_var;
}
  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable console_raw_mode_win_fn(struct SL_Code *code,
                                           struct SL_L_Function func,
                                           struct SL_Function rfunc) {

  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at console.raw_mode! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

  CONSOLE_CURSOR_INFO ConsoleCursorInfo;
  if (!GetConsoleCursorInfo(hStdOut, &ConsoleCursorInfo)) {
    return_var.type = ERROR;
    return_var.vals = "Failed to get cursor info.";
    return return_var;
  }

  if (first_arg.type != BOOLEAN) {
    return_var.vals = "All items must be typed as bool on console.raw_mode";
  }

  DWORD mode;
  mode = (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS) &
         ~ENABLE_QUICK_EDIT_MODE;

  if (first_arg.valb == 1) {
    if (!SetConsoleMode(hStdIn, mode)) {
      return_var.type = ERROR;
      return_var.vals = "Failed to set console raw mode.";
      return return_var;
    }
  } else {
    if (!SetConsoleMode(hStdIn, OldConsoleMode)) {
      return_var.type = ERROR;
      return_var.vals = "Failed to set console raw mode.";
      return return_var;
    }
  }

  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable console_get_event_win_fn(struct SL_Code *code,
                                            struct SL_L_Function func,
                                            struct SL_Function rfunc) {

  DWORD events_read = 0;
  struct SL_Variable return_var = {0};
  if (!ReadConsoleInput(hStdIn, irInBuf, 1, &events_read)) {
    return_var.type = ERROR;
    return_var.vals = "Failed to get event";
    return return_var;
  }

  switch (irInBuf[0].EventType) {
  case KEY_EVENT:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_EVENT;
    break;
  case MOUSE_EVENT:
    return_var.type = INTEGER;
    return_var.vali = SL_MOUSE_EVENT;
    break;
  case WINDOW_BUFFER_SIZE_EVENT:
    return_var.type = INTEGER;
    return_var.vali = SL_WINDOW_RESIZE_EVENT;
    break;
  case FOCUS_EVENT:
  case MENU_EVENT:
    return_var.type = INTEGER;
    return_var.vali = SL_UNDEFINED_EVENT;
    break;
  default:
    return_var.type = INTEGER;
    return_var.vali = SL_UNDEFINED_EVENT;
    break;
  }
  return return_var;
}

struct SL_Variable console_key_event_is_pressed_win_fn(
    struct SL_Code *code, struct SL_L_Function func, struct SL_Function rfunc) {

  struct SL_Variable return_var = {0};
  if (irInBuf[0].Event.KeyEvent.bKeyDown) {
    return_var.type = BOOLEAN;
    return_var.valb = 1;
    return return_var;
  }
  return_var.type = BOOLEAN;
  return_var.valb = 0;
  return return_var;
}

struct SL_Variable console_key_event_get_key_win_fn(struct SL_Code *code,
                                                    struct SL_L_Function func,
                                                    struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  KEY_EVENT_RECORD key_event = irInBuf[0].Event.KeyEvent;

  switch (key_event.wVirtualKeyCode) {
  case VK_RETURN:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_ENTER;
    break;
  case VK_ESCAPE:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_ESCAPE;
    break;
  case VK_BACK:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_BACKSPACE;
    break;
  case VK_TAB:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_TAB;
    break;

  case VK_UP:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_UP;
    break;
  case VK_DOWN:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_DOWN;
    break;
  case VK_LEFT:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_LEFT;
    break;
  case VK_RIGHT:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_RIGHT;
    break;

  case VK_HOME:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_HOME;
    break;
  case VK_END:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_END;
    break;
  case VK_INSERT:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_INSERT;
    break;
  case VK_DELETE:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_DELETE;
    break;
  case VK_PRIOR:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_PAGE_UP;
    break;
  case VK_NEXT:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_PAGE_DOWN;
    break;

  case VK_F1:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_F1;
    break;
  case VK_F2:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_F2;
    break;
  case VK_F3:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_F3;
    break;
  case VK_F4:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_F4;
    break;
  case VK_F5:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_F5;
    break;
  case VK_F6:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_F6;
    break;
  case VK_F7:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_F7;
    break;
  case VK_F8:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_F8;
    break;
  case VK_F9:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_F9;
    break;
  case VK_F10:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_F10;
    break;
  case VK_F11:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_F11;
    break;
  case VK_F12:
    return_var.type = INTEGER;
    return_var.vali = SL_KEY_F12;
    break;

  default:
    if (key_event.wVirtualKeyCode >= 'A' && key_event.wVirtualKeyCode <= 'Z') {
      return_var.type = CHAR;
      BOOL is_shift = (key_event.dwControlKeyState & SHIFT_PRESSED) != 0;
      BOOL is_caps = (key_event.dwControlKeyState & CAPSLOCK_ON) != 0;
      if (is_shift ^ is_caps) {
        return_var.valc = (char)key_event.wVirtualKeyCode;
      } else {
        return_var.valc = (char)(key_event.wVirtualKeyCode + 32);
      }
    } else if (key_event.uChar.AsciiChar != 0) {
      return_var.type = CHAR;
      return_var.valc = key_event.uChar.AsciiChar;
    } else {
      return_var.type = CHAR;
      return_var.valc = 0;
    }
    break;
  }

  return return_var;
}

struct SL_Variable console_key_event_get_mod_win_fn(struct SL_Code *code,
                                                    struct SL_L_Function func,
                                                    struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  DWORD state = irInBuf[0].Event.KeyEvent.dwControlKeyState;
  int mods = SL_MOD_NONE;

  if (state & SHIFT_PRESSED) {
    mods |= SL_MOD_SHIFT;
  }
  if (state & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
    mods |= SL_MOD_CTRL;
  }
  if (state & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) {
    mods |= SL_MOD_ALT;
  }

  return_var.type = INTEGER;
  return_var.vali = mods;
  return return_var;
}

struct SL_Variable console_mouse_event_get_type_win_fn(
    struct SL_Code *code, struct SL_L_Function func, struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  MOUSE_EVENT_RECORD mer = irInBuf[0].Event.MouseEvent;

  return_var.type = INTEGER;

  switch (mer.dwEventFlags) {
  case 0:
    if (mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
      return_var.vali = SL_MOUSE_LEFT_PRESSED;
    } else if (mer.dwButtonState & RIGHTMOST_BUTTON_PRESSED) {
      return_var.vali = SL_MOUSE_RIGHT_PRESSED;
    } else if (mer.dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) {
      return_var.vali = SL_MOUSE_MIDDLE_PRESSED;
    } else {
      return_var.vali = SL_MOUSE_NONE;
    }
    break;

  case MOUSE_MOVED:
    return_var.vali = SL_MOUSE_MOVED;
    break;

  case DOUBLE_CLICK:
    if (mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
      return_var.vali = SL_MOUSE_DOUBLE_CLICK;
    } else {
      return_var.vali = SL_MOUSE_DOUBLE_CLICK;
    }
    break;

  case MOUSE_WHEELED:
    if ((short)HIWORD(mer.dwButtonState) > 0) {
      return_var.vali = SL_MOUSE_WHEEL_UP;
    } else {
      return_var.vali = SL_MOUSE_WHEEL_DOWN;
    }
    break;

  default:
    return_var.vali = SL_MOUSE_NONE;
    break;
  }

  return return_var;
}

struct SL_Variable console_mouse_event_get_x_win_fn(struct SL_Code *code,
                                                    struct SL_L_Function func,
                                                    struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  return_var.type = INTEGER;
  return_var.vali = irInBuf[0].Event.MouseEvent.dwMousePosition.X;
  return return_var;
}

struct SL_Variable console_mouse_event_get_y_win_fn(struct SL_Code *code,
                                                    struct SL_L_Function func,
                                                    struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  return_var.type = INTEGER;
  return_var.vali = irInBuf[0].Event.MouseEvent.dwMousePosition.Y;
  return return_var;
}

struct SL_Variable console_get_width_win_fn(struct SL_Code *code,
                                            struct SL_L_Function func,
                                            struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

  return_var.type = INTEGER;
  if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
    return_var.vali = csbi.srWindow.Right - csbi.srWindow.Left + 1;
  } else {
    return_var.vali = 0;
  }

  return return_var;
}

struct SL_Variable console_get_height_win_fn(struct SL_Code *code,
                                             struct SL_L_Function func,
                                             struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

  return_var.type = INTEGER;
  if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
    return_var.vali = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
  } else {
    return_var.vali = 0;
  }

  return return_var;
}

struct SL_Variable console_enter_alt_win_fn(struct SL_Code *code,
                                            struct SL_L_Function func,
                                            struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
if (!sl_console_write_cstr("\x1b[?1049h")) {
  return_var.type = ERROR;
  return_var.vals = "Failed to enter alternate screen.";
  return return_var;
}  return return_var;
}

struct SL_Variable console_leave_alt_win_fn(struct SL_Code *code,
                                            struct SL_L_Function func,
                                            struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (!sl_console_write_cstr("\x1b[?1049l")) {
    return_var.type = ERROR;
    return_var.vals = "Failed to leave alternate screen.";
    return return_var;
  }  return return_var;
}

#else
struct termios orig_termios;
int posix_last_event_type = 0;
int posix_last_key_code = 0;
int posix_last_key_pressed = 0;
int posix_last_key_mod = 0;
int posix_last_mouse_type = 0;
int posix_last_mouse_x = 0;
int posix_last_mouse_y = 0;

void disableRawMode() {
  sl_console_flush();
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
  fflush(stdout);
  const char *disable_mouse = "\x1b[?1003l\x1b[?1006l\x1b[?7h";
  write(STDOUT_FILENO, disable_mouse, strlen(disable_mouse));
  printf("\x1b[?25h");
  sl_console_flush();
}

struct SL_Variable console_raw_mode_posix_fn(struct SL_Code *code,
                                             struct SL_L_Function func,
                                             struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at console.raw_mode! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

  if (first_arg.type != BOOLEAN) {
    return_var.vals = "All items must be typed as bool on console.raw_mode";
  }

  if (first_arg.valb == 1) {
    sl_console_flush();
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
      return_var.type = ERROR;
      return_var.vals = "Failed to set console raw mode.";
      return return_var;
    }

    fflush(stdout);
    const char *enable_mouse = "\x1b[?1003h\x1b[?1006h\x1b[?7l";
    write(STDOUT_FILENO, enable_mouse, strlen(enable_mouse));
  } else {
    disableRawMode();
  }

  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable console_clear_posix_fn(struct SL_Code *code,
                                          struct SL_L_Function func,
                                          struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  const char *clear_seq = "\x1b[2J\x1b[3J\x1b[H";

if (!sl_console_write_cstr(clear_seq)) {
  return_var.vals = "Cannot clear console.";
  return_var.type = ERROR;
  return return_var;
}  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable console_fgcolor_posix_fn(struct SL_Code *code,
                                            struct SL_L_Function func,
                                            struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at console.color! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

  const char *color_seq = "\x1b[39m";
  switch (first_arg.vali) {
  case SL_COLOR_BLACK:
    color_seq = "\x1b[30m";
    break;
  case SL_COLOR_RED:
    color_seq = "\x1b[31m";
    break;
  case SL_COLOR_GREEN:
    color_seq = "\x1b[32m";
    break;
  case SL_COLOR_YELLOW:
    color_seq = "\x1b[33m";
    break;
  case SL_COLOR_BLUE:
    color_seq = "\x1b[34m";
    break;
  case SL_COLOR_MAGENTA:
    color_seq = "\x1b[35m";
    break;
  case SL_COLOR_CYAN:
    color_seq = "\x1b[36m";
    break;
  case SL_COLOR_WHITE:
    color_seq = "\x1b[37m";
    break;
  case SL_COLOR_BRIGHT_BLACK:
    color_seq = "\x1b[90m";
    break;
  case SL_COLOR_BRIGHT_RED:
    color_seq = "\x1b[91m";
    break;
  case SL_COLOR_BRIGHT_GREEN:
    color_seq = "\x1b[92m";
    break;
  case SL_COLOR_BRIGHT_YELLOW:
    color_seq = "\x1b[93m";
    break;
  case SL_COLOR_BRIGHT_BLUE:
    color_seq = "\x1b[94m";
    break;
  case SL_COLOR_BRIGHT_MAGENTA:
    color_seq = "\x1b[95m";
    break;
  case SL_COLOR_BRIGHT_CYAN:
    color_seq = "\x1b[96m";
    break;
  case SL_COLOR_BRIGHT_WHITE:
    color_seq = "\x1b[97m";
    break;
  case SL_COLOR_DEFAULT:
  default:
    color_seq = "\x1b[39m";
    break;
  }
if (!sl_console_write_cstr(color_seq)) {
  return_var.type = ERROR;
  return_var.vals = "Failed to set foreground color.";
  return return_var;
}
  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable console_bgcolor_posix_fn(struct SL_Code *code,
                                            struct SL_L_Function func,
                                            struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at console.background_color! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

  const char *color_seq = "\x1b[49m";
  switch (first_arg.vali) {
  case SL_COLOR_BLACK:
    color_seq = "\x1b[40m";
    break;
  case SL_COLOR_RED:
    color_seq = "\x1b[41m";
    break;
  case SL_COLOR_GREEN:
    color_seq = "\x1b[42m";
    break;
  case SL_COLOR_YELLOW:
    color_seq = "\x1b[43m";
    break;
  case SL_COLOR_BLUE:
    color_seq = "\x1b[44m";
    break;
  case SL_COLOR_MAGENTA:
    color_seq = "\x1b[45m";
    break;
  case SL_COLOR_CYAN:
    color_seq = "\x1b[46m";
    break;
  case SL_COLOR_WHITE:
    color_seq = "\x1b[47m";
    break;
  case SL_COLOR_BRIGHT_BLACK:
    color_seq = "\x1b[100m";
    break;
  case SL_COLOR_BRIGHT_RED:
    color_seq = "\x1b[101m";
    break;
  case SL_COLOR_BRIGHT_GREEN:
    color_seq = "\x1b[102m";
    break;
  case SL_COLOR_BRIGHT_YELLOW:
    color_seq = "\x1b[103m";
    break;
  case SL_COLOR_BRIGHT_BLUE:
    color_seq = "\x1b[104m";
    break;
  case SL_COLOR_BRIGHT_MAGENTA:
    color_seq = "\x1b[105m";
    break;
  case SL_COLOR_BRIGHT_CYAN:
    color_seq = "\x1b[106m";
    break;
  case SL_COLOR_BRIGHT_WHITE:
    color_seq = "\x1b[107m";
    break;
  case SL_COLOR_DEFAULT:
  default:
    color_seq = "\x1b[49m";
    break;
  }

 if (!sl_console_write_cstr(color_seq)) {
  return_var.type = ERROR;
  return_var.vals = "Failed to set background color.";
  return return_var;
}

  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable console_reset_color_posix_fn(struct SL_Code *code,
                                                struct SL_L_Function func,
                                                struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  const char *reset_seq = "\x1b[0m";

  fflush(stdout);
  write(STDOUT_FILENO, reset_seq, strlen(reset_seq));

  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable console_cursor_position_posix_fn(struct SL_Code *code,
                                                    struct SL_L_Function func,
                                                    struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 2) {
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at console.cursor_position! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable second_arg = sl_get_argument(*code, func, 1);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", second_arg.vali + 1,
           first_arg.vali + 1);

if (!sl_console_write_cstr(buf)) {
  return_var.type = ERROR;
  return_var.vals = "Failed to set cursor position.";
  return return_var;
}

  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable console_cursor_visibility_posix_fn(
    struct SL_Code *code, struct SL_L_Function func, struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at console.cursor_visibility! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

const char *visibility_seq;

if (first_arg.valb == 1) {
  visibility_seq = "\x1b[?25h";
} else {
  visibility_seq = "\x1b[?25l";
}

if (!sl_console_write_cstr(visibility_seq)) {
  return_var.type = ERROR;
  return_var.vals = "Failed to set cursor visibility.";
  return return_var;
}
  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable console_get_width_posix_fn(struct SL_Code *code,
                                              struct SL_L_Function func,
                                              struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  struct winsize w;

  return_var.type = INTEGER;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1) {
    return_var.vali = w.ws_col;
  } else {
    return_var.vali = 0;
  }
  return return_var;
}

struct SL_Variable console_get_height_posix_fn(struct SL_Code *code,
                                               struct SL_L_Function func,
                                               struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  struct winsize w;

  return_var.type = INTEGER;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1) {
    return_var.vali = w.ws_row;
  } else {
    return_var.vali = 0;
  }
  return return_var;
}

struct SL_Variable console_enter_alt_posix_fn(struct SL_Code *code,
                                              struct SL_L_Function func,
                                              struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
    if (!sl_console_write_cstr("\x1b[?1049h")) {
    return_var.type = ERROR;
    return_var.vals = "Failed to enter alternate screen.";
    return return_var;
  }
  return return_var;
}

struct SL_Variable console_leave_alt_posix_fn(struct SL_Code *code,
                                              struct SL_L_Function func,
                                              struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  
  if (!sl_console_write_cstr("\x1b[?1049l")) {
    return_var.type = ERROR;
    return_var.vals = "Failed to leave alternate screen.";
    return return_var;
  }

  return return_var;
}

struct SL_Variable console_get_event_posix_fn(struct SL_Code *code,
                                              struct SL_L_Function func,
                                              struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  sl_console_flush();
  char c;
  int nread = read(STDIN_FILENO, &c, 1);

  if (nread != 1) {
    return_var.type = INTEGER;
    return_var.vali = SL_UNDEFINED_EVENT;
    return return_var;
  }

  if (c == '\x1b') {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) {
      posix_last_event_type = SL_KEY_EVENT;
      posix_last_key_code = SL_KEY_ESCAPE;
      posix_last_key_pressed = 1;
      posix_last_key_mod = SL_MOD_NONE;

      return_var.type = INTEGER;
      return_var.vali = posix_last_event_type;
      return return_var;
    }
    if (read(STDIN_FILENO, &seq[1], 1) != 1) {
      return_var.type = INTEGER;
      return_var.vali = SL_UNDEFINED_EVENT;
      return return_var;
    }

    if (seq[0] == '[') {
      if ((seq[1] >= '0' && seq[1] <= '9') || seq[1] == '<') {
        char ext[32];
        ext[0] = seq[1];
        int i = 1;
        while (i < 31) {
          if (read(STDIN_FILENO, &ext[i], 1) != 1)
            break;
          if (ext[i] == '~' || ext[i] == 'M' || ext[i] == 'm' ||
              (ext[i] >= 'A' && ext[i] <= 'Z'))
            break;
          i++;
        }
        ext[i + 1] = '\0';

        if (ext[i] == 'M' || ext[i] == 'm') {
          if (ext[0] == '<') {
            int b, x, y;
            sscanf(ext, "<%d;%d;%d", &b, &x, &y);
            posix_last_event_type = SL_MOUSE_EVENT;
            posix_last_mouse_x = x - 1;
            posix_last_mouse_y = y - 1;

            if (ext[i] == 'M') {
              if (b == 0)
                posix_last_mouse_type = SL_MOUSE_LEFT_PRESSED;
              else if (b == 1)
                posix_last_mouse_type = SL_MOUSE_MIDDLE_PRESSED;
              else if (b == 2)
                posix_last_mouse_type = SL_MOUSE_RIGHT_PRESSED;
              else if (b == 64)
                posix_last_mouse_type = SL_MOUSE_WHEEL_UP;
              else if (b == 65)
                posix_last_mouse_type = SL_MOUSE_WHEEL_DOWN;
              else
                posix_last_mouse_type = SL_MOUSE_MOVED;
            } else {
              posix_last_mouse_type = SL_MOUSE_NONE;
            }
          }
        } else if (ext[i] == '~') {
          posix_last_event_type = SL_KEY_EVENT;
          posix_last_key_pressed = 1;
          posix_last_key_mod = SL_MOD_NONE;
          int code = atoi(ext);
          switch (code) {
          case 1:
          case 7:
            posix_last_key_code = SL_KEY_HOME;
            break;
          case 2:
            posix_last_key_code = SL_KEY_INSERT;
            break;
          case 3:
            posix_last_key_code = SL_KEY_DELETE;
            break;
          case 4:
          case 8:
            posix_last_key_code = SL_KEY_END;
            break;
          case 5:
            posix_last_key_code = SL_KEY_PAGE_UP;
            break;
          case 6:
            posix_last_key_code = SL_KEY_PAGE_DOWN;
            break;
          case 15:
            posix_last_key_code = SL_KEY_F5;
            break;
          case 17:
            posix_last_key_code = SL_KEY_F6;
            break;
          case 18:
            posix_last_key_code = SL_KEY_F7;
            break;
          case 19:
            posix_last_key_code = SL_KEY_F8;
            break;
          case 20:
            posix_last_key_code = SL_KEY_F9;
            break;
          case 21:
            posix_last_key_code = SL_KEY_F10;
            break;
          case 23:
            posix_last_key_code = SL_KEY_F11;
            break;
          case 24:
            posix_last_key_code = SL_KEY_F12;
            break;
          }
        }
      } else {
        posix_last_event_type = SL_KEY_EVENT;
        posix_last_key_pressed = 1;
        posix_last_key_mod = SL_MOD_NONE;
        switch (seq[1]) {
        case 'A':
          posix_last_key_code = SL_KEY_UP;
          break;
        case 'B':
          posix_last_key_code = SL_KEY_DOWN;
          break;
        case 'C':
          posix_last_key_code = SL_KEY_RIGHT;
          break;
        case 'D':
          posix_last_key_code = SL_KEY_LEFT;
          break;
        case 'H':
          posix_last_key_code = SL_KEY_HOME;
          break;
        case 'F':
          posix_last_key_code = SL_KEY_END;
          break;
        default:
          posix_last_event_type = SL_UNDEFINED_EVENT;
          break;
        }
      }
    } else if (seq[0] == 'O') {
      posix_last_event_type = SL_KEY_EVENT;
      posix_last_key_pressed = 1;
      posix_last_key_mod = SL_MOD_NONE;
      switch (seq[1]) {
      case 'P':
        posix_last_key_code = SL_KEY_F1;
        break;
      case 'Q':
        posix_last_key_code = SL_KEY_F2;
        break;
      case 'R':
        posix_last_key_code = SL_KEY_F3;
        break;
      case 'S':
        posix_last_key_code = SL_KEY_F4;
        break;
      }
    }
  } else {
    posix_last_event_type = SL_KEY_EVENT;
    posix_last_key_pressed = 1;
    posix_last_key_mod = SL_MOD_NONE;

    if (c == '\n' || c == '\r') {
      posix_last_key_code = SL_KEY_ENTER;
    } else if (c == '\t') {
      posix_last_key_code = SL_KEY_TAB;
    } else if (c == 127 || c == '\b') {
      posix_last_key_code = SL_KEY_BACKSPACE;
    } else if (c >= 1 && c <= 26) {
      posix_last_key_mod = SL_MOD_CTRL;
      posix_last_key_code = c + 'a' - 1;
    } else {
      posix_last_key_code = c;
    }
  }

  return_var.type = INTEGER;
  return_var.vali = posix_last_event_type;
  return return_var;
}

struct SL_Variable console_key_event_is_pressed_posix_fn(
    struct SL_Code *code, struct SL_L_Function func, struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  return_var.type = BOOLEAN;
  return_var.valb = posix_last_key_pressed;
  return return_var;
}

struct SL_Variable console_key_event_get_key_posix_fn(
    struct SL_Code *code, struct SL_L_Function func, struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  if (posix_last_key_code >= 32 && posix_last_key_code <= 126) {
    return_var.type = CHAR;
    return_var.valc = (char)posix_last_key_code;
  } else {
    return_var.type = INTEGER;
    return_var.vali = posix_last_key_code;
  }

  return return_var;
}

struct SL_Variable console_key_event_get_mod_posix_fn(
    struct SL_Code *code, struct SL_L_Function func, struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  return_var.type = INTEGER;
  return_var.vali = posix_last_key_mod;
  return return_var;
}

struct SL_Variable console_mouse_event_get_type_posix_fn(
    struct SL_Code *code, struct SL_L_Function func, struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  return_var.type = INTEGER;
  return_var.vali = posix_last_mouse_type;
  return return_var;
}

struct SL_Variable console_mouse_event_get_x_posix_fn(
    struct SL_Code *code, struct SL_L_Function func, struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  return_var.type = INTEGER;
  return_var.vali = posix_last_mouse_x;
  return return_var;
}

struct SL_Variable console_mouse_event_get_y_posix_fn(
    struct SL_Code *code, struct SL_L_Function func, struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  return_var.type = INTEGER;
  return_var.vali = posix_last_mouse_y;
  return return_var;
}
#endif
struct SL_Variable console_clear_line_fn(struct SL_Code *code,
                                         struct SL_L_Function func,
                                         struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
    if (!sl_console_write_cstr("\033[2K")) {
    return_var.type = ERROR;
    return_var.vals = "Failed to clear console line.";
    return return_var;
  }
  return return_var;
}
struct SL_Variable console_begin_update_fn(
    struct SL_Code *code,
    struct SL_L_Function func,
    struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  if (!sl_console_write_cstr("\x1b[?2026h")) {
    return_var.type = ERROR;
    return_var.vals = "Failed to begin synchronized update.";
    return return_var;
  }

  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}

struct SL_Variable console_end_update_fn(
    struct SL_Code *code,
    struct SL_L_Function func,
    struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  if (!sl_console_write_cstr("\x1b[?2026l")) {
    return_var.type = ERROR;
    return_var.vals = "Failed to end synchronized update.";
    return return_var;
  }

  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}
struct SL_Variable console_auto_wrap_fn(struct SL_Code *code,
                                           struct SL_L_Function func,
                                           struct SL_Function rfunc) {

  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at console.autowrap! Not enough arguments.";
    return return_var;
  }

  struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

  if (first_arg.type != BOOLEAN) {
    return_var.vals = "All items must be typed as bool on console.autowrap";
  }


  if (first_arg.valb == 1) {
    if (!sl_console_write_cstr("\x1b[?7h")) {
      return_var.type = ERROR;
      return_var.vals = "Failed to enable autowrap.";
      return return_var;
    }
  } else {
    if (!sl_console_write_cstr("\x1b[?7l")) {
      return_var.type = ERROR;
      return_var.vals = "Failed to disable autowrap.";
      return return_var;
    }
  }

  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}
struct SL_Variable console_write_at_fn(struct SL_Code *code,
                                       struct SL_L_Function func,
                                       struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  if (func.total_arguments < 3) {
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at console.write_at! Expected x, y, text.";
    return return_var;
  }

  struct SL_Variable x_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable y_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable text_arg = sl_get_argument(*code, func, 2);

  if (x_arg.type != INTEGER || y_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "console.write_at expects integer x and y coordinates.";
    return return_var;
  }

  if (text_arg.type != STRING) {
    return_var.type = ERROR;
    return_var.vals =
        "console.write_at expects a string as the third argument.";
    return return_var;
  }

  if (x_arg.vali < 0 || y_arg.vali < 0) {
    return_var.type = ERROR;
    return_var.vals =
        "console.write_at coordinates cannot be negative.";
    return return_var;
  }

  char *text = sl_string_getter(text_arg.vals);

  if (text == NULL) {
    return_var.type = ERROR;
    return_var.vals =
        "console.write_at failed to decode text.";
    return return_var;
  }

  int prefix_len = snprintf(
      NULL,
      0,
      "\033[%d;%dH",
      y_arg.vali + 1,
      x_arg.vali + 1
  );

  if (prefix_len < 0) {
    free(text);
    return_var.type = ERROR;
    return_var.vals =
        "console.write_at failed to format cursor position.";
    return return_var;
  }

  size_t text_len = strlen(text);
  size_t total_len = (size_t)prefix_len + text_len;

  char *output = malloc(total_len + 1);

  if (output == NULL) {
    free(text);
    return_var.type = ERROR;
    return_var.vals =
        "console.write_at failed to allocate output buffer.";
    return return_var;
  }

  snprintf(
      output,
      (size_t)prefix_len + 1,
      "\033[%d;%dH",
      y_arg.vali + 1,
      x_arg.vali + 1
  );

  memcpy(output + prefix_len, text, text_len);
  output[total_len] = '\0';

  int success = sl_console_write(output, total_len);

  free(output);
  free(text);

  if (!success) {
    return_var.type = ERROR;
    return_var.vals =
        "console.write_at failed to write output.";
    return return_var;
  }

  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}
struct SL_Variable console_fill_rect_fn(struct SL_Code *code,
                                        struct SL_L_Function func,
                                        struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};

  if (func.total_arguments < 5) {
    return_var.type = ERROR;
    return_var.vals =
        "Error usage at console.fill_rect! Expected x, y, width, height, char.";
    return return_var;
  }

  struct SL_Variable x_arg = sl_get_argument(*code, func, 0);
  struct SL_Variable y_arg = sl_get_argument(*code, func, 1);
  struct SL_Variable width_arg = sl_get_argument(*code, func, 2);
  struct SL_Variable height_arg = sl_get_argument(*code, func, 3);
  struct SL_Variable fill_arg = sl_get_argument(*code, func, 4);

  if (x_arg.type != INTEGER ||
      y_arg.type != INTEGER ||
      width_arg.type != INTEGER ||
      height_arg.type != INTEGER) {
    return_var.type = ERROR;
    return_var.vals =
        "console.fill_rect expects integer x, y, width and height.";
    return return_var;
  }

  if (x_arg.vali < 0 ||
      y_arg.vali < 0 ||
      width_arg.vali <= 0 ||
      height_arg.vali <= 0) {
    return_var.type = ERROR;
    return_var.vals =
        "console.fill_rect received invalid rectangle dimensions.";
    return return_var;
  }

  char fill_char = '\0';

  if (fill_arg.type == CHAR) {
    fill_char = fill_arg.valc;
  } else if (fill_arg.type == STRING) {
    char *decoded = sl_string_getter(fill_arg.vals);

    if (decoded == NULL) {
      return_var.type = ERROR;
      return_var.vals =
          "console.fill_rect failed to decode fill character.";
      return return_var;
    }

    size_t decoded_len = strlen(decoded);

    if (decoded_len != 1) {
      free(decoded);
      return_var.type = ERROR;
      return_var.vals =
          "console.fill_rect expects one character as the fill value.";
      return return_var;
    }

    fill_char = decoded[0];
    free(decoded);
  } else {
    return_var.type = ERROR;
    return_var.vals =
        "console.fill_rect expects a CHAR or one-character STRING.";
    return return_var;
  }

  size_t row_size = (size_t)width_arg.vali;
  size_t total_size = 0;

  for (int row = 0; row < height_arg.vali; row++) {
    int cursor_len = snprintf(
        NULL,
        0,
        "\033[%d;%dH",
        y_arg.vali + row + 1,
        x_arg.vali + 1
    );

    if (cursor_len < 0) {
      return_var.type = ERROR;
      return_var.vals =
          "console.fill_rect failed to format cursor position.";
      return return_var;
    }

    total_size += (size_t)cursor_len + row_size;
  }

  char *output = smalloc(total_size + 1);

  size_t offset = 0;

  for (int row = 0; row < height_arg.vali; row++) {
    int cursor_len = snprintf(
        output + offset,
        total_size + 1 - offset,
        "\033[%d;%dH",
        y_arg.vali + row + 1,
        x_arg.vali + 1
    );

    if (cursor_len < 0) {
      free(output);
      return_var.type = ERROR;
      return_var.vals =
          "console.fill_rect failed to write cursor position.";
      return return_var;
    }

    offset += (size_t)cursor_len;

    memset(output + offset, (unsigned char)fill_char, row_size);
    offset += row_size;
  }

  output[offset] = '\0';

  int success = sl_console_write(output, offset);

  free(output);

  if (!success) {
    return_var.type = ERROR;
    return_var.vals =
        "console.fill_rect failed to write output.";
    return return_var;
  }

  return_var.type = BOOLEAN;
  return_var.valb = 1;
  return return_var;
}
/* CONSOLE */

/* MATH */
struct SL_Variable math_pow_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 2) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at math.pow! Not enough arguments.";
    return return_var;
  }
  double base, exp;
  if (!sl_get_double(sl_get_argument(*code, func, 0), &base) ||
      !sl_get_double(sl_get_argument(*code, func, 1), &exp)) {
    return_var.type = ERROR;
    return_var.vals = "Expected integer or double arguments for math.pow.";
    return return_var;
  }
  return_var.type = DOUBLE;
  return_var.valf = pow(base, exp);
  return return_var;
}

struct SL_Variable math_sqrt_fn(struct SL_Code *code, struct SL_L_Function func,
                                struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at math.sqrt! Not enough arguments.";
    return return_var;
  }
  double val;
  if (!sl_get_double(sl_get_argument(*code, func, 0), &val)) {
    return_var.type = ERROR;
    return_var.vals = "Expected integer or double argument for math.sqrt.";
    return return_var;
  }
  if (val < 0) {
    return_var.type = ERROR;
    return_var.vals = "Math domain error: sqrt of negative number!";
    return return_var;
  }
  return_var.type = DOUBLE;
  return_var.valf = sqrt(val);
  return return_var;
}

struct SL_Variable math_abs_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at math.abs! Not enough arguments.";
    return return_var;
  }
  struct SL_Variable arg = sl_get_argument(*code, func, 0);
  if (arg.type == INTEGER) {
    return_var.type = INTEGER;
    return_var.vali = labs(arg.vali);
  } else if (arg.type == DOUBLE) {
    return_var.type = DOUBLE;
    return_var.valf = fabs(arg.valf);
  } else {
    return_var.type = ERROR;
    return_var.vals = "Expected integer or double argument for math.abs.";
  }
  return return_var;
}

struct SL_Variable math_floor_fn(struct SL_Code *code,
                                 struct SL_L_Function func,
                                 struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at math.floor! Not enough arguments.";
    return return_var;
  }
  double val;
  if (!sl_get_double(sl_get_argument(*code, func, 0), &val)) {
    return_var.type = ERROR;
    return_var.vals = "Expected integer or double argument for math.floor.";
    return return_var;
  }
  return_var.type = DOUBLE;
  return_var.valf = floor(val);
  return return_var;
}

struct SL_Variable math_ceil_fn(struct SL_Code *code, struct SL_L_Function func,
                                struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at math.ceil! Not enough arguments.";
    return return_var;
  }
  double val;
  if (!sl_get_double(sl_get_argument(*code, func, 0), &val)) {
    return_var.type = ERROR;
    return_var.vals = "Expected integer or double argument for math.ceil.";
    return return_var;
  }
  return_var.type = DOUBLE;
  return_var.valf = ceil(val);
  return return_var;
}

struct SL_Variable math_round_fn(struct SL_Code *code,
                                 struct SL_L_Function func,
                                 struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at math.round! Not enough arguments.";
    return return_var;
  }
  double val;
  if (!sl_get_double(sl_get_argument(*code, func, 0), &val)) {
    return_var.type = ERROR;
    return_var.vals = "Expected integer or double argument for math.round.";
    return return_var;
  }
  return_var.type = DOUBLE;
  return_var.valf = round(val);
  return return_var;
}

struct SL_Variable math_sin_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at math.sin! Not enough arguments.";
    return return_var;
  }
  double val;
  if (!sl_get_double(sl_get_argument(*code, func, 0), &val)) {
    return_var.type = ERROR;
    return_var.vals = "Expected integer or double argument for math.sin.";
    return return_var;
  }
  return_var.type = DOUBLE;
  return_var.valf = sin(val);
  return return_var;
}

struct SL_Variable math_cos_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at math.cos! Not enough arguments.";
    return return_var;
  }
  double val;
  if (!sl_get_double(sl_get_argument(*code, func, 0), &val)) {
    return_var.type = ERROR;
    return_var.vals = "Expected integer or double argument for math.cos.";
    return return_var;
  }
  return_var.type = DOUBLE;
  return_var.valf = cos(val);
  return return_var;
}

struct SL_Variable math_tan_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at math.tan! Not enough arguments.";
    return return_var;
  }
  double val;
  if (!sl_get_double(sl_get_argument(*code, func, 0), &val)) {
    return_var.type = ERROR;
    return_var.vals = "Expected integer or double argument for math.tan.";
    return return_var;
  }
  return_var.type = DOUBLE;
  return_var.valf = tan(val);
  return return_var;
}

struct SL_Variable math_log_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at math.log! Not enough arguments.";
    return return_var;
  }
  double val;
  if (!sl_get_double(sl_get_argument(*code, func, 0), &val)) {
    return_var.type = ERROR;
    return_var.vals = "Expected integer or double argument for math.log.";
    return return_var;
  }
  if (val <= 0) {
    return_var.type = ERROR;
    return_var.vals = "Math domain error: log argument must be positive!";
    return return_var;
  }
  return_var.type = DOUBLE;
  return_var.valf = log(val);
  return return_var;
}

struct SL_Variable math_log10_fn(struct SL_Code *code,
                                 struct SL_L_Function func,
                                 struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 1) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at math.log10! Not enough arguments.";
    return return_var;
  }
  double val;
  if (!sl_get_double(sl_get_argument(*code, func, 0), &val)) {
    return_var.type = ERROR;
    return_var.vals = "Expected integer or double argument for math.log10.";
    return return_var;
  }
  if (val <= 0) {
    return_var.type = ERROR;
    return_var.vals = "Math domain error: log10 argument must be positive!";
    return return_var;
  }
  return_var.type = DOUBLE;
  return_var.valf = log10(val);
  return return_var;
}

struct SL_Variable math_min_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 2) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at math.min! Not enough arguments.";
    return return_var;
  }
  double a, b;
  if (!sl_get_double(sl_get_argument(*code, func, 0), &a) ||
      !sl_get_double(sl_get_argument(*code, func, 1), &b)) {
    return_var.type = ERROR;
    return_var.vals = "Expected integer or double arguments for math.min.";
    return return_var;
  }
  return_var.type = DOUBLE;
  return_var.valf = fmin(a, b);
  return return_var;
}

struct SL_Variable math_max_fn(struct SL_Code *code, struct SL_L_Function func,
                               struct SL_Function rfunc) {
  struct SL_Variable return_var = {0};
  if (func.total_arguments < 2) {
    return_var.type = ERROR;
    return_var.vals = "Error usage at math.max! Not enough arguments.";
    return return_var;
  }
  double a, b;
  if (!sl_get_double(sl_get_argument(*code, func, 0), &a) ||
      !sl_get_double(sl_get_argument(*code, func, 1), &b)) {
    return_var.type = ERROR;
    return_var.vals = "Expected integer or double arguments for math.max.";
    return return_var;
  }
  return_var.type = DOUBLE;
  return_var.valf = fmax(a, b);
  return return_var;
}
/* MATH */

int used_io = 0;
int used_file = 0;
int used_types = 0;
int used_sys = 0;
int used_string = 0;
int used_errors = 0;
int used_list = 0;
int used_extra = 0;
int used_time = 0;
int used_db = 0;
int used_math = 0;
int used_bytes = 0;
int used_collections = 0;
int used_enums = 0;
int used_net = 0;
int used_console = 0;

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
      sl_add_func(code, "io.print_raw", print_raw_fn);
      sl_add_func(code, "io.input", input_fn);
      sl_add_func(code, "io.getchar", io_getchar_fn);
      sl_add_func(code, "io.fflush", io_fflush_fn);
    } else if (strcmp(libstr, "file") == 0 && used_file == 0) {
      used_file = 1;
      sl_add_func(code, "file.read_to_str", file_read_to_str_fn);
      sl_add_func(code, "file.remove", file_remove_fn);
      sl_add_func(code, "file.write_from_str", file_write_from_str_fn);
      sl_add_func(code, "file.append_from_str", file_append_from_str_fn);
      sl_add_func(code, "file.read", file_read_fn);
      sl_add_func(code, "file.write", file_write_fn);
      sl_add_func(code, "file.append", file_append_fn);
    } else if (strcmp(libstr, "math") == 0 && used_math == 0) {
      used_math = 1;
      sl_add_func(code, "math.pow", math_pow_fn);
      sl_add_func(code, "math.sqrt", math_sqrt_fn);
      sl_add_func(code, "math.abs", math_abs_fn);
      sl_add_func(code, "math.floor", math_floor_fn);
      sl_add_func(code, "math.ceil", math_ceil_fn);
      sl_add_func(code, "math.round", math_round_fn);
      sl_add_func(code, "math.sin", math_sin_fn);
      sl_add_func(code, "math.cos", math_cos_fn);
      sl_add_func(code, "math.tan", math_tan_fn);
      sl_add_func(code, "math.log", math_log_fn);
      sl_add_func(code, "math.log10", math_log10_fn);
      sl_add_func(code, "math.min", math_min_fn);
      sl_add_func(code, "math.max", math_max_fn);

    } else if (strcmp(libstr, "bytes") == 0 && used_bytes == 0) {
      used_bytes = 1;
      sl_add_func(code, "byte.get", byte_get_fn);
      sl_add_func(code, "byte.set", byte_set_fn);
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
    
      /* SIZE CALCULATION */ 
      sl_add_func(code, "types.sizeof", types_sizeof_fn);

      /* BINARY PACK */ 
      sl_add_func(code, "types.pack", types_pack_fn);

      /* STRING TYPE CHECK */
      sl_add_func(code, "types.is_digit", is_digit_fn);
      sl_add_func(code, "types.is_space", is_space_fn);
      sl_add_func(code, "types.is_alpha", is_alpha_fn);
      sl_add_func(code, "types.is_alnum", is_alnum_fn);
      sl_add_func(code, "types.is_upper", is_upper_fn);
      sl_add_func(code, "types.is_lower", is_lower_fn);
      sl_add_func(code, "types.is_punct", is_punct_fn);
      sl_add_func(code, "types.is_xdigit", is_xdigit_fn);
      sl_add_func(code, "types.to_lower", to_lower_fn);
      sl_add_func(code, "types.to_upper", to_upper_fn);
    } else if (strcmp(libstr, "sys") == 0 && used_sys == 0) {
      used_sys = 1;
      sl_add_func(code, "sys.get_arg", sys_get_arg_fn);
      sl_add_func(code, "sys.exit", sys_exit_fn);
      sl_add_func(code, "sys.get_env", sys_get_env_fn);
      sl_add_func(code, "sys.popen", sys_popen_fn);
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
      sl_add_func(code, "enums.create_enum", enums_create_enum_fn);
    } else if (strcmp(libstr, "console") == 0 && used_console == 0) {
      used_console = 1;
      sl_add_fixed_int(code, "UNDEFINED_EVENT", SL_UNDEFINED_EVENT);
      sl_add_fixed_int(code, "KEY_EVENT", SL_KEY_EVENT);
      sl_add_fixed_int(code, "MOUSE_EVENT", SL_MOUSE_EVENT);
      sl_add_fixed_int(code, "WINDOW_RESIZE", SL_WINDOW_RESIZE_EVENT);
      sl_add_fixed_int(code, "KEY_UNKNOWN", SL_KEY_UNKNOWN);
      sl_add_fixed_int(code, "KEY_CHAR", SL_KEY_CHAR);

      sl_add_fixed_int(code, "KEY_ENTER", SL_KEY_ENTER);
      sl_add_fixed_int(code, "KEY_ESCAPE", SL_KEY_ESCAPE);
      sl_add_fixed_int(code, "KEY_BACKSPACE", SL_KEY_BACKSPACE);
      sl_add_fixed_int(code, "KEY_TAB", SL_KEY_TAB);

      sl_add_fixed_int(code, "MOUSE_NONE", SL_MOUSE_NONE);
      sl_add_fixed_int(code, "MOUSE_LEFT_PRESSED", SL_MOUSE_LEFT_PRESSED);
      sl_add_fixed_int(code, "MOUSE_RIGHT_PRESSED", SL_MOUSE_RIGHT_PRESSED);
      sl_add_fixed_int(code, "MOUSE_MIDDLE_PRESSED", SL_MOUSE_MIDDLE_PRESSED);
      sl_add_fixed_int(code, "MOUSE_MOVED", SL_MOUSE_MOVED);
      sl_add_fixed_int(code, "MOUSE_DOUBLE_CLICK", SL_MOUSE_DOUBLE_CLICK);
      sl_add_fixed_int(code, "MOUSE_WHEEL_UP", SL_MOUSE_WHEEL_UP);
      sl_add_fixed_int(code, "MOUSE_WHEEL_DOWN", SL_MOUSE_WHEEL_DOWN);

      sl_add_fixed_int(code, "KEY_UP", SL_KEY_UP);
      sl_add_fixed_int(code, "KEY_DOWN", SL_KEY_DOWN);
      sl_add_fixed_int(code, "KEY_LEFT", SL_KEY_LEFT);
      sl_add_fixed_int(code, "KEY_RIGHT", SL_KEY_RIGHT);

      sl_add_fixed_int(code, "KEY_HOME", SL_KEY_HOME);
      sl_add_fixed_int(code, "KEY_END", SL_KEY_END);
      sl_add_fixed_int(code, "KEY_INSERT", SL_KEY_INSERT);
      sl_add_fixed_int(code, "KEY_DELETE", SL_KEY_DELETE);

      sl_add_fixed_int(code, "KEY_PAGE_UP", SL_KEY_PAGE_UP);
      sl_add_fixed_int(code, "KEY_PAGE_DOWN", SL_KEY_PAGE_DOWN);

      sl_add_fixed_int(code, "KEY_F1", SL_KEY_F1);
      sl_add_fixed_int(code, "KEY_F2", SL_KEY_F2);
      sl_add_fixed_int(code, "KEY_F3", SL_KEY_F3);
      sl_add_fixed_int(code, "KEY_F4", SL_KEY_F4);
      sl_add_fixed_int(code, "KEY_F5", SL_KEY_F5);
      sl_add_fixed_int(code, "KEY_F6", SL_KEY_F6);
      sl_add_fixed_int(code, "KEY_F7", SL_KEY_F7);
      sl_add_fixed_int(code, "KEY_F8", SL_KEY_F8);
      sl_add_fixed_int(code, "KEY_F9", SL_KEY_F9);
      sl_add_fixed_int(code, "KEY_F10", SL_KEY_F10);
      sl_add_fixed_int(code, "KEY_F11", SL_KEY_F11);
      sl_add_fixed_int(code, "KEY_F12", SL_KEY_F12);

      sl_add_fixed_int(code, "MOD_NONE", SL_MOD_NONE);
      sl_add_fixed_int(code, "MOD_SHIFT", SL_MOD_SHIFT);
      sl_add_fixed_int(code, "MOD_CTRL", SL_MOD_CTRL);
      sl_add_fixed_int(code, "MOD_ALT", SL_MOD_ALT);
      sl_add_fixed_int(code, "MOD_SUPER", SL_MOD_SUPER);

      sl_add_fixed_int(code, "COLOR_DEFAULT", SL_COLOR_DEFAULT);

      sl_add_fixed_int(code, "COLOR_BLACK", SL_COLOR_BLACK);
      sl_add_fixed_int(code, "COLOR_RED", SL_COLOR_RED);
      sl_add_fixed_int(code, "COLOR_GREEN", SL_COLOR_GREEN);
      sl_add_fixed_int(code, "COLOR_YELLOW", SL_COLOR_YELLOW);
      sl_add_fixed_int(code, "COLOR_BLUE", SL_COLOR_BLUE);
      sl_add_fixed_int(code, "COLOR_MAGENTA", SL_COLOR_MAGENTA);
      sl_add_fixed_int(code, "COLOR_CYAN", SL_COLOR_CYAN);
      sl_add_fixed_int(code, "COLOR_WHITE", SL_COLOR_WHITE);

      sl_add_fixed_int(code, "COLOR_BRIGHT_BLACK", SL_COLOR_BRIGHT_BLACK);
      sl_add_fixed_int(code, "COLOR_BRIGHT_RED", SL_COLOR_BRIGHT_RED);
      sl_add_fixed_int(code, "COLOR_BRIGHT_GREEN", SL_COLOR_BRIGHT_GREEN);
      sl_add_fixed_int(code, "COLOR_BRIGHT_YELLOW", SL_COLOR_BRIGHT_YELLOW);
      sl_add_fixed_int(code, "COLOR_BRIGHT_BLUE", SL_COLOR_BRIGHT_BLUE);
      sl_add_fixed_int(code, "COLOR_BRIGHT_MAGENTA", SL_COLOR_BRIGHT_MAGENTA);
      sl_add_fixed_int(code, "COLOR_BRIGHT_CYAN", SL_COLOR_BRIGHT_CYAN);
      sl_add_fixed_int(code, "COLOR_BRIGHT_WHITE", SL_COLOR_BRIGHT_WHITE);
#ifdef _WIN32
      /* For windows, i just read windows-console.pdf from microsoft. */
      hStdIn = GetStdHandle(STD_INPUT_HANDLE);
      hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
      if (hStdIn == INVALID_HANDLE_VALUE || hStdOut == INVALID_HANDLE_VALUE) {
        MessageBox(NULL, TEXT("Invalid Console"),
                   TEXT("Cannot get console Standart Output/Input handle."),
                   MB_OK);
        exit(-1);
      }
      CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
      if (!GetConsoleScreenBufferInfo(hStdOut, &csbiInfo)) {
        MessageBox(NULL, TEXT("Invalid Console"),
                   TEXT("Cannot get console attributes!"), MB_OK);
        exit(-1);
      }
      if (!GetConsoleMode(hStdIn, &OldConsoleMode)) {
        MessageBox(NULL, TEXT("Invalid Console"),
                   TEXT("Cannot get console mode!"), MB_OK);
        exit(-1);
      }

      DWORD dwOutMode = 0;
      if (GetConsoleMode(hStdOut, &dwOutMode)) {
        dwOutMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hStdOut, dwOutMode);
      }

      /* STAAAY, STAAAY AWAAAY, STAY AWAAAAY */

      OldColorAttrs = csbiInfo.wAttributes;
      sl_add_func(code, "console.clear", console_clear_win_fn);
      sl_add_func(code, "console.foreground_color", console_fgcolor_win_fn);
      sl_add_func(code, "console.background_color", console_bgcolor_win_fn);
      sl_add_func(code, "console.cursor_position",
                  console_cursor_position_win_fn);
      sl_add_func(code, "console.raw_mode", console_raw_mode_win_fn);
      sl_add_func(code, "console.get_event", console_get_event_win_fn);
      sl_add_func(code, "console.key_event.is_pressed",
                  console_key_event_is_pressed_win_fn);
      sl_add_func(code, "console.key_event.get_key",
                  console_key_event_get_key_win_fn);
      sl_add_func(code, "console.key_event.get_mod",
                  console_key_event_get_mod_win_fn);
      sl_add_func(code, "console.mouse_event.get_type",
                  console_mouse_event_get_type_win_fn);
      sl_add_func(code, "console.mouse_event.get_x",
                  console_mouse_event_get_x_win_fn);
      sl_add_func(code, "console.mouse_event.get_y",
                  console_mouse_event_get_y_win_fn);
      sl_add_func(code, "console.get_width", console_get_width_win_fn);
      sl_add_func(code, "console.get_height", console_get_height_win_fn);
      sl_add_func(code, "console.enter_alt_screen", console_enter_alt_win_fn);
      sl_add_func(code, "console.leave_alt_screen", console_leave_alt_win_fn);
      sl_add_func(code, "console.cursor_visibility",
                  console_cursor_visibility_win_fn);
      sl_add_func(code, "console.reset_color", console_reset_color_win_fn);
#else
      /* FOR TERMIOS: https://viewsourcecode.org/snaptoken/kilo/ */
      tcgetattr(STDIN_FILENO, &orig_termios);
      atexit(disableRawMode);
      sl_add_func(code, "console.clear", console_clear_posix_fn);
      sl_add_func(code, "console.foreground_color", console_fgcolor_posix_fn);
      sl_add_func(code, "console.background_color", console_bgcolor_posix_fn);
      sl_add_func(code, "console.cursor_position",
                  console_cursor_position_posix_fn);
      sl_add_func(code, "console.raw_mode", console_raw_mode_posix_fn);
      sl_add_func(code, "console.get_event", console_get_event_posix_fn);
      sl_add_func(code, "console.key_event.is_pressed",
                  console_key_event_is_pressed_posix_fn);
      sl_add_func(code, "console.key_event.get_key",
                  console_key_event_get_key_posix_fn);
      sl_add_func(code, "console.key_event.get_mod",
                  console_key_event_get_mod_posix_fn);
      sl_add_func(code, "console.mouse_event.get_type",
                  console_mouse_event_get_type_posix_fn);
      sl_add_func(code, "console.mouse_event.get_x",
                  console_mouse_event_get_x_posix_fn);
      sl_add_func(code, "console.mouse_event.get_y",
                  console_mouse_event_get_y_posix_fn);
      sl_add_func(code, "console.get_width", console_get_width_posix_fn);
      sl_add_func(code, "console.get_height", console_get_height_posix_fn);
      sl_add_func(code, "console.enter_alt_screen", console_enter_alt_posix_fn);
      sl_add_func(code, "console.leave_alt_screen", console_leave_alt_posix_fn);
      sl_add_func(code, "console.cursor_visibility",
                  console_cursor_visibility_posix_fn);
      sl_add_func(code, "console.reset_color", console_reset_color_posix_fn);
#endif
      sl_add_func(code, "console.clear_line", console_clear_line_fn);
      sl_add_func(code, "console.begin_update", console_begin_update_fn);
      sl_add_func(code, "console.end_update", console_end_update_fn);
      sl_add_func(code, "console.autowrap", console_auto_wrap_fn);
      sl_add_func(code, "console.write_at", console_write_at_fn);
      sl_add_func(code, "console.fill_rect", console_fill_rect_fn);
    }
#ifdef ENABLE_NET
    else if (strcmp(libstr, "net") == 0 && used_net == 0) {
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
      sl_add_func(code, "net.select", net_select_win_fn);
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
      sl_add_func(code, "net.select", net_select_posix_fn);
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
    }
#endif
    else if (strcmp(libstr, "string") == 0 && used_string == 0) {
      used_string = 1;
      sl_add_func(code, "string.char_at", string_charat_fn);
      sl_add_func(code, "string.split", string_split_fn);
      sl_add_func(code, "string.slice", string_slice_fn);
      sl_add_func(code, "string.reverse", string_reverse_fn);
      sl_add_func(code, "string.trim", string_trim_fn);
      sl_add_func(code, "string.set_char_at", string_setcharat_fn);
      sl_add_func(code, "string.contains", string_contains_fn);
      sl_add_func(code, "string.len", string_len_fn);
      sl_add_func(code, "string.replace", string_replace_fn);
      sl_add_func(code, "string.starts_with", string_startswith_fn);
      sl_add_func(code, "string.ends_with", string_endswith_fn);
      sl_add_func(code, "string.remove_at", string_remove_at_fn);
      sl_add_func(code, "string.index_of", string_index_of_fn);
    } else if (strcmp(libstr, "list") == 0 && used_list == 0) {
      used_list = 1;
      LISTS = calloc(SL_INIT, sizeof(struct SL_List));
      sl_add_func(code, "List.new", List_new_fn);
      sl_add_func(code, "List.push", List_push_fn);
      sl_add_func(code, "List.pop", List_pop_fn);
      sl_add_func(code, "List.peek", List_peek_fn);
      sl_add_func(code, "List.set", List_set_fn);
      sl_add_func(code, "List.get", List_get_fn);
      sl_add_func(code, "List.free", List_free_fn);
      sl_add_func(code, "List.find", List_find_fn);
      sl_add_func(code, "List.next", List_next_fn);
      sl_add_func(code, "List.iter", List_iter_fn);
      sl_add_func(code, "List.reverse", List_reverse_fn);
      sl_add_func(code, "List.sort", List_sort_fn);
      sl_add_func(code, "List.copy", List_copy_fn);
      sl_add_func(code, "List.relative", List_relative_fn);
      sl_add_func(code, "List.remove", List_remove_fn);
      sl_add_func(code, "List.len", List_len_fn);
    } else if (strcmp(libstr, "extra") == 0 && used_extra == 0) {
      used_extra = 1;
      sl_add_func(code, "rand.random", random_fn);
    } else if (strcmp(libstr, "time") == 0 && used_time == 0) {
      used_time = 1;
      sl_add_func(code, "time.now", time_now_fn);
      sl_add_func(code, "time.string", time_string_fn);
      sl_add_func(code, "time.clock", time_clock_fn);
      sl_add_func(code, "time.sleep", time_sleep_fn);
      sl_add_func(code, "time.hour", time_hour_fn);
      sl_add_func(code, "time.minute", time_minute_fn);
      sl_add_func(code, "time.second", time_second_fn);
      sl_add_func(code, "time.diff", time_diff_fn);
      sl_add_func(code, "time.parse", time_parse_fn);
    } else if (strcmp(libstr, "db") == 0 && used_db == 0) {
      used_db = 1;
      sl_add_func(code, "db.from_lists", db_from_lists_fn);
      sl_add_func(code, "db.to_lists", db_to_lists_fn);
    } else {
      if (strncmp(libstr, "lib:", 4) == 0) {

      } else {
        fprintf(stderr, "Package undefined! PKG_NAME: %s\n", libstr);
      }
    }
    free(libstr);
  }

  return return_var;
}
void close_sl_stdlib();

void init_sl_stdlib(struct SL_Code *sl_code, int argc, char **argv) {
  srand(time(NULL));
  arguments = smalloc(argc * sizeof(char *));
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
      list_free(i);
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
