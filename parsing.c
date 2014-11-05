#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

#include <editline/readline.h>

//assert macro to simplify error handling
#define LASSERT(args, cond, err) \
	if(!(cond)) { lval_delete(args); return lval_err(err); }

//Forward declarations
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;
void lval_print(lval* v);
lval* lval_eval(lenv* e, lval* v);
lval* lval_pop(lval* v, int i);
lval* lval_qexpr(void);
lval* lval_take(lval* v, int i);
lval* builtin(lval* a, char* func);
lval* lenv_get(lenv* e, lval* k);
void lenv_put(lenv* e, lval* k, lval* v);

typedef lval*(*lbuiltin)(lenv*, lval*);

//Lisp Value
struct lval {
	int type;
	long num;
	//error and symbol types have string data
	char* err;
	char* sym;
	lbuiltin fun;
	//count and pointer to a list of lval* 
	int count;
	lval** cell;
};

//Lisp environment
struct lenv{
	int count;
	char** syms;
	lval** vals;
};

enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR, LVAL_FUN };

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
	strcpy(v->err, m);
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

//pointer to a new empty Qexpr lval
lval* lval_qexpr(void){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
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
	if(strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

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
		case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
		case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
		case LVAL_FUN:   printf("<function>"); break;
	}
}

void lval_delete(lval* v){
	switch(v->type){
		//do nothing special for numbers or function pointers
		case LVAL_NUM: break;
		case LVAL_FUN: break;

		//for symbols and errors, free the strings
		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;

		//if sexpr or qexpr, delete all elements inside it
		case LVAL_QEXPR:
		case LVAL_SEXPR: 
		  for(int i = 0; i < v->count; i++){
		  	lval_delete(v->cell[i]);
		  }
		  //also free memory allocated to contain the pointers
		  free(v->cell);
		break;
	}

	//free the struct
	free(v);
}

lval* lval_copy(lval* v){
	
	lval* x = malloc(sizeof(lval));
	x->type = v->type;

	switch(v->type){

		//copy functions and numbers directly
		case LVAL_FUN: x->fun = v->fun; break;
		case LVAL_NUM: x->num = v->num; break;

		//copy strings using malloc and strcpy
		case LVAL_ERR:
			x->err = malloc(strlen(v->err) + 1);
			strcpy(x->err, v->err);
		break;

		case LVAL_SYM:
			x->sym = malloc(strlen(v->sym) + 1);
		break;

		//copy lists by copying each sub expression
		case LVAL_SEXPR:
		case LVAL_QEXPR:
			x->count = v->count;
			x->cell = malloc(sizeof(lval*) * x->count);
			for(int i = 0; i < x->count; i++){
				x->cell[i] = lval_copy(v->cell[i]);
			}
		break;
	}
	return x;
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* lval_fun(lbuiltin func){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_FUN;
	v->fun = func;
	return v;
}

lval* builtin_op(lenv* e, lval* a, char* op){
	//ensure all arguments are numbers
	for(int i = 0; i < a->count; i++){
		if(a->cell[i]->type != LVAL_NUM){
			lval_delete(a);
			return lval_err("Cannot operate on non-number.");
		}
	}

	//pop the first element
	lval* x = lval_pop(a, 0);

	//if no arguments, and subtraction, perform unary negation
	if(strcmp(op, "-") == 0 && a->count == 0){
		x->num = -x->num;
	}

	//while there are still elements remaining
	while(a->count > 0){
		//pop the next element
		lval* y = lval_pop(a, 0);

		if(strcmp(op, "+") == 0){
			x->num += y->num;
			goto cleanup;
		}
		if(strcmp(op, "-") == 0){
			x->num -= y->num;
			goto cleanup;
		}
		if(strcmp(op, "*") == 0){
			x->num *= y->num;
			goto cleanup;
		}
		if(strcmp(op, "/") == 0){
			if(y->num == 0){
				lval_delete(x);
				lval_delete(y);
				x = lval_err("Division by zero does not work in this universe.");
				break;
			}
			x->num /= y->num;
		}

		cleanup:
		lval_delete(y);
	}

	lval_delete(a);
	return x;
}

lval* lval_join(lval* x, lval* y){
	//for each cell in y add it to x
	while(y->count){
		x = lval_add(x, lval_pop(y, 0));
	}

	//delete empty y and return x
	lval_delete(y);
	return x;
}

lval* builtin_head(lenv* e, lval* a){
	//check error conditions
	LASSERT(a, a->count == 1, 
		"Function 'head' received too many arguments");

	LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
		"Function 'head' passed incorrect types");
 	
 	LASSERT(a, a->cell[0]->count != 0, 
 		"Function 'head' was passed {}");
	
	//otherwise take first argument
	lval* v = lval_take(a, 0);

	//delete all elements that are not head and return
	while(v->count > 1) {
		lval_delete(lval_pop(v, 1));
	}
	return v;
}

lval* builtin_tail(lenv* e, lval* a){
	//check error conditions
	LASSERT(a, a->count == 1, 
		"Function 'tail' passed too many arguments");
	
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR, 
		"Function 'tail' passed incorrect types");
	
	LASSERT(a, a->cell[0]->count != 0,
		"Function 'tail' was passed {}");
	
	//take first argument
	lval* v = lval_take(a, 0);

	//delete first element and return
	lval_delete(lval_pop(v, 0));
	return v;
}

lval* builtin_list(lenv* e, lval* a){
	a->type = LVAL_QEXPR;
	return a;
}

lval* builtin_eval(lenv* e, lval* a){
	LASSERT(a, a->count == 1, 
		"Function 'eval' passed too many arguments");

	LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
		"Function 'eval' passed incorrect type");

	lval* x = lval_take(a, 0);
	x->type = LVAL_SEXPR;
	return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a){
	//ensure all args are q expressions
	for(int i = 0; i < a->count; i++){
		LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
			"Function 'join' passed incorrect type");
	}

	lval* x = lval_pop(a, 0);

	while(a->count){
		x = lval_join(x, lval_pop(a, 0));
	}

	lval_delete(a);
	return x;
}

lval* builtin_add(lenv* e, lval* a){
	return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a){
	return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a){
	return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a){
	return builtin_op(e, a, "/");
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func){
	lval* k = lval_sym(name);
	lval* v = lval_fun(func);
	lenv_put(e, k, v);
	//delete these, since the environment copies them
	lval_delete(k);
	lval_delete(v);
}

void lenv_add_builtins(lenv* e){
	//list functions
	lenv_add_builtin(e, "list", builtin_list);
	lenv_add_builtin(e, "head", builtin_head);
	lenv_add_builtin(e, "tail", builtin_tail);
	lenv_add_builtin(e, "eval", builtin_eval);
	lenv_add_builtin(e, "join", builtin_join);

	//math functions
	lenv_add_builtin(e, "+", builtin_add);
	lenv_add_builtin(e, "-", builtin_sub);
	lenv_add_builtin(e, "*", builtin_mul);
	lenv_add_builtin(e, "/", builtin_div);
}

lval* lval_pop(lval* v, int i){
	//find item at i
	lval* x = v->cell[i];

	//shift memory after item i over the top
	memmove(&v->cell[i], &v->cell[i+1],
		sizeof(lval*) * (v->count-i-1));

	//decrease the count of items in the list
	v->count--;

	//reallocate the memory used
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	return x;
}

lval* lval_take(lval* v, int i){
	lval* x = lval_pop(v, i);
	lval_delete(v);
	return x;
}

lval* lval_eval_sexpr(lenv* e, lval* v){
	//eval children
	for(int i = 0; i < v->count; i++){
		v->cell[i] = lval_eval(e, v->cell[i]);
	}

	//check for errors
	for(int i = 0; i < v->count; i++){
		if(v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
	}

	//empty expression
	if(v->count == 0){
		return v;
	}

	//single expression
	if(v->count == 1){
		return lval_take(v, 0);
	}

	//ensure first element is a symbol
	lval* f = lval_pop(v, 0);
	if(f->type != LVAL_FUN){
		lval_delete(f);
		lval_delete(v);
		return lval_err("First element is not a function.");
	}

	lval* result = f->fun(e, v);
	lval_delete(f);
	return result;
}

lval* lval_eval(lenv* e, lval* v){
	
	if(v->type == LVAL_SYM){
		lval* x = lenv_get(e, v);
		lval_delete(v);
		return x;
	}

	//evaluate s-expressions
	if(v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }

	//all other types remain the same
	return v;
}

lenv* lenv_new(void){
	lenv* e = malloc(sizeof(lenv));
	e->count = 0;
	e->syms = NULL;
	e->vals = NULL;
	return e;
}

void lenv_del(lenv* e){
	for(int i = 0; i < e->count; i++){
		free(e->syms[i]);
		lval_delete(e->vals[i]);
	}
	free(e->syms);
	free(e->vals);
	free(e);
}

lval* lenv_get(lenv* e, lval* k){
	//iterate over all elements in environment
	for(int i = 0; i < e->count; i++){
		//check if the stored string matches the symbol string
		//if it does, return a copy of the value
		if(strcmp(e->syms[i], k->sym) == 0){
			return lval_copy(e->vals[i]);
		}
	}
	//if no symbol found return an error
	return lval_err("unbound symbol!");
}

void lenv_put(lenv* e, lval* k, lval* v){
	//iterate over elements in environment 
	//to see if variable already exists
	for(int i = 0; i < e->count; i++){
		//if found, delete item at that position
		//and replace with given variable
		if(strcmp(e->syms[i], k->sym) == 0){
			lval_delete(e->vals[i]);
			e->vals[i] = lval_copy(v);
			return;
		}
	}

	//if no existing entry found, allocate space for new entry
	e->count++;
	e->vals = realloc(e->vals, sizeof(lval*) * e->count);
	e->syms = realloc(e->syms, sizeof(char*) * e->count);

	//copy contents of lval and symbol string into new location
	e->vals[e->count - 1] = lval_copy(v);
	e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
	strcpy(e->syms[e->count-1], k->sym);
}

int main(int argc, char** argv){
	//Make some parsers
	mpc_parser_t* Number 	= mpc_new("number");
	mpc_parser_t* Symbol    = mpc_new("symbol");
	mpc_parser_t* Sexpr 	= mpc_new("sexpr");
	mpc_parser_t* Qexpr     = mpc_new("qexpr");
	mpc_parser_t* Expr		= mpc_new("expr");
	mpc_parser_t* RyLisp 	= mpc_new("rylisp");

	//Define parsers
	mpca_lang(MPCA_LANG_DEFAULT,
		"							             								\
			number     : /-?[0-9]+/ ;            								\
			symbol     : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;  					\
			sexpr      : '(' <expr>* ')' ;             							\
			qexpr  	   : '{' <expr>* '}' ; 			    						\
			expr       : <number> | <symbol> | <sexpr> | <qexpr> ;				\
			rylisp     : /^/ <expr>* /$/ ;            							\
		",
	Number, Symbol, Sexpr, Qexpr, Expr, RyLisp);

	puts("RyLisp Version 0.0.0.0.0.0.1");
	puts("Press Ctrl-C to Exit\n");

	lenv* e = lenv_new();
	lenv_add_builtins(e);

	while(1){
		//display prompt and read input
		char* input = readline("RyLisp> ");

		//add to history
		add_history(input);

		//try to parse it
		mpc_result_t r;
		if(mpc_parse("<stdin>", input, RyLisp, &r)){
			//evaluate the AST and print the result
			lval *result = lval_eval(e, lval_read(r.output));
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

	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, RyLisp);
	return 0;
}