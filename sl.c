#include "sl.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct SL_Variable expression_parser_solver(struct SL_Code *code_s,
                                            char *expression[],
                                            enum TokenTypes *types,
                                            int *current_token, int max_tokens,
                                            int current_line);

void identifier_tokenizer(char **code, enum TokenTypes **types,
                          struct SL_Variable **fixed_values, int token_count);

struct SL_Variable sl_copy_variable(struct SL_Variable var) {
  struct SL_Variable copy = var;

  if (var.name != NULL) {
    copy.name = strdup(var.name);
  }

  if ((var.type == STRING || var.type == RETURN) && var.vals != NULL) {
    copy.vals = strdup(var.vals);
  }

  return copy;
}

struct SL_Function sl_copy_function(struct SL_Function function) {
  struct SL_Function copy = function;

  if (function.name != NULL) {
    copy.name = strdup(function.name);
  }

  if (function.arguments != NULL) {
    if (function.total_arguments > 0) {
      copy.arguments =
          malloc(function.total_arguments * sizeof(*copy.arguments));
      for (int i = 0; i < function.total_arguments; i++) {
        copy.arguments[i] = sl_copy_variable(function.arguments[i]);
      }
    } else {
      copy.arguments = NULL;
    }
  } else {
    copy.arguments = NULL;
  }

  if (function.code_tokens != NULL) {
    if (function.code_len > 0) {
      copy.code_tokens = malloc(function.code_len * sizeof(*copy.code_tokens));
      for (int i = 0; i < function.code_len; i++) {
        copy.code_tokens[i] = strdup(function.code_tokens[i]);
      }
    } else {
      copy.code_tokens = NULL;
    }
  } else {
    copy.code_tokens = NULL;
  }

  if (function.fixed_values != NULL) {
    if (function.code_len > 0) {
      copy.fixed_values =
          malloc(function.code_len * sizeof(*copy.fixed_values));
      for (int i = 0; i < function.code_len; i++) {
        copy.fixed_values[i] = sl_copy_variable(function.fixed_values[i]);
      }
    } else {
      copy.fixed_values = NULL;
    }
  } else {
    copy.fixed_values = NULL;
  }

  if (function.types != NULL) {
    if (function.code_len > 0) {
      copy.types = malloc(function.code_len * sizeof(*copy.types));
      for (int i = 0; i < function.code_len; i++) {
        copy.types[i] = function.types[i];
      }
    } else {
      copy.types = NULL;
    }
  } else {
    copy.types = NULL;
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

  size_t size = 3;

  for (size_t i = 0; str[i] != '\0'; i++) {
    if (str[i] == '"' || str[i] == '\\')
      size++;
    size++;
  }

  char *result = malloc(size);
  if (result == NULL)
    return NULL;

  size_t j = 0;
  result[j++] = '"';

  for (size_t i = 0; str[i] != '\0'; i++) {
    if (str[i] == '"' || str[i] == '\\')
      result[j++] = '\\';

    result[j++] = str[i];
  }

  result[j++] = '"';
  result[j] = '\0';

  return result;
}

int LEXER(char *bufin, char ***bufout, int max_count, char *special_tokens,
          int start_size) {
  int size_s = start_size;
  if (bufin == NULL)
    return 0;
  int token_count = 0;
  const char *p = bufin;
  while (*p != '\0') {
    if (token_count >= size_s) {
      size_s *= 2;
      char **tmp = realloc(*bufout, size_s * sizeof(char *));
      if (tmp == NULL)
        return -1;
      (*bufout) = tmp;
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
      char *in_string_tokens = malloc(stringlen + 1);
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
      char *in_string_tokens = malloc(stringlen + 1);

      if (in_string_tokens == NULL)
        return -1;

      memcpy(in_string_tokens, string_start, stringlen);
      in_string_tokens[stringlen] = '\0';

      (*bufout)[token_count++] = in_string_tokens;

    } else {
      if ((*p == '>' && *(p + 1) == '>') || (*p == '<' && *(p + 1) == '<')) {
        char *pot = malloc(3);
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

        char *word = malloc(word_len + 1);
        memcpy(word, word_start, word_len);
        word[word_len] = '\0';

        (*bufout)[token_count++] = word;
        continue;
      }
      if (lexer_special_tokens_ex(special_tokens, *p) > 0) {
        char *pot = malloc(2);
        pot[0] = *p;
        pot[1] = '\0';
        (*bufout)[token_count++] = pot;
        p++;
        continue;
      }
      const char *word_start = p;
      while (*p && !isspace(*p) &&
             lexer_special_tokens_ex(special_tokens, *p) == 0)
        p++;
      int word_len = p - word_start;
      char *word = malloc(word_len + 1);
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

int sl_init_sl_lexer(int malloc_size, char *file_name, char ***bufout,
                     char *special_tokens) {
  FILE *code_file = fopen(file_name, "r");
  if (code_file == NULL) {
    perror("init_sl_lexer failed with error:");
    return -1;
  }

  char buf[4096];
  char *code_string;

  int total_allocations = 0;

  total_allocations += malloc_size;
  code_string = malloc(total_allocations);
  code_string[0] = '\0';

  int enable_endlinemodifier = 0;
  int end_line_modifier_sit = 0;

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

    int index = strlen(buf);

    if (character_pos != NULL)
      index = character_pos - buf;

    int len = strlen(code_string);
    if (len + index + 2 > total_allocations) {
      total_allocations += malloc_size;
      code_string = realloc(code_string, total_allocations);
    }

    memcpy(code_string + len, buf, index);
    code_string[len + index] = ' ';
    code_string[len + index + 1] = '\0';
  }

  char **code_array = malloc(1024 * sizeof(char *));
  int count = LEXER(code_string, &code_array, strlen(code_string),
                    special_tokens, 1024);

  free(code_string);
  *bufout = code_array;

  fclose(code_file);
  return count;
}

struct SL_Variable getvar_from_sl(struct SL_Code code, const char *name) {
  for (int i = 0; i < code.total_vars; i++) {
    if (strcmp(code.vars[i].name, name) == 0) {
      return code.vars[i];
    }
  }
  struct SL_Variable empty = {0};
  return empty;
}

int check_number(const char *s) {
  char *endptr;

  long val_int = strtol(s, &endptr, 10);
  if (*endptr == '\0') {
    return 1;
  }
  double val_double = strtod(s, &endptr);
  if (*endptr == '\0') {
    return 2;
  }
  return 0;
}

int string_checker(char *word) {
  if (word == NULL)
    return 0;

  int size = strlen(word);

  if (size >= 6 && word[0] == '"' && word[1] == '"' && word[2] == '"' &&
      word[size - 1] == '"' && word[size - 2] == '"' && word[size - 3] == '"') {
    return 3;
  }

  if (size >= 2 && word[0] == '"' && word[size - 1] == '"') {
    return 1;
  }

  return 0;
}

char *sl_string_getter(char *word) {
  if (word == NULL)
    return NULL;

  int quote_len = string_checker(word);

  if (quote_len == 0) {
    return strdup(word);
  }

  int size = strlen(word);
  char *our_word = malloc(size);
  if (our_word == NULL)
    return NULL;

  int j = 0;

  for (int i = quote_len; i < size - quote_len; i++) {
    if (word[i] == '\\' && (i + 1) < (size - quote_len)) {
      i++;
      switch (word[i]) {
      case 'n':
        our_word[j++] = '\n';
        break;
      case 'b':
        our_word[j++] = '\b';
        break;
      case 'e':
        our_word[j++] = '\e';
        break;
      case 'a':
        our_word[j++] = '\a';
        break;
      case 'f':
        our_word[j++] = '\f';
        break;
      case 'r':
        our_word[j++] = '\r';
        break;
      case 't':
        our_word[j++] = '\t';
        break;
      case 'v':
        our_word[j++] = '\v';
        break;
      case '0':
        our_word[j++] = '\0';
        break;
      case '"':
        our_word[j++] = '\"';
        break;
      case '\\':
        our_word[j++] = '\\';
        break;
      default:
        our_word[j++] = word[i];
        break;
      }
    } else {
      our_word[j++] = word[i];
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
             word[0] == '0' &&
                 (word[1] == 'x' || word[1] == 'b' || word[1] == 'o')) {
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
  case INTEGER:
    v.vali = atoi(word);
    break;
  case DOUBLE:
    v.valf = strtod(word, NULL);
    break;
  case STRING:
    v.vals = strdup(word);
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
    int base = 0;
    if (word[0] == '0') {
      switch (word[1]) {
      case 'x':
        base = 16;
        break;
      case 'b':
        base = 2;
        break;
      case 'o':
        base = 8;
        break;
      default:
        base = 10;
        break;
      }
    }
    if (base == 2 || base == 8)
      v.valh = strtol(word + 2, NULL, base);
    else
      v.valh = strtol(word, NULL, base);
  } break;
  default:
    break;
  }
  return v;
}

int getvar_index_from_sl(struct SL_Code code, const char *name,
                         unsigned long hash) {
  for (int i = code.total_vars - 1; i >= 0; i--) {
    if (code.vars[i].name == NULL)
      continue;
    if (code.vars[i].hash == hash) {
      if (strcmp(code.vars[i].name, name) == 0) {
        return i;
      }
    }
  }
  return -1;
}

int is_has_func(struct SL_Code code, const char *name) {
  unsigned long target_hash = sl_hash_string(name);
  for (int i = 0; i < code.total_funcs; i++) {
    if (code.funcs[i].hash == target_hash) {
      if (strcmp(code.funcs[i].name, name) == 0) {
        return i;
      }
    }
  }
  return -1;
}

void sl_free_variable(struct SL_Variable *var) {
  if (var == NULL) {
    return;
  }

  if (var->name != NULL) {
    free(var->name);
    var->name = NULL;
  }

  if ((var->type == STRING || var->type == RETURN) && var->vals != NULL) {
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

  if (func->code_tokens != NULL) {
    for (int i = 0; i < func->code_len; i++) {
      if (func->code_tokens[i] != NULL) {
        free(func->code_tokens[i]);
      }
    }
    free(func->code_tokens);
    func->code_tokens = NULL;
  }

  if (func->types != NULL) {
    free(func->types);
    func->types = NULL;
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

    struct SL_Function *tmp =
        realloc(code->funcs, code->total_size_f * sizeof(*code->funcs));

    if (!tmp)
      return -1;

    code->funcs = tmp;
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
    code->funcs =
        realloc(code->funcs, code->total_size_f * sizeof(struct SL_Function));
    if (!code->funcs)
      return -1;
  }

  code->funcs[code->total_funcs].name = strdup(name);
  code->funcs[code->total_funcs].hash = sl_hash_string(name);
  code->funcs[code->total_funcs].linked_function = 1;
  code->funcs[code->total_funcs].scope_lifetime = sl_get_scope(code);
  code->funcs[code->total_funcs++].funcr = funcr;
  return 0;
}

struct SL_Variable sl_get_argument(struct SL_Code code,
                                   struct SL_L_Function func, int which_one) {
  struct SL_Variable error_val = {0};
  error_val.type = ERROR;
  if (which_one >= func.total_arguments)
    return error_val;
  return code.vars[func.starting_index + which_one];
}

int sl_add_var(struct SL_Code *code, struct SL_Variable var) {
  if (code->total_vars >= code->total_size_v) {
    code->total_size_v =
        (code->total_size_v == 0) ? SL_INIT : code->total_size_v * 2;
    code->vars =
        realloc(code->vars, code->total_size_v * sizeof(struct SL_Variable));
    if (!code->vars)
      return -1;
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

struct SL_Variable expression_solver(struct SL_Variable left_side, char op,
                                     struct SL_Variable right_side,
                                     int current_line, enum TokenTypes op_type,
                                     int is_op_type) {
  if ((left_side.type == DOUBLE && right_side.type == INTEGER) ||
      (left_side.type == INTEGER && right_side.type == DOUBLE)) {
    left_side.type = DOUBLE;
    right_side.type = DOUBLE;
    right_side.valf = right_side.vali;
  }
  struct SL_Variable error = {0};
  error.type = ERROR;

  if (left_side.type != right_side.type) {
    return error;
  }
  struct SL_Variable expression_result = {0};
  expression_result.type = left_side.type;

  if (is_op_type != 1) {
    switch (op) {
    case '+':
      switch (left_side.type) {
      case INTEGER:
        expression_result.vali = left_side.vali + right_side.vali;
        break;
      case DOUBLE:
        expression_result.valf = left_side.valf + right_side.valf;
        break;
      case STRING: {
        char *left_string = sl_string_getter(left_side.vals);
        char *right_string = sl_string_getter(right_side.vals);

        if (left_string == NULL || right_string == NULL) {
          free(left_string);
          free(right_string);
          free(left_side.vals);
          free(right_side.vals);

          expression_result.type = ERROR;
          return expression_result;
        }

        size_t joined_len = strlen(left_string) + strlen(right_string);
        char *joined_string = malloc(joined_len + 1);

        if (joined_string == NULL) {
          free(left_string);
          free(right_string);
          free(left_side.vals);
          free(right_side.vals);

          expression_result.type = ERROR;
          return expression_result;
        }

        memcpy(joined_string, left_string, strlen(left_string));
        memcpy(joined_string + strlen(left_string), right_string,
               strlen(right_string));

        joined_string[joined_len] = '\0';
        expression_result.vals = sl_quote_string(joined_string);

        free(joined_string);
        free(left_string);
        free(right_string);
        free(left_side.vals);
        free(right_side.vals);

        if (expression_result.vals == NULL) {
          expression_result.type = ERROR;
          return expression_result;
        }
      } break;
      case BOOLEAN:
        expression_result.valb = left_side.valb + right_side.valb;
        break;
      case CHAR:
        fprintf(stderr, "Chars cannot add each other.");
        return error;
        break;
      case LONG:
        expression_result.valh = left_side.valh + right_side.valh;
      default:
        break;
      }
      break;
    case '-':
      switch (left_side.type) {
      case INTEGER:
        expression_result.vali = left_side.vali - right_side.vali;
        break;
      case DOUBLE:
        expression_result.valf = left_side.valf - right_side.valf;
        break;
      case STRING:
        fprintf(stderr, "Strings cannot sub each other.");
        return error;
        break;
      case BOOLEAN:
        expression_result.valb = left_side.valb - right_side.valb;
        break;
      case CHAR:
        fprintf(stderr, "Chars cannot sub each other.");
        return error;
        break;
      case LONG:
        expression_result.valh = left_side.valh - right_side.valh;
      default:
        break;
      }
      break;
    case '*':
      switch (left_side.type) {
      case INTEGER:
        expression_result.vali = left_side.vali * right_side.vali;
        break;
      case DOUBLE:
        expression_result.valf = left_side.valf * right_side.valf;
        break;
      case STRING:
        fprintf(stderr, "Strings cannot mul each other.");
        return error;
        break;
      case BOOLEAN:
        expression_result.valb = left_side.valb * right_side.valb;
        break;
      case CHAR:
        fprintf(stderr, "Chars cannot mul each other.");
        return error;
        break;
      case LONG:
        expression_result.valh = left_side.valh * right_side.valh;
      default:
        break;
      }
      break;
    case '/':
      switch (left_side.type) {
      case INTEGER:
        expression_result.vali = left_side.vali / right_side.vali;
        break;
      case DOUBLE:
        expression_result.valf = left_side.valf / right_side.valf;
        break;
      case STRING:
        fprintf(stderr, "Strings cannot div each other.");
        return error;
        break;
      case BOOLEAN:
        expression_result.valb = left_side.valb / right_side.valb;
        break;
      case CHAR:
        fprintf(stderr, "Chars cannot div each other.");
        return error;
        break;
      case LONG:
        expression_result.valh = left_side.valh / right_side.valh;
      default:
        break;
      }
      break;
    case '&':
      switch (left_side.type) {
      case INTEGER:
        expression_result.vali = left_side.vali & right_side.vali;
        break;
      case DOUBLE:
        fprintf(stderr, "Doubles cannot and each other.");
        return error;
        break;
      case STRING:
        fprintf(stderr, "Strings cannot and each other.");
        return error;
        break;
      case BOOLEAN:
        fprintf(stderr, "Booleans cannot and each other.");
        return error;
        break;
      case CHAR:
        fprintf(stderr, "Chars cannot and each other.");
        return error;
        break;
      case LONG:
        expression_result.valh = left_side.valh & right_side.valh;
      default:
        break;
      }
      break;
    case '|':
      switch (left_side.type) {
      case INTEGER:
        expression_result.vali = left_side.vali | right_side.vali;
        break;
      case DOUBLE:
        fprintf(stderr, "Doubles cannot or each other.");
        return error;
        break;
      case STRING:
        fprintf(stderr, "Strings cannot or each other.");
        return error;
        break;
      case BOOLEAN:
        fprintf(stderr, "Booleans cannot or each other.");
        return error;
        break;
      case CHAR:
        fprintf(stderr, "Chars cannot or each other.");
        return error;
        break;
      case LONG:
        expression_result.valh = left_side.valh | right_side.valh;
      default:
        break;
      }
      break;
    case '^':
      switch (left_side.type) {
      case INTEGER:
        expression_result.vali = left_side.vali ^ right_side.vali;
        break;
      case DOUBLE:
        fprintf(stderr, "Doubles cannot xor each other.");
        return error;
        break;
      case STRING:
        fprintf(stderr, "Strings cannot xor each other.");
        return error;
        break;
      case BOOLEAN:
        fprintf(stderr, "Booleans cannot xor each other.");
        return error;
        break;
      case CHAR:
        fprintf(stderr, "Chars cannot xor each other.");
        return error;
        break;
      case LONG:
        expression_result.valh = left_side.valh ^ right_side.valh;
      default:
        break;
      }
      break;
    case '%':
      switch (left_side.type) {
      case INTEGER:
        expression_result.vali = left_side.vali % right_side.vali;
        break;
      case DOUBLE:
        fprintf(stderr, "Doubles cannot mod each other.");
        return error;
        break;
      case STRING:
        fprintf(stderr, "Strings cannot mod each other.");
        return error;
        break;
      case BOOLEAN:
        fprintf(stderr, "Booleans cannot mod each other.");
        return error;
        break;
      case CHAR:
        fprintf(stderr, "Chars cannot mod each other.");
        return error;
        break;
      case LONG:
        expression_result.valh = left_side.valh % right_side.valh;
      default:
        break;
      }
      break;
    case '>':
      expression_result.type = BOOLEAN;
      switch (left_side.type) {
      case INTEGER:
        expression_result.valb = left_side.vali > right_side.vali;
        break;
      case DOUBLE:
        expression_result.valb = left_side.valf > right_side.valf;
        break;
      case STRING:
        fprintf(stderr, "Strings cannot bigger than each other.");
        return error;
        break;
      case BOOLEAN:
        fprintf(stderr, "Booleans cannot bigger than each other.");
        return error;
        break;
      case CHAR:
        fprintf(stderr, "Chars cannot bigger than each other.");
        return error;
        break;
      case LONG:
        expression_result.valb = left_side.valh > right_side.valh;
      default:
        break;
      }
      break;
    case '<':
      expression_result.type = BOOLEAN;
      switch (left_side.type) {
      case INTEGER:
        expression_result.valb = left_side.vali < right_side.vali;
        break;
      case DOUBLE:
        expression_result.valb = left_side.valf < right_side.valf;
        break;
      case STRING:
        fprintf(stderr, "Strings cannot less than each other.");
        return error;
        break;
      case BOOLEAN:
        fprintf(stderr, "Booleans cannot less than each other.");
        return error;
        break;
      case CHAR:
        fprintf(stderr, "Chars cannot less than each other.");
        return error;
        break;
      case LONG:
        expression_result.valb = left_side.valh < right_side.valh;
      default:
        break;
      }
      break;
    }
  } else {
    switch (op_type) {
    case T_SHRIGHT:
      switch (left_side.type) {
      case INTEGER:
        expression_result.vali = left_side.vali >> right_side.vali;
        break;
      case DOUBLE:
        fprintf(stderr, "Doubles cannot shift right each other.");
        return error;
        break;
      case STRING:
        fprintf(stderr, "Strings cannot shift right each other.");
        return error;
        break;
      case BOOLEAN:
        fprintf(stderr, "Booleans cannot shift right each other.");
        return error;
        break;
      case CHAR:
        fprintf(stderr, "Chars cannot shift right each other.");
        return error;
        break;
      case LONG:
        expression_result.valh = left_side.valh >> right_side.valh;
      default:
        break;
      }
      break;
    case T_SHLEFT:
      switch (left_side.type) {
      case INTEGER:
        expression_result.vali = left_side.vali << right_side.vali;
        break;
      case DOUBLE:
        fprintf(stderr, "Doubles cannot shift left each other.");
        return error;
        break;
      case STRING:
        fprintf(stderr, "Strings cannot shift left each other.");
        return error;
        break;
      case BOOLEAN:
        fprintf(stderr, "Booleans cannot shift left each other.");
        return error;

        break;
      case CHAR:
        fprintf(stderr, "Chars cannot shift left each other.");
        return error;

        break;
      case LONG:
        expression_result.valh = left_side.valh << right_side.valh;
      default:
        break;
      }
      break;
    case T_AND:
      expression_result.type = BOOLEAN;
      switch (left_side.type) {
      case INTEGER:
        expression_result.valb = left_side.vali && right_side.vali;
        break;
      case DOUBLE:
        expression_result.valb = left_side.valf && right_side.valf;
        break;
      case STRING:
        fprintf(stderr, "Strings cannot conditional and each other.");
        return error;

        break;
      case BOOLEAN:
        expression_result.valb = left_side.valb && right_side.valb;
        break;
      case CHAR:
        expression_result.valb = left_side.valc && right_side.valc;
        break;
      case LONG:
        expression_result.valb = left_side.valh && right_side.valh;
      default:
        break;
      }
      break;
    case T_OR:
      expression_result.type = BOOLEAN;
      switch (left_side.type) {
      case INTEGER:
        expression_result.valb = left_side.vali || right_side.vali;
        break;
      case DOUBLE:
        expression_result.valb = left_side.valf || right_side.valf;
        break;
      case STRING:
        fprintf(stderr, "Strings cannot conditional or each other.");
        return error;

        break;
      case BOOLEAN:
        expression_result.valb = left_side.valb || right_side.valb;
        break;
      case CHAR:
        expression_result.valb = left_side.valc || right_side.valc;
        break;
      case LONG:
        expression_result.valb = left_side.valh || right_side.valh;
      default:
        break;
      }
      break;
    case T_EQU:
      expression_result.type = BOOLEAN;
      switch (left_side.type) {
      case INTEGER:
        expression_result.valb = left_side.vali == right_side.vali;
        break;
      case DOUBLE:
        expression_result.valb = left_side.valf == right_side.valf;
        break;
      case STRING: {
        char *left_string = sl_string_getter(left_side.vals);
        char *right_string = sl_string_getter(right_side.vals);

        if (strcmp(left_string, right_string) == 0)
          expression_result.valb = 1;
        else
          expression_result.valb = 0;

        free(left_string);
        free(right_string);
        free(left_side.vals);
        free(right_side.vals);
      } break;
      case BOOLEAN:
        expression_result.valb = left_side.valb == right_side.valb;
        break;
      case CHAR:
        expression_result.valb = left_side.valc == right_side.valc;
        break;
      case LONG:
        expression_result.valb = left_side.valh == right_side.valh;
      default:
        break;
      }
      break;
    case T_NEQ:
      expression_result.type = BOOLEAN;
      switch (left_side.type) {
      case INTEGER:
        expression_result.valb = left_side.vali != right_side.vali;
        break;
      case DOUBLE:
        expression_result.valb = left_side.valf != right_side.valf;
        break;
      case STRING: {
        char *left_string = sl_string_getter(left_side.vals);
        char *right_string = sl_string_getter(right_side.vals);

        if (strcmp(left_string, right_string) != 0)
          expression_result.valb = 1;
        else
          expression_result.valb = 0;

        free(left_string);
        free(right_string);
        free(left_side.vals);
        free(right_side.vals);
      } break;
      case BOOLEAN:
        expression_result.valb = left_side.valb != right_side.valb;
        break;
      case CHAR:
        expression_result.valb = left_side.valc != right_side.valc;
        break;
      case LONG:
        expression_result.valb = left_side.valh != right_side.valh;
      default:
        break;
      }
      break;
    case T_EQG:
      expression_result.type = BOOLEAN;
      switch (left_side.type) {
      case INTEGER:
        expression_result.valb = left_side.vali >= right_side.vali;
        break;
      case DOUBLE:
        expression_result.valb = left_side.valf >= right_side.valf;
        break;
      case STRING:
        fprintf(stderr, "Strings cannot bigger than each other.");

        return error;

        break;
      case BOOLEAN:
        fprintf(stderr, "Booleans cannot bigger than each other.");
        return error;

        break;
      case CHAR:
        fprintf(stderr, "Chars cannot bigger than each other.");
        return error;

        break;
      case LONG:
        expression_result.valb = left_side.valh >= right_side.valh;
      default:
        break;
      }
      break;
    case T_EQL:
      expression_result.type = BOOLEAN;
      switch (left_side.type) {
      case INTEGER:
        expression_result.valb = left_side.vali <= right_side.vali;
        break;
      case DOUBLE:
        expression_result.valb = left_side.valf <= right_side.valf;
        break;
      case STRING:
        fprintf(stderr, "Strings cannot less than each other.");
        return error;

        break;
      case BOOLEAN:
        fprintf(stderr, "Booleans cannot less than each other.");
        return error;

        break;
      case CHAR:
        fprintf(stderr, "Chars cannot less than each other.");
        return error;
        break;
      case LONG:
        expression_result.valb = left_side.valh <= right_side.valh;
      default:
        break;
      }
      break;
    default:
      break;
    }
  }
  return expression_result;
}

struct SL_Math_Splitter {
  char op;
  int is_op_type;
  int op_pos;
  enum TokenTypes op_type;
};

int operator_checker(char *expressions[], enum TokenTypes *types, int start,
                     int end);

int is_it_function_or_not(char **tokens, enum TokenTypes *types,
                          int current_token, int max_tokens) {
  if (current_token < 0 || current_token + 2 >= max_tokens)
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

int operator_checker(char *expressions[], enum TokenTypes *types, int start,
                     int end) {
  if (end - start > 1) {
    return 1;
  }

  int isitfunc = is_it_function_or_not(expressions, types, start, end);
  if (isitfunc != -1) {
    start = isitfunc;
  }

  for (int i = start; i < end; i++) {
    if (expressions[i] == NULL)
      return 0;

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

  while (current_token < max_tokens) {
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

    int op_prec = prec_priority(op, op_type, optype);
    if (optype == 0 && strlen(expression[current_token]) > 1) {
      op_prec = 0;
    }

    if (op_prec != 0) {
      if (tree.op == 0) {
        tree.op = op;
        if (optype == 1) {
          tree.op_type = op_type;
          tree.is_op_type = 1;
        } else {
          tree.is_op_type = 0;
        }
        tree.op_pos = current_token;
      }
      if (op_prec <= prec_priority(tree.op, tree.op_type, tree.is_op_type) &&
          op_prec != 0) {
        tree.op = op;

        if (optype == 1) {
          tree.op_type = op_type;
          tree.is_op_type = 1;
        } else {
          tree.is_op_type = 0;
        }
        tree.op_pos = current_token;
      }
    }
    current_token++;
  }
  return tree;
}

char *get_raw_function_name(char *word) {
  int len = strlen(word);
  char *returning_name = malloc(len + 1);
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

struct SL_Variable run_sl_function(struct SL_Code *code, char *name,
                                   char **tokens, enum TokenTypes *types,
                                   int current_token, int max_tokens) {
  int function_number = -1;
  struct SL_Variable return_val;
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
    return_val.type = ERROR;
    return_val.vali = 0;
    return return_val;
  }

  int start_var_index = code->total_vars;
  int start_func_index = code->total_funcs;
  struct SL_Function function = code->funcs[function_number];

  int free_tracker = 0;
  struct SL_L_Function lfunc = {0};
  if (function.total_arguments > 0 || function.vaargs == 1 ||
      function.linked_function == 1) {
    if (tokens[current_token + 1][0] != '(') {
      return_val.type = ERROR;
      return_val.vali = 1;
      return return_val;
    }
    if (tokens[current_token + 2][0] != ')') {
      current_token += 2;
      int how_much_go = 0;
      int vaargs_counter = 0;
      lfunc.argument_indexes = calloc(SL_INIT, sizeof(int));
      lfunc.starting_index = code->total_vars;

      while (current_token < code->token_count) {
        int commapos =
            sl_where_is_next_comma(tokens, current_token, code->token_count);
        if (commapos == -1) {
          return_val.type = ERROR;
          return_val.vali = 4;
          return return_val;
        }
        if (code->total_vars >= code->total_size_v) {
          return_val.type = ERROR;
          return_val.vali = 5;
          return return_val;
        }

        if (function.linked_function != 1 && function.vaargs == 0 &&
            how_much_go >= function.total_arguments) {
          return_val.type = ERROR;
          return_val.vali = 12;
          return return_val;
        }

        if (function.linked_function == 1) {
          if (lfunc.argument_indexes == NULL) {
            fprintf(stderr, "calloc() failed to allocate memory\n");
            exit(-1);
          }

          lfunc.argument_indexes[lfunc.total_arguments] = code->total_vars;
          struct SL_Variable result = expression_parser_solver(
              code, tokens, types, &current_token, commapos, 0);

          code->vars[code->total_vars] = result;
          code->vars[code->total_vars].name = NULL;
          if (result.name != NULL)
            code->vars[code->total_vars].name = strdup(result.name);
          code->total_vars++;
          free_tracker++;
          lfunc.total_arguments++;
        } else {
          if (function.total_arguments <= how_much_go && function.vaargs == 1) {
            char *function_name = malloc(SL_INIT);
            int func_len = strlen(function.name);
            snprintf(function_name, SL_INIT, "%s_VA_ARGUMENT_%d", function.name,
                     vaargs_counter);
            vaargs_counter++;
            struct SL_Variable result = expression_parser_solver(
                code, tokens, types, &current_token, commapos, 0);
            code->vars[code->total_vars] = result;
            code->vars[code->total_vars].name = strdup(function_name);
            code->vars[code->total_vars++].hash = sl_hash_string(function_name);

            free_tracker++;
            free(function_name);
          } else {
            struct SL_Variable result = expression_parser_solver(
                code, tokens, types, &current_token, commapos, 0);
            code->vars[code->total_vars] = result;
            if (function.arguments[how_much_go].name != NULL) {
              code->vars[code->total_vars].name =
                  strdup(function.arguments[how_much_go].name);
            } else {
              code->vars[code->total_vars].name = NULL;
            }

            code->vars[code->total_vars++].hash =
                function.arguments[how_much_go].hash;
          }
        }
        current_token = commapos + 1;

        if (tokens[commapos][0] == ')') {
          if (function.linked_function != 1 && function.vaargs == 0 &&
              how_much_go + 1 < function.total_arguments) {
            return_val.type = ERROR;
            return_val.vali = 11;
            return return_val;
          }
          break;
        }

        how_much_go++;
      }
    } else if (function.total_arguments > 0) {
      return_val.type = ERROR;
      return_val.vali = 11;
      return return_val;
    }

  } else if (function.total_arguments == 0) {
    while (current_token < max_tokens) {
      if (current_token != max_tokens - 1 &&
          tokens[current_token + 1][0] == '(' &&
          tokens[current_token + 2][0] == ')') {
        current_token += 2;
        break;
      } else {
        return_val.type = ERROR;
        return_val.vali = 2;
        return return_val;
      }
    }
  }

  if (function.linked_function == 1) {
    return_val = function.funcr(code, lfunc, function);
  } else {
    struct SL_Code code_def = {function.code_tokens,
                               function.types,
                               function.fixed_values,
                               function.code_len,
                               code->vars,
                               code->total_size_v,
                               code->total_vars,
                               code->funcs,
                               code->total_size_f,
                               code->total_funcs,
                               code->scope_depth + 1,
                               1};
    return_val = sl_init_sl_parser(&code_def);
    code->vars = code_def.vars;
    code->total_size_v = code_def.total_size_v;
    code->total_vars = code_def.total_vars;
    code->funcs = code_def.funcs;
    code->total_size_f = code_def.total_size_f;
    code->total_funcs = code_def.total_funcs;
    code_def.fixed_values = function.fixed_values;
    current_token--;
  }
  sl_clean_local_scope(code, start_var_index, start_func_index);
  if (lfunc.argument_indexes != NULL)
    free(lfunc.argument_indexes);

  return return_val;
}

static struct SL_Variable resolve_variable(struct SL_Code *code_s,
                                           char *expression[],
                                           enum TokenTypes *types,
                                           int current_token, int max_tokens,
                                           int old_curr) {
  struct SL_Variable var = code_s->fixed_values[current_token];
  if (var.type != RETURN)
    return sl_copy_variable(var);
  if (var.vals[0] == '$') {
    int index = var.cache_index;

    if (index == -1 || index >= code_s->total_vars ||
        code_s->vars[index].hash != var.hash) {
      index = getvar_index_from_sl(*code_s, var.vals + 1, var.hash);

      if (index != -1) {
        code_s->fixed_values[current_token].cache_index = index;
      }
    }

    if (index != -1) {
      struct SL_Variable resolved = code_s->vars[index];
      if ((resolved.type == STRING || resolved.type == RETURN) &&
          resolved.vals) {
        resolved.vals = strdup(resolved.vals);
      }
      return resolved;
    }
    if (index == -1) {
      sl_throw_an_error(*code_s, expression, old_curr, max_tokens,
                        "VARIABLE NOT FOUND!",
                        "Expected: Define a variable first.");
    }
    var = code_s->vars[index];
    if ((var.type == STRING || var.type == RETURN) && var.vals)
      var.vals = strdup(var.vals);
    return var;
  }
  if (is_has_func(*code_s, var.vals) != -1) {
    struct SL_Variable fn = run_sl_function(code_s, var.vals, expression, types,
                                            current_token, max_tokens);
    if (fn.type == ERROR) {
      if (fn.vali == 0)
        sl_throw_an_error(*code_s, expression, old_curr, max_tokens,
                          "FUNCTION NOT FOUND!",
                          "Expected: Create a function first.");
      else if (fn.vali == 1)
        sl_throw_an_error(*code_s, expression, old_curr, max_tokens,
                          "OPENING PARENTHESIS '(' NOT FOUND!",
                          "Expected: <function_name>(<arguments?>)");
      else if (fn.vali == 2)
        sl_throw_an_error(*code_s, expression, old_curr, max_tokens,
                          "OPENING/CLOSING PARENTHESIS NOT FOUND!",
                          "Expected: <function_name>()");
      else if (fn.vali == 4)
        sl_throw_an_error(
            *code_s, expression, old_curr, max_tokens,
            "CLOSING PARENTHESIS ')' OR COMMA ',' NOT FOUND!",
            "Expected: <function_name>(<arguments?>, <arguments?>)");
      else if (fn.vali == 5)
        sl_throw_an_error(*code_s, expression, old_curr, max_tokens,
                          "STACK CALL OVERFLOW!",
                          "Expected: Use less recursion or arguments (TIP: "
                          "While loop is a good alternative!)");
      else if (fn.vali == 11)
        sl_throw_an_error(*code_s, expression, old_curr, max_tokens,
                          "NOT ENOUGH ARGUMENTS!",
                          "Expected: Use all needed arguments.");
      else if (fn.vali == 12)
        sl_throw_an_error(*code_s, expression, old_curr, max_tokens,
                          "TOO MANY ARGUMENTS!",
                          "Expected: Use less arguments.");
    }
    return fn;
  }

  sl_throw_an_error(*code_s, expression, old_curr, max_tokens,
                    "FUNCTION OR VARIABLE NOT FOUND",
                    "Expected: define a function or variable first.");

  return var;
}

struct SL_Variable expression_parser_solver(struct SL_Code *code_s,
                                            char *expression[],
                                            enum TokenTypes *types,
                                            int *current_token, int max_tokens,
                                            int current_line) {
  struct SL_Variable empty = {0};
  if (!expression[*current_token])
    return empty;

  if (*current_token > max_tokens)
    return empty;

  while (expression[*current_token][0] == '(' &&
         (expression[max_tokens - 1][0] == ')' ||
          expression[max_tokens][0] == ')')) {
    int depth = 0;
    int valid = 1;
    for (int i = *current_token; i < max_tokens - 1; i++) {
      if (expression[i][0] == '(')
        depth++;
      else if (expression[i][0] == ')')
        depth--;

      if (depth == 0 && i < max_tokens - 2) {
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
    struct SL_Variable result = resolve_variable(
        code_s, expression, types, *current_token, max_tokens, old_curr);
    *current_token = max_tokens;
    return result;
  }

  if ((max_tokens - *current_token) == 3) {
    int left_pos = *current_token;
    int op_pos = *current_token + 1;
    int right_pos = *current_token + 2;

    if (operator_checker(expression, types, op_pos, right_pos) != 0) {

      struct SL_Variable left = resolve_variable(
          code_s, expression, types, left_pos, max_tokens, old_curr);
      struct SL_Variable right = resolve_variable(
          code_s, expression, types, right_pos, max_tokens, old_curr);

      char op = expression[op_pos][0];
      enum TokenTypes op_type = types[op_pos];
      int is_op_type = 0;

      switch (op_type) {
      case T_EQU:
      case T_NEQ:
      case T_EQG:
      case T_EQL:
      case T_AND:
      case T_OR:
      case T_SHLEFT:
      case T_SHRIGHT:
        is_op_type = 1;
        break;
      default:
        is_op_type = 0;
        break;
      }

      struct SL_Variable result =
          expression_solver(left, op, right, current_line, op_type, is_op_type);

      if (result.type == ERROR) {
        sl_throw_an_error(
            *code_s, expression, old_curr, max_tokens,
            "Mathematical or logical operation error. (Type mismatch or "
            "invalid operation)",
            "Ensure that the variables you are trying to operate on (String "
            "and Integer) are compatible with each other.");
      }

      *current_token = max_tokens;
      return result;
    }
  }

  struct SL_Variable left = {0};
  struct SL_Variable right = {0};
  struct SL_Variable result = {0};
  struct SL_Math_Splitter tree = expression_parser_splitter(
      *code_s, expression, types, *current_token, max_tokens, old_curr);

  if (tree.op == 0) {
    result = resolve_variable(code_s, expression, types, *current_token,
                              max_tokens, old_curr);
    *current_token = max_tokens;
    return result;
  }

  int left_pos_start = *current_token;
  int left_pos_end = tree.op_pos;

  if (operator_checker(expression, types, left_pos_start, left_pos_end) != 0) {
    left = expression_parser_solver(code_s, expression, types, &left_pos_start,
                                    left_pos_end, current_line);
  } else {
    left = resolve_variable(code_s, expression, types, left_pos_start,
                            left_pos_end, old_curr);
  }

  if (tree.is_op_type && tree.op_type == T_AND) {
    if (left.type == BOOLEAN && left.valb == 0) {
      *current_token = max_tokens;
      return left;
    }
  } else if (tree.is_op_type && tree.op_type == T_OR) {
    if (left.type == BOOLEAN && left.valb != 0) {
      *current_token = max_tokens;
      return left;
    }
  }

  int right_pos_start = tree.op_pos + 1;
  int right_pos_end = max_tokens;
  if (operator_checker(expression, types, right_pos_start, right_pos_end) !=
      0) {
    right =
        expression_parser_solver(code_s, expression, types, &right_pos_start,
                                 right_pos_end, current_line);
  } else {
    right = resolve_variable(code_s, expression, types, right_pos_start,
                             right_pos_end, old_curr);
  }

  result = expression_solver(left, tree.op, right, current_line, tree.op_type,
                             tree.is_op_type);
  if (result.type == ERROR) {
    sl_throw_an_error(*code_s, expression, old_curr, max_tokens,
                      "Mathematical or logical operation error. (Type mismatch "
                      "or invalid operation)",
                      "Ensure that the variables you are trying to operate on "
                      "(String and Integer) are compatible with each other.");
  }
  *current_token = max_tokens;
  return result;
}

int find_maxt_expr(char **code, enum TokenTypes *types, int starting, int max) {
  for (int i = starting; i < max; i++) {
    int isitfunc = is_it_function_or_not(code, types, i, max);
    if (isitfunc != -1) {
      i = isitfunc - 1;
    }

    if (i + 1 >= max)
      return max;

    if (operator_checker(code, types, i + 1, i + 2) == 1) {
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

        if (operator_checker(code, types, i + 1, i + 2) != 1)
          return i;

        i++;
        continue;
      }

      if (operator_checker(code, types, i, i + 1) == 1) {
        isitfunc = is_it_function_or_not(code, types, i + 1, max);
        if (isitfunc != -1) {
          i = isitfunc - 1;

          if (i + 1 >= max)
            return max;

          if (operator_checker(code, types, i + 1, i + 2) != 1)
            return i;

          continue;
        }
        continue;
      }

      return i;
    }
  }
  return max;
}

struct SL_Variable_Creator {
  int total_variables;
  struct SL_Variable *variable;
};

int multi_variable_checker(char *tokens[], int current_token, int max_tokens) {
  for (int i = current_token; i < max_tokens; i++) {
    if (tokens[i][0] == ',') {
      return i;
    }
  }
  return -1;
}

struct SL_Variable_Creator variable_parser(struct SL_Code *code_s,
                                           enum TokenTypes *types,
                                           char *tokens[], int *current_token,
                                           int max_tokens, int current_line) {
  struct SL_Variable_Creator variables;
  variables.variable = calloc(1024, sizeof(struct SL_Variable));
  variables.total_variables = 0;
  while (current_token != NULL && *current_token < max_tokens) {
    if (tokens[(*current_token) + 1] == NULL && max_tokens < 3) {
      if (tokens[*current_token] != NULL) {
        variables.variable[variables.total_variables++].name =
            strdup(tokens[(*current_token)]);
      }
    }
    if (strcmp(tokens[*current_token], "=") == 0) {
      int equal_start = *current_token + 1;
      int value_end = 0;
      value_end = max_tokens;
      if (current_assignment != NULL) {
        free(current_assignment);
      }
      current_assignment = strdup(tokens[(*current_token) - 1]);
      struct SL_Variable result = expression_parser_solver(
          code_s, tokens, types, &equal_start, value_end, current_line);
      variables.variable[variables.total_variables] = result;
      variables.variable[variables.total_variables].name =
          strdup(tokens[(*current_token) - 1]);
      variables.variable[variables.total_variables++].hash =
          sl_hash_string(tokens[(*current_token) - 1]);
      *current_token = value_end;
    }
    // printf("TOKEN: %s\n", tokens[*current_token]);
    (*current_token)++;
  }
  return variables;
}

int get_last_pos_of_token_for_expressions(char *token, char **tokens,
                                          int current_token, int max_tokens) {
  int last_pos = -1;
  while (current_token < max_tokens) {
    if (tokens[current_token][0] == token[0]) {
      last_pos = current_token;
    }
    current_token++;
  }
  return last_pos;
}

int is_has_token(char *token, char *tokens[], int current_position,
                 int max_tokens);

struct SL_Variable assignment_parser(struct SL_Code *code_s, char *tokens[],
                                     enum TokenTypes *types, int *current_token,
                                     int max_tokens, int current_line) {
  struct SL_Variable error_var;
  error_var.vali = 0;
  while (current_token != NULL && *current_token < max_tokens) {
    int last_pos = get_last_pos_of_token_for_expressions(
        "=", tokens, *current_token, max_tokens);
    if (last_pos == -1) {
      error_var.vali = -2;
      break;
    }
    int variable_pos = last_pos - 1;
    unsigned long hashe = sl_hash_string(tokens[variable_pos] + 1);
    int index = getvar_index_from_sl(*code_s, tokens[variable_pos] + 1, hashe);
    if (index == -1) {
      sl_throw_an_error(*code_s, tokens, *current_token, max_tokens,
                        "VARIABLE NOT FOUND ON ASSIGNMENT!",
                        "Expected: Define a variable first.");
    }
    struct SL_Variable eq_value = {0};
    int last_pos_ptr_expr_start = last_pos + 1;
    current_assignment = strdup(code_s->vars[index].name);
    eq_value = expression_parser_solver(code_s, tokens, types,
                                        &last_pos_ptr_expr_start, max_tokens,
                                        current_line);

    if ((code_s->vars[index].type == STRING ||
         code_s->vars[index].type == RETURN) &&
        code_s->vars[index].vals != NULL) {
      free(code_s->vars[index].vals);
    }

    eq_value.name = code_s->vars[index].name;
    code_s->vars[index] = eq_value;
    code_s->vars[index].hash = sl_hash_string(code_s->vars[index].name);
    if (is_has_token("=", tokens, *current_token, last_pos) == 1) {
      struct SL_Variable out_var = assignment_parser(
          code_s, tokens, types, current_token, last_pos, current_line);
    }

    free(current_assignment);
    current_assignment = NULL;
    *current_token = max_tokens;
  }
  return error_var;
}

int is_has_token(char *token, char *tokens[], int current_position,
                 int max_tokens) {
  for (int i = current_position; i < max_tokens; i++) {
    if (strcmp(token, tokens[i]) == 0) {
      return 1;
    }
  }
  return -1;
}

int sl_then_finder(char *tokens[], enum TokenTypes *types, int current_token,
                   int max_tokens, int *then_pos) {
  int start_pos = 0;

  while (current_token < max_tokens) {
    if (types[current_token] == T_THEN) {
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

struct SL_Variable sl_if_parser(struct SL_Code code_s, enum TokenTypes *types,
                                char *tokens[], int *current_token,
                                int keyword_pos, int max_tokens,
                                int find_then) {
  int if_start_pos = 0;
  struct SL_Variable expr = {0};
  int out = 0;
  if (find_then == -1) {
    out = sl_then_finder(tokens, types, *current_token, max_tokens,
                         &if_start_pos);
  } else {
    if_start_pos = find_then;
  }

  if (out == -1) {
    sl_throw_an_error(code_s, tokens, (*current_token) - 1, max_tokens,
                      "'then' NOT FOUND ON 'if' usage.",
                      "if <expression> then <code> end");
  }

  expr = expression_parser_solver(&code_s, tokens, types, current_token,
                                  if_start_pos, 0);
  (*current_token) = if_start_pos;
  return expr;
}

struct SL_Function sl_define_parser(struct SL_Code code_s, char *tokens[],
                                    enum TokenTypes *types, int *current_token,
                                    int max_tokens) {
  int then_pos;
  sl_then_finder(tokens, types, *current_token, max_tokens, &then_pos);
  // logic
  // def xx -> argxx,argxx,argxx then <code> end

  int current = *current_token;

  char *f_name = tokens[current++];
  int f_name_len = strlen(f_name);
  struct SL_Function function = {0};
  function.name = malloc(f_name_len + 1);
  function.name[f_name_len] = '\0';
  strncpy(function.name, f_name, f_name_len);
  function.hash = sl_hash_string(function.name);
  function.total_arguments = 0;
  int starting = 0;
  function.linked_function = 0;
  if (tokens[current][0] == '-' && tokens[++current][0] == '>') {
    current++;
    int arg_capacity = 8;
    function.arguments = calloc(arg_capacity, sizeof(struct SL_Variable));
    if (tokens[current][0] == ',')
      return function;
    while (types[current] != T_THEN) {
      if (strcmp(tokens[current], "...") == 0) {
        if (!types[current + 1] || types[current + 1] == T_THEN)
          sl_throw_an_error(code_s, tokens, *current_token, max_tokens,
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
        function.arguments = realloc(function.arguments,
                                     arg_capacity * sizeof(struct SL_Variable));
        if (function.arguments == NULL) {
          fprintf(stderr, "realloc() failed to allocate memory\n");
          exit(-1);
        }
      }
      int len = strlen(tokens[current]);
      if (function.arguments == NULL) {
        fprintf(stderr, "calloc() failed to allocate memory\n");
        exit(-1);
      }
      function.arguments[j].name = malloc(len + 1);
      strncpy(function.arguments[j].name, tokens[current], len);
      function.arguments[j].name[len] = '\0';
      function.arguments[j].hash = sl_hash_string(function.arguments[j].name);
      function.total_arguments++;
      current++;
    }
  }

  if (current == then_pos) {
    starting = ++current;
    int end_depth = 0;
    int code_length = 0;
    while (current < max_tokens) {
      if (types[current] == T_IF || types[current] == T_DEF ||
          types[current] == T_WHILE)
        end_depth++;

      if (types[current] == T_END && end_depth > 0) {
        end_depth--;
        current++;
        code_length++;
        continue;
      }

      if (types[current] == T_END && end_depth == 0)
        break;

      code_length++;
      current++;
    }
    function.code_tokens = malloc(code_length * sizeof(char *));
    function.types = malloc(code_length * sizeof(enum TokenTypes));
    function.fixed_values = calloc(code_length, sizeof(struct SL_Variable));
    function.code_len = code_length;
    for (int i = 0; i < code_length; i++) {
      int len = strlen(tokens[starting + i]);
      function.code_tokens[i] = malloc(len + 1);
      strncpy(function.code_tokens[i], tokens[starting + i], len);
      function.code_tokens[i][len] = '\0';
    }
    identifier_tokenizer(function.code_tokens, &function.types,
                         &function.fixed_values, function.code_len);
  }

  *current_token = current;
  return function;
}

struct Loops {
  int depth;
  int *back_pos;
  int capacity;
  int *end;
  int *then_pos;
};

int sl_find_end(char **tokens, enum TokenTypes *types, int start,
                int max_tokens, int branch) {
  int depth = 0;

  for (int i = start; i < max_tokens; i++) {
    if (types[i] == T_IF || types[i] == T_WHILE || types[i] == T_DEF) {
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

void identifier_tokenizer(char **code, enum TokenTypes **types,
                          struct SL_Variable **fixed_values, int token_count) {
  if (code == NULL || types == NULL || *types == NULL || fixed_values == NULL ||
      *fixed_values == NULL)
    return;
  enum TokenTypes *out = *types;
  struct SL_Variable *fixed = *fixed_values;

  for (int i = 0; i < token_count; ++i) {
    out[i] = identifier_tokenizer_converter(code[i]);
    if (out[i] == T_UNKNOWN) {
      fixed[i] = sl_word_to_var_converter(code[i]);
    } else {
      memset(&fixed[i], 0, sizeof(struct SL_Variable));
    }
  }
}

struct SL_Variable sl_init_sl_parser(struct SL_Code *code_s) {
  struct Loops while_loop = {0};
  while_loop.depth = 0;
  while_loop.back_pos = calloc(SL_INIT, sizeof(int));
  while_loop.end = calloc(SL_INIT, sizeof(int));
  while_loop.then_pos = calloc(SL_INIT, sizeof(int));
  if (while_loop.back_pos == NULL || while_loop.end == NULL ||
      while_loop.then_pos == NULL) {
    if (while_loop.back_pos != NULL) {
      free(while_loop.back_pos);
    }
    if (while_loop.end != NULL) {
      free(while_loop.end);
    }
    if (while_loop.then_pos != NULL) {
      free(while_loop.then_pos);
    }
    fprintf(stderr, "calloc() failed to allocate memory\n");
    exit(-1);
  }
  while_loop.capacity = SL_INIT;
  int while_sit = 0;
  struct SL_Variable return_val = {0};
  int brk_sit = 0;
  int depth = 0;
  if (code_s->types_set == 0)
    identifier_tokenizer(code_s->code, &code_s->types, &code_s->fixed_values,
                         code_s->token_count);
  int max_tokens = code_s->token_count;
  for (int current_token = 0; current_token < code_s->token_count;
       current_token++) {
    if (while_sit == 1) {
      if (code_s->types[current_token] == T_END) {
        if (while_loop.end[while_loop.depth - 1] == current_token)
          current_token = while_loop.back_pos[while_loop.depth - 1];
      }
      if (while_loop.depth == 0) {
        while_sit = 0;
      }
    }

    int end = 0;
    switch (code_s->types[current_token]) {
    case T_VAR: {
      for (int i = current_token; i < code_s->token_count; i++) {
        if (code_s->code[i][0] == '=') {
          i++;
          end = find_maxt_expr(code_s->code, code_s->types, i,
                               code_s->token_count);
          break;
        }
      }

      current_token++;
      struct SL_Variable_Creator vars = variable_parser(
          code_s, code_s->types, code_s->code, &current_token, end, 0);

      for (int i = 0; i < vars.total_variables; i++) {
        int index = getvar_index_from_sl(*code_s, vars.variable[i].name,
                                         vars.variable[i].hash);
        if (index != -1) {
          free(code_s->vars[index].name);
          if ((code_s->vars[index].type == STRING ||
               code_s->vars[index].type == RETURN) &&
              code_s->vars[index].vals != NULL) {
            free(code_s->vars[index].vals);
            code_s->vars[index].vals = NULL;
          }
          code_s->vars[index] = vars.variable[i];
        } else {
          code_s->vars[code_s->total_vars++] = vars.variable[i];
        }
        if (code_s->total_vars == code_s->total_size_v - 1) {
          code_s->total_size_v *= 2;
          code_s->vars = realloc(code_s->vars, code_s->total_size_v *
                                                   sizeof(struct SL_Variable));
        }
      }
      free(vars.variable);
      if (current_assignment != NULL) {
        free(current_assignment);
        current_assignment = NULL;
      }

      current_token--;
    } break;
    case T_IF: {
      current_token++;

      struct SL_Variable out_boolean =
          sl_if_parser(*code_s, code_s->types, code_s->code, &current_token,
                       current_token, code_s->token_count, -1);
      if (out_boolean.valb == 0) {
        while (1) {
          int out = sl_find_end(code_s->code, code_s->types, current_token,
                                code_s->token_count, 1);
          if (out == -1) {
            sl_throw_an_error(
                *code_s, code_s->code, current_token, code_s->token_count,
                "END NOT FOUND END OF THE IF",
                "Expected: if <expr> then <code> else <code> end");
          }

          if (code_s->types[out] == T_END || code_s->types[out] == T_ELSE) {
            current_token = out;
            break;
          } else if (code_s->types[out] == T_ELIF) {
            int elif_tok = out + 1;
            struct SL_Variable elif_boolean =
                sl_if_parser(*code_s, code_s->types, code_s->code, &elif_tok,
                             elif_tok, code_s->token_count, -1);
            if (elif_boolean.valb == 1) {
              current_token = elif_tok;
              break;
            } else {
              current_token = elif_tok;
            }
          }
        }
        continue;
      } else {
        int out = sl_find_end(code_s->code, code_s->types, current_token,
                              code_s->token_count, 1);
        if (out == -1) {
          sl_throw_an_error(*code_s, code_s->code, current_token,
                            code_s->token_count, "END NOT FOUND END OF THE IF",
                            "Expected: if <expr> then <code> else <code> end");
        }
      }
    } break;
    case T_WHILE: {
      int currpos = current_token;
      current_token++;

      if (while_loop.depth >= while_loop.capacity) {
        while_loop.capacity *= 2;
        while_loop.back_pos =
            realloc(while_loop.back_pos, while_loop.capacity * sizeof(int));
        while_loop.end =
            realloc(while_loop.end, while_loop.capacity * sizeof(int));
        while_loop.then_pos =
            realloc(while_loop.then_pos, while_loop.capacity * sizeof(int));

        if (while_loop.back_pos == NULL || while_loop.end == NULL) {
          fprintf(stderr, "realloc() failed to allocate memory\n");
          exit(-1);
        }
      }

      int thenp = -1;

      if (while_loop.depth > 0 &&
          while_loop.back_pos[while_loop.depth - 1] == currpos) {
        thenp = while_loop.then_pos[while_loop.depth - 1];
      }

      if (thenp == -1) {
        int out = sl_then_finder(code_s->code, code_s->types, current_token,
                                 max_tokens, &thenp);
        if (out == -1) {
          sl_throw_an_error(*code_s, code_s->code, current_token,
                            code_s->token_count, "THEN NOT FOUND ON WHILE",
                            "Expected: while <expr> then <code> end");
        }
      }

      struct SL_Variable out_boolean =
          sl_if_parser(*code_s, code_s->types, code_s->code, &current_token,
                       current_token, code_s->token_count, thenp);

      int end = -1;

      if (while_loop.depth > 0 &&
          while_loop.back_pos[while_loop.depth - 1] == currpos) {
        end = while_loop.end[while_loop.depth - 1];
      }

      if (end == -1) {
        end = sl_find_end(code_s->code, code_s->types, current_token,
                          code_s->token_count, 0);

        if (end == -1) {
          sl_throw_an_error(*code_s, code_s->code, current_token,
                            code_s->token_count,
                            "END NOT FOUND END OF THE WHILE",
                            "Expected: while <expr> then <code> end");
        }
      }

      if (out_boolean.valb == 0) {
        if (while_loop.depth > 0 &&
            while_loop.back_pos[while_loop.depth - 1] == currpos) {
          current_token = while_loop.end[--while_loop.depth];
        } else {
          current_token = end;
        }
      } else {
        if (while_loop.depth == 0 ||
            while_loop.back_pos[while_loop.depth - 1] != currpos) {
          while_loop.back_pos[while_loop.depth] = currpos;
          while_loop.end[while_loop.depth] = end;
          while_loop.then_pos[while_loop.depth] = thenp;
          while_loop.depth++;
        }

        while_sit = 1;
      }
    } break;
    case T_CONTINUE:
      if (while_sit != 1)
        sl_throw_an_error(*code_s, code_s->code, current_token,
                          code_s->token_count, "CONTINUE USAGE WITHOUT LOOP",
                          "Expected: define a loop first.");
      current_token = while_loop.back_pos[--while_loop.depth] - 1;
      break;
    case T_BREAK:
      if (while_sit != 1)
        sl_throw_an_error(*code_s, code_s->code, current_token,
                          code_s->token_count, "BREAK USAGE WITHOUT LOOP",
                          "Expected: define a loop first.");
      current_token = while_loop.end[--while_loop.depth];
      break;
    case T_END:
      break;
    case T_ELSE:
    case T_ELIF: {
      int out = sl_find_end(code_s->code, code_s->types, current_token + 1,
                            code_s->token_count, 0);
      if (out == -1)
        sl_throw_an_error(*code_s, code_s->code, current_token,
                          code_s->token_count, "END NOT FOUND END OF THE ELSE",
                          "Expected: if <expr> then <code> else <code> end");
      current_token = out;
    } break;
    case T_IMPORT:
      if (!code_s->code[current_token + 1]) {
        sl_throw_an_error(
            *code_s, code_s->code, current_token, code_s->token_count,
            "UNEXPECTED IMPORT SYNTAX!",
            "Expected: import <file_name> (without string literal!)");
        free(while_loop.end);
        free(while_loop.back_pos);
        free(while_loop.then_pos);
        exit(-1);
      }
      char *module_name = sl_string_getter(code_s->code[current_token + 1]);

      char **imported_tokens = NULL;
      int imported_count =
          sl_init_sl_lexer(1024, module_name, &imported_tokens, SPECIAL_TOKENS);

      if (imported_count > 0) {
        struct SL_Code import_code = {0};
        import_code.code = imported_tokens;
        import_code.token_count = imported_count;

        import_code.vars = code_s->vars;
        import_code.total_size_v = code_s->total_size_v;
        import_code.total_vars = code_s->total_vars;

        import_code.funcs = code_s->funcs;
        import_code.total_size_f = code_s->total_size_f;
        import_code.total_funcs = code_s->total_funcs;

        import_code.scope_depth = code_s->scope_depth;

        import_code.types = malloc(imported_count * sizeof(enum TokenTypes));
        for (int i = 0; i < imported_count; i++) {
          import_code.types[i] = T_UNKNOWN;
        }

        sl_init_sl_parser(&import_code);

        code_s->vars = import_code.vars;
        code_s->total_size_v = import_code.total_size_v;
        code_s->total_vars = import_code.total_vars;

        code_s->funcs = import_code.funcs;
        code_s->total_size_f = import_code.total_size_f;
        code_s->total_funcs = import_code.total_funcs;

        free(import_code.types);
        for (int i = 0; i < imported_count; i++) {
          if (imported_tokens[i])
            free(imported_tokens[i]);
        }
        free(imported_tokens);
      } else {
        fprintf(stderr, "[ERROR] Import failed or empty module: %s\n",
                module_name);
      }

      free(module_name);
      current_token++;
      break;
    case T_DEF:
      current_token++;
      struct SL_Function func =
          sl_define_parser(*code_s, code_s->code, code_s->types, &current_token,
                           code_s->token_count);
      if (is_has_func(*code_s, func.name) != -1) {
        sl_throw_an_error(*code_s, code_s->code, current_token, max_tokens,
                          "FUNCTION ALREADY EXISTS",
                          "Expected: try different name for your function");
      }
      code_s->funcs[code_s->total_funcs++] = func;
      if (code_s->total_funcs == code_s->total_size_f - 1) {
        code_s->total_size_f *= 2;
        code_s->funcs = realloc(code_s->funcs, code_s->total_size_f *
                                                   sizeof(struct SL_Function));
      }
      break;
    case T_RETURN:
      current_token++;
      end = find_maxt_expr(code_s->code, code_s->types, current_token,
                           code_s->token_count);
      struct SL_Variable result = expression_parser_solver(
          code_s, code_s->code, code_s->types, &current_token, end, 0);
      free(while_loop.back_pos);
      free(while_loop.end);
      free(while_loop.then_pos);
      return result;
    default:
      if (code_s->code[current_token][0] == '$') {
        int old_curr = current_token;
        for (int i = current_token; i < code_s->token_count; i++) {
          if (code_s->code[i][0] == '=') {
            i++;
            end = find_maxt_expr(code_s->code, code_s->types, i,
                                 code_s->token_count);
            break;
          }
        }
        struct SL_Variable out = assignment_parser(
            code_s, code_s->code, code_s->types, &current_token, end, 0);
        if (out.vali == -2) {
          sl_throw_an_error(*code_s, code_s->code, old_curr, old_curr + 2,
                            "ASSIGNMENT FAILED!", "$<var_name> = <expression>");
          return_val.type = ERROR;
          free(while_loop.back_pos);
          free(while_loop.end);
          free(while_loop.then_pos);
          return return_val;
        }
      } else {
        end = find_maxt_expr(code_s->code, code_s->types, current_token,
                             code_s->token_count);

        struct SL_Variable result = expression_parser_solver(
            code_s, code_s->code, code_s->types, &current_token, end, 0);
      }
    }
  }
  free(while_loop.back_pos);
  free(while_loop.end);
  free(while_loop.then_pos);
  return return_val;
}

struct SL_Code sl_init_sl_process() {
  struct SL_Code code;
  code.funcs = calloc(SL_INIT, sizeof(struct SL_Function));
  code.total_size_f = SL_INIT;
  code.vars = calloc(SL_INIT, sizeof(struct SL_Variable));
  code.total_size_v = SL_INIT;
  code.total_funcs = 0;
  code.total_vars = 0;
  code.code = NULL;
  code.types = NULL;
  code.fixed_values = NULL;
  code.types_set = 0;
  code.scope_depth = 1;
  return code;
}

int sl_open_sl_process(struct SL_Code *code, char *file_name) {
  int count = sl_init_sl_lexer(SL_INIT, file_name, &code->code, SPECIAL_TOKENS);
  if (count < 0) {
    fprintf(stderr,
            "Invalid count\nsl_init_sl_lexer() returned a negative value\n");
    return -1;
  }

  code->types = calloc(count, sizeof(enum TokenTypes));
  code->fixed_values = calloc(count, sizeof(struct SL_Variable));
  code->types_set = 0;
  if (code->types == NULL) {
    fprintf(stderr, "calloc() failed to allocate memory (types)\n");
    return -1;
  }

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

struct SL_Variable sl_dostr_sl_process(struct SL_Code *code_s, char *code) {
  struct SL_Code code_p = sl_init_sl_process();

  if (code_s->funcs != NULL) {
    for (int i = 0; i < code_s->total_funcs; i++) {
      if (code_p.total_funcs >= code_p.total_size_f) {
        code_p.total_size_f *= 2;
        code_p.funcs = realloc(code_p.funcs, code_p.total_size_f *
                                                 sizeof(struct SL_Function));
      }
      code_p.funcs[code_p.total_funcs++] = code_s->funcs[i];
    }
  }

  if (code_s->vars != NULL) {
    for (int i = 0; i < code_s->total_vars; i++) {
      if (code_p.total_vars >= code_p.total_size_v) {
        code_p.total_size_v *= 2;
        code_p.vars = realloc(code_p.vars,
                              code_p.total_size_v * sizeof(struct SL_Variable));
      }
      code_p.vars[code_p.total_vars++] = code_s->vars[i];
    }
  }

  code_p.code = malloc(1024 * sizeof(char *));
  int count = LEXER(code, &code_p.code, strlen(code), SPECIAL_TOKENS, 1024);
  code_p.token_count = count;
  code_p.types = calloc(count, sizeof(enum TokenTypes));

  struct SL_Variable init = sl_init_sl_parser(&code_p);
  if (init.type == ERROR) {
    fprintf(stderr, "sl_parser failed \n");
    exit(-1);
  };

  if (code_p.funcs != NULL) {
    for (int i = 0; i < code_p.total_funcs; i++) {
      if (i >= code_s->total_size_f) {
        code_s->total_size_f *= 2;
        code_s->funcs = realloc(code_s->funcs, code_s->total_size_f *
                                                   sizeof(struct SL_Function));
      }
      code_s->funcs[i] = code_p.funcs[i];
    }
    code_s->total_funcs = code_p.total_funcs;
    free(code_p.funcs);
  }

  if (code_p.vars != NULL) {
    for (int i = 0; i < code_p.total_vars; i++) {
      if (i >= code_s->total_size_v) {
        code_s->total_size_v *= 2;
        code_s->vars = realloc(code_s->vars, code_s->total_size_v *
                                                 sizeof(struct SL_Variable));
      }
      code_s->vars[i] = code_p.vars[i];
    }
    code_s->total_vars = code_p.total_vars;
    free(code_p.vars);
  }

  if (code_p.code != NULL) {
    for (int i = 0; i < code_p.token_count; i++) {
      if (code_p.code[i] != NULL) {
        free(code_p.code[i]);
      }
    }
    free(code_p.code);
  }

  if (code_p.types != NULL) {
    free(code_p.types);
  }

  return init;
}

int sl_close_sl_process(struct SL_Code *code) {
  if (code->vars != NULL) {
    for (int i = 0; i < code->total_vars; i++) {
      if (code->vars[i].name != NULL) {
        free(code->vars[i].name);
        code->vars[i].name = NULL;
      }
      if ((code->vars[i].type == STRING || code->vars[i].type == RETURN) &&
          code->vars[i].vals != NULL) {
        free(code->vars[i].vals);
        code->vars[i].vals = NULL;
      }
    }
    free(code->vars);
    code->vars = NULL;
  }

  if (code->funcs != NULL) {
    for (int i = 0; i < code->total_funcs; i++) {
      if (code->funcs[i].name != NULL) {
        free(code->funcs[i].name);
        code->funcs[i].name = NULL;
      }

      if (code->funcs[i].arguments != NULL) {
        for (int j = 0; j < code->funcs[i].total_arguments; j++) {
          if (code->funcs[i].arguments[j].name != NULL) {
            free(code->funcs[i].arguments[j].name);
            code->funcs[i].arguments[j].name = NULL;
          }
          if ((code->funcs[i].arguments[j].type == STRING ||
               code->funcs[i].arguments[j].type == RETURN) &&
              code->funcs[i].arguments[j].vals != NULL) {
            free(code->funcs[i].arguments[j].vals);
            code->funcs[i].arguments[j].vals = NULL;
          }
        }
        free(code->funcs[i].arguments);
        code->funcs[i].arguments = NULL;
      }

      if (code->funcs[i].code_tokens != NULL) {
        for (int k = 0; k < code->funcs[i].code_len; k++) {
          if (code->funcs[i].code_tokens[k] != NULL) {
            free(code->funcs[i].code_tokens[k]);
            code->funcs[i].code_tokens[k] = NULL;
          }
        }
        free(code->funcs[i].code_tokens);
        code->funcs[i].code_tokens = NULL;
      }
      if (code->funcs[i].types != NULL) {
        free(code->funcs[i].types);
        code->funcs[i].types = NULL;
      }
    }
    free(code->funcs);
    code->funcs = NULL;
  }

  if (code->code != NULL) {
    for (int i = 0; i < code->token_count; i++) {
      if (code->code[i] != NULL) {
        free(code->code[i]);
        code->code[i] = NULL;
      }
    }
    free(code->code);
    code->code = NULL;
  }

  if (code->fixed_values != NULL) {
    for (int i = 0; i < code->token_count; i++) {
      struct SL_Variable *var = &code->fixed_values[i];
      if ((var->type == STRING || var->type == RETURN) && var->vals != NULL) {
        free(var->vals);
        var->vals = NULL;
      }
    }
    free(code->fixed_values);
    code->fixed_values = NULL;
  }

  if (code->types != NULL) {
    free(code->types);
  }
  return 0;
}
