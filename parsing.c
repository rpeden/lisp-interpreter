#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

#include <editline/readline.h>

typedef struct {
	int type;
	long num;
	int err;
} lval;

enum { LVAL_NUM, LVAL_ERR };

enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

lval lval_num(long x){
	lval v;
	v.type = LVAL_NUM;
	v.num = x;
	return v;
}

lval lval_err(int x){
	lval v;
	v.type = LVAL_ERR;
	v.err = x;
	return v;
}

void lval_print(lval v){
	switch(v.type){
		//if it is a number, print it
		case LVAL_NUM: 
			printf("%li", v.num);
			break;
		//if it is an error, inform the user
		case  LVAL_ERR:
			if(v.err == LERR_DIV_ZERO){
				printf("Error: Division by 0");
			}
			if(v.err == LERR_BAD_OP){
				printf("Error: Invalid Operator");
			}
			if(v.err == LERR_BAD_NUM){
				printf("Error: Invalid Number");
			}
		break;
	}
}

void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval eval_op(lval x, char* op, lval y){

	//if either vzlue is an error, return it
	if(x.type == LVAL_ERR) { return x; }
	if(y.type == LVAL_ERR) { return y; }

	if(strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
	if(strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
	if(strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
	if(strcmp(op, "/") == 0) { 
		//if second number is zero, return error
		return y.num == 0 
		  ? lval_err(LERR_DIV_ZERO)
		  : lval_num(x.num / y.num);
	}
	
	return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t){

	//if it's a number, return it
	if(strstr(t->tag, "number")){
		//check for conversion errors
		errno = 0;
		long x = strtol(t->contents, NULL, 10);
		return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
	}

	//operator always second child
	char* op = t->children[1]->contents;

	//store third child in x
	lval x = eval(t->children[2]);

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
			lval result = eval(r.output);
			lval_println(result);
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