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
char* ltype_name(int t);
lenv* lenv_new(void);
void lenv_del(lenv* e);
lenv* lenv_copy(lenv* e);