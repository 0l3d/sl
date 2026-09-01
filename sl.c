#include "sl.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct SL_Variable
expression_parser_solver(struct SL_Code code_s,
			 char *expression[], enum TokenTypes *types,
			 int *current_token, int max_tokens,
			 int current_line);


struct SL_Variable
sl_copy_variable(struct SL_Variable var)
{
	struct SL_Variable copy = var;

	if (var.name != NULL) {
		copy.name = strdup(var.name);
	}

	if ((var.type == STRING || var.type == RETURN) && var.vals != NULL) {
		copy.vals = strdup(var.vals);
	}

	return copy;
}



void
sl_throw_an_error(struct SL_Code code,
		  char **tokens,
		  int current_token,
		  int max_tokens, char *error_msg, char *expected_tip)
{
	fprintf(stderr, "\n[ERROR] %s\n", error_msg);
	if (tokens != NULL && current_token >= 0
	    && current_token < max_tokens) {
		int             start =
			(current_token - 5 > 0) ? current_token - 5 : 0;
		int             end =
			(current_token + 5 <
			 max_tokens) ? current_token + 5 : max_tokens;

		fprintf(stderr, "Problematic area in your code:\n...");
		for (int i = start; i < end; i++) {
			if (i == current_token) {
				fprintf(stderr, " >>> %s <<< ", tokens[i]);
			}
			else {
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

int
lexer_special_tokens_ex(char *special_tokens, char letter)
{
	for (int i = 0; i < strlen(special_tokens); i++) {
		if (letter == special_tokens[i]) {
			return 1;
		}
	}
	return 0;
}

unsigned long
hash_string(const char *str)
{
	unsigned long   hash = 5381;
	int             c;
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}



int
LEXER(char *bufin, char ***bufout, int max_count, char *special_tokens,
      int start_size)
{
	int             size_s = start_size;
	if (bufin == NULL)
		return 0;
	int             token_count = 0;
	const char     *p = bufin;
	while (*p != '\0') {
		if (token_count + 1 == size_s) {
			size_s *= 2;
			char          **tmp =
				realloc(*bufout, size_s * sizeof(char *));
			if (tmp == NULL)
				return -1;
			(*bufout) = tmp;
		}
		if (*p == '#') {
			break;
		}
		else if (isspace(*p)) {
			p++;
			continue;
		}
		else if (p[0] == '"' && p[1] == '"' && p[2] == '"') {
			const char     *string_start = p;
			p += 3;

			while (*p && *p != '\0') {
				if (p[0] == '"' && p[1] == '"' && p[2] == '"') {
					p += 3;
					break;
				}
				p++;
			}

			int             stringlen = p - string_start;
			char           *in_string_tokens =
				malloc(stringlen + 1);
			strncpy(in_string_tokens, string_start, stringlen);
			in_string_tokens[stringlen] = '\0';
			(*bufout)[token_count++] = in_string_tokens;
		}
		else if (*p == '"' || *p == '\'') {
			char            in_string = *p;
			const char     *string_start = p++;
			while (*p && *p != in_string)
				p++;

			if (*p == in_string) {
				p++;
			}
			int             stringlen = p - string_start;
			char           *in_string_tokens =
				malloc(stringlen + 1);
			strncpy(in_string_tokens, string_start, stringlen);
			in_string_tokens[stringlen] = '\0';
			(*bufout)[token_count++] = in_string_tokens;
		}
		else {
			if ((*p == '>' && *(p + 1) == '>')
			    || (*p == '<' && *(p + 1) == '<')) {
				char           *pot = malloc(3);
				pot[0] = *p;
				pot[1] = *(p + 1);
				pot[2] = '\0';
				(*bufout)[token_count++] = pot;
				p += 2;
				continue;
			}
			if ((*p == '-' || *p == '+') &&
			    isdigit((unsigned char) *(p + 1))) {

				const char     *word_start = p++;

				while (*p &&
				       !isspace((unsigned char) *p) &&
				       lexer_special_tokens_ex(special_tokens,
							       *p) == 0)
					p++;

				int             word_len = p - word_start;

				char           *word = malloc(word_len + 1);
				memcpy(word, word_start, word_len);
				word[word_len] = '\0';

				(*bufout)[token_count++] = word;
				continue;
			}
			if (lexer_special_tokens_ex(special_tokens, *p) > 0) {
				char           *pot = malloc(2);
				pot[0] = *p;
				pot[1] = '\0';
				(*bufout)[token_count++] = pot;
				p++;
				continue;
			}
			const char     *word_start = p;
			while (*p && !isspace(*p) &&
			       lexer_special_tokens_ex(special_tokens,
						       *p) == 0)
				p++;
			int             word_len = p - word_start;
			char           *word = malloc(word_len + 1);
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

int
sl_init_sl_lexer(int malloc_size, char *file_name, char ***bufout,
		 char *special_tokens)
{
	FILE           *code_file = fopen(file_name, "r");
	if (code_file == NULL) {
		perror("init_sl_lexer failed with error:");
		return -1;
	}

	char            buf[4096];
	char           *code_string;

	int             total_allocations = 0;

	total_allocations += malloc_size;
	code_string = malloc(total_allocations);
	code_string[0] = '\0';

	int             enable_endlinemodifier = 0;
	int             end_line_modifier_sit = 0;

	while (fgets(buf, sizeof(buf), code_file)) {
		char           *character_pos = strchr(buf, '#');
		char           *p = strchr(buf, '\n');
		if (p)
			*p = '\0';

		int             index = strlen(buf);

		if (character_pos != NULL)
			index = character_pos - buf;

		int             len = strlen(code_string);
		if (len + index + 2 > total_allocations) {
			total_allocations += malloc_size;
			code_string = realloc(code_string, total_allocations);
		}

		memcpy(code_string + len, buf, index);
		code_string[len + index] = ' ';
		code_string[len + index + 1] = '\0';
	}

	char          **code_array = malloc(1024 * sizeof(char *));
	int             count =
		LEXER(code_string, &code_array, strlen(code_string),
		      special_tokens,
		      1024);

	free(code_string);
	*bufout = code_array;

	fclose(code_file);
	return count;
}

struct SL_Variable
getvar_from_sl(struct SL_Code code, const char *name)
{
	for (int i = 0; i < code.total_vars; i++) {
		if (strcmp(code.vars[i].name, name) == 0) {
			return code.vars[i];
		}
	}
	struct SL_Variable empty = { 0 };
	return empty;
}



enum ConditionTYPE
{
	EQUAL,
	GREATER,
	LESS,
	EQUALORGREATER,
	EQUALORLESS,
};

int
check_number(const char *s)
{
	char           *endptr;

	long            val_int = strtol(s, &endptr, 10);
	if (*endptr == '\0') {
		return 1;
	}
	double          val_double = strtod(s, &endptr);
	if (*endptr == '\0') {
		return 2;
	}
	return 0;
}

int
string_checker(char *word)
{
	if (word == NULL)
		return 0;

	int             size = strlen(word);

	if (size >= 6 &&
	    word[0] == '"' && word[1] == '"' && word[2] == '"' &&
	    word[size - 1] == '"' && word[size - 2] == '"'
	    && word[size - 3] == '"') {
		return 3;
	}

	if (size >= 2 && word[0] == '"' && word[size - 1] == '"') {
		return 1;
	}

	return 0;
}

char           *
sl_string_getter(char *word)
{
	if (word == NULL)
		return NULL;

	int             quote_len = string_checker(word);

	if (quote_len == 0) {
		return strdup(word);
	}

	int             size = strlen(word);
	char           *our_word = malloc(size);
	if (our_word == NULL)
		return NULL;

	int             j = 0;

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
		}
		else {
			our_word[j++] = word[i];
		}
	}

	our_word[j] = '\0';
	return our_word;
}

enum SL_Types
type_analyzer(char *word)
{
	int             check_num = check_number(word);
	if (string_checker(word) == 1) {
		return STRING;
	}
	else if (word[0] == '0' && word[1] == 'x') {
		return LONG;
	}
	else if (check_num == 1) {
		return INTEGER;
	}
	else if (check_num == 2) {
		return DOUBLE;
	}
	else if (strcmp(word, "false") == 0 || strcmp(word, "true") == 0) {
		return BOOLEAN;
	}
	else if (word[0] == '\'') {
		return CHAR;
	}
	else if (check_num == 0 && string_checker(word) != 1) {
		return RETURN;
	}
	return -1;
}

struct SL_Variable
sl_word_to_var_converter(char *word)
{
	struct SL_Variable v = { 0 };
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
		v.valc = word[1];
		break;
	case RETURN:
		v.type = RETURN;
		v.vals = strdup(word);
		break;
	case LONG:{
			int             base = 0;
			if (word[0] == '0') {
				switch (word[1]) {
				case 'x':
					base = 16;
					break;
				case 'b':
					base = 2;
				default:
					base = 10;
					break;
				}
			}
			v.valh = strtol(word, NULL, base);
		}
		break;
	default:
		break;
	}
	return v;
}

int
getvar_index_from_sl(struct SL_Code code, const char *name)
{

	unsigned long   hash = hash_string(name);
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




int
sl_add_func(struct SL_Code *code, char *name,
	    struct SL_Variable (*funcr) (struct SL_Code *,
					 struct SL_L_Function))
{
	code->funcs[code->total_funcs].name = strdup(name);
	code->funcs[code->total_funcs].hash = hash_string(name);
	code->funcs[code->total_funcs].linked_function = 1;
	code->funcs[code->total_funcs++].funcr = funcr;
	return 0;
}

struct SL_Variable
sl_get_argument(struct SL_Code code, struct SL_L_Function func, int which_one)
{
	struct SL_Variable error_val = { 0 };
	error_val.type = ERROR;
	if (which_one >= func.total_arguments)
		return error_val;
	return code.vars[func.starting_index + which_one];
}

int
sl_add_var(struct SL_Code *code, struct SL_Variable var)
{
	code->vars[code->total_vars] = var;
	code->vars[code->total_vars++].name = strdup(var.name);
	code->vars[code->total_vars++].hash = hash_string(var.name);
	return 0;

}

struct SL_Variable
sl_get_var(struct SL_Code code, const char *name)
{
	struct SL_Variable return_err = { 0 };
	return_err.type = ERROR;
	int             index = getvar_index_from_sl(code, name);
	if (index == -1)
		return return_err;

	return code.vars[index];
}


struct SL_Function
sl_get_func(struct SL_Code code, const char *name)
{
	struct SL_Function error_func = { 0 };
	error_func.name = NULL;
	for (int i = 0; i < code.total_funcs; i++) {
		if (strcmp(code.funcs[i].name, name) == 0)
			return code.funcs[i];
	}
	return error_func;
}

struct SL_Variable
expression_solver(struct SL_Variable left_side, char op,
		  struct SL_Variable right_side,
		  int current_line, enum TokenTypes op_type, int is_op_type)
{

	if ((left_side.type == DOUBLE && right_side.type == INTEGER) ||
	    (left_side.type == INTEGER && right_side.type == DOUBLE)) {
		left_side.type = DOUBLE;
		right_side.type = DOUBLE;
		right_side.valf = right_side.vali;
	}

	struct SL_Variable error = { 0 };
	error.type = ERROR;

	if (left_side.type != right_side.type) {
		return error;
	}
	struct SL_Variable expression_result = { 0 };
	expression_result.type = left_side.type;

	if (is_op_type != 1) {
		switch (op) {
		case '+':
			switch (left_side.type) {
			case INTEGER:
				expression_result.vali =
					left_side.vali + right_side.vali;
				break;
			case DOUBLE:
				expression_result.valf =
					left_side.valf + right_side.valf;
				break;
			case STRING:{
					char           *left_string =
						sl_string_getter
						(left_side.vals);
					char           *right_string =
						sl_string_getter
						(right_side.vals);
					size_t          len =
						strlen(left_string) +
						strlen(right_string) + 4;
					expression_result.vals = malloc(len);
					snprintf(expression_result.vals, len,
						 "\"%s%s\"", left_string,
						 right_string);
					free(left_string);
					free(right_string);
					free(left_side.vals);
					free(right_side.vals);
				}
				break;
			case BOOLEAN:
				expression_result.valb =
					left_side.valb + right_side.valb;
				break;
			case CHAR:
				fprintf(stderr,
					"Chars cannot add each other.");
				return error;
				break;
			case LONG:
				expression_result.valh =
					left_side.valh + right_side.valh;
			default:
				break;
			}
			break;
		case '-':
			switch (left_side.type) {
			case INTEGER:
				expression_result.vali =
					left_side.vali - right_side.vali;
				break;
			case DOUBLE:
				expression_result.valf =
					left_side.valf - right_side.valf;
				break;
			case STRING:
				fprintf(stderr,
					"Strings cannot sub each other.");
				return error;
				break;
			case BOOLEAN:
				expression_result.valb =
					left_side.valb - right_side.valb;
				break;
			case CHAR:
				fprintf(stderr,
					"Chars cannot sub each other.");
				return error;
				break;
			case LONG:
				expression_result.valh =
					left_side.valh - right_side.valh;
			default:
				break;
			}
			break;
		case '*':
			switch (left_side.type) {
			case INTEGER:
				expression_result.vali =
					left_side.vali * right_side.vali;
				break;
			case DOUBLE:
				expression_result.valf =
					left_side.valf * right_side.valf;
				break;
			case STRING:
				fprintf(stderr,
					"Strings cannot mul each other.");
				return error;
				break;
			case BOOLEAN:
				expression_result.valb =
					left_side.valb * right_side.valb;
				break;
			case CHAR:
				fprintf(stderr,
					"Chars cannot mul each other.");
				return error;
				break;
			case LONG:
				expression_result.valh =
					left_side.valh * right_side.valh;
			default:
				break;
			}
			break;
		case '/':
			switch (left_side.type) {
			case INTEGER:
				expression_result.vali =
					left_side.vali / right_side.vali;
				break;
			case DOUBLE:
				expression_result.valf =
					left_side.valf / right_side.valf;
				break;
			case STRING:
				fprintf(stderr,
					"Strings cannot div each other.");
				return error;
				break;
			case BOOLEAN:
				expression_result.valb =
					left_side.valb / right_side.valb;
				break;
			case CHAR:
				fprintf(stderr,
					"Chars cannot div each other.");
				return error;
				break;
			case LONG:
				expression_result.valh =
					left_side.valh / right_side.valh;
			default:
				break;
			}
			break;
		case '&':
			switch (left_side.type) {
			case INTEGER:
				expression_result.vali =
					left_side.vali & right_side.vali;
				break;
			case DOUBLE:
				fprintf(stderr,
					"Doubles cannot and each other.");
				return error;
				break;
			case STRING:
				fprintf(stderr,
					"Strings cannot and each other.");
				return error;
				break;
			case BOOLEAN:
				fprintf(stderr,
					"Booleans cannot and each other.");
				return error;
				break;
			case CHAR:
				fprintf(stderr,
					"Chars cannot and each other.");
				return error;
				break;
			case LONG:
				expression_result.valh =
					left_side.valh & right_side.valh;
			default:
				break;
			}
			break;
		case '|':
			switch (left_side.type) {
			case INTEGER:
				expression_result.vali =
					left_side.vali | right_side.vali;
				break;
			case DOUBLE:
				fprintf(stderr,
					"Doubles cannot or each other.");
				return error;
				break;
			case STRING:
				fprintf(stderr,
					"Strings cannot or each other.");
				return error;
				break;
			case BOOLEAN:
				fprintf(stderr,
					"Booleans cannot or each other.");
				return error;
				break;
			case CHAR:
				fprintf(stderr,
					"Chars cannot or each other.");
				return error;
				break;
			case LONG:
				expression_result.valh =
					left_side.valh | right_side.valh;
			default:
				break;
			}
			break;
		case '^':
			switch (left_side.type) {
			case INTEGER:
				expression_result.vali =
					left_side.vali ^ right_side.vali;
				break;
			case DOUBLE:
				fprintf(stderr,
					"Doubles cannot xor each other.");
				return error;
				break;
			case STRING:
				fprintf(stderr,
					"Strings cannot xor each other.");
				return error;
				break;
			case BOOLEAN:
				fprintf(stderr,
					"Booleans cannot xor each other.");
				return error;
				break;
			case CHAR:
				fprintf(stderr,
					"Chars cannot xor each other.");
				return error;
				break;
			case LONG:
				expression_result.valh =
					left_side.valh ^ right_side.valh;
			default:
				break;
			}
			break;
		case '%':
			switch (left_side.type) {
			case INTEGER:
				expression_result.vali =
					left_side.vali % right_side.vali;
				break;
			case DOUBLE:
				fprintf(stderr,
					"Doubles cannot mod each other.");
				return error;
				break;
			case STRING:
				fprintf(stderr,
					"Strings cannot mod each other.");
				return error;
				break;
			case BOOLEAN:
				fprintf(stderr,
					"Booleans cannot mod each other.");
				return error;
				break;
			case CHAR:
				fprintf(stderr,
					"Chars cannot mod each other.");
				return error;
				break;
			case LONG:
				expression_result.valh =
					left_side.valh % right_side.valh;
			default:
				break;
			}
			break;
		case '>':
			expression_result.type = BOOLEAN;
			switch (left_side.type) {
			case INTEGER:
				expression_result.valb =
					left_side.vali > right_side.vali;
				break;
			case DOUBLE:
				expression_result.valb =
					left_side.valf > right_side.valf;
				break;
			case STRING:
				fprintf(stderr,
					"Strings cannot bigger than each other.");
				return error;
				break;
			case BOOLEAN:
				fprintf(stderr,
					"Booleans cannot bigger than each other.");
				return error;
				break;
			case CHAR:
				fprintf(stderr,
					"Chars cannot bigger than each other.");
				return error;
				break;
			case LONG:
				expression_result.valb =
					left_side.valh > right_side.valh;
			default:
				break;
			}
			break;
		case '<':
			expression_result.type = BOOLEAN;
			switch (left_side.type) {
			case INTEGER:
				expression_result.valb =
					left_side.vali < right_side.vali;
				break;
			case DOUBLE:
				expression_result.valb =
					left_side.valf < right_side.valf;
				break;
			case STRING:
				fprintf(stderr,
					"Strings cannot less than each other.");
				return error;
				break;
			case BOOLEAN:
				fprintf(stderr,
					"Booleans cannot less than each other.");
				return error;
				break;
			case CHAR:
				fprintf(stderr,
					"Chars cannot less than each other.");
				return error;
				break;
			case LONG:
				expression_result.valb =
					left_side.valh < right_side.valh;
			default:
				break;
			}
			break;
		}
	}
	else {
		switch (op_type) {
		case T_SHRIGHT:
			switch (left_side.type) {
			case INTEGER:
				expression_result.vali =
					left_side.vali >> right_side.vali;
				break;
			case DOUBLE:
				fprintf(stderr,
					"Doubles cannot shift right each other.");
				return error;
				break;
			case STRING:
				fprintf(stderr,
					"Strings cannot shift right each other.");
				return error;
				break;
			case BOOLEAN:
				fprintf(stderr,
					"Booleans cannot shift right each other.");
				return error;
				break;
			case CHAR:
				fprintf(stderr,
					"Chars cannot shift right each other.");
				return error;
				break;
			case LONG:
				expression_result.valh =
					left_side.valh >> right_side.valh;
			default:
				break;
			}
			break;
		case T_SHLEFT:
			switch (left_side.type) {
			case INTEGER:
				expression_result.vali =
					left_side.vali << right_side.vali;
				break;
			case DOUBLE:
				fprintf(stderr,
					"Doubles cannot shift left each other.");
				return error;
				break;
			case STRING:
				fprintf(stderr,
					"Strings cannot shift left each other.");
				return error;
				break;
			case BOOLEAN:
				fprintf(stderr,
					"Booleans cannot shift left each other.");
				return error;

				break;
			case CHAR:
				fprintf(stderr,
					"Chars cannot shift left each other.");
				return error;

				break;
			case LONG:
				expression_result.valh =
					left_side.valh << right_side.valh;
			default:
				break;
			}
			break;
		case T_AND:
			expression_result.type = BOOLEAN;
			switch (left_side.type) {
			case INTEGER:
				expression_result.valb = left_side.vali
					&& right_side.vali;
				break;
			case DOUBLE:
				expression_result.valb = left_side.valf
					&& right_side.valf;
				break;
			case STRING:
				fprintf(stderr,
					"Strings cannot conditional and each other.");
				return error;

				break;
			case BOOLEAN:
				expression_result.valb = left_side.valb
					&& right_side.valb;
				break;
			case CHAR:
				expression_result.valb = left_side.valc
					&& right_side.valc;
				break;
			case LONG:
				expression_result.valb = left_side.valh
					&& right_side.valh;
			default:
				break;
			}
			break;
		case T_OR:
			expression_result.type = BOOLEAN;
			switch (left_side.type) {
			case INTEGER:
				expression_result.valb = left_side.vali
					|| right_side.vali;
				break;
			case DOUBLE:
				expression_result.valb = left_side.valf
					|| right_side.valf;
				break;
			case STRING:
				fprintf(stderr,
					"Strings cannot conditional or each other.");
				return error;

				break;
			case BOOLEAN:
				expression_result.valb = left_side.valb
					|| right_side.valb;
				break;
			case CHAR:
				expression_result.valb = left_side.valc
					|| right_side.valc;
				break;
			case LONG:
				expression_result.valb = left_side.valh
					|| right_side.valh;
			default:
				break;
			}
			break;
		case T_EQU:
			expression_result.type = BOOLEAN;
			switch (left_side.type) {
			case INTEGER:
				expression_result.valb =
					left_side.vali == right_side.vali;
				break;
			case DOUBLE:
				expression_result.valb =
					left_side.valf == right_side.valf;
				break;
			case STRING:{
					char           *left_string =
						sl_string_getter(left_side.
								 vals);
					char           *right_string =
						sl_string_getter(right_side.
								 vals);

					if (strcmp(left_string, right_string)
					    == 0)
						expression_result.valb = 1;
					else
						expression_result.valb = 0;

					free(left_string);
					free(right_string);
				}
				break;
			case BOOLEAN:
				expression_result.valb =
					left_side.valb == right_side.valb;
				break;
			case CHAR:
				expression_result.valb =
					left_side.valc == right_side.valc;
				break;
			case LONG:
				expression_result.valb =
					left_side.valh == right_side.valh;
			default:
				break;
			}
			break;
		case T_NEQ:
			expression_result.type = BOOLEAN;
			switch (left_side.type) {
			case INTEGER:
				expression_result.valb =
					left_side.vali != right_side.vali;
				break;
			case DOUBLE:
				expression_result.valb =
					left_side.valf != right_side.valf;
				break;
			case STRING:
				if (strcmp(left_side.vals, right_side.vals) !=
				    0)
					expression_result.valb = 1;
				else
					expression_result.valb = 0;
				break;
			case BOOLEAN:
				expression_result.valb =
					left_side.valb != right_side.valb;
				break;
			case CHAR:
				expression_result.valb =
					left_side.valc != right_side.valc;
				break;
			case LONG:
				expression_result.valb =
					left_side.valh != right_side.valh;
			default:
				break;
			}
			break;
		case T_EQG:
			expression_result.type = BOOLEAN;
			switch (left_side.type) {
			case INTEGER:
				expression_result.valb =
					left_side.vali >= right_side.vali;
				break;
			case DOUBLE:
				expression_result.valb =
					left_side.valf >= right_side.valf;
				break;
			case STRING:
				fprintf(stderr,
					"Strings cannot bigger than each other.");

				return error;

				break;
			case BOOLEAN:
				fprintf(stderr,
					"Booleans cannot bigger than each other.");
				return error;

				break;
			case CHAR:
				fprintf(stderr,
					"Chars cannot bigger than each other.");
				return error;

				break;
			case LONG:
				expression_result.valb =
					left_side.valh >= right_side.valh;
			default:
				break;
			}
			break;
		case T_EQL:
			expression_result.type = BOOLEAN;
			switch (left_side.type) {
			case INTEGER:
				expression_result.valb =
					left_side.vali <= right_side.vali;
				break;
			case DOUBLE:
				expression_result.valb =
					left_side.valf <= right_side.valf;
				break;
			case STRING:
				fprintf(stderr,
					"Strings cannot less than each other.");
				return error;

				break;
			case BOOLEAN:
				fprintf(stderr,
					"Booleans cannot less than each other.");
				return error;

				break;
			case CHAR:
				fprintf(stderr,
					"Chars cannot less than each other.");
				return error;
				break;
			case LONG:
				expression_result.valb =
					left_side.valh <= right_side.valh;
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

struct SL_Math_Splitter
{
	char            op;
	int             is_op_type;
	int             op_pos;
	enum TokenTypes op_type;
};

int             operator_checker(char *expressions[], enum TokenTypes *types,
				 int start, int end);


int
is_it_function_or_not(char **tokens, enum TokenTypes *types,
		      int current_token, int max_tokens)
{
	if (current_token < 0 || current_token + 2 >= max_tokens)
		return -1;

	int             depth = 0;

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



int
operator_checker(char *expressions[], enum TokenTypes *types, int start,
		 int end)
{
	if (end - start > 1) {
		return 1;
	}

	int             isitfunc =
		is_it_function_or_not(expressions, types, start, end);
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

int
prec_priority(char op, enum TokenTypes op_type, int is_op_type)
{
	if (is_op_type == 1) {
		if (op_type == T_SHRIGHT || op_type == T_SHLEFT) {
			return 6;
		}
		else if (op_type == T_EQU ||
			 op_type == T_EQG ||
			 op_type == T_EQL || op_type == T_NEQ) {
			return 2;
		}
		else if (op_type == T_AND || op_type == T_OR) {
			return 1;
		}
	}
	else {
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

int
is_has_func(struct SL_Code code, const char *name)
{
	unsigned long   target_hash = hash_string(name);
	for (int i = 0; i < code.total_funcs; i++) {
		if (code.funcs[i].hash == target_hash) {
			if (strcmp(code.funcs[i].name, name) == 0) {
				return 1;
			}
		}
	}
	return -1;
}

struct SL_Math_Splitter
expression_parser_splitter(struct SL_Code code, char *expression[],
			   enum TokenTypes *types, int current_token,
			   int max_tokens, int current_line)
{
	int             depth = 0;
	struct SL_Math_Splitter tree = { 0 };

	while (current_token < max_tokens) {
		if (is_has_func(code, expression[current_token]) == 1) {
			int             is_it_func =
				is_it_function_or_not(expression, types,
						      current_token,
						      max_tokens);
			if (is_it_func != -1) {
				current_token = is_it_func;
				continue;
			}
		}

		if (expression[current_token][0] == '(') {
			depth++;
		}
		else if (expression[current_token][0] == ')') {
			depth--;
		}

		if (depth < 0) {
			fprintf(stderr, "Error: One more ')' in %d line.",
				current_line);
			break;
		}

		if (depth > 0) {
			current_token++;
			continue;
		}
		enum TokenTypes op_type;
		int             optype = 0;
		char            op = expression[current_token][0];

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

		int             op_prec = prec_priority(op, op_type, optype);
		if (optype == 0 && strlen(expression[current_token]) > 1) {
			op_prec = 0;
		}
		if (op_prec != 0) {
			if (tree.op == 0) {
				tree.op = op;
				if (optype == 1) {
					tree.op_type = op_type;
					tree.is_op_type = 1;
				}
				else {
					tree.is_op_type = 0;
				}
				tree.op_pos = current_token;
			}
			if (op_prec <=
			    prec_priority(tree.op, tree.op_type,
					  tree.is_op_type)
			    && op_prec != 0) {
				tree.op = op;

				if (optype == 1) {
					tree.op_type = op_type;
					tree.is_op_type = 1;
				}
				else {
					tree.is_op_type = 0;
				}
				tree.op_pos = current_token;
			}
		}
		current_token++;
	}
	return tree;
}


char           *
get_raw_function_name(char *word)
{
	int             len = strlen(word);
	char           *returning_name = malloc(len + 1);
	int             j = 0;
	for (int i = 0; i < len; i++) {
		if (word[i] == '(') {
			return returning_name;
		}
		else {
			returning_name[j++] = word[i];
		}
	}
	returning_name[j] = '\0';
	return returning_name;
}


int
sl_where_is_next_comma(char **tokens, int current_token, int max_tokens)
{
	int             depth = 0;
	for (int i = current_token; i < max_tokens; i++) {
		if (tokens[i][0] == '(') {
			depth++;
		}
		else if (tokens[i][0] == ')') {
			if (depth == 0) {
				return i;
			}
			depth--;
		}
		else if (tokens[i][0] == ',') {
			if (depth == 0) {
				return i;
			}
		}
	}
	return -1;
}


struct SL_Variable
run_sl_function(struct SL_Code code, char *name,
		char **tokens, enum TokenTypes *types,
		int current_token, int max_tokens)
{

	int             function_number = -1;
	struct SL_Variable return_val;
	char           *rawfunc = get_raw_function_name(name);
	unsigned long   hash = hash_string(name);
	for (int i = 0; i < code.total_funcs; i++) {
		if (code.funcs[i].hash == hash) {
			if (strcmp(code.funcs[i].name, rawfunc) == 0) {
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


	struct SL_Function function = code.funcs[function_number];

	int             free_tracker = 0;
	struct SL_L_Function lfunc = { 0 };
	if (function.total_arguments > 0 || function.vaargs == 1
	    || function.linked_function == 1) {
		if (tokens[current_token + 1][0] != '(') {
			return_val.type = ERROR;
			return_val.vali = 1;
			return return_val;
		}
		if (tokens[current_token + 2][0] != ')') {
			current_token += 2;
			int             how_much_go = 0;
			int             vaargs_counter = 0;
			lfunc.argument_indexes = calloc(SL_INIT, sizeof(int));
			lfunc.starting_index = code.total_vars;

			while (current_token < code.token_count) {
				int             commapos =
					sl_where_is_next_comma(tokens,
							       current_token,
							       code.
							       token_count);
				if (commapos == -1) {
					return_val.type = ERROR;
					return_val.vali = 4;
					return return_val;
				}
				if (code.total_vars >= code.total_size_v) {
					return_val.type = ERROR;
					return_val.vali = 5;
					return return_val;
				}
				if (function.linked_function == 1) {
					lfunc.argument_indexes
						[lfunc.total_arguments]
						= code.total_vars;
					struct SL_Variable result =
						expression_parser_solver(code,
									 tokens,
									 types,
									 &current_token,
									 commapos,
									 0);

					code.vars[code.total_vars] = result;
					code.vars[code.total_vars].name =
						NULL;
					if (result.name != NULL)
						code.vars[code.total_vars].
							name =
							strdup(result.name);
					code.total_vars++;
					free_tracker++;
					lfunc.total_arguments++;
				}
				else {
					if (function.total_arguments <=
					    how_much_go
					    && function.vaargs == 1) {
						char           *function_name
							= malloc(SL_INIT);
						int             func_len =
							strlen(function.name);
						snprintf(function_name,
							 SL_INIT,
							 "%s_VA_ARGUMENT_%d",
							 function.name,
							 vaargs_counter);
						vaargs_counter++;
						struct SL_Variable result =
							expression_parser_solver
							(code,
							 tokens,
							 types,
							 &current_token,
							 commapos,
							 0);
						code.vars[code.total_vars] =
							result;
						code.vars[code.total_vars].
							name =
							strdup(function_name);
						code.vars[code.total_vars++].
							hash =
							hash_string
							(function_name);

						free_tracker++;
						free(function_name);

					}
					else {
						struct SL_Variable result =
							expression_parser_solver
							(code,
							 tokens,
							 types,
							 &current_token,
							 commapos,
							 0);
						code.vars[code.total_vars] =
							result;
						code.vars[code.total_vars].
							name =
							function.
							arguments
							[how_much_go].name;
						code.vars[code.total_vars++].
							hash =
							function.
							arguments
							[how_much_go].hash;

					}
				}
				current_token = commapos + 1;
				if (tokens[commapos][0] == ')') {
					break;
				}


				how_much_go++;
			}
		}
	}
	else if (function.total_arguments == 0) {
		while (current_token < max_tokens) {
			if (current_token != max_tokens - 1
			    && tokens[current_token + 1][0] == '('
			    && tokens[current_token + 2][0] == ')') {
				current_token += 2;
				break;
			}
			else {
				return_val.type = ERROR;
				return_val.vali = 2;
				return return_val;
			}
		}
	}

	if (function.linked_function == 1) {
		return_val = function.funcr(&code, lfunc);
	}
	else {
		struct SL_Code  code_def =
			{ function.code_tokens, function.types,
			function.code_len, code.vars,
			code.total_size_v,
			code.total_vars, code.funcs, code.total_size_f,
			code.total_funcs
		};
		return_val = sl_init_sl_parser(&code_def);
		current_token--;
	}
	for (int i = code.total_vars - 1; i >= code.total_vars - free_tracker;
	     i--) {
		free(code.vars[i].name);
		if ((code.vars[i].type == STRING
		     || code.vars[i].type == RETURN)
		    && code.vars[i].vals != NULL) {
			free(code.vars[i].vals);
		}
	}

	if (lfunc.argument_indexes != NULL)
		free(lfunc.argument_indexes);

	return return_val;
}


static struct SL_Variable
resolve_variable(struct SL_Code code_s,
		 char *expression[],
		 enum TokenTypes *types,
		 int current_token, int max_tokens, int old_curr)
{
	struct SL_Variable var =
		sl_word_to_var_converter(expression[current_token]);
	if (var.type != RETURN)
		return var;
	if (var.vals[0] == '$') {
		int             index =
			getvar_index_from_sl(code_s, var.vals + 1);
		free(var.vals);
		if (index == -1) {
			sl_throw_an_error(code_s,
					  expression,
					  old_curr,
					  max_tokens,
					  "VARIABLE NOT FOUND!",
					  "Expected: Define a variable first.");
		}
		var = code_s.vars[index];
		if ((var.type == STRING || var.type == RETURN) && var.vals)
			var.vals = strdup(var.vals);
		return var;
	}
	if (is_has_func(code_s, var.vals) == 1) {
		struct SL_Variable fn = run_sl_function(code_s, var.vals,
							expression, types,
							current_token,
							max_tokens);
		free(var.vals);
		if (fn.type == ERROR) {
			if (fn.vali == 0)
				sl_throw_an_error(code_s,
						  expression,
						  old_curr,
						  max_tokens,
						  "FUNCTION NOT FOUND!",
						  "Expected: Create a function first.");
			else if (fn.vali == 1)
				sl_throw_an_error(code_s,
						  expression,
						  old_curr,
						  max_tokens,
						  "OPENING PARENTHESIS '(' NOT FOUND!",
						  "Expected: <function_name>(<arguments?>)");
			else if (fn.vali == 2)
				sl_throw_an_error(code_s,
						  expression,
						  old_curr,
						  max_tokens,
						  "OPENING/CLOSING PARENTHESIS NOT FOUND!",
						  "Expected: <function_name>()");
			else if (fn.vali == 4)
				sl_throw_an_error(code_s,
						  expression,
						  old_curr,
						  max_tokens,
						  "CLOSING PARENTHESIS ')' OR COMMA ',' NOT FOUND!",
						  "Expected: <function_name>(<arguments?>, <arguments?>)");
			else if (fn.vali == 5)
				sl_throw_an_error(code_s,
						  expression,
						  old_curr,
						  max_tokens,
						  "STACK CALL OVERFLOW!",
						  "Expected: Use less recursion or arguments (TIP: While loop is a good alternative!)");


		}
		return fn;
	}

	sl_throw_an_error(code_s,
			  expression,
			  old_curr,
			  max_tokens,
			  "FUNCTION OR VARIABLE NOT FOUND",
			  "Expected: define a function or variable first.");

	return var;
}

struct SL_Variable
expression_parser_solver(struct SL_Code code_s,
			 char *expression[], enum TokenTypes *types,
			 int *current_token, int max_tokens, int current_line)
{
	struct SL_Variable empty = { 0 };
	if (!expression[*current_token])
		return empty;

	if (*current_token > max_tokens)
		return empty;

	while (expression[*current_token][0] == '(' &&
	       (expression[max_tokens - 1][0] == ')'
		|| expression[max_tokens][0] == ')')) {
		int             depth = 0;
		int             valid = 1;
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

	int             old_curr = *current_token;

	struct SL_Variable left = { 0 };
	struct SL_Variable right = { 0 };
	struct SL_Variable result = { 0 };
	struct SL_Math_Splitter tree =
		expression_parser_splitter(code_s, expression, types,
					   *current_token,
					   max_tokens,
					   current_line);

	if (tree.op == 0) {
		result = resolve_variable(code_s, expression, types,
					  *current_token, max_tokens,
					  old_curr);
		*current_token = max_tokens;
		return result;
	}

	int             left_pos_start = *current_token;
	int             left_pos_end = tree.op_pos;

	if (operator_checker(expression, types, left_pos_start, left_pos_end)
	    != 0) {
		left = expression_parser_solver(code_s, expression, types,
						&left_pos_start, left_pos_end,
						current_line);
	}
	else {
		left = resolve_variable(code_s, expression, types,
					left_pos_start, left_pos_end,
					old_curr);
	}

	int             right_pos_start = tree.op_pos + 1;
	int             right_pos_end = max_tokens;
	if (operator_checker
	    (expression, types, right_pos_start, right_pos_end) != 0) {
		right = expression_parser_solver(code_s, expression, types,
						 &right_pos_start,
						 right_pos_end, current_line);
	}
	else {
		right = resolve_variable(code_s, expression, types,
					 right_pos_start, right_pos_end,
					 old_curr);

	}

	result = expression_solver(left, tree.op, right, current_line,
				   tree.op_type, tree.is_op_type);
	if (result.type == ERROR) {
		sl_throw_an_error(code_s, expression, old_curr, max_tokens,
				  "Mathematical or logical operation error. (Type mismatch or invalid operation)",
				  "Ensure that the variables you are trying to operate on (String and Integer) are compatible with each other.");
	}
	*current_token = max_tokens;
	return result;
}

int
find_maxt_expr(char **code, enum TokenTypes *types, int starting, int max)
{
	for (int i = starting; i < max; i++) {
		int             isitfunc =
			is_it_function_or_not(code, types, i, max);
		if (isitfunc != -1) {
			i = isitfunc - 1;
		}

		if (i + 1 >= max)
			return max;

		if (operator_checker(code, types, i + 1, i + 2) == 1) {
			i++;
			continue;
		}
		else {
			if (code[i][0] == '(') {
				int             depth = 0;

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

				if (operator_checker
				    (code, types, i + 1, i + 2) != 1)
					return i;

				i++;
				continue;
			}


			if (operator_checker(code, types, i, i + 1) == 1) {
				isitfunc =
					is_it_function_or_not(code, types,
							      i + 1, max);
				if (isitfunc != -1) {
					i = isitfunc - 1;

					if (i + 1 >= max)
						return max;

					if (operator_checker
					    (code, types, i + 1, i + 2) != 1)
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

struct SL_Variable_Creator
{
	int             total_variables;
	struct SL_Variable *variable;
};

int
multi_variable_checker(char *tokens[], int current_token, int max_tokens)
{
	for (int i = current_token; i < max_tokens; i++) {
		if (tokens[i][0] == ',') {
			return i;
		}
	}
	return -1;
}

struct SL_Variable_Creator
variable_parser(struct SL_Code code_s, enum TokenTypes *types,
		char *tokens[], int *current_token,
		int max_tokens, int current_line)
{
	struct SL_Variable_Creator variables;
	variables.variable = malloc(sizeof(struct SL_Variable) * 1024);
	variables.total_variables = 0;
	while (current_token != NULL && *current_token < max_tokens) {
		if (tokens[(*current_token) + 1] == NULL && max_tokens < 3) {
			if (tokens[*current_token] != NULL) {
				variables.
					variable[variables.total_variables++].
					name =
					strdup(tokens[(*current_token)]);
			}
		}
		if (strcmp(tokens[*current_token], "=") == 0) {
			int             equal_start = *current_token + 1;
			int             value_end = 0;
			value_end = max_tokens;
			struct SL_Variable result =
				expression_parser_solver(code_s, tokens,
							 types,
							 &equal_start,
							 value_end,
							 current_line);
			variables.variable[variables.total_variables] =
				result;
			variables.variable[variables.total_variables].name =
				strdup(tokens[(*current_token) - 1]);
			variables.variable[variables.total_variables++].hash =
				hash_string(tokens[(*current_token) - 1]);
			*current_token = value_end;
		}
		// printf("TOKEN: %s\n", tokens[*current_token]);
		(*current_token)++;
	}
	return variables;
}

int
get_last_pos_of_token_for_expressions(char *token, char **tokens,
				      int current_token, int max_tokens)
{
	int             last_pos = -1;
	while (current_token < max_tokens) {
		if (tokens[current_token][0] == token[0]) {
			last_pos = current_token;
		}
		current_token++;
	}
	return last_pos;
}

int             is_has_token(char *token, char *tokens[],
			     int current_position, int max_tokens);

struct SL_Variable
assignment_parser(struct SL_Code code_s, char *tokens[],
		  enum TokenTypes *types, int *current_token, int max_tokens,
		  int current_line)
{
	struct SL_Variable error_var;
	error_var.vali = 0;
	while (current_token != NULL && *current_token < max_tokens) {
		int             last_pos =
			get_last_pos_of_token_for_expressions("=", tokens,
							      *current_token,
							      max_tokens);
		if (last_pos == -1) {
			error_var.vali = -2;
			break;
		}
		int             variable_pos = last_pos - 1;
		int             index =
			getvar_index_from_sl(code_s,
					     tokens[variable_pos] + 1);
		if (index == -1) {
			sl_throw_an_error(code_s, tokens, *current_token,
					  max_tokens,
					  "VARIABLE NOT FOUND ON ASSIGNMENT!",
					  "Expected: Define a variable first.");
		}
		struct SL_Variable eq_value = { 0 };
		int             last_pos_ptr_expr_start = last_pos + 1;
		eq_value =
			expression_parser_solver(code_s, tokens, types,
						 &last_pos_ptr_expr_start,
						 max_tokens, current_line);

		if ((code_s.vars[index].type == STRING
		     || code_s.vars[index].type == RETURN)
		    && code_s.vars[index].vals != NULL) {
			free(code_s.vars[index].vals);
		}

		eq_value.name = code_s.vars[index].name;
		code_s.vars[index] = eq_value;
		code_s.vars[index].hash =
			hash_string(code_s.vars[index].name);
		if (is_has_token("=", tokens, *current_token, last_pos) == 1) {
			struct SL_Variable out_var =
				assignment_parser(code_s, tokens, types,
						  current_token, last_pos,
						  current_line);
		}
		*current_token = max_tokens;
	}
	return error_var;
}

int
is_has_token(char *token, char *tokens[], int current_position,
	     int max_tokens)
{
	for (int i = current_position; i < max_tokens; i++) {
		if (strcmp(token, tokens[i]) == 0) {
			return 1;
		}
	}
	return -1;
}

int
sl_then_finder(char *tokens[], enum TokenTypes *types, int current_token,
	       int max_tokens, int *then_pos)
{
	int             start_pos = 0;

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

struct SL_Variable
sl_if_parser(struct SL_Code code_s, enum TokenTypes *types, char *tokens[],
	     int *current_token, int keyword_pos,
	     int max_tokens, int current_line)
{
	int             if_start_pos = 0;
	struct SL_Variable expr = { 0 };
	int             out =
		sl_then_finder(tokens, types, *current_token, max_tokens,
			       &if_start_pos);
	if (out == -1) {
		sl_throw_an_error(code_s, tokens, (*current_token) - 1,
				  max_tokens,
				  "'then' NOT FOUND ON 'if' usage.",
				  "if <expression> then <code> end");
	}
	expr = expression_parser_solver(code_s, tokens, types, current_token,
					if_start_pos, current_line);
	(*current_token) = if_start_pos;
	return expr;
}


struct SL_Function
sl_define_parser(struct SL_Code code_s, char *tokens[],
		 enum TokenTypes *types, int *current_token, int max_tokens)
{
	int             then_pos;
	sl_then_finder(tokens, types, *current_token, max_tokens, &then_pos);
	// logic
	// def xx -> argxx,argxx,argxx then <code> end

	int             current = *current_token;

	char           *f_name = tokens[current++];
	int             f_name_len = strlen(f_name);
	struct SL_Function function;
	function.name = malloc(f_name_len + 1);
	function.name[f_name_len] = '\0';
	strncpy(function.name, f_name, f_name_len);
	function.hash = hash_string(function.name);
	function.total_arguments = 0;
	int             starting = 0;

	if (tokens[current][0] == '-' && tokens[++current][0] == '>') {
		current++;
		function.arguments = calloc(1024, sizeof(struct SL_Variable));
		if (tokens[current][0] == ',')
			return function;
		while (types[current] != T_THEN) {
			if (strcmp(tokens[current], "...") == 0) {
				if (!types[current + 1]
				    || types[current + 1] == T_THEN)
					sl_throw_an_error(code_s, tokens,
							  *current_token,
							  max_tokens,
							  "EXPECTED 'then' AFTER '...'",
							  "Expected: def <function_name> -> <argument_name>, ... then <code> end");
				function.vaargs = 1;
				current++;
				break;
			}
			if (tokens[current][0] == ',') {
				current++;
				continue;
			}
			int             j = function.total_arguments;
			int             len = strlen(tokens[current]);
			function.arguments[j].name = malloc(len + 1);
			strncpy(function.arguments[j].name, tokens[current],
				len);
			function.arguments[j].name[len] = '\0';
			function.arguments[j].hash =
				hash_string(function.arguments[j].name);
			function.total_arguments++;
			current++;
		}
	}



	if (current == then_pos) {
		starting = ++current;
		int             end_depth = 0;
		int             code_length = 0;
		while (current < max_tokens) {

			if (types[current] == T_IF
			    || types[current] == T_DEF
			    || types[current] == T_WHILE)
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
		function.types =
			malloc(code_length * sizeof(enum TokenTypes));
		function.code_len = code_length;
		for (int i = 0; i < code_length; i++) {
			int             len = strlen(tokens[starting + i]);
			function.code_tokens[i] = malloc(len + 1);
			strncpy(function.code_tokens[i], tokens[starting + i],
				len);
			function.code_tokens[i][len] = '\0';
			function.types[i] = types[starting + i];
		}
	}

	*current_token = current;
	return function;
}

struct Loops
{
	int             depth;
	int            *back_pos;
	int             capacity;
	int            *end;
};


int
sl_find_end(char **tokens, enum TokenTypes *types, int start, int max_tokens,
	    int branch)
{
	int             depth = 0;

	for (int i = start; i < max_tokens; i++) {

		if (types[i] == T_IF ||
		    types[i] == T_WHILE || types[i] == T_DEF) {
			depth++;
		}
		else if (types[i] == T_END) {
			if (depth == 0)
				return i;

			depth--;
		}
		else if (branch == 1 && depth == 0
			 && (types[i] == T_ELSE || types[i] == T_ELIF)) {
			return i;
		}
	}

	return -1;
}


void
identifier_tokenizer(struct SL_Code *code)
{
	for (int i = 0; i < code->token_count; i++) {
		if (strcmp(code->code[i], "if") == 0) {
			code->types[i] = T_IF;
		}
		else if (strcmp(code->code[i], "while") == 0) {
			code->types[i] = T_WHILE;
		}
		else if (strcmp(code->code[i], "def") == 0) {
			code->types[i] = T_DEF;
		}
		else if (strcmp(code->code[i], "end") == 0) {
			code->types[i] = T_END;
		}
		else if (strcmp(code->code[i], "then") == 0) {
			code->types[i] = T_THEN;
		}
		else if (strcmp(code->code[i], "var") == 0) {
			code->types[i] = T_VAR;
		}
		else if (strcmp(code->code[i], "else") == 0) {
			code->types[i] = T_ELSE;
		}
		else if (strcmp(code->code[i], "elif") == 0) {
			code->types[i] = T_ELIF;
		}
		else if (strcmp(code->code[i], "break") == 0) {
			code->types[i] = T_BREAK;
		}
		else if (strcmp(code->code[i], "continue") == 0) {
			code->types[i] = T_CONTINUE;
		}
		else if (strcmp(code->code[i], "import") == 0) {
			code->types[i] = T_IMPORT;
		}
		else if (strcmp(code->code[i], "return") == 0) {
			code->types[i] = T_RETURN;
		}
		else if (strcmp(code->code[i], "equ") == 0) {
			code->types[i] = T_EQU;
		}
		else if (strcmp(code->code[i], "neq") == 0) {
			code->types[i] = T_NEQ;
		}
		else if (strcmp(code->code[i], "eqg") == 0) {
			code->types[i] = T_EQG;
		}
		else if (strcmp(code->code[i], "eql") == 0) {
			code->types[i] = T_EQL;
		}
		else if (strcmp(code->code[i], "and") == 0) {
			code->types[i] = T_AND;
		}
		else if (strcmp(code->code[i], "or") == 0) {
			code->types[i] = T_OR;
		}
		else if (strcmp(code->code[i], "<<") == 0) {
			code->types[i] = T_SHLEFT;
		}
		else if (strcmp(code->code[i], ">>") == 0) {
			code->types[i] = T_SHRIGHT;
		}

	}
}


struct SL_Variable
sl_init_sl_parser(struct SL_Code *code_s)
{

	struct Loops    while_loop = { 0 };
	while_loop.depth = 0;
	while_loop.back_pos = calloc(SL_INIT, sizeof(int));
	while_loop.end = calloc(SL_INIT, sizeof(int));
	while_loop.capacity = SL_INIT;
	int             while_sit = 0;
	struct SL_Variable return_val = { 0 };
	int             brk_sit = 0;
	int             depth = 0;
	identifier_tokenizer(code_s);
	int             max_tokens = code_s->token_count;
	for (int current_token = 0; current_token < code_s->token_count;
	     current_token++) {
		if (while_sit == 1) {
			if (code_s->types[current_token] == T_END) {
				if (while_loop.end[while_loop.depth - 1] ==
				    current_token)
					current_token =
						while_loop.back_pos
						[while_loop.depth - 1];
			}
			if (while_loop.depth == 0) {
				while_sit = 0;
			}
		}

		int             end = 0;

		if (code_s->types[current_token] == T_VAR) {
			for (int i = current_token; i < code_s->token_count;
			     i++) {
				if (code_s->code[i][0] == '=') {
					i++;
					end = find_maxt_expr(code_s->code,
							     code_s->types, i,
							     code_s->token_count);
					break;
				}
			}

			current_token++;
			struct SL_Variable_Creator vars =
				variable_parser(*code_s, code_s->types,
						code_s->code,
						&current_token, end,
						0);

			for (int i = 0; i < vars.total_variables; i++) {
				int             index =
					getvar_index_from_sl(*code_s,
							     vars.variable
							     [i].name);
				if (index != -1) {
					free(code_s->vars[index].name);
					if ((code_s->vars[index].type ==
					     STRING
					     || code_s->vars[index].type ==
					     RETURN)
					    && code_s->vars[index].vals !=
					    NULL) {
						free(code_s->vars[index].
						     vals);
						code_s->vars[index].vals =
							NULL;
					}
					code_s->vars[index] =
						vars.variable[i];
				}
				else {
					code_s->vars[code_s->total_vars++] =
						vars.variable[i];
				}
				if (code_s->total_vars ==
				    code_s->total_size_v - 1) {
					code_s->total_size_v *= 2;
					code_s->vars =
						realloc(code_s->vars,
							code_s->total_size_v *
							sizeof(struct
							       SL_Variable));
				}
			}
			free(vars.variable);
			current_token--;
		}
		else if (code_s->types[current_token] == T_IF) {
			current_token++;

			struct SL_Variable out_boolean =
				sl_if_parser(*code_s, code_s->types,
					     code_s->code,
					     &current_token,
					     current_token,
					     code_s->token_count, 0);
			if (out_boolean.valb == 0) {
				while (1) {
					int             out =
						sl_find_end(code_s->code,
							    code_s->types,
							    current_token,
							    code_s->token_count,
							    1);
					if (out == -1) {
						sl_throw_an_error(*code_s,
								  code_s->code,
								  current_token,
								  code_s->token_count,
								  "END NOT FOUND END OF THE IF",
								  "Expected: if <expr> then <code> else <code> end");
					}

					if (code_s->types[out] == T_END
					    || code_s->types[out] == T_ELSE) {
						current_token = out;
						break;
					}
					else if (code_s->types[out] == T_ELIF) {
						int             elif_tok =
							out + 1;
						struct SL_Variable
							elif_boolean =
							sl_if_parser(*code_s,
								     code_s->types,
								     code_s->code,
								     &elif_tok,
								     elif_tok,
								     code_s->token_count,
								     0);
						if (elif_boolean.valb == 1) {
							current_token =
								elif_tok;
							break;
						}
						else {
							current_token =
								elif_tok;
						}
					}
				}
				continue;
			}
			else {
				int             out =
					sl_find_end(code_s->code,
						    code_s->types,
						    current_token,
						    code_s->token_count,
						    1);
				if (out == -1) {
					sl_throw_an_error(*code_s,
							  code_s->code,
							  current_token,
							  code_s->token_count,
							  "END NOT FOUND END OF THE IF",
							  "Expected: if <expr> then <code> else <code> end");
				}


			}
		}
		else if (code_s->types[current_token] == T_WHILE) {
			int             currpos = current_token;
			current_token++;
			if (while_loop.depth >= while_loop.capacity) {
				while_loop.capacity *= 2;
				while_loop.back_pos =
					realloc(while_loop.back_pos,
						while_loop.capacity *
						sizeof(int));
				while_loop.end =
					realloc(while_loop.end,
						while_loop.capacity *
						sizeof(int));

			}

			struct SL_Variable out_boolean =
				sl_if_parser(*code_s, code_s->types,
					     code_s->code,
					     &current_token,
					     current_token,
					     code_s->token_count, 0);

			if (out_boolean.valb == 0) {
				if (while_loop.depth > 0
				    && while_loop.back_pos[while_loop.depth -
							   1] == currpos) {
					current_token =
						while_loop.
						end[--while_loop.depth];
				}
				else {
					current_token =
						sl_find_end(code_s->code,
							    code_s->types,
							    current_token,
							    code_s->token_count,
							    0);
				}
			}
			else {
				if (while_loop.depth == 0
				    || while_loop.back_pos[while_loop.depth -
							   1] != currpos) {

					while_loop.
						back_pos[while_loop.depth] =
						currpos;

					int             end =
						sl_find_end(code_s->code,
							    code_s->types,
							    current_token,
							    code_s->token_count,
							    0);

					if (end == -1) {
						sl_throw_an_error(*code_s,
								  code_s->code,
								  current_token,
								  code_s->token_count,
								  "END NOT FOUND END OF THE WHILE",
								  "Expected: while <expr> then <code> end");

					}
					while_loop.end[while_loop.depth++] =
						end;
				}
				while_sit = 1;
			}
		}
		else if (code_s->types[current_token] == T_CONTINUE) {
			if (while_sit != 1)
				sl_throw_an_error(*code_s, code_s->code,
						  current_token,
						  code_s->token_count,
						  "CONTINUE USAGE WITHOUT LOOP",
						  "Expected: define a loop first.");
			current_token =
				while_loop.back_pos[--while_loop.depth] - 1;

		}
		else if (code_s->types[current_token] == T_BREAK) {
			if (while_sit != 1)
				sl_throw_an_error(*code_s, code_s->code,
						  current_token,
						  code_s->token_count,
						  "BREAK USAGE WITHOUT LOOP",
						  "Expected: define a loop first.");
			current_token = while_loop.end[--while_loop.depth];
		}
		else if (code_s->types[current_token] == T_END) {
			continue;
		}
		else if (code_s->types[current_token] == T_ELSE
			 || code_s->types[current_token] == T_ELIF) {
			int             out =
				sl_find_end(code_s->code, code_s->types,
					    current_token + 1,
					    code_s->token_count, 0);
			if (out == -1)
				sl_throw_an_error(*code_s, code_s->code,
						  current_token,
						  code_s->token_count,
						  "END NOT FOUND END OF THE ELSE",
						  "Expected: if <expr> then <code> else <code> end");
			current_token = out;
			continue;
		}
		else if (code_s->types[current_token] == T_IMPORT) {
			if (!code_s->code[current_token + 1]) {
				sl_throw_an_error(*code_s, code_s->code,
						  current_token,
						  code_s->token_count,
						  "UNEXPECTED IMPORT SYNTAX!",
						  "Expected: import <file_name> (without string literal!)");
				exit(-1);
			}
			struct SL_Code  imported_code = sl_init_sl_process();
			if (sl_open_sl_process
			    (&imported_code,
			     code_s->code[current_token + 1]) != 0) {
				exit(-1);
			}
			if (imported_code.funcs != NULL) {
				for (int i = 0; i < imported_code.total_funcs;
				     i++) {
					code_s->funcs[code_s->total_funcs++] =
						imported_code.funcs[i];
				}
				free(imported_code.funcs);
			}
			if (imported_code.vars != NULL) {
				for (int i = 0; i < imported_code.total_vars;
				     i++) {
					code_s->vars[code_s->total_vars++] =
						imported_code.vars[i];
				}
				free(imported_code.vars);
			}


			if (imported_code.code != NULL) {
				for (int i = 0; i < imported_code.token_count;
				     i++) {
					if (imported_code.code[i] != NULL) {
						free(imported_code.code[i]);
					}
				}
				free(imported_code.code);
			}
			current_token++;
		}
		else if (code_s->types[current_token] == T_DEF) {
			current_token++;
			struct SL_Function func =
				sl_define_parser(*code_s, code_s->code,
						 code_s->types,
						 &current_token,
						 code_s->token_count);
			if (is_has_func(*code_s, func.name) == 1) {
				sl_throw_an_error(*code_s, code_s->code,
						  current_token, max_tokens,
						  "FUNCTION ALREADY EXISTS",
						  "Expected: try different name for your function");
			}
			code_s->funcs[code_s->total_funcs++] = func;
			if (code_s->total_funcs == code_s->total_size_f - 1) {
				code_s->total_size_f *= 2;
				code_s->funcs =
					realloc(code_s->funcs,
						code_s->total_size_f *
						sizeof(struct SL_Function));
			}
		}
		else if (code_s->types[current_token] == T_RETURN) {
			current_token++;
			end = find_maxt_expr(code_s->code, code_s->types,
					     current_token,
					     code_s->token_count);
			struct SL_Variable result =
				expression_parser_solver(*code_s,
							 code_s->code,
							 code_s->types,
							 &current_token, end,
							 0);
			free(while_loop.back_pos);
			free(while_loop.end);
			return result;
		}
		else {
			if (code_s->code[current_token][0] == '$') {
				int             old_curr = current_token;
				for (int i = current_token;
				     i < code_s->token_count; i++) {
					if (code_s->code[i][0] == '=') {
						i++;
						end = find_maxt_expr
							(code_s->code,
							 code_s->types, i,
							 code_s->token_count);
						break;
					}
				}
				struct SL_Variable out =
					assignment_parser(*code_s,
							  code_s->code,
							  code_s->types,
							  &current_token,
							  end, 0);
				if (out.vali == -2) {
					sl_throw_an_error(*code_s,
							  code_s->code,
							  old_curr,
							  old_curr + 2,
							  "ASSIGNMENT FAILED!",
							  "$<var_name> = <expression>");
					return_val.type = ERROR;
					free(while_loop.back_pos);
					free(while_loop.end);
					return return_val;
				}
			}
			else {
				end = find_maxt_expr(code_s->code,
						     code_s->types,
						     current_token,
						     code_s->token_count);


				struct SL_Variable result =
					expression_parser_solver(*code_s,
								 code_s->code,
								 code_s->types,
								 &current_token,
								 end,
								 0);
			}
		}
	}
	free(while_loop.back_pos);
	free(while_loop.end);
	return return_val;
}

struct SL_Code
sl_init_sl_process()
{
	struct SL_Code  code;
	code.funcs = calloc(SL_INIT, sizeof(struct SL_Function));
	code.total_size_f = SL_INIT;
	code.vars = calloc(SL_INIT, sizeof(struct SL_Variable));
	code.total_size_v = SL_INIT;
	code.total_funcs = 0;
	code.total_vars = 0;
	code.code = NULL;
	code.types = NULL;
	return code;
}

int
sl_open_sl_process(struct SL_Code *code, char *file_name)
{
	int             count =
		sl_init_sl_lexer(SL_INIT, file_name, &code->code,
				 SPECIAL_TOKENS);
	code->types = calloc(count, sizeof(enum TokenTypes));
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

struct SL_Variable
sl_dostr_sl_process(struct SL_Code *code_s, char *code)
{
	struct SL_Code  code_p = sl_init_sl_process();

	if (code_s->funcs != NULL) {
		for (int i = 0; i < code_s->total_funcs; i++) {
			if (code_p.total_funcs >= code_p.total_size_f) {
				code_p.total_size_f *= 2;
				code_p.funcs =
					realloc(code_p.funcs,
						code_p.total_size_f *
						sizeof(struct SL_Function));
			}
			code_p.funcs[code_p.total_funcs++] = code_s->funcs[i];
		}
	}

	if (code_s->vars != NULL) {
		for (int i = 0; i < code_s->total_vars; i++) {
			if (code_p.total_vars >= code_p.total_size_v) {
				code_p.total_size_v *= 2;
				code_p.vars =
					realloc(code_p.vars,
						code_p.total_size_v *
						sizeof(struct SL_Variable));
			}
			code_p.vars[code_p.total_vars++] = code_s->vars[i];
		}
	}

	code_p.code = malloc(1024 * sizeof(char *));
	int             count = LEXER(code, &code_p.code, strlen(code),
				      SPECIAL_TOKENS,
				      1024);
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
				code_s->funcs =
					realloc(code_s->funcs,
						code_s->total_size_f *
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
				code_s->vars =
					realloc(code_s->vars,
						code_s->total_size_v *
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

int
sl_close_sl_process(struct SL_Code *code)
{
	if (code->vars != NULL) {
		for (int i = 0; i < code->total_vars; i++) {
			if (code->vars[i].name != NULL) {
				free(code->vars[i].name);
				code->vars[i].name = NULL;
			}
			if ((code->vars[i].type == STRING
			     || code->vars[i].type == RETURN)
			    && code->vars[i].vals != NULL) {
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

			if (code->funcs[i].arguments != NULL
			    && code->funcs[i].total_arguments > 0) {
				for (int j = 0;
				     j < code->funcs[i].total_arguments;
				     j++) {
					if (code->funcs[i].arguments[j].
					    name != NULL) {
						free(code->funcs[i].
						     arguments[j].name);
						code->funcs[i].arguments[j].
							name = NULL;
					}
					if ((code->funcs[i].arguments[j].
					     type == STRING
					     || code->funcs[i].arguments[j].
					     type == RETURN)
					    && code->funcs[i].arguments[j].
					    vals != NULL) {
						free(code->funcs[i].
						     arguments[j].vals);
						code->funcs[i].arguments[j].
							vals = NULL;
					}
				}
				free(code->funcs[i].arguments);
				code->funcs[i].arguments = NULL;
			}

			if (code->funcs[i].code_tokens != NULL) {
				for (int k = 0; k < code->funcs[i].code_len;
				     k++) {
					if (code->funcs[i].code_tokens[k] !=
					    NULL) {
						free(code->funcs[i].
						     code_tokens[k]);
						code->funcs[i].code_tokens[k]
							= NULL;
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
	if (code->types != NULL) {
		free(code->types);
	}
	return 0;
}
