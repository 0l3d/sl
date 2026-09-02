#include "sl.h"
#include "stdlib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

int
main(int argc, char **argv)
{
	char           *code = NULL;
	int             console = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "-c") == 0) {
			console = 1;
			code = strdup("./code.sl");
		}
		else {
			code = strdup(argv[1]);
		}
	}
	else {
		code = strdup("./code.sl");
	}
	char            buff[1024];
	char          **code_array;

	struct SL_Code  sl_code = sl_init_sl_process();
	init_sl_stdlib(&sl_code, argc, argv);

	if (sl_open_sl_process(&sl_code, code) != 0) {
		free(sl_code.types);
		exit(-1);
	}


	if (sl_close_sl_process(&sl_code) == -1) {
		fprintf(stderr, "close_sl_process failed.");
		return -1;
	}
	close_sl_stdlib();
	free(code);
	return 0;
}
