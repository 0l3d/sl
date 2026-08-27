#include "sl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

			if ((varib.type == STRING || varib.type == RETURN)
			    && varib.vals != NULL) {
				free(varib.vals);
			}
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


int
main(int argc, char **argv)
{
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
	sl_dostr_sl_process(&sl_code, "var a = 10");
	sl_add_func(&sl_code, "print", print_fn);
	sl_dostr_sl_process(&sl_code, "print($a, \"\n\")");

	if (sl_open_sl_process(&sl_code, code) != 0) {
		exit(-1);
	}
	struct SL_Function getfu = sl_get_func(sl_code, "print");
	printf("Linked function? : %d\n", getfu.linked_function);
	struct SL_Variable getva = sl_get_var(sl_code, "enable_notify");
	printf("Value of [%s]: %d\n", getva.name, getva.valb);


	if (sl_close_sl_process(&sl_code) == -1) {
		fprintf(stderr, "close_sl_process failed.");
		return -1;
	}

	free(code);
	return 0;
}
