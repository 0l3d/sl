#include "sl.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPECIAL_TOKENS "$()+-/*%^&|=<>"
#define OPERATORS "*/+-%><&|^"

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
init_sl_lexer(char *bufin, char *bufout[], int max_count, char *special_tokens)
{
	if (bufin == NULL)
		return 0;
	int 		token_count = 0;
	const char     *p = bufin;
	while (*p != '\0') {
		if (*p == '#') {
			break;
		} else if (isspace(*p)) {
			p++;
			continue;
		} else if (*p == '"') {
			char 		in_string = *p;
			const char     *string_start = p++;
			while (*p && *p != in_string)
				p++;

			if (*p == in_string) {
				p++;
			}
			int 		stringlen = p - string_start;
			char           *in_string_tokens = malloc(stringlen + 1);
			strncpy(in_string_tokens, string_start, stringlen);
			in_string_tokens[stringlen] = '\0';
			bufout[token_count++] = in_string_tokens;
			if (*p)
				p++;
		} else {
			if ((*p == '>' && *(p + 1) == '>') ||
			    (*p == '<' && *(p + 1) == '<')) {
				char           *pot = malloc(3);
				pot[0] = *p;
				pot[1] = *(p + 1);
				pot[2] = '\0';
				bufout[token_count++] = pot;
				p += 2;
				continue;
			}
			if (lexer_special_tokens_ex(special_tokens, *p) > 0) {
				char           *pot = malloc(2);
				pot[0] = *p;
				pot[1] = '\0';
				bufout[token_count++] = pot;
				p++;
				continue;
			}
			const char     *word_start = p;
			while (*p && !isspace(*p) && lexer_special_tokens_ex(special_tokens, *p) == 0)
				p++;
			int 		word_len = p - word_start;
			char           *word = malloc(word_len + 1);
			strncpy(word, word_start, word_len);
			word[word_len] = '\0';
			bufout[token_count++] = word;
		}
		if (token_count >= max_count)
			break;
	}
	bufout[token_count] = NULL;
	return token_count;
}

struct SL_Variable
getvar_from_sl(struct SL_Code code, const char *name)
{
	for (int i = 0; code.vars[i].name != NULL; i++) {
		if (strcmp(code.vars[i].name, name) == 0) {
			return code.vars[i];
		}
	}
	struct SL_Variable empty = {0};
	return empty;
}

enum ConditionTYPE {
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

	long 		val_int = strtol(s, &endptr, 10);
	if (*endptr == '\0') {
		return 1;
	}
	double 		val_double = strtod(s, &endptr);
	if (*endptr == '\0') {
		return 2;
	}
	return 0;
}

int
string_checker(char *word)
{
	if (word[0] == '"') {
		int 		i = 0;
		while (word[i] != '"')
			i++;

		if (word[i] == '"')
			return 1;
	}
	return 0;
}

char           *
string_getter(char *word)
{
	int 		check = string_checker(word);
	if (check == 0) {
		fprintf(stderr, "Word isnt a string!\n");
	}
	int 		size = strlen(word);
	int 		new_size = size - 2;
	char           *our_word = malloc(new_size);
	for (int i = 0; i < size; i++) {
		our_word[i] = word[i + 1];
	}
	our_word[new_size] = '\0';
	return our_word;
}


enum SL_Types
type_analyzer(char *word)
{
	if (strchr(word, '"')) {
		return STRING;
	} else if (word[0] == '0' && word[1] == 'x') {
		return LONG;
	} else if (check_number(word) == 1) {
		return INTEGER;
	} else if (check_number(word) == 2) {
		return DOUBLE;
	} else if (strcmp(word, "false") == 0 || strcmp(word, "true") == 0) {
		return BOOLEAN;
	} else if (strchr(word, '\'')) {
		return CHAR;
	} else if (check_number(word) == 0 && !strchr(word, '"')) {
		return RETURN;
	}
	return -1;
}


struct SL_Variable
sl_word_to_var_converter(char *word)
{
	struct SL_Variable v;
	v.type = type_analyzer(word);
	if (v.type == INTEGER) {
		v.vali = atoi(word);
	} else if (v.type == DOUBLE) {
		v.valf = strtod(word, NULL);
	} else if (v.type == STRING) {
		v.vals = strdup(word);
	} else if (v.type == BOOLEAN) {
		if (strcmp(word, "false") == 0)
			v.valb = 0;
		if (strcmp(word, "true") == 0)
			v.valb = 1;
	} else if (v.type == CHAR) {
		v.valc = word[0];
	} else if (v.type == RETURN) {

	} else if (v.type == LONG) {
		int 		base = 0;
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

struct SL_Variable
expression_solver(struct SL_Variable left_side, char op, struct SL_Variable right_side, int current_line)
{
	if (left_side.type != right_side.type) {
		fprintf(stderr, "Expression types are not equal! Line: %d\n", current_line);
		exit(1);
	}
	struct SL_Variable expression_result;
	expression_result.type = left_side.type;
	switch (op) {
	case '+':
		switch (left_side.type) {
		case INTEGER:
			expression_result.vali = left_side.vali + right_side.vali;
			break;
		case DOUBLE:
			expression_result.valf = left_side.valf + right_side.valf;
			break;
		case STRING:
			char           *left_string = string_getter(left_side.vals);
			char           *right_string = string_getter(right_side.vals);
			size_t 		len = strlen(left_string) + strlen(right_string) + 1;
			expression_result.vals = malloc(len);
			snprintf(expression_result.vals, len,
				 "%s%s",
				 left_string,
				 right_string);
			free(left_string);
			free(right_string);
			free(left_side.vals);
			free(right_side.vals);
			break;
		case BOOLEAN:
			expression_result.valb = left_side.valb + right_side.valb;
			break;
		case CHAR:
			fprintf(stderr, "Chars cannot add each other. Line: %d\n", current_line);
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
			fprintf(stderr, "Strings cannot sub each other. Line: %d\n", current_line);
			break;
		case BOOLEAN:
			expression_result.valb = left_side.valb - right_side.valb;
			break;
		case CHAR:
			fprintf(stderr, "Chars cannot sub each other. Line: %d\n", current_line);
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
			fprintf(stderr, "Strings cannot mul each other. Line: %d\n", current_line);
			break;
		case BOOLEAN:
			expression_result.valb = left_side.valb * right_side.valb;
			break;
		case CHAR:
			fprintf(stderr, "Chars cannot mul each other. Line: %d\n", current_line);
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
			fprintf(stderr, "Strings cannot div each other. Line: %d\n", current_line);
			break;
		case BOOLEAN:
			expression_result.valb = left_side.valb / right_side.valb;
			break;
		case CHAR:
			fprintf(stderr, "Chars cannot div each other. Line: %d\n", current_line);
		default:
			break;
		}
		break;
	case '>':
		switch (left_side.type) {
		case INTEGER:
			expression_result.vali = left_side.vali >> right_side.vali;
			break;
		case DOUBLE:
			fprintf(stderr, "Doubles cannot shift right each other. Line: %d\n", current_line);
			break;
		case STRING:
			fprintf(stderr, "Strings cannot shift right each other. Line: %d\n", current_line);
			break;
		case BOOLEAN:
			fprintf(stderr, "Booleans cannot shift right each other. Line: %d\n", current_line);
			break;
		case CHAR:
			fprintf(stderr, "Chars cannot shift right each other. Line: %d\n", current_line);
		case LONG:
			expression_result.valh = left_side.valh >> right_side.valh;
		default:
			break;
		}
		break;
	case '<':
		switch (left_side.type) {
		case INTEGER:
			expression_result.vali = left_side.vali << right_side.vali;
			break;
		case DOUBLE:
			fprintf(stderr, "Doubles cannot shift left each other. Line: %d\n", current_line);
			break;
		case STRING:
			fprintf(stderr, "Strings cannot shift left each other. Line: %d\n", current_line);
			break;
		case BOOLEAN:
			fprintf(stderr, "Booleans cannot shift left each other. Line: %d\n", current_line);
			break;
		case CHAR:
			fprintf(stderr, "Chars cannot shift left each other. Line: %d\n", current_line);
		case LONG:
			expression_result.valh = left_side.valh << right_side.valh;
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
			fprintf(stderr, "Doubles cannot and each other. Line: %d\n", current_line);
			break;
		case STRING:
			fprintf(stderr, "Strings cannot and each other. Line: %d\n", current_line);
			break;
		case BOOLEAN:
			fprintf(stderr, "Booleans cannot and each other. Line: %d\n", current_line);
			break;
		case CHAR:
			fprintf(stderr, "Chars cannot and each other. Line: %d\n", current_line);
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
			fprintf(stderr, "Doubles cannot or each other. Line: %d\n", current_line);
			break;
		case STRING:
			fprintf(stderr, "Strings cannot or each other. Line: %d\n", current_line);
			break;
		case BOOLEAN:
			fprintf(stderr, "Booleans cannot or each other. Line: %d\n", current_line);
			break;
		case CHAR:
			fprintf(stderr, "Chars cannot or each other. Line: %d\n", current_line);
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
			fprintf(stderr, "Doubles cannot xor each other. Line: %d\n", current_line);
			break;
		case STRING:
			fprintf(stderr, "Strings cannot xor each other. Line: %d\n", current_line);
			break;
		case BOOLEAN:
			fprintf(stderr, "Booleans cannot xor each other. Line: %d\n", current_line);
			break;
		case CHAR:
			fprintf(stderr, "Chars cannot xor each other. Line: %d\n", current_line);
		case LONG:
			expression_result.valh = left_side.valh ^ right_side.valh;
		default:
			break;
		}
		break;
	}
	return expression_result;
}


struct SL_Math_Splitter {
	char 		op;
	int 		op_pos;
};

int
operator_checker(char *expressions[], int start, int end)
{
	char           *operators = OPERATORS;
	for (int i = start; i < end; i++) {
		for (int j = 0; j < strlen(operators); j++) {
			if (expressions[i][0] == operators[j]) {
				return 1;
			}
		}
	}
	return 0;
}

char
if_it_is_operator_then_return(char *word)
{
	switch (word[0]) {
		case '+':
		return '+';
	case '-':
		return '-';
	case '|':
		return '|';
	case '^':
		return '^';
	case '&':
		return '&';
	case '>':
		return '>';
	case '<':
		return '<';
	case '*':
		return '*';
	case '/':
		return '/';
	}
	return 0;
}

int
prec_priority(char op)
{
	switch (op) {
		case '|':
		return 1;
	case '^':
		return 2;
	case '&':
		return 3;
	case '>':
	case '<':
		return 4;
	case '+':
	case '-':
		return 5;
	case '*':
	case '/':
		return 6;
	}
	return 0;
}
struct SL_Math_Splitter
expression_parser_splitter(char *expression[], int current_token, int max_tokens, int current_line)
{
	int 		depth = 0;
	struct SL_Math_Splitter tree = {0, 0};

	while (current_token < max_tokens) {
		if (expression[current_token][0] == '(') {
			depth++;
		} else if (expression[current_token][0] == ')') {
			depth--;
		}
		if (depth < 0)
			fprintf(stderr, "Error: One more ')' in %d line.", current_line);

		if (depth > 0) {
			current_token++;
			continue;
		}
		char 		op = if_it_is_operator_then_return(expression[current_token]);
		if (op != 0) {
			if (tree.op == 0) {
				tree.op = op;
				tree.op_pos = current_token;
			}
			int 		op_prec = prec_priority(op);

			if (op_prec <= prec_priority(tree.op) && op_prec != 0) {
				tree.op = op;
				tree.op_pos = current_token;
			}
		}
		current_token++;
	}
	return tree;
}

struct SL_Variable
expression_parser_solver(char *expression[], int *current_token, int max_tokens, int current_line)
{
	while (expression[*current_token][0] == '(' &&
	       expression[max_tokens - 1][0] == ')') {
		int 		depth = 0;
		int 		valid = 1;
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

	struct SL_Variable left;
	struct SL_Variable right;
	struct SL_Variable result;
	struct SL_Math_Splitter tree = expression_parser_splitter(expression, *current_token, max_tokens, current_line);

	int 		left_pos_start = *current_token;
	int 		left_pos_end = tree.op_pos;
	if (operator_checker(expression, left_pos_start, left_pos_end) != 0) {
		left = expression_parser_solver(expression, &left_pos_start, left_pos_end, current_line);
	} else {
		left = sl_word_to_var_converter(expression[left_pos_start]);
	}

	if (expression[left_pos_start][0] == '(' &&
	    expression[left_pos_end - 1][0] == ')') {
	}
	int 		right_pos_start = tree.op_pos + 1;
	int 		right_pos_end = max_tokens;
	if (operator_checker(expression, right_pos_start, right_pos_end) != 0) {
		right = expression_parser_solver(expression, &right_pos_start, right_pos_end, current_line);
	} else {
		right = sl_word_to_var_converter(expression[right_pos_start]);
	}

	result = expression_solver(left, tree.op, right, current_line);
	*current_token = max_tokens;
	return result;
}

int
is_has_token(char *token, char *tokens[], int current_position, int max_tokens)
{
	for (int i = current_position; i < max_tokens; i++) {
		if (strcmp(token, tokens[i]) == 0) {
			return i;
		}
	}
	return -1;
}

int
sl_condition_parser(char *tokens[], int *current_token)
{
	enum ConditionTYPE condition_1;
	enum ConditionTYPE condition_2;
}

int
sl_if_parser(char *tokens[], int *current_token)
{
	return 0;
}

int
init_sl_parser(struct SL_Code code_s, int max_line, int max_token)
{
	int 		line = 0;
	while (line < max_line) {
		char           *tokens[max_token];
		int 		t_tokens = init_sl_lexer(code_s.code[line], tokens, max_token, SPECIAL_TOKENS);
		for (int current_token = 0; current_token < t_tokens; current_token++) {
			struct SL_Variable result = expression_parser_solver(tokens, &current_token, t_tokens, line + 1);
			printf("%d\n", result.vali);
		}

		line++;
	}

	return 1;
}
