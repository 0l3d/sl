#include "sl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>


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
};

int             LISTS_count = 0;
int             LISTS_capacity = SL_INIT;
struct SL_List *LISTS = { 0 };

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
			printf("%d", return_var.valb);
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
			printf("%d", return_var.valb);
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

	return_var.type = STRING;
	return_var.vals = malloc(1024);
	strncpy(return_var.vals, string, 1024);

	return return_var;
}

struct SL_Variable
sys_getchar_fn(struct SL_Code *code, struct SL_L_Function func)
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
			"Error usage at read.tostr! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	FILE           *file_open = fopen(first_arg.vals, "r");
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
	return return_var;
}

struct SL_Variable
file_write_from_str_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at read.tostr! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
	FILE           *file_open = fopen(first_arg.vals, "w");
	if (file_open == NULL) {
		return_var.type = ERROR;
		return_var.vals = "File not found!";
		return return_var;
	}
	size_t len = strlen(second_arg.vals);
    
	if (fwrite(second_arg.vals, 1, len, file_open) != len) {
        fclose(file_open);

        return_var.type = ERROR;
        return_var.vals = "Could not write to file!";
        return return_var;
    }

    fclose(file_open);

    return_var.type = STRING;
    return_var.vals = strdup(second_arg.vals);

	return return_var;
}

struct SL_Variable
file_append_from_str_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at read.tostr! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
	FILE           *file_open = fopen(first_arg.vals, "a");
	if (file_open == NULL) {
		return_var.type = ERROR;
		return_var.vals = "File not found!";
		return return_var;
	}
	size_t len = strlen(second_arg.vals);
    
	if (fwrite(second_arg.vals, 1, len, file_open) != len) {
        fclose(file_open);

        return_var.type = ERROR;
        return_var.vals = "Could not write to file!";
        return return_var;
    }

    fclose(file_open);

    return_var.type = STRING;
    return_var.vals = strdup(second_arg.vals);

	return return_var;
}



// EXTRA
struct SL_Variable
random_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 2) {
		fprintf(stderr,
			"Error usage at random! Not enough arguments.");
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
			"Error usage at str_to_int! Not enough arguments.");
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
			"Error usage at int_to_char! Not enough arguments.");
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
			"Error usage at char_to_int! Not enough arguments.");
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
			"Error usage at typeof! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	return_var.type = INTEGER;
	return_var.vali = first_arg.type;
	return return_var;
}



// STRING
struct SL_Variable
string_getchar_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 2) {
		fprintf(stderr,
			"Error usage at string.getchar! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable return_var = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
	if (first_arg.type != STRING) {
		return_var.type = ERROR;
		return_var.vals =
			"Expected string as the first argument to string.getchar.";
		return return_var;
	}

	if (second_arg.type != INTEGER) {
		return_var.type = ERROR;
		return_var.vals =
			"Expected integer as the second argument to string.getchar.";
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
			"Expected string as the first argument to string.getchar.";
		return return_var;
	}

	int             len = strlen(first_arg.vals);
	return_var.vali = len;
	return_var.type = INTEGER;
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
	}
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
		printf("%s\n", first_arg.vals);
		exit(-1);
	}
	return return_var;
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

	LISTS[LISTS_count].vars =
		calloc(capacity, sizeof(struct SL_Variable));
	LISTS[LISTS_count].capacity = capacity;
	LISTS[LISTS_count].fixed = fixed;
	if (fixed == 1) {
		LISTS[LISTS_count].size = capacity;
		for (int i = 0; i < capacity; i++) {
			LISTS[LISTS_count].vars[i].type = INTEGER;
			LISTS[LISTS_count].vars[i].vali = 0;
		}
	}
	else {
		LISTS[LISTS_count].size = 0;
	}

	return_var.type = INTEGER;
	return_var.vali = LISTS_count;
	LISTS_count++;

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
	struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
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

	if (list->size >= list->capacity) {
		list->capacity =
			(list->capacity == 0) ? 8 : list->capacity * 2;
		list->vars =
			realloc(list->vars,
				list->capacity * sizeof(struct SL_Variable));
	}
	list->vars[list->size++] = sl_copy_variable(second_arg);
	return_var.type = INTEGER;
	return_var.vali = 1;
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
	struct SL_Variable ret = sl_copy_variable(list->vars[list->size - 1]);
	
	if (list->vars[list->size - 1].name != NULL) {
		free(list->vars[list->size - 1].name);
		list->vars[list->size - 1].name = NULL;
	}
	
	if ((list->vars[list->size - 1].type == STRING || list->vars[list->size - 1].type == RETURN) 
	    && list->vars[list->size - 1].vals != NULL) {
		free(list->vars[list->size - 1].vals);
		list->vars[list->size - 1].vals = NULL;
	}
	
	list->size--;
	return ret;
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

	if (second_arg.vali >= list->size || second_arg.vali < 0) {
		return_var.type = ERROR;
		return_var.vals = "Buffer overflow on List element.";
		return return_var;
	}
	if ((list->vars[second_arg.vali].type == STRING
	     || list->vars[second_arg.vali].type == RETURN)
	    && list->vars[second_arg.vali].vals != NULL) {
		free(list->vars[second_arg.vali].vals);
		list->vars[second_arg.vali].vals = NULL;
	}

	if (list->vars[second_arg.vali].name != NULL) {
		free(list->vars[second_arg.vali].name);
		list->vars[second_arg.vali].name = NULL;
	}

	list->vars[second_arg.vali] = sl_copy_variable(third_arg);
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

struct SL_Code *use_code = NULL;
int used_io = 0;
int used_file = 0;
int used_types = 0; 
int used_sys = 0; 
int used_string = 0;
int used_errors = 0;
int used_list = 0; 
int used_extra = 0;


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
		char           *libstr =
					sl_string_getter(lib.vals);

		if (strcmp(libstr, "io") == 0 && used_io == 0) {
			used_io = 1;
			sl_add_func(use_code, "io.print", print_fn);
			sl_add_func(use_code, "io.input", input_fn);
		} else if (strcmp(libstr, "file") == 0 && used_file == 0) {
			used_file = 1;
			sl_add_func(use_code, "file.read_to_str", file_read_to_str_fn);
			sl_add_func(use_code, "file.write_to_str", file_write_from_str_fn);
			sl_add_func(use_code, "file.append_to_str", file_append_from_str_fn);
		} else if (strcmp(libstr, "types") == 0 && used_types == 0) {
			used_types = 1;
			sl_add_func(use_code, "types.str_to_int", str_to_int_fn);
			sl_add_func(use_code, "types.int_to_char", int_to_char_fn);
			sl_add_func(use_code, "types.char_to_int", char_to_int_fn);
			sl_add_func(use_code, "types.typeof", typeof_fn);
		} else if (strcmp(libstr, "sys") == 0 && used_sys == 0) {
			used_sys = 1;
			sl_add_func(use_code, "sys.get_arg", sys_get_arg_fn);
			sl_add_func(use_code, "sys.getchar", sys_getchar_fn);
			sl_add_func(use_code, "sys.exit", sys_exit_fn);
		} else if (strcmp(libstr, "errors") == 0 && used_errors == 0)  {
			used_errors = 1;
			sl_add_func(use_code, "errors.string", errors_string_fn);
			sl_add_func(use_code, "errors.bool", errors_bool_fn);
			sl_add_func(use_code, "errors.panic", errors_panic_fn);
		} else if (strcmp(libstr, "string") == 0 && used_string == 0)  {
			used_string = 1;
			sl_add_func(use_code, "string.getchar", string_getchar_fn);
			sl_add_func(use_code, "string.len", string_len_fn);
		} else if (strcmp(libstr, "list") == 0 && used_list == 0) {
			used_list = 1;
			sl_add_func(use_code, "List.new", List_new_fn);
			sl_add_func(use_code, "List.push", List_push_fn);
			sl_add_func(use_code, "List.pop", List_pop_fn);
			sl_add_func(use_code, "List.peek", List_peek_fn);
			sl_add_func(use_code, "List.set", List_set_fn);
			sl_add_func(use_code, "List.get", List_get_fn);
			sl_add_func(use_code, "List.len", List_len_fn);
		} else if (strcmp(libstr, "extra") == 0 && used_extra == 0) {
			used_extra = 1;
			sl_add_func(use_code, "rand.random", random_fn);
		} else {
			fprintf(stderr, "Unknown library.");
			exit(-1);
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
