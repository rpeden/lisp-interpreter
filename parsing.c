#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

#include <editline/readline.h>

long eval_op(long x, char* op, long y){
	if(strcmp(op, "+") == 0) { return x + y; }
	if(strcmp(op, "-") == 0) { return x - y; }
	if(strcmp(op, "*") == 0) { return x * y; }
	if(strcmp(op, "/") == 0) { return x / y; }
	return 0;
}

long eval(mpc_ast_t* t){

	//if it's a number, return it
	if(strstr(t->tag, "number")){
		return atoi(t->contents);
	}

	//operator always second child
	char* op = t->children[1]->contents;

	//store third child in x
	long x = eval(t->children[2]);

	//iterate and combine remaining children
	int i = 3;
	while(strstr(t->children[i]->tag, "expr")){
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	return x;
}

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

		//try to parse it
		mpc_result_t r;
		if(mpc_parse("<stdin>", input, RyLisp, &r)){
			//evaluate the AST and print the result
			long result = eval(r.output);
			printf("%li\n", result);
			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		//echo it back out
		//printf("You said %s\n", input);

		//free input
		free(input);
	}

	mpc_cleanup(4, Number, Operator, Expr, RyLisp);
	return 0;
}