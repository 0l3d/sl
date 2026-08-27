#include "sl.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct SL_Variable
expression_parser_solver(struct SL_Code code_s,
			 char *expression[],
			 int *current_token, int max_tokens,
			 int current_line);


void
sl_throw_an_error(struct SL_Code code,
		  char **tokens,
		  int current_token,
		  int max_tokens, char *error_msg, char *expected_tip)
{
	fprintf(stderr, "%s\n", error_msg);
	fprintf(stderr, "Your code:");
	for (int i = current_token; i < max_tokens; i++) {
		printf("[%s]", tokens[i]);
		fflush(stdout);
	}
	fprintf(stderr, "\n%s\n", expected_tip);
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
		else if (*p == '"') {
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
	if (word[0] == '"') {
		int             size = strlen(word);
		for (int i = 1; i < size; i++) {
			if (word[i] == '"') {
				return 1;
			}
		}
	}
	return 0;
}

char           *
sl_string_getter(char *word)
{
	int             check = string_checker(word);
	if (check == 0) {
		fprintf(stderr, "Word isnt a string!\n");
	}
	int             size = strlen(word);
	char           *our_word = malloc(size - 1);
	int             j = 0;
	for (int i = 1; i < size; i++) {
		if (word[i] == '"')
			break;
		if (word[i] == '\\') {
			i++;
			switch (word[i]) {
			case 'n':
				our_word[j++] = '\n';
				break;
			default:
				break;
			}
		}
		else {
			our_word[j] = word[i];
			j++;
		}
	}
	our_word[j] = '\0';
	return our_word;
}

enum SL_Types
type_analyzer(char *word)
{
	if (string_checker(word) == 1) {
		return STRING;
	}
	else if (word[0] == '0' && word[1] == 'x') {
		return LONG;
	}
	else if (check_number(word) == 1) {
		return INTEGER;
	}
	else if (check_number(word) == 2) {
		return DOUBLE;
	}
	else if (strcmp(word, "false") == 0 || strcmp(word, "true") == 0) {
		return BOOLEAN;
	}
	else if (strchr(word, '\'')) {
		return CHAR;
	}
	else if (check_number(word) == 0 && string_checker(word) != 1) {
		return RETURN;
	}
	return -1;
}

struct SL_Variable
sl_word_to_var_converter(char *word)
{
	struct SL_Variable v = { 0 };
	v.type = type_analyzer(word);
	if (v.type == INTEGER) {
		v.vali = atoi(word);
	}
	else if (v.type == DOUBLE) {
		v.valf = strtod(word, NULL);
	}
	else if (v.type == STRING) {
		v.vals = strdup(word);
	}
	else if (v.type == BOOLEAN) {
		if (strcmp(word, "false") == 0)
			v.valb = 0;
		if (strcmp(word, "true") == 0)
			v.valb = 1;
	}
	else if (v.type == CHAR) {
		v.valc = word[1];
	}
	else if (v.type == RETURN) {
		v.type = RETURN;
		v.vals = strdup(word);
	}
	else if (v.type == LONG) {
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
	return v;
}

int
getvar_index_from_sl(struct SL_Code code, const char *name)
{
	for (int i = code.total_vars - 1; i >= 0; i--) {
		if (code.vars[i].name == NULL)
			continue;

		if (strcmp(code.vars[i].name, name) == 0) {
			return i;
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

int sl_add_var(struct SL_Code *code, struct SL_Variable var) {
	code->vars[code->total_vars] = var;
	code->vars[code->total_vars++].name = strdup(var.name);;
	return 0;

}

struct SL_Variable sl_get_var(struct SL_Code code, const char *name) {
	struct SL_Variable return_err = { 0 };
	return_err.type = ERROR;
	int index = getvar_index_from_sl(code, name);
	if (index == -1)
		return return_err;

	return code.vars[index];
}


struct SL_Function sl_get_func(struct SL_Code code, const char *name) {
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
		  int current_line, char *op_str, int is_op_str)
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
		fprintf(stderr, "Expression types are not equal!");
		return error;
	}
	struct SL_Variable expression_result = { 0 };
	expression_result.type = left_side.type;

	if (is_op_str != 1) {
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
						sl_string_getter(left_side.
								 vals);
					char           *right_string =
						sl_string_getter
						(right_side.vals);
					size_t          len =
						strlen(left_string) +
						strlen(right_string) + 1;
					expression_result.vals = malloc(len);
					snprintf(expression_result.vals, len,
						 "%s%s", left_string,
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
		if (strcmp(op_str, ">>") == 0) {
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
		}
		else if (strcmp(op_str, "<<") == 0) {
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
		}
		else if (strcmp(op_str, "and") == 0) {
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
		}
		else if (strcmp(op_str, "or") == 0) {
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
		}
		else if (strcmp(op_str, "equ") == 0) {
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
			case STRING:
				if (strcmp(left_side.vals, right_side.vals) ==
				    0)
					expression_result.valb = 1;
				else
					expression_result.valb = 0;
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
		}
		else if (strcmp(op_str, "neq") == 0) {
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
		}
		else if (strcmp(op_str, "eqg") == 0) {
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
		}
		else if (strcmp(op_str, "eql") == 0) {
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
		}
	}
	return expression_result;
}

struct SL_Math_Splitter
{
	char            op;
	char           *op_str;
	int             is_op_str;
	int             op_pos;
};

int             operator_checker(char *expressions[], int start, int end);


int
is_it_function_or_not(char **tokens, int current_token, int max_tokens)
{
	if (current_token < 0 || current_token + 2 >= max_tokens)
		return -1;

	int             depth = 0;

	if (tokens[current_token + 1][0] == '('
	    &&
	    operator_checker(tokens, current_token + 1,
			     current_token + 2) == 0) {
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
operator_checker(char *expressions[], int start, int end)
{
	if (end - start > 1) {
		return 1;
	}

	int             isitfunc =
		is_it_function_or_not(expressions, start, end);
	if (isitfunc != -1) {
		start = isitfunc;
	}

	char           *operators = OPERATORS;
	for (int i = start; i < end; i++) {
		for (int j = 0; j < strlen(operators); j++) {
			if (expressions[i] == NULL)
				return 0;

			if (strcmp(expressions[i], "equ") == 0 ||
			    strcmp(expressions[i], "eqg") == 0 ||
			    strcmp(expressions[i], "eql") == 0 ||
			    strcmp(expressions[i], "neq") == 0 ||
			    strcmp(expressions[i], "and") == 0 ||
			    strcmp(expressions[i], "or") == 0) {
				return 1;
			}
			if (expressions[i][0] == operators[j]) {
				return 1;
			}
		}
	}

	return 0;
}

int
prec_priority(char op, char *op_str, int is_op_str)
{
	if (is_op_str == 1) {
		if (strcmp(op_str, ">>") == 0 || strcmp(op_str, "<<") == 0) {
			return 6;
		}
		else if (strcmp(op_str, "equ") == 0
			 || strcmp(op_str, "eql") == 0
			 || strcmp(op_str, "neq") == 0
			 || strcmp(op_str, "eqg") == 0) {
			return 2;
		}
		else if (strcmp(op_str, "and") == 0
			 || strcmp(op_str, "or") == 0) {
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
is_has_func(struct SL_Code code, char *name)
{
	for (int i = 0; i < code.total_funcs; i++) {
		if (strcmp(code.funcs[i].name, name) == 0)
			return 1;
	}
	return -1;
}



struct SL_Math_Splitter
expression_parser_splitter(struct SL_Code code, char *expression[],
			   int current_token,
			   int max_tokens, int current_line)
{
	int             depth = 0;
	struct SL_Math_Splitter tree = { 0 };

	while (current_token < max_tokens) {
		if (is_has_func(code, expression[current_token]) == 1) {
			int             is_it_func =
				is_it_function_or_not(expression,
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
		int             op_str = 0;
		char           *op_string = NULL;
		char            op = expression[current_token][0];
		if (strlen(expression[current_token]) > 1) {
			op_str = 1;
			op_string = strdup(expression[current_token]);
		}

		int             op_prec =
			prec_priority(op, op_string, op_str);
		if (op_prec != 0) {
			if (tree.op == 0) {
				tree.op = op;
				if (op_string != NULL) {
					if (tree.op_str != NULL)
						free(tree.op_str);
					tree.op_str = strdup(op_string);
					tree.is_op_str = 1;
				}
				else {
					if (tree.op_str != NULL)
						free(tree.op_str);
					tree.op_str = NULL;
					tree.is_op_str = 0;
				}
				tree.op_pos = current_token;
			}
			if (op_prec <=
			    prec_priority(tree.op, tree.op_str,
					  tree.is_op_str)
			    && op_prec != 0) {
				tree.op = op;

				if (op_string != NULL) {
					if (tree.op_str != NULL)
						free(tree.op_str);
					tree.op_str = strdup(op_string);
					tree.is_op_str = 1;
				}
				else {
					if (tree.op_str != NULL)
						free(tree.op_str);
					tree.op_str = NULL;
					tree.is_op_str = 0;
				}
				tree.op_pos = current_token;
			}
		}
		if (op_str == 1)
			free(op_string);
		current_token++;
	}
	return tree;
}

char           *
raw_var_name(char *word)
{
	const char     *starting = word + 1;
	char           *return_val = strdup(starting);
	return return_val;
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

int
sl_is_has_already_var(struct SL_Code code, char *name)
{
	for (int i = 0; i < code.total_vars; i++) {
		if (strcmp(code.vars[i].name, name) == 0) {
			return i;
		}
	}
	return -1;
}

struct SL_Variable
run_sl_function(struct SL_Code code, char *name,
		char **tokens,
		int current_token, int max_tokens, int *how_big_func)
{

	int             function_number = -1;
	struct SL_Variable return_val;
	for (int i = 0; i < code.total_funcs; i++) {
		char *rawfunc = get_raw_function_name(name);
		if (strcmp(code.funcs[i].name, rawfunc) ==
		    0) {
			function_number = i;
			free(rawfunc);
			break;
		}
		free(rawfunc);
	}

	if (function_number == -1) {
		return_val.type = ERROR;
		return_val.vali = 0;
		return return_val;
	}


	struct SL_Function function = code.funcs[function_number];

	int free_tracker = 0;
	struct SL_L_Function lfunc = { 0 };
	if (function.total_arguments > 0 || function.vaargs == 1
	    || function.linked_function == 1) {
		if (tokens[current_token + 1][0] != '(') {
			return_val.type = ERROR;
			return_val.vali = 1;
			*how_big_func = current_token + 2;
			return return_val;
		}
		current_token += 2;
		int             how_much_go = 0;
		int             vaargs_counter = 0;
		lfunc.argument_indexs = calloc(SL_INIT, sizeof(int));
		lfunc.starting_index = code.total_vars;
		while (current_token < code.token_count) {
			int             commapos =
				sl_where_is_next_comma(tokens, current_token,
						       code.token_count);
			if (commapos == -1) {
				return_val.type = ERROR;
				return_val.vali = 4;
				(*how_big_func)++;
				return return_val;
			}
			if (code.total_vars >= code.total_size_v) {
				return_val.type = ERROR;
				return_val.vali = 5;
				(*how_big_func)++;
				return return_val;
			}
			if (function.linked_function == 1) {
				lfunc.argument_indexs[lfunc.total_arguments] =
					code.total_vars;
				char           *function_name =
					malloc(SL_INIT);
				int             func_len =
					strlen(function.name);
				snprintf(function_name, SL_INIT,
					 "%s_LINKED_ARG_%d", function.name,
					 lfunc.total_arguments);
				struct SL_Variable result =
					expression_parser_solver(code, tokens,
								 &current_token,
								 commapos, 0);
				code.vars[code.total_vars] = result;
				code.vars[code.total_vars++].name =
					strdup(function_name);
				free_tracker++;
				free(function_name);
				lfunc.total_arguments++;
			}
			else {
				if (function.total_arguments <= how_much_go
				    && function.vaargs == 1) {
					char           *function_name =
						malloc(SL_INIT);
					int             func_len =
						strlen(function.name);
					snprintf(function_name, SL_INIT,
						 "%s_VA_ARGUMENT_%d",
						 function.name,
						 vaargs_counter);
					vaargs_counter++;
					struct SL_Variable result =
						expression_parser_solver(code,
									 tokens,
									 &current_token,
									 commapos,
									 0);
					code.vars[code.total_vars] = result;
					code.vars[code.total_vars++].name =
						strdup(function_name);
					free_tracker++;
					free(function_name);

				}
				else {
					struct SL_Variable result =
						expression_parser_solver(code,
									 tokens,
									 &current_token,
									 commapos,
									 0);
					code.vars[code.total_vars] = result;
					code.vars[code.total_vars++].name =
						function.
						arguments[how_much_go].name;
				}
			}
			current_token = commapos + 1;
			*how_big_func = commapos + 1;
			if (tokens[commapos][0] == ')')
				break;

			how_much_go++;
		}
	}

	if (function.total_arguments == 0) {
		while (current_token < max_tokens) {
			if (tokens[current_token + 1][0] == '('
			    && tokens[current_token + 2][0] == ')') {
				current_token += 2;
				*how_big_func = current_token;
				break;
			}
			else {
				return_val.type = ERROR;
				return_val.vali = 2;
				*how_big_func = current_token + 1;
				return return_val;
			}
		}
	}
	if (function.linked_function == 1) {
		return_val = function.funcr(&code, lfunc);
		if (lfunc.argument_indexs != NULL)
			free(lfunc.argument_indexs);
	}
	else {
		struct SL_Code  code_def =
			{ function.code_tokens, function.code_len, code.vars,
	     code.total_size_v,
			code.total_vars, code.funcs, code.total_size_f,
				code.total_funcs
		};
		return_val = sl_init_sl_parser(&code_def);
		current_token--;
	}
	for (int i = code.total_vars - 1; i >= code.total_vars - free_tracker; i--) {
		free(code.vars[i].name);
	}

	return return_val;
}

struct SL_Variable
expression_parser_solver(struct SL_Code code_s,
			 char *expression[],
			 int *current_token, int max_tokens, int current_line)
{
	struct SL_Variable empty = { 0 };
	if (!expression[*current_token])
		return empty;

	if (*current_token > max_tokens)
		return empty;

	int             how_big_func = 0;
	while (expression[*current_token][0] == '(' &&
	       expression[max_tokens - 1][0] == ')') {
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

	struct SL_Variable left;
	struct SL_Variable right;
	struct SL_Variable result;
	struct SL_Math_Splitter tree =
		expression_parser_splitter(code_s, expression, *current_token,
					   max_tokens,
					   current_line);

	if (tree.op == 0) {
		result = sl_word_to_var_converter(expression[*current_token]);
		if (result.type == RETURN) {
			if (result.vals[0] == '$') {
				char           *rawname =
					raw_var_name(result.vals);
				int             index =
					getvar_index_from_sl(code_s, rawname);
				if (index == -1) {
					sl_throw_an_error(code_s,
							  expression,
							  old_curr,
							  how_big_func,
							  "VARIABLE NOT FOUND!",
							  "Expected: Define a variable first.");
				}
				if (result.vals != NULL)
					free(result.vals);
				result = code_s.vars[index];
				free(rawname);
			}
			else if (is_has_func(code_s, result.vals) == 1) {
				struct SL_Variable result_side_fn =
					run_sl_function(code_s, result.vals,
							expression,
							*current_token,
							max_tokens,
							&how_big_func);
				if (result_side_fn.type == ERROR) {
					if (result_side_fn.vali == 0)
						sl_throw_an_error(code_s,
								  expression,
								  old_curr,
								  how_big_func,
								  "FUNCTION NOT FOUND!",
								  "Expected: Create a function first.");
					else if (result_side_fn.vali == 1)
						sl_throw_an_error(code_s,
								  expression,
								  old_curr,
								  how_big_func,
								  "OPENING PARENTHESIS '(' NOT FOUND!",
								  "Expected: <function_name>(<arguments?>)");
					else if (result_side_fn.vali == 2)
						sl_throw_an_error(code_s,
								  expression,
								  old_curr,
								  how_big_func,
								  "OPENING/CLOSING PARENTHESIS NOT FOUND!",
								  "Expected: <function_name>()");
					else if (result_side_fn.vali == 4)
						sl_throw_an_error(code_s,
								  expression,
								  old_curr,
								  how_big_func,
								  "CLOSING PARENTHESIS ')' OR COMMA ',' NOT FOUND!",
								  "Expected: <function_name>(<arguments?>, <arguments?>)");
					else if (result_side_fn.vali == 5)
						sl_throw_an_error(code_s,
								  expression,
								  old_curr,
								  how_big_func,
								  "STACK CALL OVERFLOW!",
								  "Expected: Use less recursion or arguments (TIP: While loop is a good alternative!)");

				}

				if (result.vals != NULL)
					free(result.vals);
				result = result_side_fn;
			}
		}
		*current_token = max_tokens;
		if (tree.op_str != NULL) {
			free(tree.op_str);
			tree.op_str = NULL;
		}
		return result;
	}

	int             left_pos_start = *current_token;
	int             left_pos_end = tree.op_pos;

	if (operator_checker(expression, left_pos_start, left_pos_end) != 0) {
		left = expression_parser_solver(code_s, expression,
						&left_pos_start, left_pos_end,
						current_line);
	}
	else {
		left = sl_word_to_var_converter(expression[left_pos_start]);
		if (left.type == RETURN) {
			if (left.vals[0] == '$') {
				char           *rawname =
					raw_var_name(left.vals);
				int             index =
					getvar_index_from_sl(code_s, rawname);
				if (index == -1) {
					sl_throw_an_error(code_s,
							  expression,
							  old_curr,
							  how_big_func,
							  "VARIABLE NOT FOUND!",
							  "Expected: Define a variable first.");
				}

				if (left.vals != NULL)
					free(left.vals);
				left = code_s.vars[index];
				free(rawname);
			}
			else if (is_has_func(code_s, left.vals) == 1) {
				struct SL_Variable left_side_fn =
					run_sl_function(code_s, left.vals,
							expression,
							left_pos_start,
							max_tokens,
							&how_big_func);
				if (left.type == ERROR) {
					if (left.vali == 0)
						sl_throw_an_error(code_s,
								  expression,
								  old_curr,
								  how_big_func,
								  "FUNCTION NOT FOUND!",
								  "Expected: Create a function first.");
					else if (left.vali == 1)
						sl_throw_an_error(code_s,
								  expression,
								  old_curr,
								  how_big_func,
								  "CLOSING PARENTHESIS ')' NOT FOUND!",
								  "Expected: <function_name>(<arguments?>)");
					else if (left.vali == 2)
						sl_throw_an_error(code_s,
								  expression,
								  old_curr,
								  how_big_func,
								  "OPENING/CLOSING PARENTHESIS NOT FOUND!",
								  "Expected: <function_name>()");
					else if (left.vali == 4)
						sl_throw_an_error(code_s,
								  expression,
								  old_curr,
								  how_big_func,
								  "CLOSING PARENTHESIS ')' OR COMMA ',' NOT FOUND!",
								  "Expected: <function_name>(<arguments?>, <arguments?>)");

					else if (left.vali == 5)
						sl_throw_an_error(code_s,
								  expression,
								  old_curr,
								  how_big_func,
								  "STACK CALL OVERFLOW!",
								  "Expected: Use less recursion or arguments");



				}



				if (left.vals != NULL)
					free(left.vals);
				left = left_side_fn;
			}
		}
	}

	int             right_pos_start = tree.op_pos + 1;
	int             right_pos_end = max_tokens;
	if (operator_checker(expression, right_pos_start, right_pos_end) != 0) {
		right = expression_parser_solver(code_s, expression,
						 &right_pos_start,
						 right_pos_end, current_line);
	}
	else {
		right = sl_word_to_var_converter(expression[right_pos_start]);
		if (right.type == RETURN) {
			if (right.vals[0] == '$') {
				char           *rawname =
					raw_var_name(right.vals);
				int             index =
					getvar_index_from_sl(code_s, rawname);
				if (index == -1) {
					sl_throw_an_error(code_s,
							  expression,
							  old_curr,
							  how_big_func,
							  "VARIABLE NOT FOUND!",
							  "Expected: Define a variable first.");
				}

				if (right.vals != NULL)
					free(right.vals);
				right = code_s.vars[index];
				free(rawname);
			}
			else if (is_has_func(code_s, right.vals) == 1) {
				struct SL_Variable right_side_fn =
					run_sl_function(code_s, right.vals,
							expression,
							right_pos_start,
							max_tokens,
							&how_big_func);
				if (right.type == ERROR) {
					if (right.vali == 0)
						sl_throw_an_error(code_s,
								  expression,
								  old_curr,
								  how_big_func,
								  "FUNCTION NOT FOUND!",
								  "Expected: Create a function first.");
					else if (right.vali == 1)
						sl_throw_an_error(code_s,
								  expression,
								  old_curr,
								  how_big_func,
								  "CLOSING PARENTHESIS ')' NOT FOUND!",
								  "Expected: <function_name>(<arguments?>)");
					else if (right.vali == 2)
						sl_throw_an_error(code_s,
								  expression,
								  old_curr,
								  how_big_func,
								  "OPENING/CLOSING PARENTHESIS NOT FOUND!",
								  "Expected: <function_name>()");
					else if (right.vali == 4)
						sl_throw_an_error(code_s,
								  expression,
								  old_curr,
								  how_big_func,
								  "CLOSING PARENTHESIS ')' OR COMMA ',' NOT FOUND!",
								  "Expected: <function_name>(<arguments?>, <arguments?>)");
					else if (right.vali == 5)
						sl_throw_an_error(code_s,
								  expression,
								  old_curr,
								  how_big_func,
								  "STACK CALL OVERFLOW!",
								  "Expected: Use less recursion or arguments");



				}

				if (right.vals != NULL)
					free(right.vals);
				right = right_side_fn;
			}
		}
	}

	result = expression_solver(left, tree.op, right, current_line,
				   tree.op_str, tree.is_op_str);
	if (tree.op_str != NULL) {
		free(tree.op_str);
		tree.op_str = NULL;
	}
	if (result.type == ERROR) {
		printf("\nYour Code: ");
		for (int i = *current_token; i < max_tokens; i++) {
			printf("[%s]", expression[i]);
		}
		printf("\n");
	}
	*current_token = max_tokens;
	return result;
}

int
find_maxt_expr(char **code, int starting, int max)
{
	for (int i = starting; i < max; i++) {
		int             isitfunc =
			is_it_function_or_not(code, i, max);
		if (isitfunc != -1) {
			i = isitfunc - 1;
		}

		if (i + 1 >= max)
			return max;

		if (operator_checker(code, i + 1, i + 2) == 1) {
			i++;
			continue;
		}
		else {
			if (code[i][0] == '(') {
				int             depth = 0;
				for (int j = i + 1; j < max; j++) {
					if (code[j][0] == '(')
						depth++;
					if (code[j][0] == ')' && depth == 0) {
						i = j;
						break;
					}
					if (code[j][0] == ')')
						depth--;

				}
				continue;
			}
			if (operator_checker(code, i, i + 1) == 1) {
				isitfunc =
					is_it_function_or_not(code, i + 1,
							      max);
				if (isitfunc != -1) {
					i = isitfunc - 1;
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
variable_parser(struct SL_Code code_s,
		char *tokens[], int *current_token,
		int max_tokens, int current_line)
{
	struct SL_Variable_Creator variables;
	variables.variable = malloc(sizeof(struct SL_Variable) * 1024);
	variables.total_variables = 0;
	while (current_token != NULL && *current_token < max_tokens) {
		if (tokens[(*current_token) + 1] == NULL && max_tokens < 3) {
			if (tokens[*current_token] != NULL) {
				variables.variable[variables.
						   total_variables++].name =
					strdup(tokens[(*current_token)]);
			}
		}
		if (strcmp(tokens[*current_token], "=") == 0) {
			int             equal_start = *current_token + 1;
			int             value_end = 0;
			value_end = max_tokens;
			struct SL_Variable result =
				expression_parser_solver(code_s, tokens,
							 &equal_start,
							 value_end,
							 current_line);
			variables.variable[variables.total_variables] =
				result;
			variables.variable[variables.total_variables++].name =
				strdup(tokens[(*current_token) - 1]);
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
		  int *current_token, int max_tokens, int current_line)
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
		char           *varname = raw_var_name(tokens[variable_pos]);
		int             index = getvar_index_from_sl(code_s, varname);
		free(varname);
		struct SL_Variable eq_value = { 0 };
		int             last_pos_ptr_expr_start = last_pos + 1;
		eq_value =
			expression_parser_solver(code_s, tokens,
						 &last_pos_ptr_expr_start,
						 max_tokens, current_line);
		eq_value.name = code_s.vars[index].name;
		code_s.vars[index] = eq_value;
		if (is_has_token("=", tokens, *current_token, last_pos) == 1) {
			struct SL_Variable out_var =
				assignment_parser(code_s, tokens,
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
sl_then_finder(char *tokens[], int current_token, int max_tokens,
	       int *then_pos)
{
	int             start_pos = 0;

	while (current_token < max_tokens) {
		if (strcmp(tokens[current_token], "then") == 0) {
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
sl_if_parser(struct SL_Code code_s, char *tokens[],
	     int *current_token, int keyword_pos,
	     int max_tokens, int current_line)
{
	int             if_start_pos = 0;
	struct SL_Variable expr = { 0 };
	int             out =
		sl_then_finder(tokens, *current_token, max_tokens,
			       &if_start_pos);
	if (out == -1) {
		sl_throw_an_error(code_s, tokens, (*current_token) - 1,
				  max_tokens,
				  "'then' NOT FOUND ON 'if' usage.",
				  "if <expression> then <code> end");
	}
	expr = expression_parser_solver(code_s, tokens, current_token,
					if_start_pos, current_line);
	(*current_token) = if_start_pos;
	return expr;
}


struct SL_Function
sl_define_parser(struct SL_Code code_s, char *tokens[], int *current_token,
		 int max_tokens)
{
	int             then_pos;
	sl_then_finder(tokens, *current_token, max_tokens, &then_pos);
	// logic
	// def xx -> argxx,argxx,argxx then <code> end

	int             current = *current_token;

	char           *f_name = tokens[current++];
	int             f_name_len = strlen(f_name);
	struct SL_Function function;
	function.name = malloc(f_name_len + 1);
	function.name[f_name_len] = '\0';
	strncpy(function.name, f_name, f_name_len);
	function.total_arguments = 0;
	int             starting = 0;

	if (tokens[current][0] == '-' && tokens[++current][0] == '>') {
		current++;
		function.arguments = calloc(1024, sizeof(struct SL_Variable));
		if (tokens[current][0] == ',')
			return function;
		while (strcmp(tokens[current], "then") != 0) {
			if (strcmp(tokens[current], "...") == 0) {
				if (!tokens[current + 1]
				    || strcmp(tokens[current + 1],
					      "then") != 0)
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
			function.total_arguments++;
			current++;
		}
	}



	if (current == then_pos) {
		starting = ++current;
		int             end_depth = 0;
		int             code_length = 0;
		while (current < max_tokens) {

			if (strcmp(tokens[current], "if") == 0
			    || strcmp(tokens[current], "def") == 0
			    || strcmp(tokens[current], "while") == 0)
				end_depth++;

			if (strcmp(tokens[current], "end") == 0
			    && end_depth > 0) {
				end_depth--;
				current++;
				code_length++;
				continue;
			}

			if (strcmp(tokens[current], "end") == 0
			    && end_depth == 0)
				break;

			code_length++;
			current++;
		}
		function.code_tokens = malloc(code_length * sizeof(char *));
		function.code_len = code_length;
		for (int i = 0; i < code_length; i++) {
			int             len = strlen(tokens[starting + i]);
			function.code_tokens[i] = malloc(len + 1);
			strncpy(function.code_tokens[i], tokens[starting + i],
				len);
			function.code_tokens[i][len] = '\0';
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
};


struct SL_Variable
sl_init_sl_parser(struct SL_Code *code_s)
{

	struct Loops    while_loop = { 0 };
	while_loop.depth = 0;
	while_loop.back_pos = calloc(SL_INIT, sizeof(int));
	while_loop.capacity = SL_INIT;
	int             while_sit = 0;
	struct SL_Variable return_val = { 0 };
	int             if_situation = 0;
	int             depth_of_if_sit = 0;

	for (int current_token = 0; current_token < code_s->token_count;
	     current_token++) {


		if (if_situation == 1) {
			if (strcmp(code_s->code[current_token], "end") == 0)
				depth_of_if_sit--;

			if (strcmp(code_s->code[current_token], "if") == 0
			    || strcmp(code_s->code[current_token], "def") == 0
			    || strcmp(code_s->code[current_token],
				      "while") == 0)
				depth_of_if_sit++;

			if (depth_of_if_sit == 0)
				if_situation = 0;

			continue;
		}

		if (while_sit == 1) {
			if (strcmp(code_s->code[current_token], "end") == 0) {
				current_token =
					while_loop.back_pos[--while_loop.
							    depth];
			}
		}

		int             max_tokens = 0;
		int             end =
			find_maxt_expr(code_s->code, current_token,
				       code_s->token_count);


		if (strcmp(code_s->code[current_token], "var") == 0) {
			for (int i = current_token; i < code_s->token_count;
			     i++) {
				if (code_s->code[i][0] == '=') {
					i++;
					end = find_maxt_expr(code_s->code, i,
							     code_s->
							     token_count);
					break;
				}
			}

			current_token++;
			struct SL_Variable_Creator vars =
				variable_parser(*code_s, code_s->code,
						&current_token, end,
						0);

			for (int i = 0; i < vars.total_variables; i++) {
				int             index =
					getvar_index_from_sl(*code_s,
							     vars.variable[i].
							     name);
				if (index != -1) {
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
		else if (strcmp(code_s->code[current_token], "if") == 0) {
			current_token++;

			struct SL_Variable out_boolean =
				sl_if_parser(*code_s, code_s->code,
					     &current_token,
					     current_token,
					     code_s->token_count, 0);

			if (out_boolean.valb == 0) {
				depth_of_if_sit++;
				if_situation = 1;
			}
		}
		else if (strcmp(code_s->code[current_token], "while") == 0) {
			current_token++;
			while_loop.back_pos[while_loop.depth++] =
				current_token - 1;
			if (while_loop.depth >= while_loop.capacity) {
				while_loop.capacity *= 2;
				while_loop.back_pos =
					realloc(while_loop.back_pos,
						while_loop.capacity *
						sizeof(int));
			}

			struct SL_Variable out_boolean =
				sl_if_parser(*code_s, code_s->code,
					     &current_token,
					     current_token,
					     code_s->token_count, 0);

			if (out_boolean.valb == 0) {
				depth_of_if_sit++;
				if_situation = 1;
				while_loop.depth--;
			}
			else {
				while_sit = 1;
			}
		}
		else if (strcmp(code_s->code[current_token], "end") == 0) {
			continue;
		}
		else if (strcmp(code_s->code[current_token], "import") == 0) {
			if (!code_s->code[current_token + 1]) {
				sl_throw_an_error(*code_s, code_s->code,
						  current_token, end,
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

		}
		else if (strcmp(code_s->code[current_token], "def") == 0) {
			current_token++;
			struct SL_Function func =
				sl_define_parser(*code_s, code_s->code,
						 &current_token,
						 code_s->token_count);
			code_s->funcs[code_s->total_funcs++] = func;
			if (code_s->total_funcs == code_s->total_size_f - 1) {
				code_s->total_size_f *= 2;
				code_s->funcs =
					realloc(code_s->funcs,
						code_s->total_size_f *
						sizeof(struct SL_Function));
			}
		}
		else if (strcmp(code_s->code[current_token], "return") == 0) {
			current_token++;
			end = find_maxt_expr(code_s->code, current_token,
					     code_s->token_count);
			struct SL_Variable result =
				expression_parser_solver(*code_s,
							 code_s->code,
							 &current_token, end,
							 0);
			free(while_loop.back_pos);
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
							(code_s->code, i,
							 code_s->token_count);
						break;
					}
				}
				struct SL_Variable out =
					assignment_parser(*code_s,
							  code_s->code,
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
					return return_val;
				}
			}
			else {
				struct SL_Variable result =
					expression_parser_solver(*code_s,
								 code_s->code,
								 &current_token,
								 end,
								 0);
			}
		}
	}
	free(while_loop.back_pos);
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
	return code;
}

int
sl_open_sl_process(struct SL_Code *code, char *file_name)
{
	int             count =
		sl_init_sl_lexer(SL_INIT, file_name, &code->code,
				 SPECIAL_TOKENS);
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

int
sl_dostr_sl_process(struct SL_Code *code_s, char *code)
{
	code_s->code = malloc(1024 * sizeof(char *));
	int             count = LEXER(code, &code_s->code, strlen(code),
				      SPECIAL_TOKENS,
				      1024);

	code_s->token_count = count;

	struct SL_Variable init = sl_init_sl_parser(code_s);
	if (init.type == ERROR) {
		fprintf(stderr, "sl_parser failed \n");
		return -1;
	};
	return 0;

}

int
sl_close_sl_process(struct SL_Code *code)
{
	if (code->vars != NULL) {
		for (int i = 0; i < code->total_vars; i++) {
			if (code->vars[i].name != NULL) {
				free(code->vars[i].name);
			}
			if ((code->vars[i].type == STRING
			     || code->vars[i].type == RETURN)
			    && code->vars[i].vals != NULL) {
				free(code->vars[i].vals);
			}
		}
		free(code->vars);
	}

	if (code->funcs != NULL) {
		for (int i = 0; i < code->total_funcs; i++) {
			if (code->funcs[i].name != NULL) {
				free(code->funcs[i].name);
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
					}
					if ((code->funcs[i].arguments[j].
					     type == STRING
					     || code->funcs[i].arguments[j].
					     type == RETURN)
					    && code->funcs[i].arguments[j].
					    vals != NULL) {
						free(code->funcs[i].
						     arguments[j].vals);
					}
				}
				free(code->funcs[i].arguments);
			}

			if (code->funcs[i].code_tokens != NULL) {
				for (int k = 0; k < code->funcs[i].code_len;
				     k++) {
					if (code->funcs[i].code_tokens[k] !=
					    NULL) {
						free(code->funcs[i].
						     code_tokens[k]);
					}
				}
				free(code->funcs[i].code_tokens);
			}
		}
		free(code->funcs);
	}

	if (code->code != NULL) {
		for (int i = 0; i < code->token_count; i++) {
			if (code->code[i] != NULL) {
				free(code->code[i]);
			}
		}
		free(code->code);
	}
	return 0;

}
