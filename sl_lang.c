#include "sl.h"
#include <stdio.h>
#include <string.h>

#define GENERAL_S_SIZE 2048

struct SL_Function funcs[GENERAL_S_SIZE];
struct SL_Variable vars[GENERAL_S_SIZE];

int 
main()
{
	FILE           *code = fopen("code.sl", "r");
	if (code == NULL) {
		perror("Code read failed.");
		return -1;
	}
	char 		buf      [GENERAL_S_SIZE];
	char           *code_lines[GENERAL_S_SIZE];
	int 		total_lines = 0;

	while (fgets(buf, sizeof(buf), code)) {
		code_lines[total_lines++] = strdup(buf);
	}
	struct SL_Code 	code_s = {code_lines, vars, funcs};
	if (init_sl_parser(code_s, total_lines, 1024) == -1) {
		fprintf(stderr, "Failed to start sl parser.");
		return -1;
	};

	fclose(code);
}
