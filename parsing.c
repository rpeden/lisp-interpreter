#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

#include <editline/readline.h>

typedef struct lval {
	int type;
	long num;
	//error and symbol types have string data
	char* err;
	char* sym;
	//count and pointer to a list of lval* 
	int count;
	struct lval** cell;
} lval;

void lval_print(lval* v);

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

lval* lval_num(long x){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

lval* lval_err(char* m){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m) + 1);
	return v;
}

lval* lval_sym(char* s){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	strcpy(v->sym, s);
	return v;
}

lval* lval_sexpr(void){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

lval* lval_read_num(mpc_ast_t* t){
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ?
	  lval_num(x) : lval_err("invalid number");
}

lval* lval_add(lval* v, lval* x) {
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count - 1] = x;
	return v;
}

lval* lval_read(mpc_ast_t* t){
	//if symbol or number, return conversion to that type
	if(strstr(t->tag, "number")) { return lval_read_num(t); }
	if(strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

	//if root (>) or sexpr then create empty list
	lval* x = NULL;
	if(strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
	if(strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }

	//fill in this list with any valid expressions contained in it
	for(int i = 0; i < t->children_num; i++){
		if(strcmp(t->children[i]->contents, "(") == 0 ) { continue; }
		if(strcmp(t->children[i]->contents, ")") == 0 ) { continue; }
		if(strcmp(t->children[i]->contents, "{") == 0 ) { continue; }
		if(strcmp(t->children[i]->contents, "}") == 0 ) { continue; }
		if(strcmp(t->children[i]->tag, "regex") == 0 ) { continue; }	
		x = lval_add(x, lval_read(t->children[i]));
	}
	return x;
}

void lval_expr_print(lval* v, char open, char close){
	putchar(open);
	for(int i =0; i < v->count; i++){

		//print the value contained within
		lval_print(v->cell[i]);

		//print space unless we're on the last element
		if(i != (v->count-1)){
			putchar(' ');
		}
	}
	putchar(close);
}

void lval_print(lval* v){
	switch(v->type){
		case LVAL_NUM:   printf("%li", v->num); break;
		case LVAL_ERR:   printf("Error: %s", v->err); break;
		case LVAL_SYM:   printf("%s", v->sym); break;
		case LVAL_SEXPR: lval_expr_print(v, '(', ')');
	}
}

void lval_delete(lval* v){
	switch(v->type){
		//do nothing special for numbers
		case LVAL_NUM: break;

		//for symbols and errors, free the strings
		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;

		//if sexpr, delete all elements inside it
		case LVAL_SEXPR: 
		  for(int i = 0; i < v->count; i++){
		  	lval_delete(v->cell[i]);
		  }
		break;
	}

	//free the struct
	free(v);
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }

/*lval eval_op(lval x, char* op, lval y){

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
	if(strcmp(op, "%") == 0) {
		//if second number is zero, return first number
		return y.num == 0
		  ? lval_num(x.num)
		  : lval_num(x.num % y.num);
	}
	
	return lval_err(LERR_BAD_OP);
}*/

/*lval eval(mpc_ast_t* t){

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
	lval* x = lval_read(r.output);
    lval_println(x);
    lval_delete(x);
	//iterate and combine remaining children
	//int i = 3;
	//while(strstr(t->children[i]->tag, "expr")){
	//	x = eval_op(x, op, eval(t->children[i]));
	//	i++;
	//}

	return x;
}*/

int main(int argc, char** argv){
	//Make some parsers
	mpc_parser_t* Number 	= mpc_new("number");
	mpc_parser_t* Symbol    = mpc_new("symbol");
	mpc_parser_t* Sexpr 	= mpc_new("sexpr");
	mpc_parser_t* Expr		= mpc_new("expr");
	mpc_parser_t* RyLisp 	= mpc_new("rylisp");

	//Define parsers
	mpca_lang(MPCA_LANG_DEFAULT,
		"							             \
			number     : /-?[0-9]+/ ;            \
			symbol   : '+' | '-' | '*' | '/' | '%' ; \
			sexpr      : '(' <expr>* ')' ;             \
			expr       : <number> | <symbol> | <sexpr> ;  \
			rylisp     : /^/ <expr>* /$/ ;    \
		",
	Number, Symbol, Sexpr, Expr, RyLisp);

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
			lval *result = lval_read(r.output);
			lval_println(result);
			lval_delete(result);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		//echo it back out
		//printf("You said %s\n", input);

		//free input
		free(input);
	}

	mpc_cleanup(4, Number, Symbol, Sexpr, Expr, RyLisp);
	return 0;
}