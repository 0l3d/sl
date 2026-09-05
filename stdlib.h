#include "sl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

struct SL_Code *use_code = NULL;

struct SL_Variable
example_fn(struct SL_Code *code,
	   struct SL_L_Function func, struct SL_Function rfunc)
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
	int             current;
};


struct SL_Collection
{
	char           *name;
	char          **attrs;
	struct SL_Function *functions;
	int             total_attrs;
	int             total_funcs;
};

struct SL_Collections
{
	struct SL_Collection *collections;
	int             size;
	int             capacity;
};


// LISTS
int             LISTS_count = 0;
int             LISTS_capacity = SL_INIT;
struct SL_List *LISTS = { 0 };

// COLLECTIONS
struct SL_Collections collections = { 0 };


/* LIST FUNCTIONS */
int
create_new_list(int capacity, int fixed)
{
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
	}
	else {
		LISTS[LISTS_count].size = 0;
	}
	int             index = LISTS_count;
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
	struct SL_Variable ret = sl_copy_variable(list->vars[list->size - 1]);

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

/* LIST FUNCTIONS */

// IO
struct SL_Variable
print_fn(struct SL_Code *code,
	 struct SL_L_Function func, struct SL_Function rfunc)
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
		case STRING:{
				char           *string =
					sl_string_getter(return_var.vals);
				printf("%s", string);
				free(string);
			}
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
input_fn(struct SL_Code *code,
	 struct SL_L_Function func, struct SL_Function rfunc)
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
		case STRING:{
				char           *string =
					sl_string_getter(return_var.vals);
				printf("%s", string);
				free(string);
			}
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
io_getchar_fn(struct SL_Code *code,
	      struct SL_L_Function func, struct SL_Function rfunc)
{
	struct SL_Variable return_var = { 0 };
	return_var.valc = getchar();
	return_var.type = CHAR;
	return return_var;
}

struct SL_Variable
file_read_to_str_fn(struct SL_Code *code,
		    struct SL_L_Function func, struct SL_Function rfunc)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at file.read_to_str! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	char           *file_name = sl_string_getter(first_arg.vals);

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
file_write_from_str_fn(struct SL_Code *code,
		       struct SL_L_Function func, struct SL_Function rfunc)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at file.write_from_str! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
	char           *file_name = sl_string_getter(first_arg.vals);

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
file_append_from_str_fn(struct SL_Code *code,
			struct SL_L_Function func, struct SL_Function rfunc)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at file.append_from_str! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
	char           *file_name = sl_string_getter(first_arg.vals);

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
random_fn(struct SL_Code *code,
	  struct SL_L_Function func, struct SL_Function rfunc)
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
str_to_int_fn(struct SL_Code *code,
	      struct SL_L_Function func, struct SL_Function rfunc)
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
int_to_char_fn(struct SL_Code *code,
	       struct SL_L_Function func, struct SL_Function rfunc)
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
char_to_int_fn(struct SL_Code *code,
	       struct SL_L_Function func, struct SL_Function rfunc)
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
char_to_str_fn(struct SL_Code *code,
	       struct SL_L_Function func, struct SL_Function rfunc)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at types.char_to_str! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable return_var = { 0 };
	return_var.vals = malloc(2);
	return_var.vals[0] = first_arg.valc;
	return_var.vals[1] = '\0';
	return_var.type = STRING;
	return return_var;
}

struct SL_Variable
int_to_str_fn(struct SL_Code *code,
	      struct SL_L_Function func, struct SL_Function rfunc)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at types.char_to_str! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable return_var = { 0 };
	int             digits = 0;
	int             temp = first_arg.vali;
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



struct SL_Variable
typeof_fn(struct SL_Code *code,
	  struct SL_L_Function func, struct SL_Function rfunc)
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
is_int_fn(struct SL_Code *code,
	  struct SL_L_Function func, struct SL_Function rfunc)
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
is_char_fn(struct SL_Code *code,
	   struct SL_L_Function func, struct SL_Function rfunc)
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
is_bool_fn(struct SL_Code *code,
	   struct SL_L_Function func, struct SL_Function rfunc)
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
is_string_fn(struct SL_Code *code,
	     struct SL_L_Function func, struct SL_Function rfunc)
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
is_double_fn(struct SL_Code *code,
	     struct SL_L_Function func, struct SL_Function rfunc)
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
is_not_initialized_fn(struct SL_Code *code,
		      struct SL_L_Function func, struct SL_Function rfunc)
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
is_digit_fn(struct SL_Code *code,
	    struct SL_L_Function func, struct SL_Function rfunc)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at types.str_to_int! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	char           *str = sl_string_getter(first_arg.vals);

	int             len = strlen(str);
	struct SL_Variable return_var = { 0 };
	return_var.type = BOOLEAN;
	return_var.valb = 0;

	for (int i = 0; i < len; i++) {
		if (!isdigit(str[i])) {
			free(str);
			return return_var;
		}
	}

	return_var.valb = 1;
	free(str);
	return return_var;
}



// STRING
struct SL_Variable
string_charat_fn(struct SL_Code *code,
		 struct SL_L_Function func, struct SL_Function rfunc)
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
string_setcharat_fn(struct SL_Code *code,
		    struct SL_L_Function func, struct SL_Function rfunc)
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
	struct SL_Variable *ref_var = sl_get_var(code, first_arg.name);
	char           *string = sl_string_getter(ref_var->vals);
	free(ref_var->vals);
	string[second_arg.vali] = third_arg.valc;
	ref_var->vals = strdup(string);
	return_var.type = BOOLEAN;
	return_var.valb = 1;
	free(string);
	return return_var;
}

struct SL_Variable
string_len_fn(struct SL_Code *code,
	      struct SL_L_Function func, struct SL_Function rfunc)
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
string_split_fn(struct SL_Code *code,
		struct SL_L_Function func, struct SL_Function rfunc)
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
	char           *splt_string = sl_string_getter(first_arg.vals);

	char           *splt_token = sl_string_getter(second_arg.vals);


	int             listind = create_new_list(256, 0);

	char           *tokenize = strtok(splt_string, splt_token);
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
string_slice_fn(struct SL_Code *code,
		struct SL_L_Function func, struct SL_Function rfunc)
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
	char           *raw_str = sl_string_getter(first_arg.vals);
	int             len = strlen(raw_str);
	if (len >= second_arg.vali || len > third_arg.vali || len < 0) {
		return_var.type = ERROR;
		return_var.vals = "Buffer over/underflow!";
	}
	char           *result = malloc(len + 1);
	strncpy(result, raw_str + second_arg.vali,
		third_arg.vali - second_arg.vali);


	free(raw_str);
	return_var.type = STRING;
	return_var.vals = strdup(result);
	free(result);
	return return_var;
}


struct SL_Variable
string_trim_fn(struct SL_Code *code,
	       struct SL_L_Function func, struct SL_Function rfunc)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at string.trim! Not enough arguments.");
		exit(-1);
	}

	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

	if (first_arg.type != STRING) {
		return_var.type = ERROR;
		return_var.vals =
			"Expected string as the first argument to string.trim.";
		return return_var;
	}

	char           *raw_str = sl_string_getter(first_arg.vals);

	int             start = 0;
	int             end = strlen(raw_str);

	while (start < end && isspace((unsigned char) raw_str[start])) {
		start++;
	}

	while (end > start && isspace((unsigned char) raw_str[end - 1])) {
		end--;
	}

	int             len = end - start;

	char           *result = malloc(len + 1);
	memcpy(result, raw_str + start, len);
	result[len] = '\0';

	return_var.type = STRING;
	return_var.vals = strdup(result);

	free(result);
	free(raw_str);

	return return_var;
}

// ERROR HANDLING
struct SL_Variable
errors_string_fn(struct SL_Code *code,
		 struct SL_L_Function func, struct SL_Function rfunc)
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
errors_bool_fn(struct SL_Code *code,
	       struct SL_L_Function func, struct SL_Function rfunc)
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
errors_panic_fn(struct SL_Code *code,
		struct SL_L_Function func, struct SL_Function rfunc)
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
sys_exit_fn(struct SL_Code *code,
	    struct SL_L_Function func, struct SL_Function rfunc)
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
sys_get_arg_fn(struct SL_Code *code,
	       struct SL_L_Function func, struct SL_Function rfunc)
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
List_new_fn(struct SL_Code *code,
	    struct SL_L_Function func, struct SL_Function rfunc)
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
List_push_fn(struct SL_Code *code,
	     struct SL_L_Function func, struct SL_Function rfunc)
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
		struct SL_Variable list_item =
			sl_get_argument(*code, func, i);
		return_var.type = INTEGER;
		return_var.vali = list_push(list, list_item);
	}

	return return_var;
}


struct SL_Variable
List_pop_fn(struct SL_Code *code,
	    struct SL_L_Function func, struct SL_Function rfunc)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at List.pop! Not enough arguments.\n");
		exit(-1);
	}

	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

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
List_peek_fn(struct SL_Code *code,
	     struct SL_L_Function func, struct SL_Function rfunc)
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
List_set_fn(struct SL_Code *code,
	    struct SL_L_Function func, struct SL_Function rfunc)
{
	if (func.total_arguments < 3) {
		fprintf(stderr,
			"Error usage at List.set! Not enough arguments.\n");
		exit(-1);
	}

	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
	struct SL_Variable third_arg = sl_get_argument(*code, func, 2);

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
List_get_fn(struct SL_Code *code,
	    struct SL_L_Function func, struct SL_Function rfunc)
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
List_remove_fn(struct SL_Code *code,
	       struct SL_L_Function func, struct SL_Function rfunc)
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

	int             success = list_remove(list, second_arg.vali);

	return_var.type = BOOLEAN;
	return_var.valb = success;
	return return_var;
}

struct SL_Variable
List_iter_fn(struct SL_Code *code,
	     struct SL_L_Function func, struct SL_Function rfunc)
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
	}
	else {
		return_var.valb = 1;
		return_var.type = BOOLEAN;
	}

	return return_var;
}



struct SL_Variable
List_next_fn(struct SL_Code *code,
	     struct SL_L_Function func, struct SL_Function rfunc)
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

	return sl_copy_variable(LISTS[first_arg.vali].vars
				[LISTS[first_arg.vali].current++]);
}


struct SL_Variable
List_len_fn(struct SL_Code *code,
	    struct SL_L_Function func, struct SL_Function rfunc)
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
db_from_lists_fn(struct SL_Code *code,
		 struct SL_L_Function func, struct SL_Function rfunc)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at db.from_lists! Not enough arguments.\n");
		exit(-1);
	}

	struct SL_Variable return_var = { 0 };

	size_t          body_cap = 1024;
	size_t          body_len = 0;
	char           *body = malloc(body_cap);
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
			return_var.vals =
				"List buffer overflow in db.from_lists!";
			return return_var;
		}

		struct SL_List *list = &LISTS[arg.vali];
		const char     *name =
			(arg.name != NULL) ? arg.name : "unnamed_list";

		size_t          n_len = strlen(name);
		if (body_len + n_len + 2 >= body_cap) {
			body_cap = body_cap * 2 + n_len + 2;
			body = realloc(body, body_cap);
		}
		strcpy(body + body_len, name);
		body_len += n_len;

		for (int j = 0; j < list->size; j++) {
			struct SL_Variable item = list->vars[j];
			char            item_str[512] = "";
			switch (item.type) {
			case INTEGER:
				snprintf(item_str, sizeof(item_str), "%d",
					 item.vali);
				break;
			case DOUBLE:
				snprintf(item_str, sizeof(item_str), "%f",
					 item.valf);
				break;
			case STRING:{
					char           *s =
						sl_string_getter(item.vals);
					snprintf(item_str, sizeof(item_str),
						 "%s", s);
					free(s);
				}
				break;
			case BOOLEAN:
				snprintf(item_str, sizeof(item_str), "%s",
					 item.valb ? "true" : "false");
				break;
			case CHAR:
				snprintf(item_str, sizeof(item_str), "%c",
					 item.valc);
				break;
			case LONG:
				snprintf(item_str, sizeof(item_str), "%lu",
					 item.valh);
				break;
			default:
				break;
			}

			size_t          is_len = strlen(item_str);
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
db_to_lists_fn(struct SL_Code *code,
	       struct SL_L_Function func, struct SL_Function rfunc)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at db.to_lists! Not enough arguments.\n");
		exit(-1);
	}

	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

	if (first_arg.type != STRING) {
		return_var.type = ERROR;
		return_var.vals =
			"Expected string as argument to db.to_lists.";
		return return_var;
	}

	int             master_list_idx = create_new_list(8, 0);
	struct SL_List *master_list = &LISTS[master_list_idx];

	char           *src = first_arg.vals;
	if (!src) {
		return_var.type = INTEGER;
		return_var.vali = master_list_idx;
		return return_var;
	}

	char           *p = src;
	int             current_list_idx = -1;
	int             is_first_token = 1;

	size_t          tok_cap = 256;
	size_t          tok_len = 0;
	char           *tok = malloc(tok_cap);

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
		}
		else if (*p == '/') {
			tok[tok_len] = '\0';
			if (is_first_token) {
				current_list_idx = create_new_list(8, 0);
				is_first_token = 0;
			}
			else {
				struct SL_Variable item = { 0 };
				item.type = STRING;
				item.vals = strdup(tok);
				list_push(&LISTS[current_list_idx], item);
			}
			tok_len = 0;
			p++;
		}
		else {
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

// COLLECTIONS
struct SL_Variable
collections_new_collection_fn(struct SL_Code *code,
			      struct SL_L_Function func,
			      struct SL_Function rfunc)
{
	struct SL_Variable return_var = { 0 };

	int             collec_index = 0;
	for (int i = 0; i < collections.size; i++) {
		if (strcmp(collections.collections[i].name, rfunc.name - 4)) {
			collec_index = i;
		}
	}

	int             scope = sl_get_scope(code);
	if (scope > 1)
		scope -= 1;

	char           *assigned_var = sl_get_assignment_var();
	int             assigned_len = strlen(assigned_var);
	for (int i = 0; i < collections.collections[collec_index].total_attrs;
	     i++) {
		char           *raw_attr =
			collections.collections[collec_index].attrs[i];
		int             assigned_var_len = strlen(assigned_var);
		int             attr_len = strlen(raw_attr);
		int             total_len = assigned_var_len + attr_len;
		char           *full_attr_name = malloc(total_len + 2);
		snprintf(full_attr_name, total_len + 2, "%s.%s", assigned_var,
			 raw_attr);
		struct SL_Variable var = { 0 };
		var.name = full_attr_name;
		var.type = INTEGER;
		var.hash = sl_hash_string(full_attr_name);
		var.scope_lifetime = scope;
		sl_add_var(code, var);
		free(full_attr_name);
	}
	for (int i = 0; i < collections.collections[collec_index].total_funcs;
	     i++) {
		struct SL_Function raw_func =
			collections.collections[collec_index].functions[i];
		int             assigned_var_len = strlen(assigned_var);
		int             funcn_len = strlen(raw_func.name);
		int             total_len = assigned_var_len + funcn_len;
		char           *full_func_name = malloc(total_len + 2);
		snprintf(full_func_name, total_len + 2, "%s:%s", assigned_var,
			 raw_func.name);
		struct SL_Function func = { 0 };
		func = raw_func;
		func.code_tokens[3] = malloc(assigned_len + 3);
		snprintf(func.code_tokens[3], assigned_len + 3, "\"%s\"",
			 assigned_var);
		func.name = full_func_name;
		func.hash = sl_hash_string(full_func_name);
		func.scope_lifetime = scope;
		sl_add_raw_func(code, &func);
		free(full_func_name);
	}

	return return_var;
}


struct SL_Variable
collections_set_attr_fn(struct SL_Code *code,
			struct SL_L_Function func, struct SL_Function rfunc)
{
	struct SL_Variable return_var = { 0 };
	struct SL_Variable self = sl_get_argument(*code, func, 0);
	struct SL_Variable attr_name = sl_get_argument(*code, func, 1);
	struct SL_Variable attr_val = sl_get_argument(*code, func, 2);

	int             scope = sl_get_scope(code);
	if (scope > 1)
		scope -= 1;

	char           *self_n = sl_string_getter(self.vals);
	char           *attr_name_r = sl_string_getter(attr_name.vals);

	int             total_size = strlen(attr_name_r) + strlen(self_n);
	char           *full_var_name = malloc(total_size + 2);
	snprintf(full_var_name, total_size + 2, "%s.%s", self_n, attr_name_r);
	struct SL_Variable var = { 0 };
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

struct SL_Variable
collections_get_attr_fn(struct SL_Code *code,
			struct SL_L_Function func, struct SL_Function rfunc)
{
	struct SL_Variable return_var = { 0 };
	struct SL_Variable self = sl_get_argument(*code, func, 0);
	struct SL_Variable attr_name = sl_get_argument(*code, func, 1);

	int             scope = sl_get_scope(code);
	if (scope > 1)
		scope -= 1;

	char           *self_n = sl_string_getter(self.vals);
	char           *attr_name_r = sl_string_getter(attr_name.vals);

	int             total_size = strlen(attr_name_r) + strlen(self_n);
	char           *full_var_name = malloc(total_size + 2);
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




struct SL_Variable
collections_create_collection_fn(struct SL_Code *code,
				 struct SL_L_Function func,
				 struct SL_Function rfunc)
{
	if (func.total_arguments < 0) {
		fprintf(stderr,
			"Error usage at Collections.create_collection! Not enough arguments.\n");
		exit(-1);
	}

	struct SL_Variable return_var = { 0 };
	char           *assigned_var = sl_get_assignment_var();


	int             scope = sl_get_scope(code);
	if (scope > 1)
		scope -= 1;

	if (collections.size >= collections.capacity) {
		collections.capacity *= 2;
		void           *tmp = realloc(collections.collections,
					      collections.capacity *
					      sizeof(struct SL_Collection));
		if (!tmp)
			perror("realloc failed on collections");
		collections.collections = tmp;
	}

	int             index = collections.size;
	struct SL_Variable name_item = sl_get_argument(*code, func, 0);

	char           *raw_name = sl_string_getter(name_item.vals);
	collections.collections[index].name = strdup(raw_name);
	for (int i = 1; i < func.total_arguments; i++) {
		struct SL_Variable item = sl_get_argument(*code, func, i);
		if (item.type != STRING) {
			return_var.vals =
				"All items must be typed as string! on Collections.create_collection.";
			return_var.type = ERROR;
			return return_var;
		}
		char           *raw_str = sl_string_getter(item.vals);
		if (raw_str[0] == 'v' && raw_str[1] == ':') {
			raw_str = raw_str + 2;	// v:name + 2 = name 
			collections.collections[index].
				attrs[collections.collections
				      [index].total_attrs++] =
				strdup(raw_str);
		}
		else if (raw_str[0] == 'f' && raw_str[1] == ':') {
			raw_str = raw_str + 2;	// f:name:link + 2 = name:link
			char           *token = strtok(raw_str, ":");
			char           *actual_name = NULL;
			char           *link_name = NULL;
			actual_name = strdup(token);
			token = strtok(NULL, ":");
			if (token != NULL)
				link_name = strdup(token);
			else
				link_name = actual_name;


			struct SL_Function *link_func_p =
				sl_get_func(code, actual_name);
			struct SL_Function link_func =
				sl_copy_function(*link_func_p);
			// [var] [self] [=] ["attr_name"] 4 more tokens
			link_func.code_tokens =
				realloc(link_func.code_tokens,
					(link_func.code_len +
					 4) * sizeof(char *));
			link_func.types =
				realloc(link_func.types,
					(link_func.code_len +
					 4) * sizeof(enum TokenTypes));
			memmove(link_func.code_tokens + 4,
				link_func.code_tokens,
				link_func.code_len * sizeof(char *));
			link_func.code_tokens[0] = strdup("var");
			link_func.code_tokens[1] = strdup("self");
			link_func.code_tokens[2] = strdup("=");
			link_func.code_len += 4;
			if (link_func.name != NULL)
				free(link_func.name);

			link_func.name = strdup(link_name);
			collections.collections[index].functions[collections.
								 collections
								 [index].
								 total_funcs++]
				= link_func;
			if (link_name != actual_name) {
				free(link_name);
			}
			free(actual_name);
		}
		free(raw_str - 2);
	}
	int             total_len = strlen(raw_name) + strlen(":new");
	char           *collection_new_name = malloc(total_len + 1);
	snprintf(collection_new_name, total_len + 1, "%s:new", raw_name);
	sl_add_func(code, collection_new_name, collections_new_collection_fn);
	free(raw_name);
	free(collection_new_name);
	collections.size++;
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
int             used_db = 0;
int             used_collections = 0;

struct SL_Variable
use_fn(struct SL_Code *code,
       struct SL_L_Function func, struct SL_Function rfunc)
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
			sl_add_func(code, "io.print", print_fn);
			sl_add_func(code, "io.input", input_fn);
			sl_add_func(code, "io.getchar", io_getchar_fn);
		}
		else if (strcmp(libstr, "file") == 0 && used_file == 0) {
			used_file = 1;
			sl_add_func(code, "file.read_to_str",
				    file_read_to_str_fn);
			sl_add_func(code, "file.write_from_str",
				    file_write_from_str_fn);
			sl_add_func(code, "file.append_from_str",
				    file_append_from_str_fn);
		}
		else if (strcmp(libstr, "types") == 0 && used_types == 0) {
			used_types = 1;
			// CONVERT
			sl_add_func(code, "types.str_to_int", str_to_int_fn);
			sl_add_func(code, "types.int_to_char",
				    int_to_char_fn);
			sl_add_func(code, "types.char_to_int",
				    char_to_int_fn);
			sl_add_func(code, "types.char_to_str",
				    char_to_str_fn);
			sl_add_func(code, "types.int_to_str", int_to_str_fn);

			// TYPE CHECK
			sl_add_func(code, "types.is_int", is_int_fn);
			sl_add_func(code, "types.is_char", is_char_fn);
			sl_add_func(code, "types.is_string", is_string_fn);
			sl_add_func(code, "types.is_double", is_double_fn);
			sl_add_func(code, "types.is_not_initialized",
				    is_not_initialized_fn);
			sl_add_func(code, "types.typeof", typeof_fn);

			// STRING TYPE CHECK
			sl_add_func(code, "types.is_digit", is_digit_fn);

		}
		else if (strcmp(libstr, "sys") == 0 && used_sys == 0) {
			used_sys = 1;
			sl_add_func(code, "sys.get_arg", sys_get_arg_fn);
			sl_add_func(code, "sys.exit", sys_exit_fn);
		}
		else if (strcmp(libstr, "errors") == 0 && used_errors == 0) {
			used_errors = 1;
			sl_add_func(code, "errors.string", errors_string_fn);
			sl_add_func(code, "errors.bool", errors_bool_fn);
			sl_add_func(code, "errors.panic", errors_panic_fn);
		}
		else if (strcmp(libstr, "collections") == 0 && used_sys == 0) {
			used_collections = 1;
			sl_add_func(code, "Collections.create_collection",
				    collections_create_collection_fn);
			sl_add_func(code, "Collections.set_attr",
				    collections_set_attr_fn);
			sl_add_func(code, "Collections.get_attr",
				    collections_get_attr_fn);
		}
		else if (strcmp(libstr, "string") == 0 && used_string == 0) {
			used_string = 1;
			sl_add_func(code, "string.char_at", string_charat_fn);
			sl_add_func(code, "string.split", string_split_fn);
			sl_add_func(code, "string.slice", string_slice_fn);
			sl_add_func(code, "string.trim", string_trim_fn);
			sl_add_func(code, "string.set_char_at",
				    string_setcharat_fn);
			sl_add_func(code, "string.len", string_len_fn);
		}
		else if (strcmp(libstr, "list") == 0 && used_list == 0) {
			used_list = 1;
			sl_add_func(code, "List.new", List_new_fn);
			sl_add_func(code, "List.push", List_push_fn);
			sl_add_func(code, "List.pop", List_pop_fn);
			sl_add_func(code, "List.peek", List_peek_fn);
			sl_add_func(code, "List.set", List_set_fn);
			sl_add_func(code, "List.get", List_get_fn);
			sl_add_func(code, "List.next", List_next_fn);
			sl_add_func(code, "List.iter", List_iter_fn);
			sl_add_func(code, "List.remove", List_remove_fn);
			sl_add_func(code, "List.len", List_len_fn);
		}
		else if (strcmp(libstr, "extra") == 0 && used_extra == 0) {
			used_extra = 1;
			sl_add_func(code, "rand.random", random_fn);
		}
		else if (strcmp(libstr, "db") == 0 && used_db == 0) {
			used_db = 1;
			sl_add_func(code, "db.from_lists", db_from_lists_fn);
			sl_add_func(code, "db.to_lists", db_to_lists_fn);
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
	collections.collections =
		calloc(SL_INIT, sizeof(struct SL_Collection));
	collections.collections->attrs = calloc(SL_INIT, sizeof(char *));
	collections.collections->functions =
		calloc(SL_INIT, sizeof(struct SL_Function));
	collections.size = 0;
	collections.capacity = SL_INIT;
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

	for (int i = 0; i < collections.size; i++) {

		if (collections.collections[i].name != NULL) {
			free(collections.collections[i].name);
			collections.collections[i].name = NULL;
		}

		if (collections.collections[i].attrs != NULL) {
			for (int j = 0;
			     j < collections.collections[i].total_attrs;
			     j++) {
				if (collections.collections[i].attrs[j] !=
				    NULL) {
					free(collections.collections[i].
					     attrs[j]);
					collections.collections[i].attrs[j] =
						NULL;
				}
			}

			free(collections.collections[i].attrs);
			collections.collections[i].attrs = NULL;
		}

		if (collections.collections[i].functions != NULL) {
			for (int j = 0;
			     j < collections.collections[i].total_funcs;
			     j++) {

				struct SL_Function *func =
					&collections.collections[i].
					functions[j];

				if (func->name != NULL) {
					free(func->name);
					func->name = NULL;
				}

				if (func->code_tokens != NULL) {
					for (int k = 0; k < func->code_len;
					     k++) {
						if (func->code_tokens[k] !=
						    NULL) {
							free(func->code_tokens
							     [k]);
							func->code_tokens[k] =
								NULL;
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
					for (int arg_idx = 0;
					     arg_idx < func->total_arguments;
					     arg_idx++) {
						if (func->
						    arguments[arg_idx].name !=
						    NULL) {
							free(func->arguments
							     [arg_idx].name);
							func->arguments
								[arg_idx].name
								= NULL;
						}

						if ((func->
						     arguments[arg_idx].type
						     == STRING
						     ||
						     func->arguments
						     [arg_idx].type == RETURN)
						    &&
						    func->arguments[arg_idx].
						    vals != NULL) {

							free(func->arguments
							     [arg_idx].vals);
							func->arguments
								[arg_idx].vals
								= NULL;
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
