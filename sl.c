#include "sl.h"
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__)
#define SL_UNLIKELY(condition) __builtin_expect(!!(condition), 0)
#define SL_COLD_FUNCTION __attribute__((noinline, cold))
#else
#define SL_UNLIKELY(condition) (condition)
#define SL_COLD_FUNCTION
#endif

static SL_COLD_FUNCTION void stack_grow(struct SL_Code *code) {
  code->obj.stack_capacity =
      (code->obj.stack_capacity == 0) ? SL_INIT : code->obj.stack_capacity * 2;
  code->obj.stack =
      srealloc(NULL, code->obj.stack,
               code->obj.stack_capacity * sizeof(struct SL_Variable));
}

static SL_COLD_FUNCTION void stack_underflow(void) {
  fprintf(stderr, "[VM ERROR] Stack underflow!\n");
  exit(-1);
}

void push(struct SL_Code *code, struct SL_Variable val) {
  if (SL_UNLIKELY(code->obj.stack_size >= code->obj.stack_capacity))
    stack_grow(code);
  code->obj.stack[code->obj.stack_size++] = val;
}

struct SL_Variable pop(struct SL_Code *code) {
  if (SL_UNLIKELY(code->obj.stack_size == 0))
    stack_underflow();
  return code->obj.stack[--code->obj.stack_size];
}

void emit(struct SL_Code *code, uint32_t value) {
  if (code->bytecode_len >= code->bytecode_capacity) {
    code->bytecode_capacity += SL_REALLOC_DEFAULT;
    code->bytecode = srealloc(NULL, code->bytecode,
                              code->bytecode_capacity * sizeof(uint32_t));
  }
  code->bytecode[code->bytecode_len++] = value;
}

static void frame_reserve(struct SL_Code *code, size_t needed) {
  size_t old_cap = (size_t)code->frame.total_vars_capacity;
  if (needed <= old_cap && code->frame.vars != NULL)
    return;
  size_t new_cap = old_cap == 0 ? SL_INIT : old_cap;
  while (new_cap < needed)
    new_cap *= 2;
  code->frame.vars =
      srealloc(NULL, code->frame.vars, new_cap * sizeof(struct SL_Variable));
  memset(code->frame.vars + old_cap, 0,
         (new_cap - old_cap) * sizeof(struct SL_Variable));
  code->frame.total_vars_capacity = (int)new_cap;
}

static struct SL_FrameScope **g_scopes = NULL;
static int g_scope_n = 0;
static int g_scope_cap = 0;
static int g_scope_floor = 0;

static void scope_push(struct SL_FrameScope *scope) {
  if (g_scope_n >= g_scope_cap) {
    g_scope_cap = (g_scope_cap == 0) ? 16 : g_scope_cap * 2;
    g_scopes =
        srealloc(NULL, g_scopes, (size_t)g_scope_cap * sizeof(*g_scopes));
  }
  g_scopes[g_scope_n++] = scope;
}

static void scope_pop(void) {
  if (g_scope_n > 0)
    g_scope_n--;
}

static int find_outer_local(const char *name, unsigned long hash, int *depth) {
  for (int k = g_scope_n - 1; k >= g_scope_floor; k--) {
    struct SL_FrameScope *s = g_scopes[k];
    for (int i = s->saved_outer_count - 1; i >= 0; i--) {
      struct SL_Variable *v = &s->saved_outer_vars[i];
      if (v->name && v->hash == hash && strcmp(v->name, name) == 0) {
        *depth = g_scope_n - k;
        return i;
      }
    }
  }
  return -1;
}

char *sl_bytes_copy(const char *bytes, size_t length) {
  if (bytes == NULL && length != 0)
    return NULL;

  char *copy = smalloc(NULL, length);

  if (copy == NULL && length != 0)
    return NULL;

  if (length != 0)
    memcpy(copy, bytes, length);

  return copy;
}

struct SL_Variable sl_copy_variable(struct SL_Variable var) {
  struct SL_Variable copy = var;

  if (var.name != NULL) {
    copy.name = strdup(var.name);
  }

  if ((var.type == STRING || var.type == RETURN) && var.vals != NULL) {
    copy.vals = strdup(var.vals);
  } else if (var.type == BYTES && var.vals != NULL) {
    copy.vals = sl_bytes_copy(var.vals, var.length);
  }

  return copy;
}

static struct SL_Variable copy_value_without_name(struct SL_Variable var) {
  struct SL_Variable copy = var;
  copy.name = NULL;

  if ((var.type == STRING || var.type == RETURN) && var.vals != NULL) {
    copy.vals = strdup(var.vals);
  } else if (var.type == BYTES && var.vals != NULL) {
    copy.vals = sl_bytes_copy(var.vals, var.length);
  }

  return copy;
}

struct SL_Function sl_copy_function(struct SL_Function function) {
  struct SL_Function copy = function;

  if (function.name != NULL) {
    copy.name = strdup(function.name);
  }

  if (function.arguments != NULL && function.total_arguments > 0) {
    copy.arguments =
        smalloc(NULL, function.total_arguments * sizeof(*copy.arguments));
    for (int i = 0; i < function.total_arguments; i++) {
      copy.arguments[i] = sl_copy_variable(function.arguments[i]);
    }
  } else {
    copy.arguments = NULL;
  }

  return copy;
}

void sl_throw_an_error(struct SL_Code code, char **tokens, int current_token,
                       int max_tokens, char *error_msg, char *expected_tip) {
  fprintf(stderr, "\n[ERROR] %s\n", error_msg);
  if (tokens != NULL && current_token >= 0 && current_token < max_tokens) {
    int start = (current_token - 5 > 0) ? current_token - 5 : 0;
    int end = (current_token + 5 < max_tokens) ? current_token + 5 : max_tokens;

    fprintf(stderr, "Problematic area in your code:\n...");
    for (int i = start; i < end; i++) {
      if (i == current_token) {
        fprintf(stderr, " >>> %s <<< ", tokens[i]);
      } else {
        fprintf(stderr, "%s ", tokens[i]);
      }
    }
    fprintf(stderr, "...\n");
  }
  if (expected_tip != NULL) {
    fprintf(stderr, "\n[HINT] %s\n\n", expected_tip);
  }
  exit(-1);
}

int lexer_special_tokens_ex(const char *special_tokens, char letter) {
  for (const char *p = special_tokens; *p != '\0'; p++) {
    if (*p == letter) {
      return 1;
    }
  }

  return 0;
}

unsigned long sl_hash_string(const char *str) {
  unsigned long hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash;
}

static int sl_is_escaped(const char *start, const char *pos) {
  int slash_count = 0;

  while (pos > start && pos[-1] == '\\') {
    slash_count++;
    pos--;
  }

  return slash_count % 2;
}

char *sl_quote_string(const char *str) {
  if (str == NULL)
    return NULL;

  size_t length = strlen(str);
  size_t escapes = 0;
  for (const char *p = str; *(p += strcspn(p, "\"\\")) != '\0'; p++)
    escapes++;

  char *result = smalloc(NULL, length + escapes + 3);

  size_t j = 0;
  result[j++] = '"';

  const char *p = str;
  while (*p != '\0') {
    size_t run = strcspn(p, "\"\\");
    memcpy(result + j, p, run);
    j += run;
    p += run;
    if (*p != '\0') {
      result[j++] = '\\';
      result[j++] = *p++;
    }
  }

  result[j++] = '"';
  result[j] = '\0';

  return result;
}

static struct SL_Code *lexing_code = NULL;

int sl_find_custom_token(struct SL_Code *code, const char *token_name) {
  if (code == NULL || token_name == NULL)
    return -1;

  for (int i = 0; i < code->custom_token_count; i++) {
    if (strcmp(code->custom_token[i], token_name) == 0)
      return i;
  }
  return -1;
}

static int sl_custom_operator_index(struct SL_Code *code,
                                    const char *token_name) {
  int index = sl_find_custom_token(code, token_name);
  if (index == -1)
    return -1;

  if (code->custom_token_precedence[index] <= 0 ||
      code->custom_tokenr[index] == NULL)
    return -1;
  return index;
}

static int sl_match_custom_token(const char *position) {
  if (lexing_code == NULL)
    return 0;

  int longest_match = 0;
  for (int i = 0; i < lexing_code->custom_token_count; i++) {
    int length = (int)strlen(lexing_code->custom_token[i]);
    if (length > longest_match &&
        strncmp(position, lexing_code->custom_token[i], length) == 0)
      longest_match = length;
  }
  return longest_match;
}

int sl_raw_lexer(char *bufin, char ***bufout, size_t max_count,
                 char *special_tokens, int start_size) {
  int size_s = start_size;
  if (bufin == NULL)
    return 0;
  int token_count = 0;
  const char *p = bufin;
  while (*p != '\0') {
    if (token_count + 2 >= size_s) {
      size_s *= 2;
      (*bufout) = srealloc(NULL, *bufout, size_s * sizeof(char *));
    }
    if (*p == '#') {
      break;
    } else if (isspace(*p)) {
      p++;
      continue;
    } else if (p[0] == '"' && p[1] == '"' && p[2] == '"') {
      const char *string_start = p;
      p += 3;

      while (*p && *p != '\0') {
        if (p[0] == '"' && p[1] == '"' && p[2] == '"') {
          p += 3;
          break;
        }
        p++;
      }

      int stringlen = p - string_start;
      char *in_string_tokens = smalloc(NULL, stringlen + 1);
      strncpy(in_string_tokens, string_start, stringlen);
      in_string_tokens[stringlen] = '\0';
      (*bufout)[token_count++] = in_string_tokens;
    } else if (*p == '"' || *p == '\'') {
      char in_string = *p;
      const char *string_start = p++;

      while (*p != '\0') {
        if (*p == '\\' && p[1] != '\0') {
          p += 2;
          continue;
        }

        if (*p == in_string) {
          p++;
          break;
        }

        p++;
      }

      int stringlen = (int)(p - string_start);
      char *in_string_tokens = smalloc(NULL, stringlen + 1);
      memcpy(in_string_tokens, string_start, stringlen);
      in_string_tokens[stringlen] = '\0';

      (*bufout)[token_count++] = in_string_tokens;

    } else {
      int custom_length = sl_match_custom_token(p);
      if (custom_length > 0) {
        char *custom_word = smalloc(NULL, custom_length + 1);
        memcpy(custom_word, p, custom_length);
        custom_word[custom_length] = '\0';
        (*bufout)[token_count++] = custom_word;
        p += custom_length;
        continue;
      }
      if ((*p == '>' && *(p + 1) == '>') || (*p == '<' && *(p + 1) == '<')) {
        char *pot = smalloc(NULL, 3);
        pot[0] = *p;
        pot[1] = *(p + 1);
        pot[2] = '\0';
        (*bufout)[token_count++] = pot;
        p += 2;
        continue;
      }
      if ((p == bufin || (!isalnum((unsigned char)*(p - 1)) &&
                          *(p - 1) != '_' && *(p - 1) != ')')) &&
          (*p == '-' || *p == '+') && isdigit((unsigned char)*(p + 1))) {
        const char *word_start = p++;

        while (*p && !isspace((unsigned char)*p) &&
               lexer_special_tokens_ex(special_tokens, *p) == 0)
          p++;

        int word_len = p - word_start;

        char *word = smalloc(NULL, word_len + 1);
        memcpy(word, word_start, word_len);
        word[word_len] = '\0';

        (*bufout)[token_count++] = word;
        continue;
      }
      if (lexer_special_tokens_ex(special_tokens, *p) > 0) {
        char *pot = smalloc(NULL, 2);
        pot[0] = *p;
        pot[1] = '\0';
        (*bufout)[token_count++] = pot;
        p++;
        continue;
      }
      const char *word_start = p;
      while (*p && !isspace(*p) &&
             lexer_special_tokens_ex(special_tokens, *p) == 0 &&
             sl_match_custom_token(p) == 0)
        p++;
      int word_len = p - word_start;
      char *word = smalloc(NULL, word_len + 1);
      strncpy(word, word_start, word_len);
      word[word_len] = '\0';
      (*bufout)[token_count++] = word;
    }
    if (token_count >= max_count)
      break;
  }
  (*bufout)[token_count] = NULL;
  return token_count;
}

static int sl_lex_file(struct SL_Code *code, size_t malloc_size,
                       const char *restrict file_name, char ***bufout,
                       char *special_tokens) {
  FILE *code_file = fopen(file_name, "r");
  if (code_file == NULL) {
    fprintf(stderr, "Failed to open file under the name \"%s\"\n", file_name);
    sl_close_sl_process(code);
    exit(EXIT_FAILURE);
  }

  char buf[4096];
  char *code_string;

  size_t total_allocations = 0;

  total_allocations += malloc_size;
  code_string = smalloc(NULL, total_allocations);
  code_string[0] = '\0';

  while (fgets(buf, sizeof(buf), code_file)) {
    char *character_pos = NULL;
    char *p = strchr(buf, '\n');

    if (p)
      *p = '\0';

    int in_string = 0;
    int in_char = 0;

    for (char *q = buf; *q != '\0'; q++) {
      if (*q == '"' && !in_char && !sl_is_escaped(buf, q)) {
        in_string = !in_string;
        continue;
      }

      if (*q == '\'' && !in_string && !sl_is_escaped(buf, q)) {
        in_char = !in_char;
        continue;
      }
      if (*q == '#' && !in_string && !in_char) {
        character_pos = q;
        break;
      }
    }

    size_t index = strlen(buf);

    if (character_pos != NULL)
      index = character_pos - buf;

    size_t len = strlen(code_string);
    if (len + index + 2 > total_allocations) {
      total_allocations += malloc_size;
      code_string = srealloc(NULL, code_string, total_allocations);
    }

    memcpy(code_string + len, buf, index);
    code_string[len + index] = ' ';
    code_string[len + index + 1] = '\0';
  }
  fclose(code_file);

  char **code_array = smalloc(NULL, 1024 * sizeof(char *));
  int count = sl_raw_lexer(code_string, &code_array, strlen(code_string),
                           special_tokens, 1024);

  free(code_string);
  *bufout = code_array;

  return count;
}

struct SL_ImportRegistry {
  char **module_names;
  int total_modules;
  int modules_capacity;
};

static inline enum TokenTypes identifier_tokenizer_converter(const char *s);

static int sl_import_registry_has(struct SL_ImportRegistry *registry,
                                  const char *module_name) {
  for (int i = 0; i < registry->total_modules; i++) {
    if (strcmp(registry->module_names[i], module_name) == 0)
      return 1;
  }
  return 0;
}

static void sl_import_registry_add(struct SL_ImportRegistry *registry,
                                   const char *module_name) {
  if (registry->total_modules >= registry->modules_capacity) {
    registry->modules_capacity =
        (registry->modules_capacity == 0) ? 16 : registry->modules_capacity * 2;
    registry->module_names =
        srealloc(NULL, registry->module_names,
                 registry->modules_capacity * sizeof(char *));
  }
  registry->module_names[registry->total_modules++] = strdup(module_name);
}

static void sl_import_registry_free(struct SL_ImportRegistry *registry) {
  for (int i = 0; i < registry->total_modules; i++) {
    free(registry->module_names[i]);
  }
  free(registry->module_names);
  registry->module_names = NULL;
  registry->total_modules = 0;
  registry->modules_capacity = 0;
}

static void sl_token_list_append(char ***tokens, int *total_tokens,
                                 int *tokens_capacity, char *token) {
  if (*total_tokens + 1 >= *tokens_capacity) {
    *tokens_capacity = *tokens_capacity * 2;
    *tokens = srealloc(NULL, *tokens, *tokens_capacity * sizeof(char *));
  }
  (*tokens)[(*total_tokens)++] = token;
}

static int sl_expand_imports(char ***tokens, int token_count,
                             size_t malloc_size, char *special_tokens,
                             struct SL_ImportRegistry *registry) {
  char **source_tokens = *tokens;

  int has_import = 0;
  for (int i = 0; i < token_count; i++) {
    if (identifier_tokenizer_converter(source_tokens[i]) == T_IMPORT) {
      has_import = 1;
      break;
    }
  }
  if (!has_import)
    return token_count;

  int expanded_capacity = token_count + 1;
  int total_expanded = 0;
  char **expanded_tokens = smalloc(NULL, expanded_capacity * sizeof(char *));

  for (int current_token = 0; current_token < token_count; current_token++) {
    if (identifier_tokenizer_converter(source_tokens[current_token]) !=
        T_IMPORT) {
      sl_token_list_append(&expanded_tokens, &total_expanded,
                           &expanded_capacity, source_tokens[current_token]);
      continue;
    }

    if (current_token + 1 >= token_count) {
      struct SL_Code no_code_context = {0};
      sl_throw_an_error(
          no_code_context, source_tokens, current_token, token_count,
          "UNEXPECTED IMPORT SYNTAX!",
          "Expected: import <file_name> (without string literal!)");
    }

    char *module_name = strdup(source_tokens[current_token + 1]);
    int last_name_token = current_token + 1;
    while (last_name_token + 2 < token_count &&
           source_tokens[last_name_token + 1][0] == '/') {
      size_t old_length = strlen(module_name);
      size_t part_length = strlen(source_tokens[last_name_token + 2]);

      module_name = srealloc(NULL, module_name, old_length + part_length + 2);
      module_name[old_length] = '/';
      memcpy(module_name + old_length + 1, source_tokens[last_name_token + 2],
             part_length + 1);
      last_name_token += 2;
    }

    for (int i = current_token; i <= last_name_token; i++) {
      free(source_tokens[i]);
    }
    current_token = last_name_token;

    if (!sl_import_registry_has(registry, module_name)) {
      sl_import_registry_add(registry, module_name);

      char **module_tokens = NULL;
      int total_module_tokens = sl_lex_file(NULL, malloc_size, module_name,
                                            &module_tokens, special_tokens);
      total_module_tokens =
          sl_expand_imports(&module_tokens, total_module_tokens, malloc_size,
                            special_tokens, registry);

      for (int i = 0; i < total_module_tokens; i++) {
        sl_token_list_append(&expanded_tokens, &total_expanded,
                             &expanded_capacity, module_tokens[i]);
      }
      free(module_tokens);
    }
    free(module_name);
  }

  sl_import_registry_free(registry);
  expanded_tokens[total_expanded] = NULL;
  free(source_tokens);
  *tokens = expanded_tokens;
  return total_expanded;
}

int sl_init_sl_lexer(struct SL_Code *code, size_t malloc_size,
                     const char *restrict file_name, char ***bufout,
                     char *special_tokens) {
  struct SL_ImportRegistry registry = {0};
  sl_import_registry_add(&registry, file_name);

  lexing_code = code;
  int token_count =
      sl_lex_file(code, malloc_size, file_name, bufout, special_tokens);
  token_count = sl_expand_imports(bufout, token_count, malloc_size,
                                  special_tokens, &registry);
  lexing_code = NULL;

  sl_import_registry_free(&registry);
  return token_count;
}

int check_number(const char *s) {
  char *endptr;

  strtol(s, &endptr, 10);
  if (*endptr == '\0') {
    return 1;
  }
  strtod(s, &endptr);
  if (*endptr == '\0') {
    return 2;
  }
  return 0;
}

static int string_quote_length(const char *word, size_t size) {
  if (word == NULL)
    return 0;

  if (size >= 6 && word[0] == '"' && word[1] == '"' && word[2] == '"' &&
      word[size - 1] == '"' && word[size - 2] == '"' && word[size - 3] == '"') {
    return 3;
  }

  if (size >= 2 && word[0] == '"' && word[size - 1] == '"') {
    return 1;
  }

  return 0;
}

int string_checker(char *word) {
  if (word == NULL)
    return 0;
  return string_quote_length(word, strlen(word));
}

static char sl_unescape_char(char escaped) {
  switch (escaped) {
  case 'n':
    return '\n';
  case 'b':
    return '\b';
  case 'e':
    return '\e';
  case 'a':
    return '\a';
  case 'f':
    return '\f';
  case 'r':
    return '\r';
  case 't':
    return '\t';
  case 'v':
    return '\v';
  case '0':
    return '\0';
  default:
    return escaped;
  }
}

char *sl_string_getter(char *word) {
  if (word == NULL)
    return NULL;

  size_t size = strlen(word);
  int quote_len = string_quote_length(word, size);

  if (quote_len == 0) {
    return sl_bytes_copy(word, size + 1);
  }

  char *our_word = smalloc(NULL, size);
  size_t end = size - (size_t)quote_len;
  size_t i = (size_t)quote_len;
  size_t j = 0;

  while (i < end) {
    size_t run = strcspn(word + i, "\\");
    if (run > end - i)
      run = end - i;
    memcpy(our_word + j, word + i, run);
    i += run;
    j += run;
    if (i >= end)
      break;

    if (i + 1 < end) {
      our_word[j++] = sl_unescape_char(word[i + 1]);
      i += 2;
    } else {
      our_word[j++] = word[i++];
    }
  }

  our_word[j] = '\0';
  return our_word;
}

enum SL_Types type_analyzer(char *word) {
  int check_num = check_number(word);
  if (string_checker(word) == 3 || string_checker(word) == 1) {
    return STRING;
  } else if ((word[0] == '-' && word[1] == '0' &&
              (word[2] == 'x' || word[2] == 'o' || word[2] == 'b')) ||
             (word[0] == '0' &&
              (word[1] == 'x' || word[1] == 'b' || word[1] == 'o'))) {
    return LONG;
  } else if (check_num == 1) {
    return INTEGER;
  } else if (check_num == 2) {
    return DOUBLE;
  } else if (strcmp(word, "false") == 0 || strcmp(word, "true") == 0) {
    return BOOLEAN;
  } else if (word[0] == '\'') {
    return CHAR;
  } else if (check_num == 0 && string_checker(word) != 1) {
    return RETURN;
  }
  return -1;
}

struct SL_Variable sl_word_to_var_converter(char *word) {
  struct SL_Variable v = {0};
  v.type = type_analyzer(word);
  switch (v.type) {
  case INTEGER: {
    char *endptr = NULL;
    /* convert word to base 10 integer */
    v.vali = (int)strtol(word, &endptr, 10);
    if (*(endptr) != '\0') {
      v.vals = "Invalid characters in integer value";
      v.type = ERROR;
    }
  } break;
  case DOUBLE:
    v.valf = strtod(word, NULL);
    break;
  case STRING:
    v.vals = sstrdup(&v, word);
    if (v.type == ERROR) {
      fprintf(stderr, "error: in sl_word_to_var_converter(): %s\n", v.vals);
      exit(EXIT_FAILURE);
    }
    break;
  case BOOLEAN:
    if (word[0] == 'f')
      v.valb = 0;
    if (word[0] == 't')
      v.valb = 1;
    break;
  case CHAR:
    if (word[1] == '\\') {
      switch (word[2]) {
      case '0':
        v.valc = '\0';
        break;
      case 'n':
        v.valc = '\n';
        break;
      case 't':
        v.valc = '\t';
        break;
      case 'r':
        v.valc = '\r';
        break;
      case '\\':
        v.valc = '\\';
        break;
      case '\'':
        v.valc = '\'';
        break;
      default:
        v.valc = word[2];
        break;
      }
    } else {
      v.valc = word[1];
    }
    break;
  case RETURN:
    v.type = RETURN;
    v.vals = strdup(word);
    v.cache_index = -1;

    if (word[0] == '$') {
      v.hash = sl_hash_string(word + 1);
    }
    break;
  case LONG: {
    char *endptr = NULL;
    int neg = (word[0] == '-');
    const char *p = word + neg;
    int base = 10;
    if (p[0] == '0') {
      if (p[1] == 'x')
        base = 16;
      else if (p[1] == 'b')
        base = 2;
      else if (p[1] == 'o')
        base = 8;
    }
    const char *digits = (base == 10) ? p : p + 2;
    errno = 0;
    intmax_t val = strtoimax(digits, &endptr, base);
    if (*digits == '\0' || *endptr != '\0' || errno == ERANGE) {
      v.vals = "Invalid characters or out-of-range value in literal";
      v.type = ERROR;
      break;
    }
    v.valh = neg ? -val : val;
  } break;
  default:
    break;
  }
  return v;
}

int getvar_index_from_sl_p(const struct SL_Code *code, const char *name,
                           unsigned long hash) {
  for (int i = code->total_vars - 1; i >= 0; i--) {
    if (code->vars[i].name == NULL)
      continue;
    if (code->vars[i].hash == hash) {
      if (strcmp(code->vars[i].name, name) == 0) {
        return i;
      }
    }
  }
  return -1;
}
#define getvar_index_from_sl(code, name, hash)                                 \
  getvar_index_from_sl_p(&(code), (name), (hash))

int getvar_local_index_from_sl(struct SL_Code code, const char *name,
                               unsigned long hash) {
  for (int i = code.frame.total_vars - 1; i >= 0; i--) {
    if (code.frame.vars[i].name == NULL)
      continue;
    if (code.frame.vars[i].hash == hash) {
      if (strcmp(code.frame.vars[i].name, name) == 0) {
        return i;
      }
    }
  }
  return -1;
}

int is_has_func_hashed_p(const struct SL_Code *code, const char *name,
                         unsigned long target_hash) {
  for (int i = 0; i < code->total_funcs; i++) {
    if (code->funcs[i].hash == target_hash) {
      if (strcmp(code->funcs[i].name, name) == 0) {
        return i;
      }
    }
  }
  return -1;
}

int is_has_func_p(const struct SL_Code *code, const char *name) {
  return is_has_func_hashed_p(code, name, sl_hash_string(name));
}

#define is_has_func(code, name) is_has_func_p(&(code), (name))

void sl_free_variable(struct SL_Variable *var) {
  if (var == NULL) {
    return;
  }
  if (var->name != NULL) {
    free(var->name);
    var->name = NULL;
  }

  if ((var->type == STRING || var->type == RETURN || var->type == BYTES) &&
      var->vals != NULL) {
    free(var->vals);
    var->vals = NULL;
  }
}

void sl_free_function(struct SL_Function *func) {
  if (func == NULL) {
    return;
  }

  if (func->arguments != NULL) {
    for (int i = 0; i < func->total_arguments; i++) {
      sl_free_variable(&func->arguments[i]);
    }
    free(func->arguments);
    func->arguments = NULL;
  }

  if (func->name != NULL) {
    free(func->name);
    func->name = NULL;
  }
}

int sl_add_raw_func(struct SL_Code *code, struct SL_Function *function) {
  if (code->total_funcs >= code->total_size_f) {
    code->total_size_f =
        code->total_size_f == 0 ? SL_INIT : code->total_size_f * 2;

    code->funcs =
        srealloc(NULL, code->funcs, code->total_size_f * sizeof(*code->funcs));
  }

  int has_func = is_has_func(*code, function->name);
  if (has_func == -1)
    code->funcs[code->total_funcs++] = sl_copy_function(*function);
  else {
    sl_free_function(&code->funcs[has_func]);
    code->funcs[has_func] = sl_copy_function(*function);
  }

  return 0;
}

int sl_add_func(struct SL_Code *code, char *name,
                struct SL_Variable (*funcr)(struct SL_Code *,
                                            struct SL_L_Function,
                                            struct SL_Function)) {
  if (code->total_funcs >= code->total_size_f) {
    code->total_size_f =
        (code->total_size_f == 0) ? SL_INIT : code->total_size_f * 2;
    code->funcs = srealloc(NULL, code->funcs,
                           code->total_size_f * sizeof(struct SL_Function));
  }

  code->funcs[code->total_funcs].name = strdup(name);
  code->funcs[code->total_funcs].hash = sl_hash_string(name);
  code->funcs[code->total_funcs].linked_function = 1;
  code->funcs[code->total_funcs].scope_lifetime = sl_get_scope(code);
  code->funcs[code->total_funcs++].funcr = funcr;
  return 0;
}

struct SL_Variable sl_get_argument_p(const struct SL_Code *code,
                                     struct SL_L_Function func, int which_one) {
  struct SL_Variable error_val = {0};
  error_val.type = ERROR;
  if (which_one >= func.total_arguments)
    return error_val;
  return code->vars[func.starting_index + which_one];
}

int sl_add_var(struct SL_Code *code, struct SL_Variable var) {
  if (code->total_vars >= code->total_size_v) {
    code->total_size_v =
        (code->total_size_v == 0) ? SL_INIT : code->total_size_v * 2;
    code->vars = srealloc(NULL, code->vars,
                          code->total_size_v * sizeof(struct SL_Variable));
  }
  unsigned long hashe = sl_hash_string(var.name);
  int index = getvar_index_from_sl(*code, var.name, hashe);
  if (index == -1) {
    code->vars[code->total_vars++] = sl_copy_variable(var);
  } else {
    sl_free_variable(&code->vars[index]);
    code->vars[index] = sl_copy_variable(var);
  }
  return 0;
}

struct SL_Variable *sl_get_var(struct SL_Code *code, const char *name) {
  unsigned long hashe = sl_hash_string(name);
  int index = getvar_index_from_sl(*code, name, hashe);
  if (index == -1)
    return NULL;

  return &code->vars[index];
}

struct SL_Function *sl_get_func(struct SL_Code *code, const char *name) {
  for (int i = 0; i < code->total_funcs; i++) {
    if (strcmp(code->funcs[i].name, name) == 0)
      return &code->funcs[i];
  }
  return NULL;
}

void expression_solver(struct SL_Code *code_s, char op, enum TokenTypes op_type,
                       int is_op_type) {
  if (is_op_type != 1) {
    switch (op) {
    case '+':
      emit(code_s, OPCODE_ADD);
      break;
    case '-':
      emit(code_s, OPCODE_SUB);
      break;
    case '*':
      emit(code_s, OPCODE_MUL);
      break;
    case '/':
      emit(code_s, OPCODE_DIV);
      break;
    case '%':
      emit(code_s, OPCODE_MOD);
      break;
    case '&':
      emit(code_s, OPCODE_BAND);
      break;
    case '|':
      emit(code_s, OPCODE_BOR);
      break;
    case '^':
      emit(code_s, OPCODE_XOR);
      break;
    case '>':
      emit(code_s, OPCODE_BIGGER);
      break;
    case '<':
      emit(code_s, OPCODE_LESSER);
      break;
    default:
      break;
    }
  } else {
    switch (op_type) {
    case T_SHRIGHT:
      emit(code_s, OPCODE_SHR);
      break;
    case T_SHLEFT:
      emit(code_s, OPCODE_SHL);
      break;
    case T_AND:
      emit(code_s, OPCODE_LAND);
      break;
    case T_OR:
      emit(code_s, OPCODE_LOR);
      break;
    case T_EQU:
      emit(code_s, OPCODE_EQU);
      break;
    case T_NEQ:
      emit(code_s, OPCODE_NEQ);
      break;
    case T_EQG:
      emit(code_s, OPCODE_EQG);
      break;
    case T_EQL:
      emit(code_s, OPCODE_EQL);
      break;
    default:
      break;
    }
  }
}

struct SL_Math_Splitter {
  char op;
  int is_op_type;
  int op_pos;
  int precedence;
  int custom_index;
  enum TokenTypes op_type;
};

int operator_checker(struct SL_Code *code, char *expressions[],
                     enum TokenTypes *types, int start, int end);

int is_it_function_or_not(char **tokens, enum TokenTypes *types,
                          int current_token, int max_tokens) {
  if (tokens[current_token][0] == '(')
    return -1;

  if (tokens[current_token + 1] && tokens[current_token + 1][0] == '(' &&
      tokens[current_token + 2] && tokens[current_token + 2][0] == ')')
    return current_token + 3;

  if (current_token < 0 || current_token + 2 > max_tokens)
    return -1;

  int depth = 0;

  if (tokens[current_token + 1][0] == '(') {
    for (int i = current_token + 2; i < max_tokens; i++) {
      if (tokens[i][0] == '(')
        depth++;
      if (tokens[i][0] == ')' && depth == 0) {
        return i + 1;
      }
      if (tokens[i][0] == ')')
        depth--;
    }
  }
  return -1;
}

int operator_checker(struct SL_Code *code, char *expressions[],
                     enum TokenTypes *types, int start, int end) {
  if (end - start > 1) {
    return 1;
  }

  int isitfunc = is_it_function_or_not(expressions, types, start, end);
  if (isitfunc != -1) {
    start = isitfunc;
  }

  if (code->custom_splitter_count > 0) {
    int old_curr = start;
    for (int i = 0; i < code->custom_splitter_count; i++) {
      if (code->custom_splitter[i] && expressions[start] &&
          strcmp(code->custom_splitter[i], code->code[start]) == 0) {
        old_curr = code->custom_splitterr[i](code, &start);
      }
    }
    if (start != old_curr) {
      start = old_curr;
    }
  }

  for (int i = start; i < end; i++) {
    if (expressions[i] == NULL)
      return 0;

    if (sl_custom_operator_index(code, expressions[i]) != -1)
      return 1;

    switch (types[i]) {
    case T_EQU:
    case T_EQG:
    case T_EQL:
    case T_NEQ:
    case T_AND:
    case T_OR:
    case T_SHRIGHT:
    case T_SHLEFT:
      return 1;
    default:
      break;
    }

    if (expressions[i][1] == '\0') {
      switch (expressions[i][0]) {
      case '+':
      case '-':
      case '*':
      case '/':
      case '%':
      case '<':
      case '>':
      case '&':
      case '|':
      case '^':
        return 1;
      }
    }
  }

  return 0;
}

int prec_priority(char op, enum TokenTypes op_type, int is_op_type) {
  if (is_op_type == 1) {
    if (op_type == T_SHRIGHT || op_type == T_SHLEFT) {
      return 6;
    } else if (op_type == T_EQU || op_type == T_EQG || op_type == T_EQL ||
               op_type == T_NEQ) {
      return 2;
    } else if (op_type == T_AND || op_type == T_OR) {
      return 1;
    }
  } else {
    switch (op) {
    case '<':
    case '>':
      return 2;
    case '|':
      return 3;
    case '^':
      return 4;
    case '&':
      return 5;
    case '+':
    case '-':
      return 7;
    case '*':
    case '/':
    case '%':
      return 8;
    }
  }
  return 0;
}

struct SL_Math_Splitter
expression_parser_splitter(struct SL_Code code, char *expression[],
                           enum TokenTypes *types, int current_token,
                           int max_tokens, int old_curr) {
  int depth = 0;
  struct SL_Math_Splitter tree = {0};
  tree.op = 0;
  tree.custom_index = -1;

  while (current_token < max_tokens) {
    if (code.custom_splitter_count > 0) {
      int old_curr = current_token;
      for (int i = 0; i < code.custom_splitter_count; i++) {
        if (code.custom_splitter[i] && expression[current_token] &&
            strcmp(code.custom_splitter[i], expression[current_token]) == 0) {
          old_curr = code.custom_splitterr[i](&code, &current_token);
        }
      }
      if (current_token != old_curr) {
        current_token = old_curr;
        continue;
      }
    }

    if (is_has_func(code, expression[current_token]) != -1) {
      int is_it_func =
          is_it_function_or_not(expression, types, current_token, max_tokens);
      if (is_it_func != -1) {
        current_token = is_it_func;
        continue;
      }
    }

    if (expression[current_token][0] == '(') {
      depth++;
    } else if (expression[current_token][0] == ')') {
      depth--;
    }

    if (depth < 0) {
      sl_throw_an_error(code, expression, old_curr, max_tokens,
                        "THERES AN EXTRA ')'(closing parenthesis)",
                        "Expected: fewer ')' (closing parenthesis)");
      break;
    }

    if (depth > 0) {
      current_token++;
      continue;
    }
    enum TokenTypes op_type;
    int optype = 0;
    char op = expression[current_token][0];

    switch (types[current_token]) {
    case T_UNKNOWN:
    case T_IF:
    case T_DEF:
    case T_WHILE:
    case T_THEN:
    case T_END:
    case T_VAR:
    case T_ELSE:
    case T_ELIF:
    case T_IMPORT:
    case T_BREAK:
    case T_CONTINUE:
    case T_RETURN:
      break;
    case T_EQU:
    case T_NEQ:
    case T_EQG:
    case T_EQL:
    case T_AND:
    case T_OR:
    case T_SHLEFT:
    case T_SHRIGHT:
      op_type = types[current_token];
      optype = 1;
      break;
    }

    int custom_index =
        sl_custom_operator_index(&code, expression[current_token]);
    int op_prec =
        prec_priority(op, op_type, optype) * SL_BUILTIN_PRECEDENCE_SCALE;
    if (optype == 0 && strlen(expression[current_token]) > 1) {
      op_prec = 0;
    }
    if (custom_index != -1) {
      op_prec = code.custom_token_precedence[custom_index];
    }

    if (op_prec != 0 && (tree.op == 0 || op_prec <= tree.precedence)) {
      tree.op = op;
      tree.op_pos = current_token;
      tree.precedence = op_prec;
      tree.custom_index = custom_index;
      tree.is_op_type = optype;
      if (optype == 1) {
        tree.op_type = op_type;
      }
    }
    current_token++;
  }
  return tree;
}

char *get_raw_function_name(const char *word) {
  size_t len = strlen(word);
  char *returning_name = smalloc(NULL, len + 1);
  int j = 0;
  for (int i = 0; i < len; i++) {
    if (word[i] == '(') {
      returning_name[j] = '\0';
      return returning_name;
    } else {
      returning_name[j++] = word[i];
    }
  }
  returning_name[j] = '\0';
  return returning_name;
}

int sl_where_is_next_comma(char **tokens, int current_token, int max_tokens) {
  int depth = 0;
  for (int i = current_token; i < max_tokens; i++) {
    if (tokens[i][0] == '(') {
      depth++;
    } else if (tokens[i][0] == ')') {
      if (depth == 0) {
        return i;
      }
      depth--;
    } else if (tokens[i][0] == ',') {
      if (depth == 0) {
        return i;
      }
    }
  }
  return -1;
}

char *current_assignment = NULL;

char *sl_get_assignment_var() { return current_assignment; }

int sl_get_scope(struct SL_Code *code) { return code->scope_depth; }

void sl_clean_local_scope(struct SL_Code *code, int starting_var_index,
                          int starting_func_index) {
  int new_total_vars = starting_var_index;

  for (int i = starting_var_index; i < code->total_vars; i++) {
    if (code->vars[i].scope_lifetime != 0) {
      if (code->vars[i].scope_lifetime > 0) {
        code->vars[i].scope_lifetime--;
      }
      if (new_total_vars != i) {
        code->vars[new_total_vars] = code->vars[i];
      }
      new_total_vars++;
    } else {
      sl_free_variable(&code->vars[i]);
    }
  }
  code->total_vars = new_total_vars;

  int new_total_funcs = starting_func_index;

  for (int i = starting_func_index; i < code->total_funcs; i++) {
    if (code->funcs[i].scope_lifetime != 0) {
      if (code->funcs[i].scope_lifetime > 0) {
        code->funcs[i].scope_lifetime--;
      }
      if (new_total_funcs != i) {
        code->funcs[new_total_funcs] = code->funcs[i];
      }
      new_total_funcs++;
    } else {
      sl_free_function(&code->funcs[i]);
    }
  }
  code->total_funcs = new_total_funcs;
}

void compile_sl_function_call(struct SL_Code *code, const char *name,
                              char **tokens, enum TokenTypes *types,
                              int current_token, int max_tokens) {
  int function_number = -1;
  char *rawfunc = get_raw_function_name(name);
  unsigned long hash = sl_hash_string(name);
  for (int i = 0; i < code->total_funcs; i++) {
    if (code->funcs[i].hash == hash) {
      if (strcmp(code->funcs[i].name, rawfunc) == 0) {
        function_number = i;
        break;
      }
    }
  }
  free(rawfunc);

  if (function_number == -1) {
    sl_throw_an_error(*code, tokens, current_token, max_tokens,
                      "FUNCTION NOT FOUND!",
                      "Expected: Create a function first.");
  }
  struct SL_Function function = code->funcs[function_number];

  int how_much_go = 0;
  struct SL_L_Function lfunc = {0};
  if (function.total_arguments > 0 || function.vaargs == 1 ||
      function.linked_function == 1) {
    if (tokens[current_token + 2][0] != ')') {
      current_token += 2;
      int vaargs_counter = 0;
      lfunc.argument_indexes = scalloc(NULL, SL_INIT, sizeof(int));
      lfunc.starting_index = code->total_vars;

      while (current_token < code->token_count) {
        int commapos =
            sl_where_is_next_comma(tokens, current_token, code->token_count);
        if (function.linked_function == 1) {
          lfunc.argument_indexes[lfunc.total_arguments] = code->total_vars;
          sl_expression_solver(code, tokens, types, &current_token, commapos);
        } else {
          if (function.total_arguments <= how_much_go && function.vaargs == 1) {
            char *function_name = smalloc(NULL, SL_INIT);
            snprintf(function_name, SL_INIT, "%s_VA_ARGUMENT_%d", function.name,
                     vaargs_counter);
            vaargs_counter++;
            sl_expression_solver(code, tokens, types, &current_token, commapos);
            code->frame.vars[code->frame.total_vars].name =
                strdup(function_name);
            code->frame.vars[code->frame.total_vars].hash =
                sl_hash_string(function_name);
            free(function_name);
          } else {
            sl_expression_solver(code, tokens, types, &current_token, commapos);
            code->frame.vars[code->frame.total_vars].name = NULL;

            code->frame.vars[code->frame.total_vars].hash =
                function.arguments[how_much_go].hash;
          }
        }
        current_token = commapos + 1;
        how_much_go++;

        if (tokens[commapos][0] == ')') {
          break;
        }
      }
    }
  } else if (function.total_arguments == 0) {
    while (current_token < max_tokens) {
      if (current_token != max_tokens - 1 &&
          tokens[current_token + 1][0] == '(' &&
          tokens[current_token + 2][0] == ')') {
        current_token += 2;
        break;
      }
    }
  }
  if (function.linked_function == 1) {
    emit(code, OPCODE_CALL_LINKED_FUNC);
    emit(code, (uint32_t)function_number);
    emit(code, (uint32_t)how_much_go);
  } else {
    emit(code, OPCODE_FUNC_CALL);
    emit(code, (uint32_t)function_number);
    emit(code, (uint32_t)how_much_go);
  }

  free(lfunc.argument_indexes);
}

static void resolve_variable(struct SL_Code *code_s, char *expression[],
                             enum TokenTypes *types, int *current_token,
                             int max_tokens, int old_curr) {
  if (!expression[*current_token])
    return;

  for (int i = 0; i < code_s->custom_expr_count; i++) {
    if (strcmp(code_s->custom_expr[i], expression[*current_token]) == 0) {
      code_s->custom_exprr[i](code_s, current_token);
      return;
    }
  }

  struct SL_Variable var = code_s->fixed_values[*current_token];

  if (var.type != RETURN) {
    sl_emit_push_constant(code_s, sl_copy_variable(var));
    return;
  }

  if (var.vals[0] == '$') {
    int index = getvar_local_index_from_sl(*code_s, var.vals + 1, var.hash);
    if (index != -1) {
      emit(code_s, OPCODE_LOAD_LOCAL_VAR);
      emit(code_s, (uint32_t)index);
    } else {
      int depth;
      int oidx = find_outer_local(var.vals + 1, var.hash, &depth);
      if (oidx != -1) {
        emit(code_s, OPCODE_LOAD_OUTER_LOCAL);
        emit(code_s, (uint32_t)depth);
        emit(code_s, (uint32_t)oidx);
        return;
      }
      int index = getvar_index_from_sl(*code_s, var.vals + 1, var.hash);
      if (index != -1) {
        emit(code_s, OPCODE_LOAD_VAR);
        emit(code_s, (uint32_t)index);
      } else {
        int name_pool_index = sl_add_constant(code_s, sl_copy_variable(var));
        emit(code_s, OPCODE_LOAD_DYNAMIC_ADDED_VAR);
        emit(code_s, (uint32_t)name_pool_index);

        *current_token = max_tokens;
        return;
      }
    }
    return;
  } else {
    int func_idx = is_has_func(*code_s, expression[*current_token]);
    if (func_idx != -1) {
      compile_sl_function_call(code_s, expression[*current_token], expression,
                               types, *current_token, max_tokens);
      *current_token = max_tokens;
      return;
    } else {
      int token_idx = *current_token;
      int arg_count = 0;
      if (expression[token_idx + 1] && expression[token_idx + 1][0] == '(') {
        token_idx += 2;
        if (expression[token_idx][0] != ')') {
          while (token_idx < max_tokens) {
            int commapos =
                sl_where_is_next_comma(expression, token_idx, max_tokens);
            if (commapos == -1)
              commapos = max_tokens - 1;

            sl_expression_solver(code_s, expression, types, &token_idx,
                                 commapos);
            arg_count++;

            token_idx = commapos + 1;
            if (expression[commapos] && expression[commapos][0] == ')') {
              break;
            }
          }
        }
      }

      int name_pool_index = sl_add_constant(code_s, sl_copy_variable(var));
      emit(code_s, OPCODE_CALL_DYNAMIC_LINKED_FUNC);
      emit(code_s, (uint32_t)name_pool_index);
      emit(code_s, (uint32_t)arg_count);

      *current_token = max_tokens;
      return;
    }
  }
}

static int try_emit_increment(struct SL_Code *code_s, char *expression[],
                              int current_token, int max_tokens) {
  if (max_tokens - current_token != 3) {
    return 0;
  }

  char *operator_token = expression[current_token + 1];
  char *amount_token = expression[current_token + 2];
  if (operator_token == NULL || amount_token == NULL) {
    return 0;
  }

  int is_plus = operator_token[0] == '+' && operator_token[1] == '\0';
  int is_minus = operator_token[0] == '-' && operator_token[1] == '\0';
  if (!is_plus && !is_minus) {
    return 0;
  }

  if (strcmp(amount_token, "1") != 0) {
    return 0;
  }

  struct SL_Variable variable = code_s->fixed_values[current_token];
  if (variable.type != RETURN || variable.vals[0] != '$') {
    return 0;
  }

  char *variable_name = variable.vals + 1;
  if (getvar_local_index_from_sl(*code_s, variable_name, variable.hash) != -1) {
    return 0;
  }

  int variable_index =
      getvar_index_from_sl(*code_s, variable_name, variable.hash);
  if (variable_index == -1) {
    return 0;
  }

  emit(code_s, is_plus ? OPCODE_INC_VAR : OPCODE_DEC_VAR);
  emit(code_s, (uint32_t)variable_index);
  return 1;
}

void sl_expression_solver(struct SL_Code *code_s, char *expression[],
                          enum TokenTypes *types, int *current_token,
                          int max_tokens) {
  if (!expression[*current_token] || *current_token > max_tokens)
    return;

  while (*current_token < max_tokens && expression[*current_token][0] == '(') {
    if (max_tokens - 1 <= *current_token ||
        expression[max_tokens - 1] == NULL ||
        expression[max_tokens - 1][0] != ')') {
      break;
    }
    int depth = 0;
    int valid = 1;
    for (int i = *current_token; i < max_tokens - 1; i++) {
      if (expression[i][0] == '(')
        depth++;
      else if (expression[i][0] == ')')
        depth--;

      if (depth == 0) {
        valid = 0;
        break;
      }
    }
    if (!valid)
      break;
    (*current_token)++;
    max_tokens--;
  }

  int old_curr = *current_token;

  if (max_tokens - *current_token == 1) {
    resolve_variable(code_s, expression, types, current_token, max_tokens,
                     old_curr);
    *current_token = max_tokens;
    return;
  }

  if (max_tokens - *current_token == 3) {
    if (try_emit_increment(code_s, expression, *current_token, max_tokens)) {
      *current_token = max_tokens;
      return;
    }
  }

  struct SL_Math_Splitter tree = expression_parser_splitter(
      *code_s, expression, types, *current_token, max_tokens, old_curr);

  if (tree.op == 0) {
    resolve_variable(code_s, expression, types, current_token, max_tokens,
                     old_curr);
    *current_token = max_tokens;
    return;
  }

  if (tree.custom_index != -1) {
    code_s->custom_tokenr[tree.custom_index](code_s, *current_token,
                                             tree.op_pos, max_tokens);
    *current_token = max_tokens;
    return;
  }

  int left_pos_start = *current_token;
  int left_pos_end = tree.op_pos;
  sl_expression_solver(code_s, expression, types, &left_pos_start,
                       left_pos_end);

  int right_pos_start = tree.op_pos + 1;
  int right_pos_end = max_tokens;
  sl_expression_solver(code_s, expression, types, &right_pos_start,
                       right_pos_end);

  expression_solver(code_s, tree.op, tree.op_type, tree.is_op_type);

  *current_token = max_tokens;
  return;
}

static int sl_looks_like_call(char **code, int i, int max) {
  if (i < 0 || i + 1 >= max || !code[i] || !code[i + 1])
    return 0;
  unsigned char c = (unsigned char)code[i][0];
  if (!(isalpha(c) || c == '_' || c == '$'))
    return 0;
  return code[i + 1][0] == '(';
}

static int sl_skip_call(char **code, int i, int max) {
  if (i + 1 >= max || !code[i + 1] || code[i + 1][0] != '(')
    return i;
  int depth = 0;
  for (int j = i + 1; j < max; j++) {
    if (code[j][0] == '(')
      depth++;
    else if (code[j][0] == ')') {
      depth--;
      if (depth == 0)
        return j;
    }
  }
  return i;
}

int sl_find_end_of_expr(struct SL_Code *code_s, char **code,
                        enum TokenTypes *types, int starting, int max) {
  for (int i = starting; i < max; i++) {
    if (code_s->custom_splitter_count > 0) {
      int old_curr = i;

      for (int k = 0; k < code_s->custom_splitter_count; k++) {
        if (strcmp(code_s->custom_splitter[k], code[i]) == 0) {
          old_curr = code_s->custom_splitterr[k](code_s, &i);
          if (old_curr + 1 >= max)
            return max;

          i = old_curr;
          break;
        }
      }
    }
    if (is_has_func(*code_s, code[i]) != -1 ||
        sl_looks_like_call(code, i, max)) {
      i = sl_skip_call(code, i, max);
    }

    if (i + 1 >= max)
      return max;

    if (operator_checker(code_s, code, types, i + 1, i + 2) == 1) {
      i++;
      continue;
    } else {
      if (code[i][0] == '(') {
        int depth = 0;

        for (int j = i + 1; j < max; j++) {
          if (code[j][0] == '(')
            depth++;
          else if (code[j][0] == ')') {
            if (depth == 0) {
              i = j;
              break;
            }
            depth--;
          }
        }

        if (i + 1 >= max)
          return max;

        if (operator_checker(code_s, code, types, i + 1, i + 2) != 1)
          return i + 1;

        i++;
        continue;
      }

      if (operator_checker(code_s, code, types, i, i + 1) == 1) {
        if (is_has_func(*code_s, code[i + 1]) != -1 ||
            sl_looks_like_call(code, i + 1, max)) {
          i = sl_skip_call(code, i + 1, max);

          if (i + 1 >= max)
            return max;

          if (operator_checker(code_s, code, types, i + 1, i + 2) != 1)
            return i + 1;

          continue;
        }
        continue;
      }

      return i + 1;
    }
  }
  return max;
}

void variable_parser(struct SL_Code *code_s, enum TokenTypes *types,
                     char *tokens[], int *current_token, int max_tokens,
                     int current_line) {

  while (current_token != NULL && *current_token < max_tokens) {
    if (strcmp(tokens[*current_token], "=") == 0) {
      char *var_name = tokens[(*current_token) - 1];
      if (var_name[0] == '$')
        var_name++;

      unsigned long hash = sl_hash_string(var_name);

      if (code_s->scope_depth > 1) {
        int lidx = getvar_local_index_from_sl(*code_s, var_name, hash);
        int equal_start = *current_token + 1;
        sl_emit_push_string(code_s, var_name);
        emit(code_s, OPCODE_SET_ASSIGNMENT);
        sl_expression_solver(code_s, tokens, types, &equal_start, max_tokens);

        if (lidx == -1) {
          frame_reserve(code_s, (size_t)code_s->frame.total_vars + 2);
          lidx = code_s->frame.total_vars++;
          memset(&code_s->frame.vars[lidx], 0, sizeof(struct SL_Variable));
          code_s->frame.vars[lidx].name = strdup(var_name);
          code_s->frame.vars[lidx].hash = hash;
        }

        emit(code_s, OPCODE_STORE_LOCAL_VAR);
        emit(code_s, (uint32_t)lidx);
        *current_token = max_tokens;
        (*current_token)++;
        continue;
      }

      int index = getvar_index_from_sl(*code_s, var_name, hash);

      if (index == -1) {
        index = code_s->total_vars;

        if (code_s->total_vars >= code_s->total_size_v) {
          code_s->total_size_v =
              (code_s->total_size_v == 0) ? 1024 : code_s->total_size_v * 2;
          code_s->vars =
              srealloc(NULL, code_s->vars,
                       code_s->total_size_v * sizeof(struct SL_Variable));
        }

        memset(&code_s->vars[index], 0, sizeof(struct SL_Variable));
        code_s->vars[index].name = strdup(var_name);
        code_s->vars[index].hash = hash;
        code_s->total_vars++;
      }

      int equal_start = *current_token + 1;
      sl_emit_push_string(code_s, var_name);
      emit(code_s, OPCODE_SET_ASSIGNMENT);
      sl_expression_solver(code_s, tokens, types, &equal_start, max_tokens);

      emit(code_s, OPCODE_STORE_VAR);
      emit(code_s, (uint32_t)index);

      *current_token = max_tokens;
    }
    (*current_token)++;
  }
}

int get_last_pos_of_token_for_expressions(struct SL_Code *code_s, char *token,
                                          char **tokens, int current_token,
                                          int max_tokens) {
  int last_pos = -1;
  while (current_token < max_tokens) {
    int skipped_to = current_token;
    for (int i = 0; i < code_s->custom_splitter_count; i++) {
      if (strcmp(code_s->custom_splitter[i], tokens[current_token]) == 0) {
        skipped_to = code_s->custom_splitterr[i](code_s, &current_token);
        break;
      }
    }
    if (skipped_to != current_token) {
      current_token = skipped_to + 1;
      continue;
    }

    if (tokens[current_token][0] == token[0]) {
      last_pos = current_token;
    }
    current_token++;
  }
  return last_pos;
}

void assignment_parser(struct SL_Code *code_s, char *tokens[],
                       enum TokenTypes *types, int *current_token,
                       int max_tokens, int current_line) {

  while (current_token != NULL && *current_token < max_tokens) {
    int last_pos = get_last_pos_of_token_for_expressions(
        code_s, "=", tokens, *current_token, max_tokens);
    if (last_pos == -1) {
      break;
    }

    int variable_pos = last_pos - 1;
    char *var_name = tokens[variable_pos];
    if (var_name[0] == '$')
      var_name++;

    unsigned long hashe = sl_hash_string(var_name);

    int lidx = getvar_local_index_from_sl(*code_s, var_name, hashe);
    if (lidx != -1) {
      int lexpr_start = last_pos + 1;
      sl_expression_solver(code_s, tokens, types, &lexpr_start, max_tokens);
      emit(code_s, OPCODE_STORE_LOCAL_VAR);
      emit(code_s, (uint32_t)lidx);
      *current_token = max_tokens;
      continue;
    }

    int depth;
    int oidx = find_outer_local(var_name, hashe, &depth);
    if (oidx != -1) {
      int e = last_pos + 1;
      sl_expression_solver(code_s, tokens, types, &e, max_tokens);
      emit(code_s, OPCODE_STORE_OUTER_LOCAL);
      emit(code_s, (uint32_t)depth);
      emit(code_s, (uint32_t)oidx);
      *current_token = max_tokens;
      continue;
    }

    int index = getvar_index_from_sl(*code_s, var_name, hashe);

    if (index == -1) {
      int dynamic_expr_start = last_pos + 1;
      sl_expression_solver(code_s, tokens, types, &dynamic_expr_start,
                           max_tokens);
      sl_emit_push_string(code_s, var_name);
      emit(code_s, OPCODE_STORE_DYNAMIC_ADDED_VAR);
      *current_token = max_tokens;
      continue;
    }

    int expr_start = last_pos + 1;
    sl_expression_solver(code_s, tokens, types, &expr_start, max_tokens);

    emit(code_s, OPCODE_STORE_VAR);
    emit(code_s, (uint32_t)index);

    *current_token = max_tokens;
  }
}

int sl_then_finder(char *tokens[], enum TokenTypes *types, int current_token,
                   int max_tokens, int *then_pos) {
  int start_pos = 0;
  int depth = 0;
  while (current_token < max_tokens) {
    if (tokens[current_token][0] == '(')
      depth++;
    if (tokens[current_token][0] == ')')
      depth--;
    if (depth > 0) {
      current_token++;
      continue;
    }

    if (types[current_token] == T_THEN && depth == 0) {
      start_pos = current_token;
      break;
    }
    current_token++;
  }
  *then_pos = start_pos;

  if (start_pos == 0) {
    return -1;
  }
  return 0;
}

void sl_if_parser(struct SL_Code *code_s, enum TokenTypes *types,
                  char *tokens[], int *current_token, int max_tokens,
                  int find_then) {
  int then_pos = find_then;

  if (then_pos == -1) {
    if (sl_then_finder(tokens, types, *current_token, max_tokens, &then_pos) ==
        -1) {
      sl_throw_an_error(*code_s, tokens, (*current_token) - 1, max_tokens,
                        "'then' NOT FOUND ON 'if' usage.",
                        "Expected: if <expression> then <code> end");
    }
  }
  sl_expression_solver(code_s, tokens, types, current_token, then_pos);
  emit(code_s, OPCODE_IF_JUMP_FALSE);
  size_t patch_location = code_s->bytecode_len;
  emit(code_s, 0);

  *current_token = then_pos + 1;

  int end_pos = sl_find_end(tokens, types, *current_token, max_tokens, 1);
  if (end_pos == -1) {
    sl_throw_an_error(*code_s, tokens, *current_token, max_tokens,
                      "END NOT FOUND ON IF BLOCK",
                      "Expected: if <expression> then <code> end");
  }

  int old_starting_token = code_s->starting_token;
  int old_token_count = code_s->token_count;

  code_s->starting_token = *current_token;
  code_s->token_count = end_pos;

  sl_init_sl_parser(code_s);

  code_s->starting_token = old_starting_token;
  code_s->token_count = old_token_count;

  size_t exit_jmp_patch = 0;
  if (types[end_pos] == T_ELIF || types[end_pos] == T_ELSE) {
    emit(code_s, OPCODE_JMP);
    exit_jmp_patch = code_s->bytecode_len;
    emit(code_s, 0);
  }

  code_s->bytecode[patch_location] = (uint32_t)code_s->bytecode_len;

  *current_token = end_pos;

  if (types[end_pos] == T_ELIF) {
    (*current_token)++;
    sl_if_parser(code_s, types, tokens, current_token, max_tokens, -1);
  } else if (types[end_pos] == T_ELSE) {
    (*current_token)++;
    int final_end = sl_find_end(tokens, types, *current_token, max_tokens, 0);

    code_s->starting_token = *current_token;
    code_s->token_count = final_end;

    sl_init_sl_parser(code_s);

    code_s->starting_token = old_starting_token;
    code_s->token_count = old_token_count;
    *current_token = final_end;
  }

  if (exit_jmp_patch != 0) {
    code_s->bytecode[exit_jmp_patch] = (uint32_t)code_s->bytecode_len;
  }
}

typedef struct {
  uint32_t start_pc;
  size_t *break_patches;
  int break_count;
  int break_capacity;
} SL_LoopContext;

typedef struct {
  int depth;
  int capacity;
  SL_LoopContext *contexts;
} SL_LoopStack;

static SL_LoopStack global_loop_stack = {0};

void sl_while_parser(struct SL_Code *code_s, enum TokenTypes *types,
                     char *tokens[], int *current_token, int max_tokens,
                     int find_then) {
  int then_pos = find_then;

  if (then_pos == -1) {
    if (sl_then_finder(tokens, types, *current_token, max_tokens, &then_pos) ==
        -1) {
      sl_throw_an_error(*code_s, tokens, *current_token, max_tokens,
                        "THEN NOT FOUND ON WHILE",
                        "Expected: while <expr> then <code> end");
    }
  }

  sl_loop_begin(code_s);

  sl_expression_solver(code_s, tokens, types, current_token, then_pos);

  emit(code_s, OPCODE_IF_JUMP_FALSE);
  size_t exit_jump_patch = code_s->bytecode_len;
  emit(code_s, 0);

  *current_token = then_pos + 1;

  int end_pos = sl_find_end(tokens, types, *current_token, max_tokens, 0);
  if (end_pos == -1) {
    sl_throw_an_error(*code_s, tokens, *current_token, max_tokens,
                      "END NOT FOUND END OF THE WHILE",
                      "Expected: while <expr> then <code> end");
  }

  int old_start = code_s->starting_token;
  int old_count = code_s->token_count;

  code_s->starting_token = *current_token;
  code_s->token_count = end_pos;

  sl_init_sl_parser(code_s);

  code_s->starting_token = old_start;
  code_s->token_count = old_count;

  emit(code_s, OPCODE_JMP);
  emit(code_s, sl_loop_start_pc());

  sl_patch_jump_here(code_s, exit_jump_patch);
  sl_loop_end(code_s);

  *current_token = end_pos;
}

struct SL_Function sl_define_parser(struct SL_Code *code_s, char *tokens[],
                                    enum TokenTypes *types, int *current_token,
                                    int max_tokens) {
  int then_pos;
  sl_then_finder(tokens, types, *current_token, max_tokens, &then_pos);

  int current = *current_token;

  char *f_name = tokens[current++];
  size_t f_name_len = strlen(f_name);
  struct SL_Function function = {0};
  function.name = smalloc(NULL, f_name_len + 1);
  function.name[f_name_len] = '\0';
  strncpy(function.name, f_name, f_name_len);
  function.hash = sl_hash_string(function.name);
  function.total_arguments = 0;
  function.linked_function = 0;
  int outer_frame_count = code_s->frame.total_vars;
  struct SL_Variable *outer_frame = NULL;
  if (outer_frame_count > 0) {
    outer_frame = smalloc(NULL, outer_frame_count * sizeof(struct SL_Variable));
    /* smalloc() could use outer_frame SL_Variable as error check */
    memcpy(outer_frame, code_s->frame.vars,
           outer_frame_count * sizeof(struct SL_Variable));
  }
  code_s->frame.total_vars = 0;
  if (tokens[current][0] == '-' && tokens[++current][0] == '>') {
    current++;
    int arg_capacity = 8;
    function.arguments =
        scalloc(NULL, arg_capacity, sizeof(struct SL_Variable));
    if (tokens[current][0] == ',')
      return function;
    while (types[current] != T_THEN) {
      if (strcmp(tokens[current], "...") == 0) {
        if (!types[current + 1] || types[current + 1] == T_THEN)
          sl_throw_an_error(*code_s, tokens, *current_token, max_tokens,
                            "EXPECTED 'then' AFTER '...'",
                            "Expected: def <function_name> -> <argument_name>, "
                            "... then <code> end");
        function.vaargs = 1;
        current++;
        break;
      }
      if (tokens[current][0] == ',') {
        current++;
        continue;
      }
      int j = function.total_arguments;
      if (j >= arg_capacity) {
        arg_capacity *= 2;
        function.arguments =
            srealloc(NULL, function.arguments,
                     arg_capacity * sizeof(struct SL_Variable));
      }
      size_t len = strlen(tokens[current]);
      function.arguments[j].name = smalloc(NULL, len + 1);
      /* smalloc() could use outer_frame SL_Variable as error check */
      strncpy(function.arguments[j].name, tokens[current], len);
      function.arguments[j].name[len] = '\0';
      function.arguments[j].hash = sl_hash_string(function.arguments[j].name);

      frame_reserve(code_s, (size_t)code_s->frame.total_vars + 2);
      code_s->frame.vars[code_s->frame.total_vars] =
          sl_copy_variable(function.arguments[j]);
      code_s->frame.vars[code_s->frame.total_vars].scope_lifetime = 0;
      code_s->frame.total_vars++;

      function.total_arguments++;
      current++;
    }
  }

  emit(code_s, OPCODE_JMP);
  size_t skip_func_patch = code_s->bytecode_len;
  emit(code_s, 0);

  function.entry_pc = (int)code_s->bytecode_len;

  if (is_has_func(*code_s, function.name) != -1) {
    sl_throw_an_error(*code_s, tokens, *current_token, max_tokens,
                      "FUNCTION ALREADY EXISTS",
                      "Expected: try different name for your function");
  }
  if (code_s->total_funcs >= code_s->total_size_f - 1) {
    code_s->total_size_f *= 2;
    code_s->funcs = srealloc(NULL, code_s->funcs,
                             code_s->total_size_f * sizeof(struct SL_Function));
  }
  int func_index = code_s->total_funcs++;
  code_s->funcs[func_index] = function;

  if (current == then_pos) {
    int starting = ++current;
    int end_pos = sl_find_end(tokens, types, starting, max_tokens, 0);
    if (end_pos == -1) {
        sl_throw_an_error(*code_s, tokens, *current_token, max_tokens,
                      "END NOT FOUND ON DEF BLOCK",
                      "Expected: def <name> then <code> end");
    }

    int old_start = code_s->starting_token;
    int old_count = code_s->token_count;

    code_s->starting_token = starting;
    code_s->token_count = end_pos;
    code_s->scope_depth++;

    int saved_floor = g_scope_floor;
    g_scope_floor = g_scope_n;
    sl_init_sl_parser(code_s);
    g_scope_floor = saved_floor;
    code_s->scope_depth--;

    code_s->starting_token = old_start;
    code_s->token_count = old_count;
    current = end_pos;
  }

  function.frame_size = code_s->frame.total_vars;
  code_s->funcs[func_index].frame_size = function.frame_size;
  for (int i = 0; i < code_s->frame.total_vars; i++) {
    sl_free_variable(&code_s->frame.vars[i]);
    memset(&code_s->frame.vars[i], 0, sizeof(struct SL_Variable));
  }
  code_s->frame.total_vars = 0;

  if (outer_frame != NULL) {
    frame_reserve(code_s, (size_t)outer_frame_count + 2);
    memcpy(code_s->frame.vars, outer_frame,
           outer_frame_count * sizeof(struct SL_Variable));
    code_s->frame.total_vars = outer_frame_count;
    free(outer_frame);
  }

  emit(code_s, OPCODE_RETURN);

  code_s->bytecode[skip_func_patch] = (uint32_t)code_s->bytecode_len;

  *current_token = current;
  return function;
}

size_t sl_current_pc(struct SL_Code *code) { return code->bytecode_len; }

void sl_emit(struct SL_Code *code, uint32_t value) { emit(code, value); }

size_t sl_emit_jump(struct SL_Code *code, uint32_t jump_opcode) {
  emit(code, jump_opcode);
  size_t patch_position = code->bytecode_len;
  emit(code, 0);
  return patch_position;
}

void sl_patch_jump_to(struct SL_Code *code, size_t patch_position,
                      size_t target_pc) {
  code->bytecode[patch_position] = (uint32_t)target_pc;
}

void sl_patch_jump_here(struct SL_Code *code, size_t patch_position) {
  sl_patch_jump_to(code, patch_position, code->bytecode_len);
}

int sl_add_constant(struct SL_Code *code, struct SL_Variable constant) {
  if (code->constant_pool_len >= code->constant_pool_capacity) {
    code->constant_pool_capacity = (code->constant_pool_capacity == 0)
                                       ? 64
                                       : code->constant_pool_capacity * 2;
    code->constant_pool =
        srealloc(NULL, code->constant_pool,
                 code->constant_pool_capacity * sizeof(struct SL_Variable));
  }
  code->constant_pool[code->constant_pool_len] = constant;
  return code->constant_pool_len++;
}

void sl_emit_push_constant(struct SL_Code *code, struct SL_Variable constant) {
  int pool_index = sl_add_constant(code, constant);
  emit(code, OPCODE_PUSH_POOL_CONST);
  emit(code, (uint32_t)pool_index);
}

void sl_emit_push_string(struct SL_Code *code, const char *text) {
  char *quoted_text = sl_quote_string(text);
  struct SL_Variable string_constant = sl_word_to_var_converter(quoted_text);
  free(quoted_text);
  sl_emit_push_constant(code, string_constant);
}

void sl_compile_statements(struct SL_Code *code, int first_token,
                           int end_token) {
  int old_starting_token = code->starting_token;
  int old_token_count = code->token_count;
  int old_types_set = code->types_set;

  code->starting_token = first_token;
  code->token_count = end_token;
  code->types_set = 1;

  sl_init_sl_parser(code);

  code->types_set = old_types_set;
  code->starting_token = old_starting_token;
  code->token_count = old_token_count;
}

void sl_compile_expression(struct SL_Code *code, int first_token,
                           int end_token) {
  int current_token = first_token;
  sl_expression_solver(code, code->code, code->types, &current_token,
                       end_token);
}

void sl_begin_frame_scope(struct SL_Code *code, struct SL_FrameScope *scope) {
  scope->saved_outer_count = code->frame.total_vars;
  scope->saved_outer_vars = NULL;
  if (scope->saved_outer_count > 0) {
    scope->saved_outer_vars =
        smalloc(NULL, scope->saved_outer_count * sizeof(struct SL_Variable));
    memcpy(scope->saved_outer_vars, code->frame.vars,
           scope->saved_outer_count * sizeof(struct SL_Variable));
  }
  code->frame.total_vars = 0;
  scope_push(scope);
  code->scope_depth++;
}

int sl_end_frame_scope(struct SL_Code *code, struct SL_FrameScope *scope) {
  scope_pop();
  code->scope_depth--;

  int frame_size = code->frame.total_vars;
  for (int i = 0; i < code->frame.total_vars; i++) {
    sl_free_variable(&code->frame.vars[i]);
    memset(&code->frame.vars[i], 0, sizeof(struct SL_Variable));
  }
  code->frame.total_vars = 0;

  if (scope->saved_outer_vars != NULL) {
    frame_reserve(code, (size_t)scope->saved_outer_count + 2);
    memcpy(code->frame.vars, scope->saved_outer_vars,
           scope->saved_outer_count * sizeof(struct SL_Variable));
    code->frame.total_vars = scope->saved_outer_count;
    free(scope->saved_outer_vars);
    scope->saved_outer_vars = NULL;
  }
  return frame_size;
}

void sl_loop_begin(struct SL_Code *code) {
  if (global_loop_stack.capacity == 0) {
    global_loop_stack.capacity = 16;
    global_loop_stack.contexts =
        scalloc(NULL, global_loop_stack.capacity, sizeof(SL_LoopContext));
  } else if (global_loop_stack.depth >= global_loop_stack.capacity) {
    global_loop_stack.capacity *= 2;
    global_loop_stack.contexts =
        srealloc(NULL, global_loop_stack.contexts,
                 global_loop_stack.capacity * sizeof(SL_LoopContext));
  }

  SL_LoopContext *context =
      &global_loop_stack.contexts[global_loop_stack.depth++];
  context->start_pc = (uint32_t)code->bytecode_len;
  context->break_count = 0;
  context->break_capacity = 16;
  context->break_patches =
      scalloc(NULL, context->break_capacity, sizeof(size_t));
}

uint32_t sl_loop_start_pc(void) {
  return global_loop_stack.contexts[global_loop_stack.depth - 1].start_pc;
}

void sl_loop_end(struct SL_Code *code) {
  SL_LoopContext *context =
      &global_loop_stack.contexts[global_loop_stack.depth - 1];
  for (int i = 0; i < context->break_count; i++) {
    sl_patch_jump_here(code, context->break_patches[i]);
  }
  free(context->break_patches);
  context->break_patches = NULL;
  global_loop_stack.depth--;
}

int sl_find_function_index(struct SL_Code *code, const char *name) {
  return is_has_func(*code, name);
}

void sl_emit_call_function(struct SL_Code *code, int function_index,
                           int argument_count) {
  if (code->funcs[function_index].linked_function == 1)
    emit(code, OPCODE_CALL_LINKED_FUNC);
  else
    emit(code, OPCODE_FUNC_CALL);
  emit(code, (uint32_t)function_index);
  emit(code, (uint32_t)argument_count);
}

int sl_find_end(char **tokens, enum TokenTypes *types, int start,
                int max_tokens, int branch) {
  int depth = 0;

  for (int i = start; i < max_tokens; i++) {
    if (types[i] == T_IF || types[i] == T_DEF || types[i] == T_WHILE) {
      depth++;
    } else if (types[i] == T_END) {
      if (depth == 0)
        return i;

      depth--;
    } else if (branch == 1 && depth == 0 &&
               (types[i] == T_ELSE || types[i] == T_ELIF)) {
      return i;
    }
  }

  return -1;
}

static inline enum TokenTypes identifier_tokenizer_converter(const char *s) {
  switch (s[0]) {
  case 'i':
    if (s[1] == 'f' && s[2] == '\0')
      return T_IF;

    if (s[1] == 'm' && s[2] == 'p' && s[3] == 'o' && s[4] == 'r' &&
        s[5] == 't' && s[6] == '\0')
      return T_IMPORT;
    break;

  case 'w':
    if (s[1] == 'h' && s[2] == 'i' && s[3] == 'l' && s[4] == 'e' &&
        s[5] == '\0')
      return T_WHILE;
    break;

  case 'd':
    if (s[1] == 'e' && s[2] == 'f' && s[3] == '\0')
      return T_DEF;
    break;

  case 'e':
    if (s[1] == 'n' && s[2] == 'd' && s[3] == '\0')
      return T_END;

    if (s[1] == 'l' && s[2] == 's' && s[3] == 'e' && s[4] == '\0')
      return T_ELSE;

    if (s[1] == 'l' && s[2] == 'i' && s[3] == 'f' && s[4] == '\0')
      return T_ELIF;

    if (s[1] == 'q' && s[2] == 'u' && s[3] == '\0')
      return T_EQU;

    if (s[1] == 'q' && s[2] == 'g' && s[3] == '\0')
      return T_EQG;

    if (s[1] == 'q' && s[2] == 'l' && s[3] == '\0')
      return T_EQL;
    break;

  case 't':
    if (s[1] == 'h' && s[2] == 'e' && s[3] == 'n' && s[4] == '\0')
      return T_THEN;
    break;

  case 'v':
    if (s[1] == 'a' && s[2] == 'r' && s[3] == '\0')
      return T_VAR;
    break;

  case 'b':
    if (s[1] == 'r' && s[2] == 'e' && s[3] == 'a' && s[4] == 'k' &&
        s[5] == '\0')
      return T_BREAK;
    break;

  case 'c':
    if (s[1] == 'o' && s[2] == 'n' && s[3] == 't' && s[4] == 'i' &&
        s[5] == 'n' && s[6] == 'u' && s[7] == 'e' && s[8] == '\0')
      return T_CONTINUE;
    break;

  case 'r':
    if (s[1] == 'e' && s[2] == 't' && s[3] == 'u' && s[4] == 'r' &&
        s[5] == 'n' && s[6] == '\0')
      return T_RETURN;
    break;

  case 'n':
    if (s[1] == 'e' && s[2] == 'q' && s[3] == '\0')
      return T_NEQ;
    break;

  case 'a':
    if (s[1] == 'n' && s[2] == 'd' && s[3] == '\0')
      return T_AND;
    break;

  case 'o':
    if (s[1] == 'r' && s[2] == '\0')
      return T_OR;
    break;

  case '<':
    if (s[1] == '<' && s[2] == '\0')
      return T_SHLEFT;
    break;

  case '>':
    if (s[1] == '>' && s[2] == '\0')
      return T_SHRIGHT;
    break;
  }

  return T_UNKNOWN;
}

void sl_identifier_tokenizer(struct SL_Code *code_s, char **code,
                             enum TokenTypes **types,
                             struct SL_Variable **fixed_values,
                             int token_count) {
  if (code == NULL || types == NULL || *types == NULL || fixed_values == NULL ||
      *fixed_values == NULL)
    return;
  enum TokenTypes *out = *types;
  struct SL_Variable *fixed = *fixed_values;

  for (int i = 0; i < token_count; ++i) {
    sl_free_variable(&fixed[i]);

    out[i] = identifier_tokenizer_converter(code[i]);

    if (out[i] == T_UNKNOWN) {
      fixed[i] = sl_word_to_var_converter(code[i]);
      if (fixed[i].type == ERROR) {
        sl_throw_an_error(*code_s, code, i, token_count, fixed[i].vals,
                          "Integers can only contain digits.");
      }
    }
  }
}

static void sl_vm_error(struct SL_Code *code, uint32_t token_idx,
                        char *error_msg, char *expected_tip) {
  if (code->code != NULL) {
    sl_throw_an_error(*code, code->code, (int)token_idx, code->token_count,
                      error_msg, expected_tip);
  }
  fprintf(stderr, "[VM ERROR] %s\n", error_msg);
  if (expected_tip != NULL)
    fprintf(stderr, "[HINT] %s\n", expected_tip);
  exit(-1);
}

static void vm_enter_function(struct SL_Code *code, uint32_t func_idx,
                              uint32_t arg_count, size_t *pc) {
  if (code->obj.call_frame_size >= code->obj.call_frame_capacity) {
    code->obj.call_frame_capacity = (code->obj.call_frame_capacity == 0)
                                        ? SL_INIT
                                        : code->obj.call_frame_capacity * 2;
    code->obj.call_frames =
        srealloc(NULL, code->obj.call_frames,
                 code->obj.call_frame_capacity * sizeof(struct SL_CallFrame));
  }
  code->obj.call_frames[code->obj.call_frame_size++] =
      (struct SL_CallFrame){.frame_base = code->current_frame_base,
                            .saved_total_vars = code->frame.total_vars};

  size_t new_base = code->frame.total_vars;
  int nfixed = code->funcs[func_idx].total_arguments;
  int fsize = code->funcs[func_idx].frame_size;
  if (fsize < nfixed)
    fsize = nfixed;
  size_t extra =
      ((int)arg_count > nfixed) ? (size_t)((int)arg_count - nfixed) : 0;

  frame_reserve(code, new_base + fsize + extra + 1);

  for (int i = (int)arg_count - 1; i >= 0; i--) {
    struct SL_Variable value = pop(code);
    size_t slot = (i < nfixed) ? new_base + i : new_base + fsize + (i - nfixed);
    sl_free_variable(&code->frame.vars[slot]);
    if (value.name != NULL) {
      free(value.name);
      value.name = NULL;
    }
    code->frame.vars[slot] = value;
  }

  code->current_frame_base = new_base;
  code->frame.total_vars = new_base + fsize + extra;

  if (code->obj.call_stack_size >= code->obj.call_stack_capacity) {
    code->obj.call_stack_capacity *= 2;
    code->obj.call_stack =
        srealloc(NULL, code->obj.call_stack,
                 code->obj.call_stack_capacity * sizeof(size_t));
  }
  code->obj.call_stack[code->obj.call_stack_size++] = *pc;
  *pc = (size_t)code->funcs[func_idx].entry_pc;
}

static const char *vm_type_name(enum SL_Types type) {
  switch (type) {
  case INIT:
    return "INIT";
  case INTEGER:
    return "INTEGER";
  case DOUBLE:
    return "DOUBLE";
  case CHAR:
    return "CHAR";
  case STRING:
    return "STRING";
  case BOOLEAN:
    return "BOOLEAN";
  case RETURN:
    return "RETURN";
  case LONG:
    return "LONG";
  case ERROR:
    return "ERROR";
  case POINTER:
    return "POINTER";
  case BYTES:
    return "BYTES";
  }
  return "UNKNOWN";
}

static void vm_prepare_operands(struct SL_Code *code, struct SL_Variable *left,
                                struct SL_Variable *right) {
  if (left->type == DOUBLE && right->type == INTEGER) {
    right->valf = (double)right->vali;
    right->type = DOUBLE;
  } else if (left->type == INTEGER && right->type == DOUBLE) {
    left->valf = (double)left->vali;
    left->type = DOUBLE;
  }

  if (left->type == right->type) {
    return;
  }

  static char error_message[128];
  snprintf(error_message, sizeof(error_message),
           "Type mismatch: cannot operate between %s and %s",
           vm_type_name(left->type), vm_type_name(right->type));
  sl_vm_error(code, (uint32_t)-1, error_message,
              "Ensure that the variables you are trying to operate on "
              "(String and Integer) are compatible with each other.");
}

static void vm_push_stepped_variable(struct SL_Code *code, uint32_t var_idx,
                                     int step, const char *opcode_name) {
  struct SL_Variable variable = code->vars[var_idx];

  if (variable.type == INTEGER) {
    variable.vali += step;
  } else if (variable.type == DOUBLE) {
    variable.valf += step;
  } else {
    static char error_message[128];
    snprintf(error_message, sizeof(error_message),
             "Type mismatch: %s expects INTEGER or DOUBLE, got %s", opcode_name,
             vm_type_name(variable.type));
    sl_vm_error(code, (uint32_t)-1, error_message,
                "Ensure that the variable you are incrementing or "
                "decrementing is a number.");
  }

  push(code, copy_value_without_name(variable));
}

static int find_assignment_token(struct SL_Code *code, const char *var_name) {
  if (code->code == NULL) {
    return -1;
  }

  for (int i = 0; i + 1 < code->token_count; i++) {
    char *token = code->code[i];
    char *next_token = code->code[i + 1];
    if (token == NULL || next_token == NULL) {
      continue;
    }

    if (token[0] == '$') {
      token++;
    }
    if (strcmp(token, var_name) == 0 && strcmp(next_token, "=") == 0) {
      return i;
    }
  }
  return -1;
}

static int find_token_by_name(struct SL_Code *code, const char *name) {
  if (code->code == NULL || name == NULL)
    return -1;

  for (int i = 0; i < code->token_count; i++) {
    char *token = code->code[i];
    if (token != NULL && strcmp(token, name) == 0)
      return i;
  }
  return -1;
}

static inline int vm_top_pair_plain_matched(struct SL_Code *code,
                                            struct SL_Variable **lhs,
                                            struct SL_Variable **rhs) {
  if (code->obj.stack_size < 2)
    return 0;
  *rhs = &code->obj.stack[code->obj.stack_size - 1];
  *lhs = *rhs - 1;
  if ((*lhs)->name != NULL || (*rhs)->name != NULL)
    return 0;
  if ((*lhs)->type == DOUBLE && (*rhs)->type == INTEGER) {
    (*rhs)->valf = (double)(*rhs)->vali;
    (*rhs)->type = DOUBLE;
  } else if ((*lhs)->type == INTEGER && (*rhs)->type == DOUBLE) {
    (*lhs)->valf = (double)(*lhs)->vali;
    (*lhs)->type = DOUBLE;
  }
  return (*lhs)->type == (*rhs)->type;
}

static inline void vm_collapse_pair_to_double(struct SL_Code *code,
                                              double value) {
  struct SL_Variable *slot = &code->obj.stack[code->obj.stack_size - 2];
  memset(slot, 0, sizeof *slot);
  slot->type = DOUBLE;
  slot->valf = value;
  code->obj.stack_size--;
}

static inline void vm_collapse_pair_to_integer(struct SL_Code *code,
                                               long value) {
  struct SL_Variable *slot = &code->obj.stack[code->obj.stack_size - 2];
  memset(slot, 0, sizeof *slot);
  slot->type = INTEGER;
  slot->vali = value;
  code->obj.stack_size--;
}

static inline void vm_collapse_pair_to_boolean(struct SL_Code *code,
                                               long value) {
  struct SL_Variable *slot = &code->obj.stack[code->obj.stack_size - 2];
  memset(slot, 0, sizeof *slot);
  slot->type = BOOLEAN;
  slot->valb = value;
  code->obj.stack_size--;
}

#define VM_FAST_NUMERIC(DOUBLE_EXPR, INTEGER_EXPR)                             \
  {                                                                            \
    struct SL_Variable *lhs, *rhs;                                             \
    if (vm_top_pair_plain_matched(code, &lhs, &rhs)) {                         \
      if (lhs->type == DOUBLE) {                                               \
        vm_collapse_pair_to_double(code, (DOUBLE_EXPR));                       \
        break;                                                                 \
      }                                                                        \
      if (lhs->type == INTEGER) {                                              \
        vm_collapse_pair_to_integer(code, (INTEGER_EXPR));                     \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
  }

#define VM_FAST_ARITHMETIC(OP)                                                 \
  VM_FAST_NUMERIC(lhs->valf OP rhs->valf, lhs->vali OP rhs->vali)

#define VM_FAST_DIVISION                                                       \
  VM_FAST_NUMERIC(rhs->valf != 0 ? lhs->valf / rhs->valf : 0,                  \
                  rhs->vali != 0 ? lhs->vali / rhs->vali : 0)

#define VM_FAST_MODULO                                                         \
  {                                                                            \
    struct SL_Variable *lhs, *rhs;                                             \
    if (vm_top_pair_plain_matched(code, &lhs, &rhs) && lhs->type == INTEGER && \
        rhs->vali != 0) {                                                      \
      vm_collapse_pair_to_integer(code, lhs->vali % rhs->vali);                \
      break;                                                                   \
    }                                                                          \
  }

#define VM_FAST_COMPARISON(OP)                                                 \
  {                                                                            \
    struct SL_Variable *lhs, *rhs;                                             \
    if (vm_top_pair_plain_matched(code, &lhs, &rhs)) {                         \
      if (lhs->type == DOUBLE) {                                               \
        vm_collapse_pair_to_boolean(code, lhs->valf OP rhs->valf);             \
        break;                                                                 \
      }                                                                        \
      if (lhs->type == INTEGER) {                                              \
        vm_collapse_pair_to_boolean(code, lhs->vali OP rhs->vali);             \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
  }

static struct SL_Variable
vm_call_native(struct SL_Code *code, uint32_t func_idx, uint32_t arg_count) {
  int starting_index = code->total_vars;
  int starting_index_func = code->total_funcs;
  if (code->total_vars + arg_count >= code->total_size_v) {
    code->total_size_v =
        (code->total_size_v == 0) ? SL_INIT : code->total_size_v * 2;
    while (code->total_vars + arg_count >= code->total_size_v) {
      code->total_size_v *= 2;
    }
    code->vars = srealloc(NULL, code->vars,
                          code->total_size_v * sizeof(struct SL_Variable));
  }

  for (int i = (int)arg_count - 1; i >= 0; i--) {
    code->vars[starting_index + i] = pop(code);
  }
  code->total_vars += arg_count;

  struct SL_L_Function lfunc = {0};
  lfunc.total_arguments = arg_count;
  lfunc.starting_index = starting_index;

  struct SL_Variable result =
      code->funcs[func_idx].funcr(code, lfunc, code->funcs[func_idx]);

  if (result.type == STRING && result.vals != NULL) {
    for (int i = starting_index; i < code->total_vars; i++) {
      if (result.vals == code->vars[i].vals) {
        result.vals = strdup(result.vals);
        break;
      }
    }
  }

  sl_clean_local_scope(code, starting_index, starting_index_func);
  return result;
}

static size_t vm_append_quoted_content(char *out, size_t j,
                                       const char *quoted) {
  size_t end = strlen(quoted) - 1;
  for (size_t i = 1; i < end; i++) {
    char ch = quoted[i];
    if (ch == '\\' && i + 1 < end)
      ch = sl_unescape_char(quoted[++i]);
    if (ch == '\0')
      break;
    if (ch == '"' || ch == '\\')
      out[j++] = '\\';
    out[j++] = ch;
  }
  return j;
}

static char *vm_concat_quoted(const char *left, const char *right) {
  char *out = smalloc(NULL, 2 * (strlen(left) + strlen(right)) + 3);
  size_t j = 0;
  out[j++] = '"';
  j = vm_append_quoted_content(out, j, left);
  j = vm_append_quoted_content(out, j, right);
  out[j++] = '"';
  out[j] = '\0';
  return out;
}

static int vm_resolve_function_constant(struct SL_Code *code,
                                        uint32_t pool_index) {
  struct SL_Variable *name_constant = &code->constant_pool[pool_index];
  if (name_constant->hash == 0)
    name_constant->hash = sl_hash_string(name_constant->vals);

  int cached = name_constant->cache_index - 1;
  if (cached >= 0 && cached < code->total_funcs &&
      code->funcs[cached].hash == name_constant->hash &&
      strcmp(code->funcs[cached].name, name_constant->vals) == 0)
    return cached;

  int found =
      is_has_func_hashed_p(code, name_constant->vals, name_constant->hash);
  if (found != -1)
    name_constant->cache_index = found + 1;
  return found;
}

static inline void vm_push_variable_value(struct SL_Code *code,
                                          const struct SL_Variable *source) {
  if (SL_UNLIKELY(code->obj.stack_size >= code->obj.stack_capacity))
    stack_grow(code);
  struct SL_Variable *slot = &code->obj.stack[code->obj.stack_size++];
  *slot = *source;
  slot->name = NULL;
  if ((source->type == STRING || source->type == RETURN) &&
      source->vals != NULL) {
    slot->vals = strdup(source->vals);
  } else if (source->type == BYTES && source->vals != NULL) {
    slot->vals = sl_bytes_copy(source->vals, source->length);
  }
}

static void vm_loop(struct SL_Code *code, size_t pc) {
  while (pc < code->bytecode_len) {
    uint32_t instruction = code->bytecode[pc++];

    switch (instruction) {
      /*
case OPCODE_PUSH_CONST: {
uint32_t const_idx = code->bytecode[pc++];
push(code, copy_value_without_name(code->fixed_values[const_idx]));
break;
} OLD USAGE -> 3151 */

    case OPCODE_LOAD_VAR: {
      uint32_t var_idx = code->bytecode[pc++];
      vm_push_variable_value(code, &code->vars[var_idx]);
      break;
    }

    case OPCODE_INC_VAR: {
      uint32_t var_idx = code->bytecode[pc++];
      vm_push_stepped_variable(code, var_idx, 1, "INC");
      break;
    }

    case OPCODE_DEC_VAR: {
      uint32_t var_idx = code->bytecode[pc++];
      vm_push_stepped_variable(code, var_idx, -1, "DEC");
      break;
    }

    case OPCODE_STORE_VAR: {
      uint32_t var_idx = code->bytecode[pc++];
      struct SL_Variable value = pop(code);
      struct SL_Variable *slot = &code->vars[var_idx];
      char *name = slot->name;
      unsigned long hash = slot->hash;
      slot->name = NULL;
      sl_free_variable(slot);
      if (value.name != NULL) {
        free(value.name);
        value.name = NULL;
      }
      *slot = value;
      slot->name = name;
      slot->hash = hash;
      break;
    }

    case OPCODE_LOAD_LOCAL_VAR: {
      uint32_t var_idx = code->bytecode[pc++];
      size_t actual_idx = code->current_frame_base + var_idx;
      vm_push_variable_value(code, &code->frame.vars[actual_idx]);
      break;
    }

    case OPCODE_STORE_LOCAL_VAR: {
      uint32_t var_idx = code->bytecode[pc++];
      size_t actual_idx = code->current_frame_base + var_idx;
      struct SL_Variable value = pop(code);
      sl_free_variable(&code->frame.vars[actual_idx]);
      if (value.name != NULL) {
        free(value.name);
        value.name = NULL;
      }
      code->frame.vars[actual_idx] = value;
      break;
    }

    case OPCODE_LOAD_OUTER_LOCAL: {
      uint32_t depth = code->bytecode[pc++];
      uint32_t idx = code->bytecode[pc++];
      size_t base =
          code->obj.call_frames[code->obj.call_frame_size - depth].frame_base;
      vm_push_variable_value(code, &code->frame.vars[base + idx]);
      break;
    }

    case OPCODE_STORE_OUTER_LOCAL: {
      uint32_t depth = code->bytecode[pc++];
      uint32_t idx = code->bytecode[pc++];
      size_t base =
          code->obj.call_frames[code->obj.call_frame_size - depth].frame_base;
      size_t actual = base + idx;
      struct SL_Variable value = pop(code);
      sl_free_variable(&code->frame.vars[actual]);
      if (value.name != NULL) {
        free(value.name);
        value.name = NULL;
      }
      code->frame.vars[actual] = value;
      break;
    }

    case OPCODE_FUNC_CALL: {
      uint32_t func_idx = code->bytecode[pc++];
      uint32_t arg_count = code->bytecode[pc++];
      vm_enter_function(code, func_idx, arg_count, &pc);
      break;
    }

    case OPCODE_PUSH_POOL_CONST: {
      uint32_t pool_index = code->bytecode[pc++];
      vm_push_variable_value(code, &code->constant_pool[pool_index]);
      break;
    }

    case OPCODE_PUSH_EMPTY: {
      struct SL_Variable empty_value = {0};
      push(code, empty_value);
      break;
    }

    case OPCODE_SET_ASSIGNMENT: {
      struct SL_Variable name_var = pop(code);
      free(current_assignment);
      current_assignment = sl_string_getter(name_var.vals);
      sl_free_variable(&name_var);
      break;
    }
    case OPCODE_CALL_ADDRESS: {
      uint32_t call_target_pc = code->bytecode[pc++];
      uint32_t callee_frame_size = code->bytecode[pc++];

      if (code->obj.call_frame_size >= code->obj.call_frame_capacity) {
        code->obj.call_frame_capacity = (code->obj.call_frame_capacity == 0)
                                            ? SL_INIT
                                            : code->obj.call_frame_capacity * 2;
        code->obj.call_frames = srealloc(NULL, code->obj.call_frames,
                                         code->obj.call_frame_capacity *
                                             sizeof(struct SL_CallFrame));
      }
      code->obj.call_frames[code->obj.call_frame_size++] =
          (struct SL_CallFrame){.frame_base = code->current_frame_base,
                                .saved_total_vars = code->frame.total_vars};

      size_t callee_base = code->frame.total_vars;
      frame_reserve(code, callee_base + callee_frame_size + 1);
      code->current_frame_base = callee_base;
      code->frame.total_vars = callee_base + callee_frame_size;

      if (code->obj.call_stack_size >= code->obj.call_stack_capacity) {
        code->obj.call_stack_capacity *= 2;
        code->obj.call_stack =
            srealloc(NULL, code->obj.call_stack,
                     code->obj.call_stack_capacity * sizeof(size_t));
      }
      code->obj.call_stack[code->obj.call_stack_size++] = pc;
      pc = (size_t)call_target_pc;
      break;
    }

    case OPCODE_RETURN: {
      if (code->obj.call_stack_size == 0) {
        return;
      }

      for (size_t i = code->current_frame_base;
           i < (size_t)code->frame.total_vars; i++) {
        sl_free_variable(&code->frame.vars[i]);
        memset(&code->frame.vars[i], 0, sizeof(struct SL_Variable));
      }

      if (code->obj.call_frame_size > 0) {
        struct SL_CallFrame prev_frame =
            code->obj.call_frames[--code->obj.call_frame_size];
        code->current_frame_base = prev_frame.frame_base;
        code->frame.total_vars = prev_frame.saved_total_vars;
      }

      pc = code->obj.call_stack[--code->obj.call_stack_size];
      break;
    }

    case OPCODE_CALL_LINKED_FUNC: {
      uint32_t func_idx = code->bytecode[pc++];
      uint32_t arg_count = code->bytecode[pc++];

      struct SL_Variable result = vm_call_native(code, func_idx, arg_count);

      push(code, result);
      break;
    }

    case OPCODE_CALL_DYNAMIC_LINKED_FUNC: {
      uint32_t name_const_idx = code->bytecode[pc++];
      uint32_t arg_count = code->bytecode[pc++];

      char *func_name = code->constant_pool[name_const_idx].vals;

      int func_idx = vm_resolve_function_constant(code, name_const_idx);
      if (func_idx == -1) {
        int error_token = find_token_by_name(code, func_name);
        sl_vm_error(
            code, (uint32_t)error_token, "FUNCTION NOT FOUND!",
            "Expected: Create a function first (variables need a '$' prefix).");
      }

      if (code->funcs[func_idx].linked_function != 1) {
        vm_enter_function(code, func_idx, arg_count, &pc);
        break;
      }

      struct SL_Variable result = vm_call_native(code, func_idx, arg_count);

      push(code, result);
      break;
    }

    case OPCODE_LOAD_DYNAMIC_ADDED_VAR: {
      uint32_t var_idx = code->bytecode[pc++];
      char *var_name = code->constant_pool[var_idx].vals;

      struct SL_Variable *name_constant = &code->constant_pool[var_idx];
      if (name_constant->hash == 0)
        name_constant->hash = sl_hash_string(var_name + 1);
      int real_idx =
          getvar_index_from_sl(*code, var_name + 1, name_constant->hash);
      if (real_idx == -1) {
        static char error_message[128];
        snprintf(error_message, sizeof(error_message),
                 "VARIABLE NOT FOUND! (%s)", var_name);

        int error_token = find_token_by_name(code, var_name);
        sl_vm_error(code, (uint32_t)error_token, error_message,
                    "Expected: Define a variable first.");
      }
      push(code, copy_value_without_name(code->vars[real_idx]));
      break;
    }

    case OPCODE_STORE_DYNAMIC_ADDED_VAR: {
      struct SL_Variable name_var = pop(code);
      struct SL_Variable value = pop(code);
      char *var_name = sl_string_getter(name_var.vals);
      sl_free_variable(&name_var);

      int real_idx =
          getvar_index_from_sl(*code, var_name, sl_hash_string(var_name));
      if (real_idx == -1) {
        static char error_message[128];
        snprintf(error_message, sizeof(error_message),
                 "VARIABLE NOT FOUND ON ASSIGNMENT! (%s)", var_name);
        int error_token = find_assignment_token(code, var_name);
        sl_vm_error(code, (uint32_t)error_token, error_message,
                    "Expected: Define a variable first with 'var'.");
      }
      free(var_name);

      struct SL_Variable *slot = &code->vars[real_idx];
      char *name = slot->name;
      unsigned long hash = slot->hash;
      slot->name = NULL;
      sl_free_variable(slot);
      if (value.name != NULL) {
        free(value.name);
        value.name = NULL;
      }
      *slot = value;
      slot->name = name;
      slot->hash = hash;
      break;
    }

    case OPCODE_IF_JUMP_FALSE: {
      uint32_t target_pc = code->bytecode[pc++];
      if (code->obj.stack_size > 0) {
        struct SL_Variable *top = &code->obj.stack[code->obj.stack_size - 1];
        if (top->type == BOOLEAN && top->name == NULL) {
          code->obj.stack_size--;
          if (top->valb == 0)
            pc = target_pc;
          break;
        }
      }
      struct SL_Variable condition = pop(code);

      int is_false = 0;
      if (condition.type == BOOLEAN)
        is_false = (condition.valb == 0);
      else if (condition.type == INIT)
        is_false = 1;

      sl_free_variable(&condition);

      if (is_false) {
        pc = target_pc;
      }
      break;
    }

    case OPCODE_JMP: {
      uint32_t target_pc = code->bytecode[pc++];
      pc = target_pc;
      break;
    }

      /* Arithmetic Logic Unit */
    case OPCODE_ADD: {
      VM_FAST_ARITHMETIC(+)
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {0};

      result.type = left.type;
      switch (left.type) {
      case INTEGER:
        result.vali = left.vali + right.vali;
        break;
      case DOUBLE:
        result.valf = left.valf + right.valf;
        break;
      case BOOLEAN:
        result.valb = left.valb + right.valb;
        break;
      case LONG:
        result.valh = left.valh + right.valh;
        break;
      case STRING: {
        if (string_checker(left.vals) == 1 && string_checker(right.vals) == 1) {
          result.vals = vm_concat_quoted(left.vals, right.vals);
          break;
        }
        char *left_str = sl_string_getter(left.vals);
        char *right_str = sl_string_getter(right.vals);
        size_t len = strlen(left_str) + strlen(right_str);
        char *joined = smalloc(&result, len + 1);
        if (result.type == ERROR) {
          /* free everything on failure */
          free(left_str);
          free(right_str);
          sl_free_variable(&left);
          sl_free_variable(&right);
          fprintf(stderr, "error: in vm_loop(): %s", result.vals);
          exit(EXIT_FAILURE);
        }
        strcpy(joined, left_str);
        strcat(joined, right_str);
        result.vals = sl_quote_string(joined);
        free(joined);
        free(left_str);
        free(right_str);
        break;
      }
      default:
        break;
      }
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    case OPCODE_SUB: {
      VM_FAST_ARITHMETIC(-)
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {0};

      result.type = left.type;
      switch (left.type) {
      case INTEGER:
        result.vali = left.vali - right.vali;
        break;
      case DOUBLE:
        result.valf = left.valf - right.valf;
        break;
      case BOOLEAN:
        result.valb = left.valb - right.valb;
        break;
      case LONG:
        result.valh = left.valh - right.valh;
        break;
      default:
        break;
      }
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    case OPCODE_MUL: {
      VM_FAST_ARITHMETIC(*)
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {0};

      result.type = left.type;
      switch (left.type) {
      case INTEGER:
        result.vali = left.vali * right.vali;
        break;
      case DOUBLE:
        result.valf = left.valf * right.valf;
        break;
      case BOOLEAN:
        result.valb = left.valb * right.valb;
        break;
      case LONG:
        result.valh = left.valh * right.valh;
        break;
      default:
        break;
      }
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    case OPCODE_DIV: {
      VM_FAST_DIVISION
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {0};

      result.type = left.type;
      switch (left.type) {
      case INTEGER:
        if (right.vali != 0)
          result.vali = left.vali / right.vali;
        break;
      case DOUBLE:
        if (right.valf != 0)
          result.valf = left.valf / right.valf;
        break;
      case BOOLEAN:
        if (right.valb != 0)
          result.valb = left.valb / right.valb;
        break;
      case LONG:
        if (right.valh != 0)
          result.valh = left.valh / right.valh;
        break;
      default:
        break;
      }
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    case OPCODE_MOD: {
      VM_FAST_MODULO
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {0};
      result.type = left.type;
      if (left.type == INTEGER)
        result.vali = left.vali % right.vali;
      else if (left.type == LONG)
        result.valh = left.valh % right.valh;
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    /* BITWISE */
    case OPCODE_BAND: {
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {0};
      result.type = left.type;
      if (left.type == INTEGER)
        result.vali = left.vali & right.vali;
      else if (left.type == LONG)
        result.valh = left.valh & right.valh;
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    case OPCODE_BOR: {
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {0};
      result.type = left.type;
      if (left.type == INTEGER)
        result.vali = left.vali | right.vali;
      else if (left.type == LONG)
        result.valh = left.valh | right.valh;
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    case OPCODE_XOR: {
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {0};
      result.type = left.type;
      if (left.type == INTEGER)
        result.vali = left.vali ^ right.vali;
      else if (left.type == LONG)
        result.valh = left.valh ^ right.valh;
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    case OPCODE_SHR: {
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {0};
      result.type = left.type;
      if (left.type == INTEGER)
        result.vali = left.vali >> right.vali;
      else if (left.type == LONG)
        result.valh = left.valh >> right.valh;
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    case OPCODE_SHL: {
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {0};
      result.type = left.type;
      if (left.type == INTEGER)
        result.vali = left.vali << right.vali;
      else if (left.type == LONG)
        result.valh = left.valh << right.valh;
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    /* CONDITIONS */
    case OPCODE_BIGGER: {
      VM_FAST_COMPARISON(>)
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {.type = BOOLEAN};
      if (left.type == INTEGER)
        result.valb = left.vali > right.vali;
      else if (left.type == DOUBLE)
        result.valb = left.valf > right.valf;
      else if (left.type == LONG)
        result.valb = left.valh > right.valh;
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    case OPCODE_LESSER: {
      VM_FAST_COMPARISON(<)
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {.type = BOOLEAN};
      if (left.type == INTEGER)
        result.valb = left.vali < right.vali;
      else if (left.type == DOUBLE)
        result.valb = left.valf < right.valf;
      else if (left.type == LONG)
        result.valb = left.valh < right.valh;
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    case OPCODE_EQU: {
      VM_FAST_COMPARISON(==)
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {.type = BOOLEAN};
      if (left.type == INTEGER)
        result.valb = left.vali == right.vali;
      else if (left.type == DOUBLE)
        result.valb = left.valf == right.valf;
      else if (left.type == CHAR)
        result.valb = left.valc == right.valc;
      else if (left.type == BOOLEAN)
        result.valb = left.valb == right.valb;
      else if (left.type == LONG)
        result.valb = left.valh == right.valh;
      else if (left.type == STRING) {
        char *ls = sl_string_getter(left.vals);
        char *rs = sl_string_getter(right.vals);
        result.valb = (strcmp(ls, rs) == 0);
        free(ls);
        free(rs);
      }
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    case OPCODE_NEQ: {
      VM_FAST_COMPARISON(!=)
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {.type = BOOLEAN};
      if (left.type == INTEGER)
        result.valb = left.vali != right.vali;
      else if (left.type == DOUBLE)
        result.valb = left.valf != right.valf;
      else if (left.type == BOOLEAN)
        result.valb = left.valb != right.valb;
      else if (left.type == CHAR)
        result.valb = left.valc != right.valc;
      else if (left.type == LONG)
        result.valb = left.valh != right.valh;
      else if (left.type == STRING) {
        char *ls = sl_string_getter(left.vals);
        char *rs = sl_string_getter(right.vals);
        result.valb = (strcmp(ls, rs) != 0);
        free(ls);
        free(rs);
      }
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    case OPCODE_EQG: {
      VM_FAST_COMPARISON(>=)
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {.type = BOOLEAN};
      if (left.type == INTEGER)
        result.valb = left.vali >= right.vali;
      else if (left.type == DOUBLE)
        result.valb = left.valf >= right.valf;
      else if (left.type == LONG)
        result.valb = left.valh >= right.valh;
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    case OPCODE_EQL: {
      VM_FAST_COMPARISON(<=)
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {.type = BOOLEAN};
      if (left.type == INTEGER)
        result.valb = left.vali <= right.vali;
      else if (left.type == DOUBLE)
        result.valb = left.valf <= right.valf;
      else if (left.type == LONG)
        result.valb = left.valh <= right.valh;
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    case OPCODE_LAND: {
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {.type = BOOLEAN};
      if (left.type == INTEGER)
        result.valb = left.vali && right.vali;
      else if (left.type == BOOLEAN)
        result.valb = left.valb && right.valb;
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }

    case OPCODE_LOR: {
      struct SL_Variable right = pop(code);
      struct SL_Variable left = pop(code);
      vm_prepare_operands(code, &left, &right);
      struct SL_Variable result = {.type = BOOLEAN};
      if (left.type == INTEGER)
        result.valb = left.vali || right.vali;
      else if (left.type == BOOLEAN)
        result.valb = left.valb || right.valb;
      push(code, result);
      sl_free_variable(&left);
      sl_free_variable(&right);
      break;
    }
      /* Arithmetic Logic Unit */
    default:
      fprintf(stderr, "Unknown opcode: 0x%02X at PC: %zu\n", instruction,
              pc - 1);
      return;
    }
  }
}

static void print_disassembled_value(struct SL_Variable value) {
  switch (value.type) {
  case INTEGER:
    printf("%ld", value.vali);
    break;
  case DOUBLE:
    printf("%g", value.valf);
    break;
  case BOOLEAN:
    printf("%s", value.valb ? "true" : "false");
    break;
  case CHAR:
    printf("'%c'", value.valc);
    break;
  case LONG:
    printf("%jd", value.valh);
    break;
  case STRING:
  case RETURN:
    printf("%s", value.vals != NULL ? value.vals : "(null)");
    break;
  default:
    printf("<%s>", vm_type_name(value.type));
    break;
  }
}

static const char *get_simple_opcode_name(uint32_t opcode) {
  switch (opcode) {
  case OPCODE_ADD:
    return "ADD";
  case OPCODE_SUB:
    return "SUB";
  case OPCODE_MUL:
    return "MUL";
  case OPCODE_DIV:
    return "DIV";
  case OPCODE_MOD:
    return "MOD";
  case OPCODE_BAND:
    return "BAND";
  case OPCODE_BOR:
    return "BOR";
  case OPCODE_XOR:
    return "XOR";
  case OPCODE_SHR:
    return "SHR";
  case OPCODE_SHL:
    return "SHL";
  case OPCODE_BIGGER:
    return "BIGGER";
  case OPCODE_LESSER:
    return "LESSER";
  case OPCODE_EQU:
    return "EQU";
  case OPCODE_NEQ:
    return "NEQ";
  case OPCODE_EQG:
    return "EQG";
  case OPCODE_EQL:
    return "EQL";
  case OPCODE_LAND:
    return "LAND";
  case OPCODE_LOR:
    return "LOR";
  case OPCODE_RETURN:
    return "RETURN";
  case OPCODE_PUSH_EMPTY:
    return "PUSH_EMPTY";
  case OPCODE_SET_ASSIGNMENT:
    return "SET_ASSIGNMENT";
  case OPCODE_STORE_DYNAMIC_ADDED_VAR:
    return "STORE_DYNAMIC_ADDED_VAR";
  default:
    return NULL;
  }
}

static uint32_t read_disassembly_operand(struct SL_Code *code, size_t *pc) {
  if (*pc >= code->bytecode_len) {
    return 0;
  }
  return code->bytecode[(*pc)++];
}

static void print_global_variable_operand(struct SL_Code *code,
                                          uint32_t var_idx) {
  if (var_idx < (uint32_t)code->total_vars &&
      code->vars[var_idx].name != NULL) {
    printf("[%s]", code->vars[var_idx].name);
  } else {
    printf("[var#%u]", var_idx);
  }
}

static void print_pool_constant_operand(struct SL_Code *code,
                                        uint32_t pool_idx) {
  printf("[");
  if (pool_idx < (uint32_t)code->constant_pool_len) {
    print_disassembled_value(code->constant_pool[pool_idx]);
  } else {
    printf("pool#%u", pool_idx);
  }
  printf("]");
}

static void print_function_operand(struct SL_Code *code, uint32_t func_idx,
                                   uint32_t arg_count) {
  if (func_idx < (uint32_t)code->total_funcs &&
      code->funcs[func_idx].name != NULL) {
    printf("[%s] args=%u", code->funcs[func_idx].name, arg_count);
  } else {
    printf("[func#%u] args=%u", func_idx, arg_count);
  }
}

static void disassemble_instruction(struct SL_Code *code, size_t *pc) {
  size_t instruction_pc = *pc;
  uint32_t opcode = code->bytecode[(*pc)++];

  printf("%04zu  ", instruction_pc);

  const char *simple_name = get_simple_opcode_name(opcode);
  if (simple_name != NULL) {
    printf("%s\n", simple_name);
    return;
  }

  switch (opcode) {
  case OPCODE_PUSH_POOL_CONST: {
    uint32_t pool_idx = read_disassembly_operand(code, pc);
    printf("%-24s ", "PUSH_POOL_CONST");
    print_pool_constant_operand(code, pool_idx);
    printf("\n");
    break;
  }

  case OPCODE_LOAD_VAR:
  case OPCODE_STORE_VAR:
  case OPCODE_INC_VAR:
  case OPCODE_DEC_VAR: {
    uint32_t var_idx = read_disassembly_operand(code, pc);
    const char *name = "LOAD_VAR";
    if (opcode == OPCODE_STORE_VAR)
      name = "STORE_VAR";
    if (opcode == OPCODE_INC_VAR)
      name = "INC";
    if (opcode == OPCODE_DEC_VAR)
      name = "DEC";
    printf("%-24s ", name);
    print_global_variable_operand(code, var_idx);
    printf("\n");
    break;
  }

  case OPCODE_LOAD_LOCAL_VAR:
  case OPCODE_STORE_LOCAL_VAR: {
    uint32_t slot = read_disassembly_operand(code, pc);
    const char *name = (opcode == OPCODE_LOAD_LOCAL_VAR) ? "LOAD_LOCAL_VAR"
                                                         : "STORE_LOCAL_VAR";
    printf("%-24s [local#%u]\n", name, slot);
    break;
  }

  case OPCODE_LOAD_DYNAMIC_ADDED_VAR: {
    uint32_t pool_idx = read_disassembly_operand(code, pc);
    printf("%-24s ", "LOAD_DYNAMIC_ADDED_VAR");
    print_pool_constant_operand(code, pool_idx);
    printf("\n");
    break;
  }

  case OPCODE_JMP:
  case OPCODE_IF_JUMP_FALSE: {
    uint32_t target_pc = read_disassembly_operand(code, pc);
    const char *name = (opcode == OPCODE_JMP) ? "JMP" : "IF_JUMP_FALSE";
    printf("%-24s -> %04u\n", name, target_pc);
    break;
  }

  case OPCODE_FUNC_CALL:
  case OPCODE_CALL_LINKED_FUNC: {
    uint32_t func_idx = read_disassembly_operand(code, pc);
    uint32_t arg_count = read_disassembly_operand(code, pc);
    const char *name =
        (opcode == OPCODE_FUNC_CALL) ? "FUNC_CALL" : "CALL_LINKED_FUNC";
    printf("%-24s ", name);
    print_function_operand(code, func_idx, arg_count);
    printf("\n");
    break;
  }

  case OPCODE_CALL_DYNAMIC_LINKED_FUNC: {
    uint32_t pool_idx = read_disassembly_operand(code, pc);
    uint32_t arg_count = read_disassembly_operand(code, pc);
    printf("%-24s ", "CALL_DYNAMIC_LINKED_FUNC");
    print_pool_constant_operand(code, pool_idx);
    printf(" args=%u\n", arg_count);
    break;
  }

  case OPCODE_CALL_ADDRESS: {
    uint32_t target_pc = read_disassembly_operand(code, pc);
    uint32_t frame_size = read_disassembly_operand(code, pc);
    printf("%-24s -> %04u frame_size=%u\n", "CALL_ADDRESS", target_pc,
           frame_size);
    break;
  }

  case OPCODE_LOAD_OUTER_LOCAL:
  case OPCODE_STORE_OUTER_LOCAL: {
    uint32_t depth = read_disassembly_operand(code, pc);
    uint32_t slot = read_disassembly_operand(code, pc);
    printf("%-24s [outer depth=%u local#%u]\n",
           opcode == OPCODE_LOAD_OUTER_LOCAL ? "LOAD_OUTER_LOCAL"
                                             : "STORE_OUTER_LOCAL",
           depth, slot);
    break;
  }

  default:
    printf("UNKNOWN                  0x%02X\n", opcode);
    break;
  }
}

void sl_disassemble(struct SL_Code *code) {
  size_t pc = 0;
  while (pc < code->bytecode_len) {
    disassemble_instruction(code, &pc);
  }
}

void sl_run_vm(struct SL_Code *code) {
  if (code->disassemble) {
    sl_disassemble(code);
    return;
  }

  if (code->obj.call_frame_size == 0) {
    if (code->frame.vars != NULL && code->frame.total_vars_capacity > 0)
      memset(code->frame.vars, 0,
             (size_t)code->frame.total_vars_capacity *
                 sizeof(struct SL_Variable));
    code->frame.total_vars = 0;
    code->current_frame_base = 0;
  }
  vm_loop(code, 0);
}

struct SL_Variable sl_call_function(struct SL_Code *code, int func_idx,
                                    struct SL_Variable *args, int arg_count) {
  struct SL_Variable result = {0};
  size_t stack_before = code->obj.stack_size;

  for (int i = 0; i < arg_count; i++)
    push(code, sl_copy_variable(args[i]));

  size_t pc = (size_t)-1;
  vm_enter_function(code, (uint32_t)func_idx, (uint32_t)arg_count, &pc);
  vm_loop(code, pc);

  if (code->obj.stack_size > stack_before)
    result = pop(code);
  return result;
}

struct SL_Variable sl_init_sl_parser(struct SL_Code *code_s) {
  if (code_s->types_set == 0)
    sl_identifier_tokenizer(code_s, code_s->code, &code_s->types,
                            &code_s->fixed_values, code_s->token_count);

  struct SL_Variable return_val = {0};

  for (int current_token = code_s->starting_token;
       current_token < code_s->token_count; current_token++) {

    if (code_s->custom_keyword_count > 0) {
      int found = 0;
      for (int i = 0; i < code_s->custom_keyword_count; i++) {
        if (strcmp(code_s->custom_keyword[i], code_s->code[current_token]) ==
            0) {
          code_s->custom_keywordr[i](code_s, &current_token);
          found = 1;
        }
      }
      if (found == 1)
        continue;
    }

    int end = 0;
    switch (code_s->types[current_token]) {

    case T_VAR: {
      for (int i = current_token; i < code_s->token_count; i++) {
        if (code_s->code[i][0] == '=') {
          i++;
          end = sl_find_end_of_expr(code_s, code_s->code, code_s->types, i,
                                    code_s->token_count);
          break;
        }
      }

      current_token++;
      variable_parser(code_s, code_s->types, code_s->code, &current_token, end,
                      0);
      if (current_assignment != NULL) {
        free(current_assignment);
        current_assignment = NULL;
      }
      if (end > 0)
        current_token = end - 1;
      else
        current_token--;
    } break;

    case T_IF: {
      current_token++;
      sl_if_parser(code_s, code_s->types, code_s->code, &current_token,
                   code_s->token_count, -1);
      current_token--;
    } break;

    case T_WHILE: {
      current_token++;
      sl_while_parser(code_s, code_s->types, code_s->code, &current_token,
                      code_s->token_count, -1);
      current_token--;
    } break;

    case T_CONTINUE: {
      if (global_loop_stack.depth <= 0) {
        sl_throw_an_error(*code_s, code_s->code, current_token,
                          code_s->token_count, "CONTINUE USAGE WITHOUT LOOP",
                          "Expected: define a loop first.");
      }

      SL_LoopContext *curr =
          &global_loop_stack.contexts[global_loop_stack.depth - 1];
      emit(code_s, OPCODE_JMP);
      emit(code_s, curr->start_pc);
    } break;

    case T_BREAK: {
      if (global_loop_stack.depth <= 0) {
        sl_throw_an_error(*code_s, code_s->code, current_token,
                          code_s->token_count, "BREAK USAGE WITHOUT LOOP",
                          "Expected: define a loop first.");
      }
      SL_LoopContext *curr =
          &global_loop_stack.contexts[global_loop_stack.depth - 1];

      if (curr->break_count >= curr->break_capacity) {
        curr->break_capacity *= 2;
        curr->break_patches = srealloc(NULL, curr->break_patches,
                                       curr->break_capacity * sizeof(size_t));
      }

      emit(code_s, OPCODE_JMP);
      curr->break_patches[curr->break_count++] = code_s->bytecode_len;
      emit(code_s, 0);
    } break;

    case T_ELIF:
    case T_ELSE:
    case T_END:
      break;

    case T_IMPORT: {
      sl_throw_an_error(
          *code_s, code_s->code, current_token, code_s->token_count,
          "IMPORT WAS NOT RESOLVED!",
          "Expected: tokens from sl_init_sl_lexer() or "
          "sl_dostr_sl_process(), imports are embedded while lexing.");
    } break;

    case T_DEF: {
      current_token++;
      struct SL_Function func =
          sl_define_parser(code_s, code_s->code, code_s->types, &current_token,
                           code_s->token_count);
      (void)func;
    } break;

    case T_RETURN: {
      current_token++;
      end = sl_find_end_of_expr(code_s, code_s->code, code_s->types,
                                current_token, code_s->token_count);
      sl_expression_solver(code_s, code_s->code, code_s->types, &current_token,
                           end);
      emit(code_s, OPCODE_RETURN);
      current_token = end - 1;
    } break;

    default: {
      if (code_s->code[current_token][0] == '$' &&
          code_s->code[current_token + 1] != NULL &&
          strcmp(code_s->code[current_token + 1], "=") == 0) {
        for (int i = current_token; i < code_s->token_count; i++) {
          if (code_s->code[i][0] == '=') {
            i++;
            end = sl_find_end_of_expr(code_s, code_s->code, code_s->types, i,
                                      code_s->token_count);
            break;
          }
        }
        assignment_parser(code_s, code_s->code, code_s->types, &current_token,
                          end, 0);
        current_token = end - 1;
      } else {
        end = sl_find_end_of_expr(code_s, code_s->code, code_s->types,
                                  current_token, code_s->token_count);
        sl_expression_solver(code_s, code_s->code, code_s->types,
                             &current_token, end);
        current_token = end - 1;
      }
    } break;
    }
  }

  return return_val;
}

struct SL_Code sl_init_sl_process() {
  struct SL_Code code;
  code.bytecode = scalloc(NULL, SL_INIT, sizeof(uint32_t));
  code.bytecode_capacity = SL_INIT;
  code.bytecode_len = 0;

  code.obj.stack_capacity = SL_INIT;
  code.obj.stack_size = 0;
  code.obj.stack =
      scalloc(NULL, code.obj.stack_capacity, sizeof(struct SL_Variable));

  code.obj.call_stack_capacity = SL_INIT;
  code.obj.call_stack_size = 0;
  code.obj.call_stack =
      scalloc(NULL, code.obj.call_stack_capacity, sizeof(size_t));

  code.frame.total_vars_capacity = SL_INIT;
  code.frame.total_vars = 0;
  code.frame.vars =
      scalloc(NULL, code.frame.total_vars_capacity, sizeof(struct SL_Variable));

  code.obj.call_frame_capacity = SL_INIT;
  code.obj.call_frame_size = 0;
  code.obj.call_frames =
      scalloc(NULL, code.obj.call_frame_capacity, sizeof(struct SL_CallFrame));
  code.current_frame_base = 0;

  code.funcs = scalloc(NULL, SL_INIT, sizeof(struct SL_Function));
  code.total_size_f = SL_INIT;
  code.vars = scalloc(NULL, SL_INIT, sizeof(struct SL_Variable));
  code.total_size_v = SL_INIT;
  code.total_funcs = 0;
  code.total_vars = 0;
  code.code = NULL;
  code.types = NULL;
  code.fixed_values = NULL;
  code.types_set = 0;
  code.custom_keyword = NULL;
  code.custom_keyword_count = 0;
  code.custom_expr = NULL;
  code.custom_expr_count = 0;
  code.custom_splitter = NULL;
  code.custom_splitter_count = 0;
  code.custom_splitter_capacity = 0;
  code.custom_expr_capacity = 0;
  code.custom_keyword_capacity = 0;
  code.custom_token = NULL;
  code.custom_token_precedence = NULL;
  code.custom_tokenr = NULL;
  code.custom_token_count = 0;
  code.custom_token_capacity = 0;
  code.scope_depth = 1;
  code.special_tokens = NULL;
  code.starting_token = 0;
  code.constant_pool = NULL;
  code.constant_pool_len = 0;
  code.constant_pool_capacity = 0;
  code.disassemble = 0;
  return code;
}

int sl_open_sl_process(struct SL_Code *code, const char *restrict file_name) {
  if (code->special_tokens == NULL)
    code->special_tokens = DEFAULT_SPECIAL_TOKENS;

  int count = sl_init_sl_lexer(code, SL_INIT, file_name, &code->code,
                               code->special_tokens);
  if (count < 0) {
    fprintf(stderr,
            "Invalid count\nsl_init_sl_lexer() returned a negative value\n");
    return -1;
  }

  code->types = scalloc(NULL, count, sizeof(enum TokenTypes));
  code->fixed_values = scalloc(NULL, count, sizeof(struct SL_Variable));
  code->types_set = 0;
  if (count <= 0) {
    fprintf(stderr, "sl_lexer failed\n");
    return -1;
  }
  code->token_count = count;

  struct SL_Variable init = sl_init_sl_parser(code);
  if (init.type == ERROR) {
    fprintf(stderr, "sl_parser failed \n");
    return -1;
  };
  return 0;
}

int sl_add_custom_expr(struct SL_Code *code, char *expr_start,
                       struct SL_Variable (*custom_exprr)(struct SL_Code *,
                                                          int *current_token)) {
  int count = code->custom_expr_count;

  if (count == 0) {
    code->custom_expr_capacity += 128;
    code->custom_exprr =
        smalloc(NULL, code->custom_expr_capacity * sizeof(custom_exprr));
    code->custom_expr =
        smalloc(NULL, code->custom_expr_capacity * sizeof(char *));
  }

  if (count >= code->custom_expr_capacity) {
    code->custom_expr_capacity += 128;
    code->custom_exprr =
        srealloc(NULL, code->custom_exprr,
                 code->custom_expr_capacity * sizeof(custom_exprr));
    code->custom_expr = srealloc(NULL, code->custom_expr,
                                 code->custom_expr_capacity * sizeof(char *));
  }
  code->custom_expr[count] = strdup(expr_start);
  code->custom_exprr[count] = custom_exprr;
  code->custom_expr_count++;
  return 1;
}

int sl_add_custom_keyword(struct SL_Code *code, char *keyword_name,
                          void (*custom_keywordr)(struct SL_Code *,
                                                  int *current_token)) {
  int count = code->custom_keyword_count;

  if (count == 0) {
    code->custom_keyword_capacity += 128;
    code->custom_keywordr =
        smalloc(NULL, code->custom_keyword_capacity * sizeof(custom_keywordr));
    code->custom_keyword =
        smalloc(NULL, code->custom_keyword_capacity * sizeof(char *));
  }

  if (count >= code->custom_keyword_capacity) {
    code->custom_keyword_capacity += 128;
    code->custom_keywordr =
        srealloc(NULL, code->custom_keywordr,
                 code->custom_keyword_capacity * sizeof(custom_keywordr));
    code->custom_keyword =
        srealloc(NULL, code->custom_keyword,
                 code->custom_keyword_capacity * sizeof(char *));
  }
  code->custom_keyword[count] = strdup(keyword_name);
  code->custom_keywordr[count] = custom_keywordr;
  code->custom_keyword_count++;
  return 1;
}

int sl_add_custom_splitter(struct SL_Code *code, char *splitter_name,
                           int (*custom_splitterr)(struct SL_Code *,
                                                   int *current_token)) {
  int count = code->custom_splitter_count;

  if (count == 0) {
    code->custom_splitter_capacity += 128;
    code->custom_splitterr = smalloc(NULL, code->custom_splitter_capacity *
                                               sizeof(custom_splitterr));
    code->custom_splitter =
        smalloc(NULL, code->custom_splitter_capacity * sizeof(char *));
  }

  if (count >= code->custom_splitter_capacity) {
    code->custom_splitter_capacity += 128;
    code->custom_splitterr =
        srealloc(NULL, code->custom_splitterr,
                 code->custom_splitter_capacity * sizeof(custom_splitterr));
    code->custom_splitter =
        srealloc(NULL, code->custom_splitter,
                 code->custom_splitter_capacity * sizeof(char *));
  }
  code->custom_splitter[count] = strdup(splitter_name);
  code->custom_splitterr[count] = custom_splitterr;
  code->custom_splitter_count++;
  return 1;
}

int sl_remove_custom_expr(struct SL_Code *code, int index) {
  if (code == NULL || code->custom_expr_count <= 0)
    return 0;

  if (index < 0 || index >= code->custom_expr_count)
    return 0;

  free(code->custom_expr[index]);
  code->custom_expr[index] = NULL;

  for (int i = index; i < code->custom_expr_count - 1; i++) {
    code->custom_expr[i] = code->custom_expr[i + 1];
    code->custom_exprr[i] = code->custom_exprr[i + 1];
  }

  code->custom_expr_count--;

  if (code->custom_expr_count == 0) {
    free(code->custom_expr);
    code->custom_expr = NULL;

    free(code->custom_exprr);
    code->custom_exprr = NULL;

    code->custom_expr_capacity = 0;
  }

  return 1;
}

int sl_remove_custom_keyword(struct SL_Code *code, int index) {
  if (code == NULL || code->custom_keyword_count <= 0)
    return 0;

  if (index < 0 || index >= code->custom_keyword_count)
    return 0;

  free(code->custom_keyword[index]);
  code->custom_keyword[index] = NULL;

  for (int i = index; i < code->custom_keyword_count - 1; i++) {
    code->custom_keyword[i] = code->custom_keyword[i + 1];
    code->custom_keywordr[i] = code->custom_keywordr[i + 1];
  }

  code->custom_keyword_count--;

  if (code->custom_keyword_count == 0) {
    free(code->custom_keyword);
    code->custom_keyword = NULL;

    free(code->custom_keywordr);
    code->custom_keywordr = NULL;

    code->custom_keyword_capacity = 0;
  }

  return 1;
}

int sl_remove_custom_splitter(struct SL_Code *code, int index) {
  if (code == NULL || code->custom_splitter_count <= 0)
    return 0;

  if (index < 0 || index >= code->custom_splitter_count)
    return 0;

  free(code->custom_splitter[index]);
  code->custom_splitter[index] = NULL;

  for (int i = index; i < code->custom_splitter_count - 1; i++) {
    code->custom_splitter[i] = code->custom_splitter[i + 1];
    code->custom_splitterr[i] = code->custom_splitterr[i + 1];
  }

  code->custom_splitter_count--;

  if (code->custom_splitter_count == 0) {
    free(code->custom_splitter);
    code->custom_splitter = NULL;

    free(code->custom_splitterr);
    code->custom_splitterr = NULL;

    code->custom_splitter_capacity = 0;
  }

  return 1;
}

int sl_add_custom_token(struct SL_Code *code, char *token_name, int precedence,
                        void (*custom_tokenr)(struct SL_Code *, int first_token,
                                              int operator_token,
                                              int end_token)) {
  if (code == NULL || token_name == NULL || token_name[0] == '\0')
    return 0;

  int index = sl_find_custom_token(code, token_name);
  if (index != -1) {
    code->custom_token_precedence[index] = precedence;
    code->custom_tokenr[index] = custom_tokenr;
    return 1;
  }

  if (code->custom_token_count >= code->custom_token_capacity) {
    code->custom_token_capacity += 128;
    code->custom_token = srealloc(NULL, code->custom_token,
                                  code->custom_token_capacity * sizeof(char *));
    code->custom_token_precedence =
        srealloc(NULL, code->custom_token_precedence,
                 code->custom_token_capacity * sizeof(int));
    code->custom_tokenr =
        srealloc(NULL, code->custom_tokenr,
                 code->custom_token_capacity * sizeof(custom_tokenr));
  }

  index = code->custom_token_count;
  code->custom_token[index] = strdup(token_name);
  code->custom_token_precedence[index] = precedence;
  code->custom_tokenr[index] = custom_tokenr;
  code->custom_token_count++;
  return 1;
}

int sl_remove_custom_token(struct SL_Code *code, int index) {
  if (code == NULL || code->custom_token_count <= 0)
    return 0;

  if (index < 0 || index >= code->custom_token_count)
    return 0;

  free(code->custom_token[index]);

  for (int i = index; i < code->custom_token_count - 1; i++) {
    code->custom_token[i] = code->custom_token[i + 1];
    code->custom_token_precedence[i] = code->custom_token_precedence[i + 1];
    code->custom_tokenr[i] = code->custom_tokenr[i + 1];
  }

  code->custom_token_count--;
  return 1;
}

struct SL_Variable sl_dostr_sl_process(struct SL_Code *code_s, char *code) {
  struct SL_Variable result = {0};
  if (code_s == NULL || code == NULL)
    return result;

  struct SL_Code code_p = sl_init_sl_process();
  code_p.special_tokens =
      code_s->special_tokens ? code_s->special_tokens : DEFAULT_SPECIAL_TOKENS;

  for (int i = 0; i < code_s->total_funcs; i++) {
    if (code_s->funcs[i].linked_function != 1)
      continue;
    if (code_p.total_funcs >= code_p.total_size_f) {
      code_p.total_size_f *= 2;
      code_p.funcs = srealloc(NULL, code_p.funcs,
                              code_p.total_size_f * sizeof(struct SL_Function));
    }
    code_p.funcs[code_p.total_funcs++] = sl_copy_function(code_s->funcs[i]);
  }

  for (int i = 0; i < code_s->total_vars; i++) {
    if (code_p.total_vars >= code_p.total_size_v) {
      code_p.total_size_v *= 2;
      code_p.vars = srealloc(NULL, code_p.vars,
                             code_p.total_size_v * sizeof(struct SL_Variable));
    }
    code_p.vars[code_p.total_vars++] = sl_copy_variable(code_s->vars[i]);
  }

  for (int i = 0; i < code_s->custom_keyword_count; i++)
    sl_add_custom_keyword(&code_p, code_s->custom_keyword[i],
                          code_s->custom_keywordr[i]);
  for (int i = 0; i < code_s->custom_expr_count; i++)
    sl_add_custom_expr(&code_p, code_s->custom_expr[i],
                       code_s->custom_exprr[i]);
  for (int i = 0; i < code_s->custom_splitter_count; i++)
    sl_add_custom_splitter(&code_p, code_s->custom_splitter[i],
                           code_s->custom_splitterr[i]);
  for (int i = 0; i < code_s->custom_token_count; i++)
    sl_add_custom_token(&code_p, code_s->custom_token[i],
                        code_s->custom_token_precedence[i],
                        code_s->custom_tokenr[i]);

  SL_LoopStack saved_loop_stack = global_loop_stack;
  char *saved_assignment = current_assignment;
  global_loop_stack = (SL_LoopStack){0};
  current_assignment = NULL;

  code_p.code = smalloc(&result, 1024 * sizeof(char *));
  if (result.type == ERROR) {
    fprintf(stderr, "error in sl_dostr_sl_process(): %s\n", strerror(errno));
    exit(EXIT_FAILURE); /* malloc failed */
  }
  lexing_code = &code_p;
  int count = sl_raw_lexer(code, &code_p.code, strlen(code),
                           code_p.special_tokens, 1024);

  struct SL_ImportRegistry registry = {0};
  count = sl_expand_imports(&code_p.code, count, SL_INIT, code_p.special_tokens,
                            &registry);
  sl_import_registry_free(&registry);
  lexing_code = NULL;

  if (count > 0) {
    code_p.token_count = count;
    code_p.types = scalloc(NULL, count, sizeof(enum TokenTypes));
    code_p.fixed_values = scalloc(NULL, count, sizeof(struct SL_Variable));
    code_p.types_set = 0;

    struct SL_Variable init = sl_init_sl_parser(&code_p);
    if (init.type == ERROR) {
      free(code_p.fixed_values);
      free(code_p.types);
      free(code_p.code);
      result.type = ERROR;
      result.vals =
          "sl_init_sl_parser() failed\nCalled by: sl_dostr_sl_process()\n";
      return result;
    }

    vm_loop(&code_p, 0);
    if (code_p.obj.stack_size > 0) {
      result = pop(&code_p);
      if (result.name != NULL) {
        free(result.name);
        result.name = NULL;
      }
    }
  } else {
    code_p.token_count = 0;
  }

  sl_close_sl_process(&code_p);

  global_loop_stack = saved_loop_stack;
  current_assignment = saved_assignment;

  return result;
}

int sl_close_sl_process(struct SL_Code *code) {
  if (code == NULL)
    return -1;

  if (code->funcs != NULL) {
    for (int i = 0; i < code->total_funcs; i++) {
      sl_free_function(&code->funcs[i]);
    }
    free(code->funcs);
    code->funcs = NULL;
  }

  if (code->constant_pool != NULL) {
    for (int i = 0; i < code->constant_pool_len; i++) {
      sl_free_variable(&code->constant_pool[i]);
    }
    free(code->constant_pool);
    code->constant_pool = NULL;
    code->constant_pool_len = 0;
  }

  if (code->bytecode != NULL) {
    free(code->bytecode);
    code->bytecode = NULL;
    code->bytecode_len = 0;
  }

  if (code->obj.stack != NULL) {
    for (size_t i = 0; i < code->obj.stack_size; i++)
      sl_free_variable(&code->obj.stack[i]);
    free(code->obj.stack);
    code->obj.stack = NULL;
    code->obj.stack_size = 0;
  }

  free(code->obj.call_stack);
  code->obj.call_stack = NULL;
  free(code->obj.call_frames);
  code->obj.call_frames = NULL;

  if (code->frame.vars != NULL) {
    for (int i = 0; i < code->frame.total_vars_capacity; i++)
      sl_free_variable(&code->frame.vars[i]);
    free(code->frame.vars);
    code->frame.vars = NULL;
    code->frame.total_vars = 0;
  }

  if (global_loop_stack.contexts != NULL) {
    for (int i = 0; i < global_loop_stack.depth; i++)
      free(global_loop_stack.contexts[i].break_patches);
    free(global_loop_stack.contexts);
    global_loop_stack.contexts = NULL;
    global_loop_stack.depth = 0;
    global_loop_stack.capacity = 0;
  }

  if (current_assignment != NULL) {
    free(current_assignment);
    current_assignment = NULL;
  }

  if (code->vars != NULL) {
    for (int i = 0; i < code->total_vars; i++) {
      sl_free_variable(&code->vars[i]);
    }
    free(code->vars);
    code->vars = NULL;
  }

  if (code->code != NULL) {
    for (int i = 0; i < code->token_count; i++) {
      if (code->code[i] != NULL) {
        free(code->code[i]);
      }
    }
    free(code->code);
    code->code = NULL;
  }

  if (code->types != NULL) {
    free(code->types);
    code->types = NULL;
  }

  if (code->fixed_values != NULL) {
    for (int i = 0; i < code->token_count; i++) {
      sl_free_variable(&code->fixed_values[i]);
    }
    free(code->fixed_values);
    code->fixed_values = NULL;
  }

  if (code->custom_keyword_count > 0) {
    for (int i = 0; i < code->custom_keyword_count; i++) {
      if (code->custom_keyword[i] != NULL) {
        free(code->custom_keyword[i]);
        code->custom_keyword[i] = NULL;
      }
    }
    free(code->custom_keyword);
    code->custom_keyword = NULL;

    free(code->custom_keywordr);
    code->custom_keywordr = NULL;
  }
  if (code->custom_expr_count > 0) {
    for (int i = 0; i < code->custom_expr_count; i++) {
      if (code->custom_expr[i] != NULL) {
        free(code->custom_expr[i]);
        code->custom_expr[i] = NULL;
      }
    }
    free(code->custom_expr);
    code->custom_expr = NULL;

    free(code->custom_exprr);
    code->custom_exprr = NULL;
  }

  if (code->custom_splitter_count > 0) {
    for (int i = 0; i < code->custom_splitter_count; i++) {
      free(code->custom_splitter[i]);
      code->custom_splitter[i] = NULL;
    }

    free(code->custom_splitter);
    code->custom_splitter = NULL;

    free(code->custom_splitterr);
    code->custom_splitterr = NULL;
  }

  if (code->custom_token_count > 0) {
    for (int i = 0; i < code->custom_token_count; i++) {
      free(code->custom_token[i]);
      code->custom_token[i] = NULL;
    }
    code->custom_token_count = 0;
  }
  free(code->custom_token);
  code->custom_token = NULL;
  free(code->custom_token_precedence);
  code->custom_token_precedence = NULL;
  free(code->custom_tokenr);
  code->custom_tokenr = NULL;
  code->custom_token_capacity = 0;

  if (g_scope_n == 0) {
    free(g_scopes);
    g_scopes = NULL;
    g_scope_cap = 0;
    g_scope_floor = 0;
  }

  return 0;
}

static void *sl_alloc_fail(struct SL_Variable *error, const char *msg,
                           size_t bytes) {
  if (error == NULL) {
    fprintf(stderr, "%s", msg);
    exit(EXIT_FAILURE);
  }
  error->vals = (char *)msg;
  error->type = ERROR;
  error->length = bytes;
  error->info = errno;
  return NULL;
}

void *smalloc(struct SL_Variable *error, size_t size) {
  if (size == 0) {
    return sl_alloc_fail(
        error, "smalloc(): invalid allocation size, size is 0\n", size);
  }
  void *ptr = malloc(size);
  if (ptr == NULL) {
    return sl_alloc_fail(error, "smalloc(): malloc() failed\n", size);
  }
  return ptr;
}

void *scalloc(struct SL_Variable *error, size_t how_much, size_t size) {
  if (how_much == 0 || size == 0) {
    return sl_alloc_fail(
        error, "scalloc(): invalid allocation size, size is 0\n", size);
  }
  void *ptr = calloc(how_much, size);
  if (ptr == NULL) {
    return sl_alloc_fail(error, "scalloc(): calloc() failed\n",
                         how_much * size);
  }
  return ptr;
}

void *srealloc(struct SL_Variable *error, void *pptr, size_t size) {
  if (size == 0) {
    return sl_alloc_fail(
        error, "srealloc(): invalid allocation size, size is 0\n", size);
  }
  void *ptr = realloc(pptr, size);
  if (ptr == NULL) {
    return sl_alloc_fail(error, "srealloc(): realloc() failed\n", size);
  }
  return ptr;
}

char *sstrdup(struct SL_Variable *error, const char *restrict str) {
  if (str == NULL) {
    return sl_alloc_fail(error, "sstrdup(): str is NULL\n", 0);
  }
  char *new_str = strdup(str);
  if (new_str == NULL) {
    return sl_alloc_fail(error, "sstrdup(): strdup() failed\n",
                         strlen(str) + 1);
  }
  return new_str;
}

void sl_run_process(struct SL_Code *code, char *file_name) {
  if (sl_open_sl_process(code, file_name) != 0) {
    fprintf(stderr, "sl_open_sl_process failed\n");
    return;
  }
  sl_run_vm(code);
  if (sl_close_sl_process(code) != 0) {
    fprintf(stderr, "sl_close_sl_process() failed\n");
    return;
  }
  return;
}

#define SL_X_MAGIC 0x534C580000000002ULL
#define SL_X_HEADER_FIXED_FIELDS 7
#define SL_X_WORD_SIZE 8
#define SL_X_CONSTANT_EXTRA_CAPACITY 64

struct SL_X_Buffer {
  unsigned char *data;
  size_t length;
  size_t capacity;
};

struct SL_X_Cursor {
  const unsigned char *data;
  size_t length;
  size_t position;
  int failed;
};

static void sl_x_buffer_append(struct SL_X_Buffer *buffer, const void *bytes,
                               size_t count) {
  if (buffer->length + count > buffer->capacity) {
    size_t new_capacity = buffer->capacity == 0 ? SL_INIT : buffer->capacity;
    while (new_capacity < buffer->length + count)
      new_capacity *= 2;
    buffer->data = srealloc(NULL, buffer->data, new_capacity);
    buffer->capacity = new_capacity;
  }
  if (count > 0)
    memcpy(buffer->data + buffer->length, bytes, count);
  buffer->length += count;
}

static void sl_x_put_u32(struct SL_X_Buffer *buffer, uint32_t value) {
  unsigned char bytes[4];
  for (int i = 0; i < 4; i++)
    bytes[i] = (unsigned char)(value >> (8 * i));
  sl_x_buffer_append(buffer, bytes, sizeof(bytes));
}

static void sl_x_put_u64(struct SL_X_Buffer *buffer, uint64_t value) {
  unsigned char bytes[8];
  for (int i = 0; i < 8; i++)
    bytes[i] = (unsigned char)(value >> (8 * i));
  sl_x_buffer_append(buffer, bytes, sizeof(bytes));
}

static void sl_x_put_signed(struct SL_X_Buffer *buffer, int64_t value) {
  sl_x_put_u64(buffer, (uint64_t)value);
}

static void sl_x_put_text(struct SL_X_Buffer *buffer, const char *text,
                          uint64_t length) {
  if (text == NULL) {
    sl_x_put_u64(buffer, 0);
    return;
  }
  sl_x_put_u64(buffer, length + 1);
  sl_x_buffer_append(buffer, text, (size_t)length);
}

static uint64_t sl_x_variable_payload(const struct SL_Variable *variable) {
  switch (variable->type) {
  case INTEGER:
    return (uint64_t)(int64_t)variable->vali;
  case DOUBLE: {
    uint64_t bits = 0;
    memcpy(&bits, &variable->valf, sizeof(double));
    return bits;
  }
  case CHAR:
    return (uint64_t)(unsigned char)variable->valc;
  case BOOLEAN:
    return (uint64_t)(int64_t)variable->valb;
  case LONG:
    return (uint64_t)(int64_t)variable->valh;
  default:
    return 0;
  }
}

static void sl_x_put_variable(struct SL_X_Buffer *buffer,
                              const struct SL_Variable *variable) {
  int has_text = (variable->type == STRING || variable->type == RETURN ||
                  variable->type == BYTES) &&
                 variable->vals != NULL;
  uint64_t text_length = 0;
  if (has_text)
    text_length = (variable->type == BYTES) ? (uint64_t)variable->length
                                            : (uint64_t)strlen(variable->vals);

  sl_x_put_signed(buffer, (int64_t)variable->type);
  sl_x_put_signed(buffer, (int64_t)variable->scope_lifetime);
  sl_x_put_signed(buffer, (int64_t)variable->cache_index);
  sl_x_put_signed(buffer, (int64_t)variable->info);
  sl_x_put_u64(buffer, (uint64_t)variable->length);
  sl_x_put_u64(buffer, sl_x_variable_payload(variable));
  sl_x_put_text(buffer, variable->name,
                variable->name != NULL ? (uint64_t)strlen(variable->name) : 0);
  sl_x_put_text(buffer, has_text ? variable->vals : NULL, text_length);
}

static void sl_x_put_function(struct SL_X_Buffer *buffer,
                              const struct SL_Function *function) {
  uint64_t argument_count =
      function->arguments != NULL ? (uint64_t)function->total_arguments : 0;

  sl_x_put_text(buffer, function->name,
                function->name != NULL ? (uint64_t)strlen(function->name) : 0);
  sl_x_put_signed(buffer, (int64_t)function->entry_pc);
  sl_x_put_signed(buffer, (int64_t)function->info);
  sl_x_put_signed(buffer, (int64_t)function->vaargs);
  sl_x_put_signed(buffer, (int64_t)function->linked_function);
  sl_x_put_signed(buffer, (int64_t)function->scope_lifetime);
  sl_x_put_signed(buffer, (int64_t)function->frame_size);
  sl_x_put_u64(buffer, argument_count);
  for (uint64_t i = 0; i < argument_count; i++)
    sl_x_put_variable(buffer, &function->arguments[i]);
}

static uint64_t sl_x_get_u64(struct SL_X_Cursor *cursor) {
  if (cursor->failed || cursor->length - cursor->position < 8) {
    cursor->failed = 1;
    return 0;
  }
  uint64_t value = 0;
  for (int i = 0; i < 8; i++)
    value |= (uint64_t)cursor->data[cursor->position + i] << (8 * i);
  cursor->position += 8;
  return value;
}

static int64_t sl_x_get_signed(struct SL_X_Cursor *cursor) {
  return (int64_t)sl_x_get_u64(cursor);
}

static char *sl_x_get_text(struct SL_X_Cursor *cursor, uint64_t *text_length) {
  uint64_t marker = sl_x_get_u64(cursor);
  if (text_length != NULL)
    *text_length = 0;
  if (cursor->failed || marker == 0)
    return NULL;

  uint64_t length = marker - 1;
  if (cursor->length - cursor->position < length) {
    cursor->failed = 1;
    return NULL;
  }

  char *text = smalloc(NULL, (size_t)length + 1);
  memcpy(text, cursor->data + cursor->position, (size_t)length);
  text[length] = '\0';
  cursor->position += (size_t)length;
  if (text_length != NULL)
    *text_length = length;
  return text;
}

static void sl_x_get_variable(struct SL_X_Cursor *cursor,
                              struct SL_Variable *variable) {
  memset(variable, 0, sizeof(*variable));
  variable->type = (enum SL_Types)sl_x_get_signed(cursor);
  variable->scope_lifetime = (int)sl_x_get_signed(cursor);
  variable->cache_index = (int)sl_x_get_signed(cursor);
  variable->info = (int)sl_x_get_signed(cursor);
  variable->length = (size_t)sl_x_get_u64(cursor);
  uint64_t payload = sl_x_get_u64(cursor);
  variable->name = sl_x_get_text(cursor, NULL);
  char *text = sl_x_get_text(cursor, NULL);

  switch (variable->type) {
  case INTEGER:
    variable->vali = (int)(int64_t)payload;
    break;
  case DOUBLE:
    memcpy(&variable->valf, &payload, sizeof(double));
    break;
  case CHAR:
    variable->valc = (char)payload;
    break;
  case BOOLEAN:
    variable->valb = (int)(int64_t)payload;
    break;
  case LONG:
    variable->valh = (intmax_t)(int64_t)payload;
    break;
  case STRING:
  case RETURN:
  case BYTES:
    variable->vals = text;
    text = NULL;
    break;
  default:
    break;
  }
  free(text);

  if (variable->name != NULL)
    variable->hash = sl_hash_string(variable->name);
  else if (variable->type == RETURN && variable->vals != NULL &&
           variable->vals[0] == '$')
    variable->hash = sl_hash_string(variable->vals + 1);
}

static void sl_x_get_function(struct SL_X_Cursor *cursor,
                              struct SL_Function *function) {
  memset(function, 0, sizeof(*function));
  function->name = sl_x_get_text(cursor, NULL);
  function->entry_pc = (int)sl_x_get_signed(cursor);
  function->info = (int)sl_x_get_signed(cursor);
  function->vaargs = (int)sl_x_get_signed(cursor);
  function->linked_function = (int)sl_x_get_signed(cursor);
  function->scope_lifetime = (int)sl_x_get_signed(cursor);
  function->frame_size = (int)sl_x_get_signed(cursor);
  uint64_t argument_count = sl_x_get_u64(cursor);

  if (function->name != NULL)
    function->hash = sl_hash_string(function->name);
  if (cursor->failed)
    return;
  if (argument_count > cursor->length / SL_X_WORD_SIZE) {
    cursor->failed = 1;
    return;
  }
  if (argument_count > 0) {
    function->arguments =
        scalloc(NULL, (size_t)argument_count, sizeof(struct SL_Variable));
    function->total_arguments = (int)argument_count;
    for (uint64_t i = 0; i < argument_count; i++)
      sl_x_get_variable(cursor, &function->arguments[i]);
  }
}

static void sl_x_free_file(struct SL_X_File *slx_file) {
  for (int i = 0; i < slx_file->total_vars; i++)
    sl_free_variable(&slx_file->vars[i]);
  for (int i = 0; i < slx_file->total_funcs; i++)
    sl_free_function(&slx_file->funcs[i]);
  for (int i = 0; i < slx_file->total_fixeds; i++)
    sl_free_variable(&slx_file->fixeds[i]);
  for (int i = 0; i < slx_file->total_constants; i++)
    sl_free_variable(&slx_file->constants[i]);
  free(slx_file->vars);
  free(slx_file->funcs);
  free(slx_file->fixeds);
  free(slx_file->constants);
  memset(slx_file, 0, sizeof(*slx_file));
}

static int sl_x_write(FILE *file, const void *data, size_t size) {
  return size == 0 || fwrite(data, 1, size, file) == size;
}

static int sl_x_flush_item(FILE *file, struct SL_X_Buffer *item,
                           uint64_t *item_size, uint64_t *section_size) {
  *item_size = item->length;
  *section_size += item->length;
  int written = sl_x_write(file, item->data, item->length);
  item->length = 0;
  return written;
}

int sl_save_slx(struct SL_Code *code, const char *file_name) {
  struct SL_X_File slx_file = {code->vars,
                               code->total_vars,
                               code->funcs,
                               code->total_funcs,
                               NULL,
                               0,
                               code->constant_pool,
                               code->constant_pool_len};
  struct SL_X_Header header = {0};
  struct SL_X_Buffer item = {0};
  struct SL_X_Buffer header_bytes = {0};
  int written = 1;

  FILE *file = fopen(file_name, "wb");
  if (file == NULL) {
    fprintf(stderr, "Failed to create file under the name \"%s\"\n", file_name);
    return -1;
  }

  header.magic = SL_X_MAGIC;
  header.total_vars = (uint64_t)slx_file.total_vars;
  header.total_funcs = (uint64_t)slx_file.total_funcs;
  header.total_fixeds = (uint64_t)slx_file.total_fixeds;
  header.total_constants = (uint64_t)slx_file.total_constants;
  header.var_sizes =
      scalloc(NULL, (size_t)header.total_vars + 1, sizeof(uint64_t));
  header.func_sizes =
      scalloc(NULL, (size_t)header.total_funcs + 1, sizeof(uint64_t));
  header.fixed_sizes =
      scalloc(NULL, (size_t)header.total_fixeds + 1, sizeof(uint64_t));
  header.constant_sizes =
      scalloc(NULL, (size_t)header.total_constants + 1, sizeof(uint64_t));

  for (size_t i = 0; i < code->bytecode_len; i++)
    sl_x_put_u32(&item, code->bytecode[i]);
  header.bytecode_size = item.length;
  written = sl_x_write(file, item.data, item.length);
  item.length = 0;

  for (int i = 0; written && i < slx_file.total_vars; i++) {
    sl_x_put_variable(&item, &slx_file.vars[i]);
    written =
        sl_x_flush_item(file, &item, &header.var_sizes[i], &header.file_size);
  }
  for (int i = 0; written && i < slx_file.total_funcs; i++) {
    sl_x_put_function(&item, &slx_file.funcs[i]);
    written =
        sl_x_flush_item(file, &item, &header.func_sizes[i], &header.file_size);
  }
  for (int i = 0; written && i < slx_file.total_fixeds; i++) {
    sl_x_put_variable(&item, &slx_file.fixeds[i]);
    written =
        sl_x_flush_item(file, &item, &header.fixed_sizes[i], &header.file_size);
  }
  for (int i = 0; written && i < slx_file.total_constants; i++) {
    sl_x_put_variable(&item, &slx_file.constants[i]);
    written = sl_x_flush_item(file, &item, &header.constant_sizes[i],
                              &header.file_size);
  }

  if (written) {
    sl_x_put_u64(&header_bytes, header.magic);
    sl_x_put_u64(&header_bytes, header.bytecode_size);
    sl_x_put_u64(&header_bytes, header.file_size);
    sl_x_put_u64(&header_bytes, header.total_vars);
    sl_x_put_u64(&header_bytes, header.total_funcs);
    sl_x_put_u64(&header_bytes, header.total_fixeds);
    sl_x_put_u64(&header_bytes, header.total_constants);
    for (uint64_t i = 0; i < header.total_vars; i++)
      sl_x_put_u64(&header_bytes, header.var_sizes[i]);
    for (uint64_t i = 0; i < header.total_funcs; i++)
      sl_x_put_u64(&header_bytes, header.func_sizes[i]);
    for (uint64_t i = 0; i < header.total_fixeds; i++)
      sl_x_put_u64(&header_bytes, header.fixed_sizes[i]);
    for (uint64_t i = 0; i < header.total_constants; i++)
      sl_x_put_u64(&header_bytes, header.constant_sizes[i]);
    header.header_size = header_bytes.length + SL_X_WORD_SIZE;
    sl_x_put_u64(&header_bytes, header.header_size);
    written = sl_x_write(file, header_bytes.data, header_bytes.length);
  }

  if (fclose(file) != 0)
    written = 0;
  if (!written) {
    fprintf(stderr, "Failed to write file under the name \"%s\"\n", file_name);
    remove(file_name);
  }

  free(header.var_sizes);
  free(header.func_sizes);
  free(header.fixed_sizes);
  free(header.constant_sizes);
  free(item.data);
  free(header_bytes.data);
  return written ? 0 : -1;
}

static int sl_x_read_at(FILE *file, uint64_t offset, unsigned char *target,
                        size_t size) {
  if (size == 0)
    return 1;
  if (fseek(file, (long)offset, SEEK_SET) != 0)
    return 0;
  return fread(target, 1, size, file) == size;
}

static int sl_x_read_item(FILE *file, uint64_t item_size,
                          unsigned char **scratch, size_t *scratch_capacity,
                          struct SL_X_Cursor *cursor) {
  if (item_size + 1 > *scratch_capacity) {
    *scratch_capacity = (size_t)item_size + 1;
    *scratch = srealloc(NULL, *scratch, *scratch_capacity);
  }
  if (item_size > 0 && fread(*scratch, 1, (size_t)item_size, file) != item_size)
    return 0;
  cursor->data = *scratch;
  cursor->length = (size_t)item_size;
  cursor->position = 0;
  cursor->failed = 0;
  return 1;
}

static int sl_x_item_is_valid(const struct SL_X_Cursor *cursor) {
  return !cursor->failed && cursor->position == cursor->length;
}

static void sl_x_report_corrupt_file(const char *file_name) {
  fprintf(stderr, "File \"%s\" is not a valid .slx file\n", file_name);
}

int sl_load_slx(struct SL_Code *code, const char *file_name) {
  struct SL_X_Header header = {0};
  struct SL_X_File slx_file = {0};
  struct SL_X_Cursor cursor = {0};
  unsigned char trailer[SL_X_WORD_SIZE];
  unsigned char *header_bytes = NULL;
  unsigned char *scratch = NULL;
  size_t scratch_capacity = 0;
  uint32_t *bytecode = NULL;
  size_t bytecode_len = 0;
  uint64_t total_size = 0;
  uint64_t counted_size = 0;
  uint64_t file_section_offset = 0;
  int vars_capacity = 0;
  int funcs_capacity = 0;
  int constants_capacity = 0;
  int status = -1;

  FILE *file = fopen(file_name, "rb");
  if (file == NULL) {
    fprintf(stderr, "Failed to open file under the name \"%s\"\n", file_name);
    return -1;
  }

  if (fseek(file, 0, SEEK_END) != 0)
    goto corrupt;
  long measured_size = ftell(file);
  if (measured_size < (long)(SL_X_HEADER_FIXED_FIELDS + 1) * SL_X_WORD_SIZE)
    goto corrupt;
  total_size = (uint64_t)measured_size;

  if (!sl_x_read_at(file, total_size - SL_X_WORD_SIZE, trailer,
                    sizeof(trailer)))
    goto corrupt;
  cursor = (struct SL_X_Cursor){trailer, sizeof(trailer), 0, 0};
  header.header_size = sl_x_get_u64(&cursor);
  if (header.header_size < (SL_X_HEADER_FIXED_FIELDS + 1) * SL_X_WORD_SIZE ||
      header.header_size > total_size)
    goto corrupt;

  header_bytes = smalloc(NULL, (size_t)header.header_size);
  if (!sl_x_read_at(file, total_size - header.header_size, header_bytes,
                    (size_t)header.header_size))
    goto corrupt;

  cursor = (struct SL_X_Cursor){header_bytes, (size_t)header.header_size, 0, 0};
  header.magic = sl_x_get_u64(&cursor);
  header.bytecode_size = sl_x_get_u64(&cursor);
  header.file_size = sl_x_get_u64(&cursor);
  header.total_vars = sl_x_get_u64(&cursor);
  header.total_funcs = sl_x_get_u64(&cursor);
  header.total_fixeds = sl_x_get_u64(&cursor);
  header.total_constants = sl_x_get_u64(&cursor);

  uint64_t max_items = header.header_size / SL_X_WORD_SIZE;
  if (header.magic != SL_X_MAGIC || header.bytecode_size > total_size ||
      header.file_size > total_size || header.total_vars > max_items ||
      header.total_funcs > max_items || header.total_fixeds > max_items ||
      header.total_constants > max_items || header.bytecode_size % 4 != 0)
    goto corrupt;
  if (header.bytecode_size + header.file_size + header.header_size !=
      total_size)
    goto corrupt;
  if ((SL_X_HEADER_FIXED_FIELDS + 1 + header.total_vars + header.total_funcs +
       header.total_fixeds + header.total_constants) *
          SL_X_WORD_SIZE !=
      header.header_size)
    goto corrupt;

  header.var_sizes =
      scalloc(NULL, (size_t)header.total_vars + 1, sizeof(uint64_t));
  header.func_sizes =
      scalloc(NULL, (size_t)header.total_funcs + 1, sizeof(uint64_t));
  header.fixed_sizes =
      scalloc(NULL, (size_t)header.total_fixeds + 1, sizeof(uint64_t));
  header.constant_sizes =
      scalloc(NULL, (size_t)header.total_constants + 1, sizeof(uint64_t));
  for (uint64_t i = 0; i < header.total_vars; i++)
    counted_size += header.var_sizes[i] = sl_x_get_u64(&cursor);
  for (uint64_t i = 0; i < header.total_funcs; i++)
    counted_size += header.func_sizes[i] = sl_x_get_u64(&cursor);
  for (uint64_t i = 0; i < header.total_fixeds; i++)
    counted_size += header.fixed_sizes[i] = sl_x_get_u64(&cursor);
  for (uint64_t i = 0; i < header.total_constants; i++)
    counted_size += header.constant_sizes[i] = sl_x_get_u64(&cursor);
  if (cursor.failed || counted_size != header.file_size)
    goto corrupt;

  vars_capacity = (int)header.total_vars + SL_INIT;
  funcs_capacity = (int)header.total_funcs + SL_INIT;
  constants_capacity =
      (int)header.total_constants + SL_X_CONSTANT_EXTRA_CAPACITY;
  slx_file.vars =
      scalloc(NULL, (size_t)vars_capacity, sizeof(struct SL_Variable));
  slx_file.funcs =
      scalloc(NULL, (size_t)funcs_capacity, sizeof(struct SL_Function));
  slx_file.fixeds = scalloc(NULL, (size_t)header.total_fixeds + 1,
                            sizeof(struct SL_Variable));
  slx_file.constants =
      scalloc(NULL, (size_t)constants_capacity, sizeof(struct SL_Variable));
  slx_file.total_vars = (int)header.total_vars;
  slx_file.total_funcs = (int)header.total_funcs;
  slx_file.total_fixeds = (int)header.total_fixeds;
  slx_file.total_constants = (int)header.total_constants;

  file_section_offset = total_size - header.header_size - header.file_size;
  if (fseek(file, (long)file_section_offset, SEEK_SET) != 0)
    goto corrupt;

  for (int i = 0; i < slx_file.total_vars; i++) {
    if (!sl_x_read_item(file, header.var_sizes[i], &scratch, &scratch_capacity,
                        &cursor))
      goto corrupt;
    sl_x_get_variable(&cursor, &slx_file.vars[i]);
    if (!sl_x_item_is_valid(&cursor))
      goto corrupt;
  }
  for (int i = 0; i < slx_file.total_funcs; i++) {
    if (!sl_x_read_item(file, header.func_sizes[i], &scratch, &scratch_capacity,
                        &cursor))
      goto corrupt;
    sl_x_get_function(&cursor, &slx_file.funcs[i]);
    if (!sl_x_item_is_valid(&cursor))
      goto corrupt;
  }
  for (int i = 0; i < slx_file.total_fixeds; i++) {
    if (!sl_x_read_item(file, header.fixed_sizes[i], &scratch,
                        &scratch_capacity, &cursor))
      goto corrupt;
    sl_x_get_variable(&cursor, &slx_file.fixeds[i]);
    if (!sl_x_item_is_valid(&cursor))
      goto corrupt;
  }
  for (int i = 0; i < slx_file.total_constants; i++) {
    if (!sl_x_read_item(file, header.constant_sizes[i], &scratch,
                        &scratch_capacity, &cursor))
      goto corrupt;
    sl_x_get_variable(&cursor, &slx_file.constants[i]);
    if (!sl_x_item_is_valid(&cursor))
      goto corrupt;
  }

  bytecode_len = (size_t)(header.bytecode_size / 4);
  bytecode =
      smalloc(NULL, (bytecode_len + SL_REALLOC_DEFAULT) * sizeof(uint32_t));
  if (header.bytecode_size + 1 > scratch_capacity) {
    scratch_capacity = (size_t)header.bytecode_size + 1;
    scratch = srealloc(NULL, scratch, scratch_capacity);
  }
  if (!sl_x_read_at(file, 0, scratch, (size_t)header.bytecode_size))
    goto corrupt;
  for (size_t i = 0; i < bytecode_len; i++) {
    uint32_t instruction = 0;
    for (int byte = 0; byte < 4; byte++)
      instruction |= (uint32_t)scratch[i * 4 + byte] << (8 * byte);
    bytecode[i] = instruction;
  }

  for (int i = 0; i < slx_file.total_funcs; i++) {
    if (slx_file.funcs[i].linked_function != 1)
      continue;
    struct SL_Function *native = slx_file.funcs[i].name != NULL
                                     ? sl_get_func(code, slx_file.funcs[i].name)
                                     : NULL;
    if (native == NULL || native->linked_function != 1) {
      fprintf(stderr,
              "Native function \"%s\" used by \"%s\" is not registered\n",
              slx_file.funcs[i].name != NULL ? slx_file.funcs[i].name : "?",
              file_name);
      goto failed;
    }
  }

  free(code->bytecode);
  code->bytecode = bytecode;
  code->bytecode_len = bytecode_len;
  code->bytecode_capacity = bytecode_len + SL_REALLOC_DEFAULT;
  bytecode = NULL;

  for (int i = 0; i < code->constant_pool_len; i++)
    sl_free_variable(&code->constant_pool[i]);
  free(code->constant_pool);
  code->constant_pool = slx_file.constants;
  code->constant_pool_len = slx_file.total_constants;
  code->constant_pool_capacity = constants_capacity;

  if (code->fixed_values != NULL) {
    for (int i = 0; i < code->token_count; i++)
      sl_free_variable(&code->fixed_values[i]);
    free(code->fixed_values);
  }
  if (code->code != NULL) {
    for (int i = 0; i < code->token_count; i++)
      free(code->code[i]);
    free(code->code);
    code->code = NULL;
  }
  free(code->types);
  code->types = NULL;
  code->fixed_values = slx_file.fixeds;
  code->token_count = slx_file.total_fixeds;
  code->types_set = 1;

  struct SL_Variable *host_vars = code->vars;
  int host_total_vars = code->total_vars;
  code->vars = slx_file.vars;
  code->total_vars = slx_file.total_vars;
  code->total_size_v = vars_capacity;
  for (int i = 0; i < host_total_vars; i++) {
    if (host_vars[i].name != NULL)
      sl_add_var(code, host_vars[i]);
    sl_free_variable(&host_vars[i]);
  }
  free(host_vars);

  struct SL_Function *host_funcs = code->funcs;
  int host_total_funcs = code->total_funcs;
  code->funcs = slx_file.funcs;
  code->total_funcs = slx_file.total_funcs;
  code->total_size_f = funcs_capacity;
  for (int i = 0; i < host_total_funcs; i++) {
    sl_add_raw_func(code, &host_funcs[i]);
    sl_free_function(&host_funcs[i]);
  }
  free(host_funcs);

  memset(&slx_file, 0, sizeof(slx_file));
  status = 0;
  goto done;

corrupt:
  sl_x_report_corrupt_file(file_name);
failed:
  sl_x_free_file(&slx_file);
  free(bytecode);
done:
  free(header.var_sizes);
  free(header.func_sizes);
  free(header.fixed_sizes);
  free(header.constant_sizes);
  free(header_bytes);
  free(scratch);
  fclose(file);
  return status;
}
