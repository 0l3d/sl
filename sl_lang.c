#include "sl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

char          **arguments = NULL;
int             argcN;

struct SL_List
{
	int             capacity;
	struct SL_Variable *vars;
	int             size;
	int fixed;
};

int             LISTS_count = 0;
int             LISTS_capacity = SL_INIT;
struct SL_List *LISTS = { 0 };


struct SL_Variable
print_fn(struct SL_Code *code, struct SL_L_Function func)
{
	struct SL_Variable varib = { 0 };
	for (int i = 0; i < func.total_arguments; i++) {
		varib = sl_get_argument(*code, func, i);
		switch (varib.type) {
		case INTEGER:
			printf("%d", varib.vali);
			break;
		case DOUBLE:
			printf("%f", varib.valf);
			break;
		case STRING:
			if (strchr(varib.vals, '"')) {
				char           *string =
					sl_string_getter(varib.vals);
				printf("%s", string);
				free(string);
			}
			else
				printf("%s", varib.vals);

			break;
		case BOOLEAN:
			printf("%d", varib.valb);
			break;
		case CHAR:
			printf("%c", varib.valc);
			break;
		case LONG:
			printf("%lu", varib.valh);
			break;
		default:
			break;
		}

	}
	return varib;
}

struct SL_Variable
input_fn(struct SL_Code *code, struct SL_L_Function func)
{
	struct SL_Variable varib = { 0 };
	for (int i = 0; i < func.total_arguments; i++) {
		varib = sl_get_argument(*code, func, i);
		switch (varib.type) {
		case INTEGER:
			printf("%d", varib.vali);
			break;
		case DOUBLE:
			printf("%f", varib.valf);
			break;
		case STRING:
			if (strchr(varib.vals, '"')) {
				char           *string =
					sl_string_getter(varib.vals);
				printf("%s", string);
				free(string);
			}
			else
				printf("%s", varib.vals);

			break;
		case BOOLEAN:
			printf("%d", varib.valb);
			break;
		case CHAR:
			printf("%c", varib.valc);
			break;
		case LONG:
			printf("%lu", varib.valh);
			break;
		default:
			break;
		}

	}
	char            string[1024];
	fgets(string, sizeof(string), stdin);

	varib.type = STRING;
	varib.vals = malloc(1024);
	strncpy(varib.vals, string, 1024);

	return varib;
}

struct SL_Variable
sys_getchar_fn(struct SL_Code *code, struct SL_L_Function func)
{
	struct SL_Variable varib = { 0 };
	varib.valc = getchar();
	varib.type = CHAR;
	return varib;
}



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
	struct SL_Variable varib = { 0 };
	varib.type = INTEGER;
	varib.vali = random;
	return varib;
}

struct SL_Variable
str_to_int_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at str_to_int! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable varib = { 0 };
	varib.type = INTEGER;
	varib.vali = atoi(first_arg.vals);
	return varib;
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
	struct SL_Variable varib = { 0 };
	if (first_arg.vali > 255) {
		varib.type = ERROR;
		varib.vals = "Char overflow!";
		return varib;
	}
	varib.valc = first_arg.vali;
	varib.type = CHAR;
	return varib;
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
	struct SL_Variable varib = { 0 };
	varib.vali = first_arg.valc;
	varib.type = INTEGER;
	return varib;
}

struct SL_Variable
read_tostr_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at read.tostr! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable varib = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	FILE           *file_open = fopen(first_arg.vals, "r");
	if (file_open == NULL) {
		varib.type = ERROR;
		varib.vals = "File not found!";
		return varib;
	}

	fseek(file_open, 0, SEEK_END);
	long int        size = ftell(file_open);
	rewind(file_open);

	char           *buffer = malloc(size + 1);
	fread(buffer, 1, size, file_open);
	buffer[size] = '\0';
	fclose(file_open);

	varib.type = STRING;
	varib.vals = strdup(buffer);
	free(buffer);
	return varib;
}

struct SL_Variable
string_getchar_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 2) {
		fprintf(stderr,
			"Error usage at string.getchar! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable varib = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
	if (first_arg.type != STRING) {
		varib.type = ERROR;
		varib.vals =
			"Expected string as the first argument to string.getchar.";
		return varib;
	}

	if (second_arg.type != INTEGER) {
		varib.type = ERROR;
		varib.vals =
			"Expected integer as the second argument to string.getchar.";
		return varib;
	}

	int             len = strlen(first_arg.vals);

	if (second_arg.vali >= len) {
		varib.type = ERROR;
		varib.vals = "Buffer overflow!";
		return varib;
	}

	if (second_arg.vali < 0) {
		varib.type = ERROR;
		varib.vals = "Buffer underflow!";
		return varib;
	}

	varib.valc = first_arg.vals[second_arg.vali];
	varib.type = CHAR;
	return varib;
}

struct SL_Variable
string_len_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at string.len! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable varib = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);

	if (first_arg.type != STRING) {
		varib.type = ERROR;
		varib.vals =
			"Expected string as the first argument to string.getchar.";
		return varib;
	}

	int             len = strlen(first_arg.vals);
	varib.vali = len;
	varib.type = INTEGER;
	return varib;
}



struct SL_Variable
errors_string_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at errors.string! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable varib = { 0 };
	if (first_arg.type == ERROR) {
		printf("%s\n", first_arg.vals);
	}
	return varib;
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
	struct SL_Variable varib = { 0 };
	if (first_arg.type == ERROR) {
		varib.valb = 1;
		varib.type = BOOLEAN;
	}
	return varib;
}

struct SL_Variable
sys_exit_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at sys.exit! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable varib = { 0 };
	if (first_arg.type == INTEGER) {
		exit(first_arg.vali);
	}
	return varib;
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
	struct SL_Variable varib = { 0 };
	if (first_arg.type == INTEGER && first_arg.vali < argcN) {
		varib.type = STRING;
		varib.vals = strdup(arguments[first_arg.vali]);
	}
	else {
		varib.type = ERROR;
		varib.vals = "Argument not found!";
	}
	return varib;
}

struct SL_Variable
typeof_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr,
			"Error usage at typeof! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable varib = { 0 };
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	varib.type = INTEGER;
	varib.vali = first_arg.type;
	return varib;
}



struct SL_Variable
List_new_fn(struct SL_Code *code, struct SL_L_Function func)
{
	struct SL_Variable first_arg;
	int fixed = 0; 
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

	struct SL_Variable varib = { 0 };
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
	} else {
		LISTS[LISTS_count].size = 0;
	}

	varib.type = INTEGER;
	varib.vali = LISTS_count;
	LISTS_count++;

	return varib;
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
	struct SL_Variable varib = { 0 };

	if (first_arg.type != INTEGER) {
		varib.type = ERROR;
		varib.vals =
			"Expected list_variable as the first argument to List.push.";
		return varib;
	}

	if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
		varib.type = ERROR;
		varib.vals = "List buffer overflow!";
		return varib;
	}

	struct SL_List *list = &LISTS[first_arg.vali];

	if (list->size >= list->capacity) {
		list->capacity =
			(list->capacity == 0) ? 8 : list->capacity * 2;
		list->vars =
			realloc(list->vars,
				list->capacity * sizeof(struct SL_Variable));
	}
	list->vars[list->size++] = sl_copy_variable(second_arg);
	varib.type = INTEGER;
	varib.vali = 1;
	return varib;
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
	struct SL_Variable varib = { 0 };

	if (first_arg.type != INTEGER) {
		varib.type = ERROR;
		varib.vals =
			"Expected list_variable as the first argument to List.set.";
		return varib;
	}
	if (second_arg.type != INTEGER) {
		varib.type = ERROR;
		varib.vals =
			"Expected integer as the second argument to List.set.";
		return varib;
	}

	if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
		varib.type = ERROR;
		varib.vals = "List buffer overflow!";
		return varib;
	}

	struct SL_List *list = &LISTS[first_arg.vali];

	if (second_arg.vali >= list->size || second_arg.vali < 0) {
		varib.type = ERROR;
		varib.vals = "Buffer overflow on List element.";
		return varib;
	}
	if ((list->vars[second_arg.vali].type == STRING
	     || list->vars[second_arg.vali].type == RETURN)
	    && list->vars[second_arg.vali].vals != NULL) {
		free(list->vars[second_arg.vali].vals);
	}

	if (list->vars[second_arg.vali].name != NULL) {
		free(list->vars[second_arg.vali].name);
	}

	list->vars[second_arg.vali] = sl_copy_variable(third_arg);

	varib.type = INTEGER;
	varib.vali = 1;
	return varib;
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
	struct SL_Variable varib = { 0 };

	if (first_arg.type != INTEGER) {
		varib.type = ERROR;
		varib.vals =
			"Expected list_variable as the first argument to List.get.";
		return varib;
	}

	if (second_arg.type != INTEGER) {
		varib.type = ERROR;
		varib.vals =
			"Expected integer as the second argument to List.get.";
		return varib;
	}

	if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
		varib.type = ERROR;
		varib.vals = "List buffer overflow!";
		return varib;
	}

	if (second_arg.vali >= LISTS[first_arg.vali].size
	    || second_arg.vali < 0) {
		varib.type = ERROR;
		varib.vals = "Buffer overflow on List element.";
		return varib;
	}

	varib = sl_copy_variable(LISTS[first_arg.vali].vars[second_arg.vali]);
	return varib;
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
	struct SL_Variable varib = { 0 };

	if (first_arg.type != INTEGER) {
		varib.type = ERROR;
		varib.vals =
			"Expected list_variable as the first argument to List.len.";
		return varib;
	}

	if (first_arg.vali >= LISTS_count || first_arg.vali < 0) {
		varib.type = ERROR;
		varib.vals = "List buffer overflow!";
		return varib;
	}

	varib.type = INTEGER;
	varib.vali = LISTS[first_arg.vali].size;
	return varib;
}

int
main(int argc, char **argv)
{
	srand(time(NULL));
	arguments = malloc(argc * sizeof(char *));
	for (int i = 0; i < argc; i++) {
		arguments[i] = strdup(argv[i]);
	}
	argcN = argc;
	char           *code = strdup("./code.sl");
	int             console = 0;
	if (argc > 1) {
		if (strcmp(argv[1], "-c") == 0) {
			console = 1;
		}
		else {
			code = strdup(argv[1]);
		}
	}

	LISTS = calloc(SL_INIT, sizeof(struct SL_List));


	char            buff[1024];
	char          **code_array;

	struct SL_Code  sl_code = sl_init_sl_process();
	sl_add_func(&sl_code, "print", print_fn);
	sl_add_func(&sl_code, "input", input_fn);
	sl_add_func(&sl_code, "random", random_fn);
	sl_add_func(&sl_code, "str_to_int", str_to_int_fn);
	sl_add_func(&sl_code, "int_to_char", int_to_char_fn);
	sl_add_func(&sl_code, "char_to_int", char_to_int_fn);
	sl_add_func(&sl_code, "sys.get_arg", sys_get_arg_fn);
	sl_add_func(&sl_code, "typeof", typeof_fn);
	sl_add_func(&sl_code, "errors.string", errors_string_fn);
	sl_add_func(&sl_code, "errors.bool", errors_bool_fn);
	sl_add_func(&sl_code, "sys.getchar", sys_getchar_fn);
	sl_add_func(&sl_code, "sys.exit", sys_exit_fn);
	sl_add_func(&sl_code, "read.tostr", read_tostr_fn);
	sl_add_func(&sl_code, "string.getchar", string_getchar_fn);
	sl_add_func(&sl_code, "string.len", string_len_fn);
	sl_add_func(&sl_code, "List.new", List_new_fn);
	sl_add_func(&sl_code, "List.push", List_push_fn);
	sl_add_func(&sl_code, "List.set", List_set_fn);
	sl_add_func(&sl_code, "List.get", List_get_fn);
	sl_add_func(&sl_code, "List.len", List_len_fn);



	if (sl_open_sl_process(&sl_code, code) != 0) {
		exit(-1);
	}

	if (sl_close_sl_process(&sl_code) == -1) {
		fprintf(stderr, "close_sl_process failed.");
		return -1;
	}

	for (int i = 0; i < argc; i++) {
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

	free(code);
	return 0;
}
