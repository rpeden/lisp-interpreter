#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

#include <editline/readline.h>

int main(int argc, char** argv){
	//Make some parsers
	mpc_parser_t* Number 	= mpc_new("number");
	mpc_parser_t* Operator 	= mpc_new("operator");
	mpc_parser_t* Expr		= mpc_new("expr");
	mpc_parser_t* RyLisp 	= mpc_new("rylisp");

	//Define parsers
	mpca_lang(MPCA_LANG_DEFAULT,
		"							             \
			number     : /-?[0-9]+/ ;            \
			operator   : '+' | '-' | '*' | '/' ; \
			expr       : <number> | '(' <operator> <expr>+ ')' ;  \
			rylisp     : /^/ <operator> <expr>+ /$/ ;    \
		",
	Number, Operator, Expr, RyLisp);

	puts("RyLisp Version 0.0.0.0.0.0.1");
	puts("Press Ctrl-C to Exit\n");

	while(1){
		//display prompt and read input
		char* input = readline("RyLisp> ");

		//add to history
		add_history(input);

		//echo it back out
		printf("You said %s\n", input);

		//free input
		free(input);
	}

	mpc_cleanup(4, Number, Operator, Expr, RyLisp);
	return 0;
}