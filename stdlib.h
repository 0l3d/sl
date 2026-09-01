#include "sl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

struct SL_Code *use_code = NULL;

struct SL_Variable
example_fn(struct SL_Code *code, struct SL_L_Function func)
{

	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

	return return_var;
}



char          **arguments = NULL;
int             argcN;

struct SL_List
{
	int             capacity;
	struct SL_Variable *vars;
	int             size;
	int             fixed;
	int 			current;
};


int             LISTS_count = 0;
int             LISTS_capacity = SL_INIT;
struct SL_List *LISTS = { 0 };


int create_new_list(int capacity, int fixed) {
	LISTS[LISTS_count].vars =
		calloc(capacity, sizeof(struct SL_Variable));
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

int
list_push(struct SL_List *list, struct SL_Variable value)
{
	if (list->size >= list->capacity) {
		list->capacity =
			(list->capacity == 0) ? 8 : list->capacity * 2;

		list->vars =
			realloc(list->vars,
				list->capacity * sizeof(struct SL_Variable));
	}

	list->vars[list->size++] = sl_copy_variable(value);

	return 1;
}

struct SL_Variable
list_pop(struct SL_List *list)
{
	struct SL_Variable ret =
		sl_copy_variable(list->vars[list->size - 1]);

	if (list->vars[list->size - 1].name != NULL) {
		free(list->vars[list->size - 1].name);
		list->vars[list->size - 1].name = NULL;
	}

	if ((list->vars[list->size - 1].type == STRING
	     || list->vars[list->size - 1].type == RETURN)
	    && list->vars[list->size - 1].vals != NULL) {
		free(list->vars[list->size - 1].vals);
		list->vars[list->size - 1].vals = NULL;
	}

	list->size--;

	return ret;
}

int
list_set(struct SL_List *list, int index, struct SL_Variable value)
{
	if (index < 0 || index >= list->size)
		return 0;

	if ((list->vars[index].type == STRING
	     || list->vars[index].type == RETURN)
	    && list->vars[index].vals != NULL) {
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

int
list_remove(struct SL_List *list, int index)
{
    if (index < 0 || index >= list->size)
        return 0;

    if ((list->vars[index].type == STRING
         || list->vars[index].type == RETURN)
        && list->vars[index].vals != NULL) {
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

// IO
struct SL_Variable
print_fn(struct SL_Code *code, struct SL_L_Function func)
{
	struct SL_Variable return_var = { 0 };
	for (int i = 0; i < func.total_arguments; i++) {
		return_var = sl_get_argument(*code, func, i);
		switch (return_var.type) {
		case INTEGER:
			printf("%d", return_var.vali);
			break;
		case DOUBLE:
			printf("%f", return_var.valf);
			break;
		case STRING:
			if (strchr(return_var.vals, '"')) {
				char           *string =
					sl_string_getter(return_var.vals);
				printf("%s", string);
				free(string);
			}
			else
				printf("%s", return_var.vals);

			break;
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
			printf("%lu", return_var.valh);
			break;
		default:
			break;
		}

	}
	return return_var;
}

struct SL_Variable
input_fn(struct SL_Code *code, struct SL_L_Function func)
{
	struct SL_Variable return_var = { 0 };
	for (int i = 0; i < func.total_arguments; i++) {
		return_var = sl_get_argument(*code, func, i);
		switch (return_var.type) {
		case INTEGER:
			printf("%d", return_var.vali);
			break;
		case DOUBLE:
			printf("%f", return_var.valf);
			break;
		case STRING:
			if (strchr(return_var.vals, '"')) {
				char           *string =
					sl_string_getter(return_var.vals);
				printf("%s", string);
				free(string);
			}
			else
				printf("%s", return_var.vals);

			break;
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
			printf("%lu", return_var.valh);
			break;
		default:
			break;
		}

	}
	char            string[1024];
	fgets(string, sizeof(string), stdin);
	string[strcspn(string, "\n")] = '\0';

	return_var.type = STRING;
	return_var.vals = malloc(1024);
	strncpy(return_var.vals, string, 1024);

	return return_var;
}

struct SL_Variable
io_getchar_fn(struct SL_Code *code, struct SL_L_Function func)
{
	struct SL_Variable return_var = { 0 };
	return_var.valc = getchar();
	return_var.type = CHAR;
	return return_var;
}

struct SL_Variable
file_read_to_str_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at file.read_to_str! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	char *file_name = strdup(first_arg.vals);
	if (strchr(first_arg.vals, '"')) {
			free(file_name);
			file_name = sl_string_getter(first_arg.vals);
	}
	
	FILE           *file_open = fopen(file_name, "r");
	if (file_open == NULL) {
		return_var.type = ERROR;
		return_var.vals = "File not found!";
		return return_var;
	}

	fseek(file_open, 0, SEEK_END);
	long int        size = ftell(file_open);
	rewind(file_open);

	char           *buffer = malloc(size + 1);
	fread(buffer, 1, size, file_open);
	buffer[size] = '\0';
	fclose(file_open);

	return_var.type = STRING;
	return_var.vals = strdup(buffer);
	free(buffer);
	free(file_name);
	return return_var;
}

struct SL_Variable
file_write_from_str_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at file.write_from_str! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
	char *file_name = strdup(first_arg.vals);
	if (strchr(first_arg.vals, '"')) {
			free(file_name);
			file_name = sl_string_getter(first_arg.vals);
	}
	
	FILE           *file_open = fopen(file_name, "w");
	if (file_open == NULL) {
		return_var.type = ERROR;
		return_var.vals = "File not found!";
		return return_var;
	}
	size_t          len = strlen(second_arg.vals);

	if (fwrite(second_arg.vals, 1, len, file_open) != len) {
		fclose(file_open);

		return_var.type = ERROR;
		return_var.vals = "Could not write to file!";
		return return_var;
	}

	fclose(file_open);

	return_var.type = STRING;
	return_var.vals = strdup(second_arg.vals);
	free(file_name);
	return return_var;
}

struct SL_Variable
file_append_from_str_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at file.append_from_str! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
	char *file_name = strdup(first_arg.vals);
	if (strchr(first_arg.vals, '"')) {
			free(file_name);
			file_name = sl_string_getter(first_arg.vals);
	}	
	FILE           *file_open = fopen(file_name, "a");
	if (file_open == NULL) {
		return_var.type = ERROR;
		return_var.vals = "File not found!";
		return return_var;
	}
	size_t          len = strlen(second_arg.vals);

	if (fwrite(second_arg.vals, 1, len, file_open) != len) {
		fclose(file_open);

		return_var.type = ERROR;
		return_var.vals = "Could not write to file!";
		return return_var;
	}

	fclose(file_open);

	return_var.type = STRING;
	return_var.vals = strdup(second_arg.vals);
	free(file_name);
	return return_var;
}



// EXTRA
struct SL_Variable
random_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 2) {
		fprintf(stderr,
			"Error usage at rand.random! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
	int             random =
		rand() % (second_arg.vali - first_arg.vali + 1) +
		first_arg.vali;
	struct SL_Variable return_var = { 0 };
	return_var.type = INTEGER;
	return_var.vali = random;
	return return_var;
}

// TYPES
struct SL_Variable
str_to_int_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at types.str_to_int! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable return_var = { 0 };
	return_var.type = INTEGER;
	return_var.vali = atoi(first_arg.vals);
	return return_var;
}

struct SL_Variable
int_to_char_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at types.int_to_char! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable return_var = { 0 };
	if (first_arg.vali > 255) {
		return_var.type = ERROR;
		return_var.vals = "Char overflow!";
		return return_var;
	}
	return_var.valc = first_arg.vali;
	return_var.type = CHAR;
	return return_var;
}

struct SL_Variable
char_to_int_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at types.char_to_int! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable return_var = { 0 };
	return_var.vali = first_arg.valc;
	return_var.type = INTEGER;
	return return_var;
}

struct SL_Variable
typeof_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at types.typeof! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	return_var.type = INTEGER;
	return_var.vali = first_arg.type;
	return return_var;
}

struct SL_Variable
is_int_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at types.is_int! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	if (first_arg.type == INTEGER)
		return_var.valb = 1;
	return_var.type = BOOLEAN;
	return return_var;
}

struct SL_Variable
is_char_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at types.is_char! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	if (first_arg.type == CHAR)
		return_var.valb = 1;
	return_var.type = BOOLEAN;
	return return_var;
}

struct SL_Variable
is_bool_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at types.is_bool! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	if (first_arg.type == BOOLEAN)
		return_var.valb = 1;
	return_var.type = BOOLEAN;
	return return_var;
}


struct SL_Variable
is_string_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at types.is_string! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	if (first_arg.type == STRING)
		return_var.valb = 1;
	return_var.type = BOOLEAN;
	return return_var;
}

struct SL_Variable
is_double_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at types.is_string! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	if (first_arg.type == DOUBLE)
		return_var.valb = 1;
	return_var.type = BOOLEAN;
	return return_var;
}

struct SL_Variable
is_not_initialized_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at types.is_string! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	if (first_arg.type == INIT)
		return_var.valb = 1;
	return_var.type = BOOLEAN;
	return return_var;
}

struct SL_Variable
is_digit_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at types.str_to_int! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	char *str = strdup(first_arg.vals);
	if (strchr(str, '"')) {
		free(str);
		str = sl_string_getter(first_arg.vals);
	}
	
	int len = strlen(str);
	struct SL_Variable return_var = { 0 };
	return_var.type = BOOLEAN;
	return_var.valb = 0;

	for (int i = 0; i < len; i++) {
		if (!isdigit(str[i]))
			return return_var;
	}
	
	return_var.valb = 1;
	return return_var;
}



// STRING
struct SL_Variable
string_charat_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 2) {
		fprintf(stderr,
			"Error usage at string.char_at! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
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

	int             len = strlen(first_arg.vals);

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

	return_var.valc = first_arg.vals[second_arg.vali];
	return_var.type = CHAR;
	return return_var;
}

struct SL_Variable
string_setcharat_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 2) {
		fprintf(stderr,
			"Error usage at string.char_at! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
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


	int             len = strlen(first_arg.vals);

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
	struct SL_Variable ref_var = sl_get_var(*use_code, first_arg.name);
	if (strchr(ref_var.vals, '"')) {
		char           *string =
					sl_string_getter(ref_var.vals);
		free(ref_var.vals);
		string[second_arg.vali] = third_arg.valc;
		ref_var.vals = strdup(string);
		return_var.type = BOOLEAN;
		return_var.valb = 1;
		free(string);
	}
	return return_var;
}

struct SL_Variable
string_len_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at string.len! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

	if (first_arg.type != STRING) {
		return_var.type = ERROR;
		return_var.vals =
			"Expected string as the first argument to string.len.";
		return return_var;
	}

	int             len = strlen(first_arg.vals);
	return_var.vali = len;
	return_var.type = INTEGER;
	return return_var;
}

struct SL_Variable
string_split_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at string.len! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
	if (first_arg.type != STRING) {
		return_var.type = ERROR;
		return_var.vals =
			"Expected string as the first argument to string.split.";
		return return_var;
	}
	if (second_arg.type != STRING) {
		return_var.type = ERROR;
		return_var.vals =
			"Expected string as the second argument to string.split.";
		return return_var;
	}
	char *splt_string = strdup(first_arg.vals);
	if (strchr(first_arg.vals, '"')) {
		free(splt_string);
		splt_string = sl_string_getter(first_arg.vals);
	}

	char *splt_token = strdup(second_arg.vals);
	if (strchr(second_arg.vals, '"')) {
		free(splt_token);
		splt_token = sl_string_getter(second_arg.vals);
	}


	int	listind = create_new_list(256, 0);

	char *tokenize = strtok(splt_string, splt_token);
	while (tokenize != NULL) {
		struct SL_Variable push_val = { 0 };
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

struct SL_Variable
string_slice_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at string.len! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
	struct SL_Variable third_arg = sl_get_argument(*code, func, 2);
	if (first_arg.type != STRING) {
		return_var.type = ERROR;
		return_var.vals =
			"Expected string as the first argument to string.slice.";
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
		return_var.vals =
			"Expected integer as the third argument to string.slice.";
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



// ERROR HANDLING
struct SL_Variable
errors_string_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at errors.string! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable return_var = { 0 };
	if (first_arg.type == ERROR) {
		printf("%s\n", first_arg.vals);
	}
	return return_var;
}

struct SL_Variable
errors_bool_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at errors.bool! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable return_var = { 0 };
	if (first_arg.type == ERROR) {
		return_var.valb = 1;
		return_var.type = BOOLEAN;
		return return_var;
	}
	return_var.valb = 0;
	return_var.type = BOOLEAN;
	return return_var;
}

struct SL_Variable
errors_panic_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at errors.string! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable return_var = { 0 };
	if (first_arg.type == ERROR) {
		printf("Program panicked with error: %s\n", first_arg.vals);
		exit(-1);
	}
	return first_arg;
}


// SYS
struct SL_Variable
sys_exit_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at sys.exit! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable return_var = { 0 };
	if (first_arg.type == INTEGER) {
		exit(first_arg.vali);
	}
	return return_var;
}

struct SL_Variable
sys_get_arg_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at sys.get_arg! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable return_var = { 0 };
	if (first_arg.type == INTEGER && first_arg.vali < argcN) {
		return_var.type = STRING;
		return_var.vals = strdup(arguments[first_arg.vali]);
	}
	else {
		return_var.type = ERROR;
		return_var.vals = "Argument not found!";
	}
	return return_var;
}



// LISTS
struct SL_Variable
List_new_fn(struct SL_Code *code, struct SL_L_Function func)
{
	struct SL_Variable first_arg;
	int             fixed = 0;
	if (func.total_arguments > 0) {
		first_arg = sl_get_argument(*code, func, 0);
		fixed = 1;
	}
	else {
		first_arg.vali = 128;
	}

	if (func.total_arguments > 0 && first_arg.type != INTEGER) {
		struct SL_Variable err;
		err.type = ERROR;
		err.vals =
			"Expected integer as the first argument to List.new.";
		return err;
	}

	struct SL_Variable return_var = { 0 };
	int             capacity = first_arg.vali > 0 ? first_arg.vali : 1;

	return_var.type = INTEGER;
	return_var.vali = create_new_list(capacity, fixed);

	return return_var;
}

struct SL_Variable
List_push_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 2) {
		fprintf(stderr,
			"Error usage at List.push! Not enough arguments.\n");
		exit(-1);
	}

	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable return_var = { 0 };

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


struct SL_Variable
List_pop_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at List.pop! Not enough arguments.\n");
		exit(-1);
	}

	struct SL_Variable first_arg =
		sl_get_argument(*code, func, 0);

	struct SL_Variable return_var = { 0 };

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


struct SL_Variable
List_peek_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at List.peek! Not enough arguments.\n");
		exit(-1);
	}

	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable return_var = { 0 };

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




struct SL_Variable
List_set_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 3) {
		fprintf(stderr,
			"Error usage at List.set! Not enough arguments.\n");
		exit(-1);
	}

	struct SL_Variable first_arg =
		sl_get_argument(*code, func, 0);
	struct SL_Variable second_arg =
		sl_get_argument(*code, func, 1);
	struct SL_Variable third_arg =
		sl_get_argument(*code, func, 2);

	struct SL_Variable return_var = { 0 };

	if (first_arg.type != INTEGER) {
		return_var.type = ERROR;
		return_var.vals =
			"Expected list_variable as the first argument to List.set.";
		return return_var;
	}

	if (second_arg.type != INTEGER) {
		return_var.type = ERROR;
		return_var.vals =
			"Expected integer as the second argument to List.set.";
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


struct SL_Variable
List_get_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 2) {
		fprintf(stderr,
			"Error usage at List.get! Not enough arguments.\n");
		exit(-1);
	}

	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
	struct SL_Variable return_var = { 0 };

	if (first_arg.type != INTEGER) {
		return_var.type = ERROR;
		return_var.vals =
			"Expected list_variable as the first argument to List.get.";
		return return_var;
	}

	if (second_arg.type != INTEGER) {
		return_var.type = ERROR;
		return_var.vals =
			"Expected integer as the second argument to List.get.";
		return return_var;
	}

	if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
		return_var.type = ERROR;
		return_var.vals = "List buffer overflow!";
		return return_var;
	}

	if (second_arg.vali >= LISTS[first_arg.vali].size
	    || second_arg.vali < 0) {
		return_var.type = ERROR;
		return_var.vals = "Buffer overflow on List element.";
		return return_var;
	}

	return sl_copy_variable(LISTS[first_arg.vali].vars[second_arg.vali]);
}


struct SL_Variable
List_remove_fn(struct SL_Code *code, struct SL_L_Function func)
{
    if (func.total_arguments < 2) {
        fprintf(stderr,
            "Error usage at List.remove! Not enough arguments.\n");
        exit(-1);
    }

    struct SL_Variable return_var = { 0 };
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

struct SL_Variable
List_iter_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at List.iter! Not enough arguments.\n");
		exit(-1);
	}

	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable return_var = { 0 };

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



struct SL_Variable
List_next_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at List.next! Not enough arguments.\n");
		exit(-1);
	}

	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable return_var = { 0 };

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

	return sl_copy_variable(LISTS[first_arg.vali].vars[LISTS[first_arg.vali].current++]);
}


struct SL_Variable
List_len_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at List.len! Not enough arguments.\n");
		exit(-1);
	}

	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable return_var = { 0 };

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


// DB
struct SL_Variable
db_from_lists_fn(struct SL_Code *code, struct SL_L_Function func)
{
    if (func.total_arguments < 1) {
        fprintf(stderr, "Error usage at db.from_lists! Not enough arguments.\n");
        exit(-1);
    }

    struct SL_Variable return_var = { 0 };
    
    size_t body_cap = 1024;
    size_t body_len = 0;
    char *body = malloc(body_cap);
    body[0] = '\0';

    for (int i = 0; i < func.total_arguments; i++) {
        struct SL_Variable arg = sl_get_argument(*code, func, i);
        if (arg.type != INTEGER) {
            free(body);
            return_var.type = ERROR;
            return_var.vals = "Expected integer list index as argument to db.from_lists.";
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
                case INTEGER: snprintf(item_str, sizeof(item_str), "%d", item.vali); break;
                case DOUBLE: snprintf(item_str, sizeof(item_str), "%f", item.valf); break;
                case STRING: 
                    if (item.vals && strchr(item.vals, '"')) {
                        char *s = sl_string_getter(item.vals);
                        snprintf(item_str, sizeof(item_str), "%s", s);
                        free(s);
                    } else {
                        snprintf(item_str, sizeof(item_str), "%s", item.vals ? item.vals : "");
                    }
                    break;
                case BOOLEAN: snprintf(item_str, sizeof(item_str), "%s", item.valb ? "true" : "false"); break;
                case CHAR: snprintf(item_str, sizeof(item_str), "%c", item.valc); break;
                case LONG: snprintf(item_str, sizeof(item_str), "%lu", item.valh); break;
                default: break;
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

struct SL_Variable
db_to_lists_fn(struct SL_Code *code, struct SL_L_Function func)
{
    if (func.total_arguments < 1) {
        fprintf(stderr, "Error usage at db.to_lists! Not enough arguments.\n");
        exit(-1);
    }

    struct SL_Variable return_var = { 0 };
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
                struct SL_Variable item = { 0 };
                item.type = STRING;
                item.vals = strdup(tok);
                list_push(&LISTS[current_list_idx], item);
            }
            if (current_list_idx != -1) {
                struct SL_Variable list_ref = { 0 };
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
                struct SL_Variable item = { 0 };
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
        struct SL_Variable item = { 0 };
        item.type = STRING;
        item.vals = strdup(tok);
        list_push(&LISTS[current_list_idx], item);

        struct SL_Variable list_ref = { 0 };
        list_ref.type = INTEGER;
        list_ref.vali = current_list_idx;
        list_push(master_list, list_ref);
    }

    free(tok);

    return_var.type = INTEGER;
    return_var.vali = master_list_idx;
    return return_var;
}

int             used_io = 0;
int             used_file = 0;
int             used_types = 0;
int             used_sys = 0;
int             used_string = 0;
int             used_errors = 0;
int             used_list = 0;
int             used_extra = 0;
int 			used_db = 0;


struct SL_Variable
use_fn(struct SL_Code *code, struct SL_L_Function func)
{
	struct SL_Variable return_var = { 0 };
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at use! Not enough arguments.\n");
		exit(-1);
	}
	for (int i = 0; i < func.total_arguments; i++) {
		struct SL_Variable lib = sl_get_argument(*code, func, i);
		char           *libstr = sl_string_getter(lib.vals);

		if (strcmp(libstr, "io") == 0 && used_io == 0) {
			used_io = 1;
			sl_add_func(use_code, "io.print", print_fn);
			sl_add_func(use_code, "io.input", input_fn);
			sl_add_func(use_code, "io.getchar", io_getchar_fn);
		}
		else if (strcmp(libstr, "file") == 0 && used_file == 0) {
			used_file = 1;
			sl_add_func(use_code, "file.read_to_str",
				    file_read_to_str_fn);
			sl_add_func(use_code, "file.write_from_str",
				    file_write_from_str_fn);
			sl_add_func(use_code, "file.append_from_str",
				    file_append_from_str_fn);
		}
		else if (strcmp(libstr, "types") == 0 && used_types == 0) {
			used_types = 1;
			// CONVERT
			sl_add_func(use_code, "types.str_to_int",
				    str_to_int_fn);
			sl_add_func(use_code, "types.int_to_char",
				    int_to_char_fn);
			sl_add_func(use_code, "types.char_to_int",
				    char_to_int_fn);

			// TYPE CHECK
			sl_add_func(use_code, "types.is_int", is_int_fn);
			sl_add_func(use_code, "types.is_char", is_char_fn);
			sl_add_func(use_code, "types.is_string",
				    is_string_fn);
			sl_add_func(use_code, "types.is_double",
				    is_double_fn);
			sl_add_func(use_code, "types.is_not_initialized",
				    is_not_initialized_fn);
			sl_add_func(use_code, "types.typeof", typeof_fn);

			// STRING TYPE CHECK
			sl_add_func(use_code, "types.is_digit", is_digit_fn);

		}
		else if (strcmp(libstr, "sys") == 0 && used_sys == 0) {
			used_sys = 1;
			sl_add_func(use_code, "sys.get_arg", sys_get_arg_fn);
			sl_add_func(use_code, "sys.exit", sys_exit_fn);
		}
		else if (strcmp(libstr, "errors") == 0 && used_errors == 0) {
			used_errors = 1;
			sl_add_func(use_code, "errors.string",
				    errors_string_fn);
			sl_add_func(use_code, "errors.bool", errors_bool_fn);
			sl_add_func(use_code, "errors.panic",
				    errors_panic_fn);
		}
		else if (strcmp(libstr, "string") == 0 && used_string == 0) {
			used_string = 1;
			sl_add_func(use_code, "string.char_at",
				    string_charat_fn);
			sl_add_func(use_code, "string.split", string_split_fn);
			sl_add_func(use_code, "string.slice", string_slice_fn);
			sl_add_func(use_code, "string.set_char_at",
				    string_setcharat_fn);
			sl_add_func(use_code, "string.len", string_len_fn);
		}
		else if (strcmp(libstr, "list") == 0 && used_list == 0) {
			used_list = 1;
			sl_add_func(use_code, "List.new", List_new_fn);
			sl_add_func(use_code, "List.push", List_push_fn);
			sl_add_func(use_code, "List.pop", List_pop_fn);
			sl_add_func(use_code, "List.peek", List_peek_fn);
			sl_add_func(use_code, "List.set", List_set_fn);
			sl_add_func(use_code, "List.get", List_get_fn);
			sl_add_func(use_code, "List.next", List_next_fn);
			sl_add_func(use_code, "List.iter", List_iter_fn);
			sl_add_func(use_code, "List.remove", List_remove_fn);
			sl_add_func(use_code, "List.len", List_len_fn);
		}
		else if (strcmp(libstr, "extra") == 0 && used_extra == 0) {
			used_extra = 1;
			sl_add_func(use_code, "rand.random", random_fn);
		} else if (strcmp(libstr, "db") == 0 && used_db == 0) {
			used_db = 1;
			sl_add_func(use_code, "db.from_lists", db_from_lists_fn);
			sl_add_func(use_code, "db.to_lists", db_to_lists_fn);
		}
		free(libstr);
	}

	return return_var;
}

void
init_sl_stdlib(struct SL_Code *sl_code, int argc, char **argv)
{
	srand(time(NULL));
	arguments = malloc(argc * sizeof(char *));
	for (int i = 0; i < argc; i++) {
		arguments[i] = strdup(argv[i]);
	}
	argcN = argc;
	LISTS = calloc(SL_INIT, sizeof(struct SL_List));
	use_code = sl_code;
	sl_add_func(sl_code, "use", use_fn);
}


void
close_sl_stdlib()
{
	for (int i = 0; i < argcN; i++) {
		free(arguments[i]);
	}
	if (arguments != NULL)
		free(arguments);

	for (int i = 0; i < LISTS_count; i++) {
		for (int size = 0; size < LISTS[i].size; size++) {

			if (LISTS[i].vars[size].name != NULL) {
				free(LISTS[i].vars[size].name);
				LISTS[i].vars[size].name = NULL;
			}

			if ((LISTS[i].vars[size].type == STRING
			     || LISTS[i].vars[size].type == RETURN)
			    && LISTS[i].vars[size].vals != NULL) {
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
