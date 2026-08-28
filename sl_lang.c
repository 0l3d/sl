#include "sl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

char **arguments = NULL;
int argcN;

struct SL_Variable
print_fn(struct SL_Code *code, struct SL_L_Function func)
{
	struct SL_Variable varib;
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
	struct SL_Variable varib;
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
	char string[1024];
	fgets(string, sizeof(string), stdin);

	varib.type = STRING;
	varib.vals = malloc(1024);
	strncpy(varib.vals, string, 1024);

	return varib;
}

struct SL_Variable
random_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 2) {
		fprintf(stderr, "Error usage at random! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable second_arg = sl_get_argument(*code, func, 1);
	int random = rand() % (second_arg.vali - first_arg.vali + 1) + first_arg.vali;
	struct SL_Variable varib;
	varib.type = INTEGER;
	varib.vali = random;
	return varib;
}

struct SL_Variable
str_to_int_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr, "Error usage at str_to_int! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);	
	struct SL_Variable varib;
	varib.type = INTEGER;
	varib.vali = atoi(first_arg.vals);
	return varib;
}

struct SL_Variable
read_tostr_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr, "Error usage at read.tostr! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable varib;
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	FILE *file_open = fopen(first_arg.vals, "r");
	if (file_open == NULL) {
		varib.type = ERROR;
		varib.vals = "File not found!";
		return varib;
	}
	
	fseek(file_open, 0, SEEK_END);
	long int size = ftell(file_open);
	rewind(file_open);

	char*buffer = malloc(size + 1);
	fread(buffer, 1, size, file_open);
	buffer[size] = '\0';
	fclose(file_open);

	varib.type = STRING;
	varib.vals = strdup(buffer);
	free(buffer);
	return varib;
}



struct SL_Variable
errors_string_fn(struct SL_Code *code, struct SL_L_Function func)
{
	if (func.total_arguments < 1) {
		fprintf(stderr, "Error usage at errors.string! Not enough arguments.");
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
		fprintf(stderr, "Error usage at errors.bool! Not enough arguments.");
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
		fprintf(stderr, "Error usage at sys.exit! Not enough arguments.");
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
		fprintf(stderr, "Error usage at sys.get_arg! Not enough arguments.");
		exit(-1);
	}
	struct SL_Variable first_arg = sl_get_argument(*code, func, 0);
	struct SL_Variable varib;
	if (first_arg.type == INTEGER && first_arg.vali < argcN) {
		varib.type = STRING;
		varib.vals = strdup(arguments[first_arg.vali]);
	} else {
		varib.type = ERROR;
		varib.vals = "Argument not found!";
	}
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

	char            buff[1024];
	char          **code_array;

	struct SL_Code  sl_code = sl_init_sl_process();
	sl_add_func(&sl_code, "print", print_fn);
	sl_add_func(&sl_code, "input", input_fn);
	sl_add_func(&sl_code, "random", random_fn);
	sl_add_func(&sl_code, "str_to_int", str_to_int_fn);
	sl_add_func(&sl_code, "sys.get_arg", sys_get_arg_fn);
	sl_add_func(&sl_code, "errors.string", errors_string_fn);
	sl_add_func(&sl_code, "errors.bool", errors_bool_fn);
	sl_add_func(&sl_code, "sys.exit", sys_exit_fn);
	sl_add_func(&sl_code, "read.tostr", read_tostr_fn);



	if (sl_open_sl_process(&sl_code, code) != 0) {
		exit(-1);
	}

	if (sl_close_sl_process(&sl_code) == -1) {
		fprintf(stderr, "close_sl_process failed.");
		return -1;
	}
	
	for(int i = 0; i < argc; i++) {
		free(arguments[i]);
	}
	if (arguments != NULL)
		free(arguments);

	free(code);
	return 0;
}
