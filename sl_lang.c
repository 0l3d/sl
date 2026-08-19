#include "sl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define GENERAL_S_SIZE 131072
#define MAX_CODE_SIZE 1048576

struct SL_Function funcs[GENERAL_S_SIZE];
struct SL_Variable vars[GENERAL_S_SIZE];


int 
main(int argc, char**argv)
{		
	char *code = strdup("./code.sl");
	int console = 0;
	if (argc > 1) {
		if (strcmp(argv[1], "-c") == 0) {
			console = 1;
		} else {
			code = strdup(argv[1]);
		}
	}

	char buff[1024];
	char **code_array;
	
	if (console == 1) {
		FILE* sl_code = fopen(code, "a+");
		printf("                     SL_LANG Console by 0l3d Under No License (only for testing.)\n                     Copyright (C) 0l3d (Do not Copy/distribute without permission)\n");

		while (console == 1) {
			printf("\n]");
			if (fgets(buff, sizeof(buff), stdin) != NULL) {
		        buff[strcspn(buff, "\n")] = '\0';
    		}
			if (strcmp(buff, "RUN") == 0) {
				fflush(sl_code);
				int count = init_sl_lexer(SL_INIT, code, &code_array, SPECIAL_TOKENS);
				if (count <= 0) {
					fprintf(stderr, "sl_lexer failed\n");
					return -1;
				}
				struct SL_Code code_s = {code_array, count,vars, 0, funcs, 0};
				if (init_sl_parser(code_s) == -1) {
					fprintf(stderr, "sl_parser failed \n");
					return -1;
				};
			} else if (strcmp(buff, "LIST") == 0) {
				char buf2[1024];
		        fflush(sl_code);

		        rewind(sl_code);
    			while (fgets(buf2, sizeof(buf2), sl_code) != NULL) {
        			printf("%s", buf2);
    			}
	            clearerr(sl_code);
    	        fseek(sl_code, 0, SEEK_END);
			} else if (strcmp(buff, "QUIT") == 0) {
				console = 0;
			} else {
				fprintf(sl_code, "%s\n", buff);
				fflush(sl_code);
			}
		}
		fclose(sl_code);
		remove(code);
		free(code);
		return 0;
	}


	int count = init_sl_lexer(SL_INIT, code, &code_array, SPECIAL_TOKENS);
	if (count <= 0) {
		fprintf(stderr, "sl_lexer failed\n");
		return -1;
	}
	
	
	//for (int i = 0; i < count; i++) {
	//	printf("TOKEN[%d]: %s\n", i, code_array[i]);
	//} 
	
	
	struct SL_Code code_s = {code_array, count,vars, 0, funcs, 0};
	if (init_sl_parser(code_s) == -1) {
		fprintf(stderr, "sl_parser failed \n");
		return -1;
	};
	free(code);
	return 0;
}
