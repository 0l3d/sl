#ifndef SL_H
#define SL_H

enum SL_Types {
	INIT = 0,
	INTEGER = 1,
	DOUBLE = 2,
	CHAR = 3,
	STRING = 4,
	BOOLEAN = 5,
	RETURN = 6,
	LONG = 7,
};

struct SL_Variable {
	char           *name;
	enum SL_Types 	type;
	union {
		int 		vali;
		double 		valf;
		int 		valb;
		char 		valc;
		char           *vals;
		long 		valh;
	};
};

struct SL_Function {
	char           *name;
	enum SL_Types 	rettype;
	struct SL_Variable **arguments;
};

struct SL_Code {
	char          **code;
	struct SL_Variable *vars;
	int total_vars;
	struct SL_Function *funcs;
	int total_funcs;
};

int 		add_func_to_sl(struct SL_Code * code, struct SL_Function * func);
int 		add_var_to_sl(struct SL_Code * code, struct SL_Variable * var);
struct SL_Variable getvar_from_sl(struct SL_Code code, const char *name);
struct SL_Function getfunc_from_sl(struct SL_Code code, const char *name);
int 		init_sl_lexer(char *bufin, char *bufout[], int max_count, char *special_tokens);
int 		init_sl_parser(struct SL_Code code_s, int max_line, int max_token);
int 		use_custom_sl_parser();

#endif
