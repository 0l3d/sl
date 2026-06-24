#include "sl.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPECIAL_TOKENS "()+-/*%^&|=<>,"
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
	for (int i = 0; i < code.total_vars; i++) {
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
		int size = strlen(word);
		for (int i = 1; i < size; i++) {
			if (word[i] == '"') {
				return 1;
			}
		}
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
	int j = 0; 
	for (int i = 1; i < size; i++) {
		if (word[i + 1] == '"') break;
		if (word[i] == '\\') {
			switch(word[i + 1]) {
			case 'n':
				our_word[j] = '\n';
				break;
			default:
				break;
			}
		} else {
			our_word[j] = word[i];
			j++;
		}
	}
	our_word[new_size] = '\0';
	return our_word;
}


enum SL_Types
type_analyzer(char *word)
{
	if (string_checker(word) == 1) {
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
	} else if (check_number(word) == 0 && string_checker(word) != 1) {
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
		v.valc = word[1];
	} else if (v.type == RETURN) {
		v.type = RETURN;
		v.vals = strdup(word);
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
expression_solver(struct SL_Variable left_side, char op, struct SL_Variable right_side, int current_line, char* op_str, int is_op_str)
{

	if ((left_side.type == DOUBLE && right_side.type == INTEGER) || (left_side.type == INTEGER && right_side.type == DOUBLE)) {
		left_side.type = DOUBLE;
		right_side.type = DOUBLE;
		right_side.valf = right_side.vali;
	}

	if (left_side.type != right_side.type) {
		fprintf(stderr, "Expression types are not equal! Line: %d\n", current_line);
		exit(1);
	}
	struct SL_Variable expression_result;
	expression_result.type = left_side.type;

	if (is_op_str != 1) {
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
				fprintf(stderr, "Strings cannot sub each other. Line: %d\n", current_line);
				break;
			case BOOLEAN:
				expression_result.valb = left_side.valb - right_side.valb;
				break;
			case CHAR:
				fprintf(stderr, "Chars cannot sub each other. Line: %d\n", current_line);
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
				fprintf(stderr, "Strings cannot mul each other. Line: %d\n", current_line);
				break;
			case BOOLEAN:
				expression_result.valb = left_side.valb * right_side.valb;
				break;
			case CHAR:
				fprintf(stderr, "Chars cannot mul each other. Line: %d\n", current_line);
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
				fprintf(stderr, "Strings cannot div each other. Line: %d\n", current_line);
				break;
			case BOOLEAN:
				expression_result.valb = left_side.valb / right_side.valb;
				break;
			case CHAR:
				fprintf(stderr, "Chars cannot div each other. Line: %d\n", current_line);
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
				fprintf(stderr, "Doubles cannot mod each other. Line: %d\n", current_line);
				break;
			case STRING:
				fprintf(stderr, "Strings cannot mod each other. Line: %d\n", current_line);
				break;
			case BOOLEAN:
				fprintf(stderr, "Booleans cannot mod each other. Line: %d\n", current_line);
				break;
			case CHAR:
				fprintf(stderr, "Chars cannot mod each other. Line: %d\n", current_line);
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
				fprintf(stderr, "Strings cannot bigger than each other. Line: %d\n", current_line);
				break;
			case BOOLEAN:
				fprintf(stderr, "Booleans cannot bigger than each other. Line: %d\n", current_line);
				break;
			case CHAR:
				fprintf(stderr, "Chars cannot bigger than each other. Line: %d\n", current_line);
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
				fprintf(stderr, "Strings cannot less than each other. Line: %d\n", current_line);
				break;
			case BOOLEAN:
				fprintf(stderr, "Booleans cannot less than each other. Line: %d\n", current_line);
				break;
			case CHAR:
				fprintf(stderr, "Chars cannot less than each other. Line: %d\n", current_line);
				break;
			case LONG:
				expression_result.valb = left_side.valh < right_side.valh;
			default:
				break;
			}
			break;
		}
	} else {
		if (strcmp(op_str, ">>") == 0) {
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
				break;
			case LONG:
				expression_result.valh = left_side.valh >> right_side.valh;
			default:
				break;
			}
		} else if (strcmp(op_str, "<<") == 0) {
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
				break;
			case LONG:
				expression_result.valh = left_side.valh << right_side.valh;
			default:
				break;
			}
		} else if (strcmp(op_str, "and") == 0) {
			expression_result.type = BOOLEAN;
			switch (left_side.type) {
			case INTEGER:
				expression_result.valb = left_side.vali && right_side.vali;
				break;
			case DOUBLE:
				expression_result.valb = left_side.valf && right_side.valf;
				break;
			case STRING:
				fprintf(stderr, "Strings cannot conditional and each other. Line: %d\n", current_line);
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
		} else if (strcmp(op_str, "or") == 0) {
			expression_result.type = BOOLEAN;
			switch (left_side.type) {
			case INTEGER:
				expression_result.valb = left_side.vali || right_side.vali;
				break;
			case DOUBLE:
				expression_result.valb = left_side.valf || right_side.valf;
				break;
			case STRING:
				fprintf(stderr, "Strings cannot conditional or each other. Line: %d\n", current_line);
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
		} else if (strcmp(op_str, "equ") == 0) {
			expression_result.type = BOOLEAN;
			switch (left_side.type) {
			case INTEGER:
				expression_result.valb = left_side.vali == right_side.vali;
				break;
			case DOUBLE:
				expression_result.valb = left_side.valf == right_side.valf;
				break;
			case STRING:
				if (strcmp(left_side.vals, right_side.vals) == 0)
					expression_result.valb = 1;
				else
					expression_result.valb = 0;
				break;
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
		} else if (strcmp(op_str, "neq") == 0) {
			expression_result.type = BOOLEAN;
			switch (left_side.type) {
			case INTEGER:
				expression_result.valb = left_side.vali != right_side.vali;
				break;
			case DOUBLE:
				expression_result.valb = left_side.valf != right_side.valf;
				break;
			case STRING:
				if (strcmp(left_side.vals, right_side.vals) != 0)
					expression_result.valb = 1;
				else
					expression_result.valb = 0;
				break;
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
		} else if (strcmp(op_str, "eqg") == 0) {
			expression_result.type = BOOLEAN;
			switch (left_side.type) {
			case INTEGER:
				expression_result.valb = left_side.vali >= right_side.vali;
				break;
			case DOUBLE:
				expression_result.valb = left_side.valf >= right_side.valf;
				break;
			case STRING:
				fprintf(stderr, "Strings cannot bigger than each other. Line: %d\n", current_line);
				break;
			case BOOLEAN:
				fprintf(stderr, "Booleans cannot bigger than each other. Line: %d\n", current_line);
				break;
			case CHAR:
				fprintf(stderr, "Chars cannot bigger than each other. Line: %d\n", current_line);
				break;
			case LONG:
				expression_result.valb = left_side.valh >= right_side.valh;
			default:
				break;
			}
		} else if (strcmp(op_str, "eql") == 0) {
			expression_result.type = BOOLEAN;
			switch (left_side.type) {
			case INTEGER:
				expression_result.valb = left_side.vali <= right_side.vali;
				break;
			case DOUBLE:
				expression_result.valb = left_side.valf <= right_side.valf;
				break;
			case STRING:
				fprintf(stderr, "Strings cannot less than each other. Line: %d\n", current_line);
				break;
			case BOOLEAN:
				fprintf(stderr, "Booleans cannot less than each other. Line: %d\n", current_line);
				break;
			case CHAR:
				fprintf(stderr, "Chars cannot less than each other. Line: %d\n", current_line);
			case LONG:
				expression_result.valb = left_side.valh <= right_side.valh;
			default:
				break;
			}
		} 
	}
	return expression_result;
}


struct SL_Math_Splitter {
	char 		op;
	char 		*op_str;
	int			is_op_str;
	int 		op_pos;
};

int
operator_checker(char *expressions[], int start, int end)
{
	if (end - start > 1) {
        return 1;
    }

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

int
prec_priority(char op, char* op_str, int is_op_str)
{
	if (is_op_str == 1) {
		if (strcmp(op_str, ">>") == 0 || strcmp(op_str, "<<") == 0) {
			return 6;
		} else if (strcmp(op_str, "equ") == 0 || strcmp(op_str, "eql") == 0 || strcmp(op_str, "neq") == 0 || strcmp(op_str, "eqg") == 0) {
			return 2;
		} else if (strcmp(op_str, "and") == 0 || strcmp(op_str, "or") == 0) {
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
expression_parser_splitter(char *expression[], int current_token, int max_tokens, int current_line)
{
	int 		depth = 0;
	struct SL_Math_Splitter tree = {0};

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
		int op_str = 0; 
		char *op_string = NULL;
		char op = expression[current_token][0];
		if (strlen(expression[current_token]) > 1) {
			op_str = 1;
			op_string = strdup(expression[current_token]);
		}

		int op_prec = prec_priority(op, op_string, op_str);
		if (op_prec != 0) {
			if (tree.op == 0) {
				tree.op = op;
				if (op_string != NULL) {
					tree.op_str = op_string;
					tree.is_op_str = 1;
				} else {
					tree.op_str = NULL;
					tree.is_op_str = 0;
				}
				tree.op_pos = current_token;
			}
			if (op_prec <= prec_priority(tree.op, tree.op_str, tree.is_op_str) && op_prec != 0) {
				tree.op = op;

				if (op_string != NULL) {
					tree.op_str = op_string;
					tree.is_op_str = 1;
				} else {
					tree.op_str = NULL;
					tree.is_op_str = 0;
				}
				tree.op_pos = current_token;
			}
		}
		
		current_token++;
	}
	return tree;
}

int
getvar_index_from_sl(struct SL_Code code, const char *name)
{
	for (int i = 0; i < code.total_vars; i++) {
		if (strcmp(code.vars[i].name, name) == 0) {
			return i;
		}
	}
	return 0;
}

char* raw_var_name(char *word) {
	const char *starting = word + 1;
	char *return_val = strdup(starting);
	return return_val;
}


struct SL_Variable
expression_parser_solver(struct SL_Code code_s, char *expression[], int *current_token, int max_tokens, int current_line)
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
if (tree.op == 0) {
	result = sl_word_to_var_converter(expression[*current_token]);
	if (result.type == RETURN) {
		if (result.vals[0] == '$') {	
			int index = getvar_index_from_sl(code_s, raw_var_name(result.vals));
			result = code_s.vars[index];
		}
	}

	return result;
}

int 		left_pos_start = *current_token;
int 		left_pos_end = tree.op_pos;
if (operator_checker(expression, left_pos_start, left_pos_end) != 0) {
	left = expression_parser_solver(code_s, expression, &left_pos_start, left_pos_end, current_line);
} else {
	left = sl_word_to_var_converter(expression[left_pos_start]);
	if (left.type == RETURN) {
		if (left.vals[0] == '$') {	
			int index = getvar_index_from_sl(code_s, raw_var_name(left.vals));
			left = code_s.vars[index];
		}
	}
}

if (expression[left_pos_start][0] == '(' &&
	expression[left_pos_end - 1][0] == ')') {
}
int 		right_pos_start = tree.op_pos + 1;
int 		right_pos_end = max_tokens;
if (operator_checker(expression, right_pos_start, right_pos_end) != 0) {
	right = expression_parser_solver(code_s, expression, &right_pos_start, right_pos_end, current_line);
} else {
	right = sl_word_to_var_converter(expression[right_pos_start]);
	if (right.type == RETURN) {
		if (right.vals[0] == '$') {	
			int index = getvar_index_from_sl(code_s, raw_var_name(right.vals));
			right = code_s.vars[index];
		}
	}
}

result = expression_solver(left, tree.op, right, current_line, tree.op_str, tree.is_op_str);
*current_token = max_tokens;
return result;
}

struct SL_Variable_Creator {
	int total_variables;
	struct SL_Variable *variable;
};

int
multi_variable_checker(char *tokens[], int current_token, int max_tokens) {
	for (int i = current_token; i < max_tokens; i++) {
		if (tokens[i][0] == ',') {
			return i;
		}
	}
	return -1;
}

struct SL_Variable_Creator 
variable_parser(struct SL_Code code_s, char *tokens[], int*current_token, int max_tokens, int current_line) {
	struct SL_Variable_Creator variables;
	variables.variable = malloc(sizeof(struct SL_Variable) * 1024);
	variables.total_variables  = 0;
	while (current_token != NULL && *current_token < max_tokens) {
		if (strcmp(tokens[*current_token], ",") == 0) {
			if (tokens[(*current_token) - 2] != NULL && strcmp(tokens[(*current_token) - 2], "=") != 0) {
				variables.variable[variables.total_variables++].name = strdup(tokens[(*current_token) - 1]);
			}
		}
		if (tokens[(*current_token) + 1] == NULL && max_tokens < 3) {
			if (tokens[*current_token] != NULL) {
				variables.variable[variables.total_variables++].name = strdup(tokens[(*current_token)]);
			}
		}
		if (strcmp(tokens[*current_token], "=") == 0) {
			int equal_start = *current_token + 1;	
			int value_end = 0;
			int out = multi_variable_checker(tokens, *current_token, max_tokens);
			if (out != -1) {
				value_end = out;
			} else {
				value_end = max_tokens;
			}
			struct SL_Variable result = expression_parser_solver(code_s, tokens, &equal_start, value_end, current_line);
			variables.variable[variables.total_variables] = result;
			variables.variable[variables.total_variables++].name = strdup(tokens[(*current_token) - 1]);
			*current_token = value_end;
		}
		// printf("TOKEN: %s\n", tokens[*current_token]);
		(*current_token)++;
	}
	return variables;
}

int
get_last_pos_of_token_for_expressions(char *token, char **tokens, int current_token, int max_tokens) {
	int last_pos = -1;
	while (current_token < max_tokens) {
		if (tokens[current_token][0] == token[0]) {
			last_pos = current_token;
		}
		current_token++;
	}
	return last_pos;
}


int is_has_token(char *token, char *tokens[], int current_position, int max_tokens);

struct SL_Variable
assignment_parser(struct SL_Code code_s,char* tokens[], int *current_token, int max_tokens, int current_line) {
	struct SL_Variable error_var;
	while (current_token != NULL && *current_token < max_tokens) {
		int last_pos = get_last_pos_of_token_for_expressions("=", tokens, *current_token, max_tokens);
		if (last_pos == -1) {
			error_var.vali = -2;
			break;
		}
		int variable_pos = last_pos - 1;
		int comma_pos = get_last_pos_of_token_for_expressions(",", tokens, *current_token, max_tokens);
		if (comma_pos == -1) {
			comma_pos = max_tokens;
		} 	
		int index = getvar_index_from_sl(code_s, raw_var_name(tokens[variable_pos]));
		struct SL_Variable eq_value = {0};
		int last_pos_ptr_expr_start = last_pos + 1;
		eq_value = expression_parser_solver(code_s, tokens, &last_pos_ptr_expr_start, comma_pos, current_line);
		eq_value.name = code_s.vars[index].name;
		code_s.vars[index] = eq_value;
		if (is_has_token("=", tokens, *current_token, last_pos) == 1) {
			struct SL_Variable out_var = assignment_parser(code_s, tokens, current_token, last_pos, current_line);

		}	 
		*current_token = max_tokens;
	}
	return error_var;
} 

int
is_has_token(char *token, char *tokens[], int current_position, int max_tokens)
{
	for (int i = current_position; i < max_tokens; i++) {
		if (strcmp(token, tokens[i]) == 0) {
			return 1;
		}
	}
	return -1;
}

int
sl_condition_parser(char *tokens[], int current_token, int max_tokens, int *then_pos, int *end_pos)
{
	int if_start_pos = 0; 
	int if_end_pos = 0; 

	while (current_token < max_tokens) {
	 	if (strcmp(tokens[current_token], "then") == 0) {
			if_start_pos = current_token;
		} else if (strcmp(tokens[current_token], "end") == 0) {	
			if_end_pos = current_token;
		}
		current_token++;
	}
	*then_pos = if_start_pos;
	*end_pos = if_end_pos;
	return 0;
}

struct SL_Variable
sl_if_parser(struct SL_Code code_s, char *tokens[], int *current_token, int keyword_pos, int max_tokens, int current_line, int max_line) {
	int if_start_pos = 0; 
	int if_end_pos = 0; 
	struct SL_Variable expr = {0};
	while (current_token != NULL && *current_token < max_tokens) {
		sl_condition_parser(tokens, *current_token, max_tokens, &if_start_pos, &if_end_pos);
		expr = expression_parser_solver(code_s, tokens, current_token, if_start_pos, current_line);
		(*current_token)++;
	}
	return expr;
}

int
init_sl_parser(struct SL_Code code_s, int max_line, int max_token)
{
	int 		line = 0;
	while (line < max_line) {
		char           *tokens[max_token];
		int 		t_tokens = init_sl_lexer(code_s.code[line], tokens, max_token, SPECIAL_TOKENS);
		for (int current_token = 0; current_token < t_tokens; current_token++) {
			if (strcmp(tokens[current_token], "var") == 0) {
				current_token++;
				struct SL_Variable_Creator vars = variable_parser(code_s, tokens, &current_token, t_tokens, line);
				for (int i = 0; i < vars.total_variables; i++) {
					code_s.vars[code_s.total_vars++] = vars.variable[i];
				}
			} else if (strcmp(tokens[current_token], "if") == 0) {
				current_token++;
				struct SL_Variable a = sl_if_parser(code_s, tokens, &current_token, current_token, t_tokens, line, max_line);
				printf("A: %d\n", a.valb);
			} else {
				if (is_has_token("=", tokens, current_token, t_tokens) == 1) {
					struct SL_Variable out = assignment_parser(code_s, tokens, &current_token, t_tokens, line);
					if (out.vali == -2) {
						fprintf(stderr, "$ in no name, example usage: $<name of variable>, your usage: $<no name>\n");
						return -1;
					}
				} else {
					struct SL_Variable result = expression_parser_solver(code_s, tokens, &current_token, t_tokens, line + 1);
					switch (result.type) {
					case INTEGER:
						printf("%d", result.vali);
						break;
					case DOUBLE:
						printf("%f", result.valf);
						break;
					case STRING:
						if (strchr(result.vals, '"')) 
							printf("%s", string_getter(result.vals));
						else 
						printf("%s", result.vals);
						break;
					case BOOLEAN:
						printf("%d", result.valb);
						break;
					case CHAR:
						printf("%c", result.valc);
						break;
					case LONG:
						printf("%lu", result.valh);
						break;
					default:
						break;
					}
				}
			}
			// printf("TOKEN: %s\n", tokens[current_token]);
		}
		line++;
	}

	return 1;
}
