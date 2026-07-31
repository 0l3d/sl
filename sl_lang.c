#include "sl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define GENERAL_S_SIZE 131072
#define MAX_CODE_SIZE 1048576

struct SL_Function funcs[GENERAL_S_SIZE];
struct SL_Variable vars[GENERAL_S_SIZE];

#define SPECIAL_TOKENS "()+-/*%^&|=<>,"

int 
main()
{	
	char **code_array;
	int count = init_sl_lexer(SL_INIT, "./code.sl", &code_array, SPECIAL_TOKENS);
	if (count <= 0) {
		fprintf(stderr, "sl_lexer failed\n");
		return -1;
	}
	printf("TOKEN [0]: %s", code_array[0]);

	

	/*
	struct SL_Code code_s = {code_array, count,vars, 0, funcs, 0};
	if (init_sl_parser(code_s) == -1) {
		fprintf(stderr, "sl_parser failed \n");
		return -1;
	};*/
}
