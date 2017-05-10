#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"
#include <editline/readline.h>

/* Forward declarations */
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;


// Pre definitins

lval* lval_eval_sexpr(lenv* e,lval* v);

/* lval types */
enum { LVAL_NUM, LVAL_ERR, LVAL_FUN,
	   LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

typedef lval*(*lbuiltin)(lenv*, lval*);

/* lval struct */
typedef struct lval {
	int type;
	long num;
	char* err;
	char* sym;
	lbuiltin fun;
	int count;
	struct lval** cell;
} lval ;

lval* lval_num(long x){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

lval* lval_fun(lbuiltin func) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_FUN;
	v->fun  = func;
	return v;
}

lval* lval_qexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

/* Create a new error type lval */
lval* lval_err(char* fmt, ...) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;

  /* Create a va list and initialize it */
  va_list va;
  va_start(va, fmt);

  /* Allocate 512 bytes of space */
  v->err = malloc(512);

  /* printf the error string with a maximum of 511 characters */
  vsnprintf(v->err, 511, fmt, va);

  /* Reallocate to number of bytes actually used */
  v->err = realloc(v->err, strlen(v->err)+1);

  /* Cleanup our va list */
  va_end(va);

  return v;
}
/*Symbol lval */
lval* lval_sym(char* s) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	strcpy(v->sym, s);
	return v;
}

/*Sexpr lval */
lval* lval_sexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type =LVAL_SEXPR;
	v->count =0;
	v->cell =NULL;
	return v;
}

lval* lval_copy(lval* v) {
	lval* x = malloc(sizeof(lval));
	x->type = v->type;

	switch(v->type) {
		// Copy functions and numbers directly
		case LVAL_FUN: x->fun = v->fun; break;
		case LVAL_NUM: x->num = v->num; break;

		// Copy strings using malloc and strcpy
		case LVAL_ERR:
			x->err = malloc(strlen(v->err) + 1);
			strcpy(x->err, v->err); break;

		case LVAL_SYM:
			x->sym = malloc(strlen(v->sym) + 1);
			strcpy(x->sym,v->sym); break;

		// Copy lists by coping each subexpression
		case LVAL_SEXPR:
		case LVAL_QEXPR:
			x->count = v->count;
			x->cell = malloc(sizeof(lval*) * x->count);
			for(int i = 0; i < x->count; i++) {
				x->cell[i] = lval_copy(v->cell[i]);
			}
		break;
	}
	return x;
}

void lval_del(lval* v) {
	switch(v->type) {
		case LVAL_NUM: break;
		case LVAL_FUN: break;

		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;

		/* Delete all elements in sexpr or qexpr */
		case LVAL_QEXPR:
		case LVAL_SEXPR:
		for(int i = 0; i < v->count; i++) {
		       lval_del(v->cell[i]);
	       }
	       /* FRee memory allocated to pointers */
	       free(v->cell);
	       break;
	}
	/* Free memory allocated to struct */
	free(v);
}

lval* lval_add(lval* v, lval* x) {
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}


lval* lval_join(lval* x, lval* y) {
	for (int i = 0; i < y->count; i++) {
      x = lval_add(x, y->cell[i]);
    }
    free(y->cell);
    free(y);
    return x;
}

lval* lval_pop(lval* v, int i) {
	/* find item at "i" */
	lval* x = v->cell[i];

	/* Shift the memory after i over the top */
	memmove(&v->cell[i], &v->cell[i+1],sizeof(lval*) * (v->count-i-1));

	/* Decrement count of items on list */
	v->count--;

	/* Realloc memory */
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	return x;
}

lval* lval_take(lval* v, int i) {
	lval* x = lval_pop(v, i);
	lval_del(v);
	return x;
}

void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
	putchar(open);
	for(int i = 0; i < v->count; i++){
		/* print value */
		lval_print(v->cell[i]);

		/* dont print trailing lastspace */
		if(i != (v->count-1)) {
			putchar(' ');
		}
	}
	putchar(close);
}
/* Print an lval */
void lval_print(lval* v) {
	switch(v->type) {
		case LVAL_NUM: printf("%li", v->num); break;
		case LVAL_ERR: printf("Error: %s", v->err); break;
		case LVAL_SYM: printf("%s", v->sym); break;
		case LVAL_FUN: printf("<function>"); break;
		case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
		case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
	}
}

/* print an lval and new line */
void lval_println(lval* v) {
	lval_print(v);
   	putchar('\n');
}

char* ltype_name(int t) {
  switch(t) {
    case LVAL_FUN: return "Function";
    case LVAL_NUM: return "Number";
    case LVAL_ERR: return "Error";
    case LVAL_SYM: return "Symbol";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default: return "Unknown";
  }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////// LISP enviromnent ////////////////////////////////////////////////
struct lenv {
	int count;
	char** syms;
	lval** vals;
};

lenv* lenv_new(void) {
	lenv* e = malloc(sizeof(lenv));
	e->count = 0;
	e->syms = NULL;
	e->vals = NULL;
	return e;
}

void lenv_del(lenv* e) {

	// delete all environment items
	for(int i = 0; i < e->count; i++) {
		free(e->syms[i]);
		lval_del(e->vals[i]);
	}
	free(e->syms);
	free(e->vals);
	free(e);
}

lval* lenv_get(lenv* e, lval* k) {
	// Iterate over environment
	for(int i = 0; i < e->count; i++) {
		// Check if the stored string matches symbol string
		// If so, returna copy of the value
		if(strcmp(e->syms[i], k->sym) == 0) {
			return lval_copy(e->vals[i]);
		}
	}
	// Error if not found
	return lval_err("Unbound symbol '%s'", k->sym);
}

void lenv_put(lenv* e, lval* k, lval* v) {

	// Check if item exists
	for(int i = 0; i < e->count; i++) {
		// If found, delete at position and replace with user supplied value
		if(strcmp(e->syms[i], k->sym) == 0) {
			lval_del(e->vals[i]);
			e->vals[i] = lval_copy(v);
			return;
		}
	}

	// If no entry found, allocate space
	e->count++;
	e->vals = realloc(e->vals, sizeof(lval*) * e->count);
	e->syms = realloc(e->syms, sizeof(char*) * e->count);

	// Copy contents
	e->vals[e->count-1] = lval_copy(v);
	e->syms[e->count-1] = malloc(strlen(k->sym) + 1);
	strcpy(e->syms[e->count-1], k->sym);
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////// builtins ////////////////////////////////////////////

#define LASSERT(args, cond, fmt, ...) \
  if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err; }

#define LASSERT_TYPE(func, args, index, expect) \
  LASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
    func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
  LASSERT(args, args->count == num, \
    "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
    func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
  LASSERT(args, args->cell[index]->count != 0, \
    "Function '%s' passed {} for argument %i.", func, index);


lval* lval_eval(lenv* e,lval* v);

lval* builtin_list(lenv* e, lval* a) {
	a->type = LVAL_QEXPR;
	return a;
}

lval* builtin_head(lenv* e, lval* a) {
	/* Check error conditions */
	LASSERT_NUM("head", a, 1);
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("head", a, 0);

	/* Otherwise take first argument */
	lval* v = lval_take(a, 0);

	/* Delete all elements thare are not head and return */
	while(v->count > 1) { lval_del(lval_pop(v, 1));}
	return v;
}

lval* builtin_tail(lenv* e, lval* a) {
	LASSERT_NUM("tail", a, 1);
	LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
	LASSERT_NOT_EMPTY("tail", a, 0);

	/* Take first argument */
	lval* v = lval_take(a, 0);

	/* Delete first element and return */
	lval_del(lval_pop(v, 0));
	return v;
}

lval* builtin_eval(lenv* e, lval* a) {
	LASSERT_NUM("eval", a, 1);
	LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

	lval* x = lval_take(a, 0);
	x->type = LVAL_SEXPR;
	return lval_eval(e, x);
}


lval* builtin_join(lenv* e, lval* a) {

	for (int i = 0; i < a->count; i++) {
		LASSERT_TYPE("join", a, i, LVAL_QEXPR);
	}

	lval* x = lval_pop(a, 0);

	while(a->count) {
		x = lval_join(x,lval_pop(a,0));
	}

	lval_del(a);
	return x;
}

lval* builtin_op(lenv* e, lval* a, char* op) {
	/* ENsure all arguments are numbers */
	for (int i = 0; i < a->count; i++) {
		LASSERT_TYPE(op, a, i, LVAL_NUM);
    }

	/* pop first element */
	lval* x =lval_pop(a, 0);

	/* If no arguments and sub thenperform unary negation */
	if((strcmp(op, "-") == 0) && a->count == 0) {
		x->num = -x->num;
	}

	/* While there are still elements remaining */
	while(a-> count > 0) {
		/* pop the next element */
		lval* y = lval_pop(a,0);

		if(strcmp(op, "+") == 0) { x->num += y->num; }
		if(strcmp(op, "-") == 0) { x->num -= y->num; }
		if(strcmp(op, "*") == 0) { x->num *= y->num; }
		if(strcmp(op, "/") == 0) {
			if(y->num == 0) {
				lval_del(x);
				lval_del(y);
				x = lval_err("Division by zero!");
				break;
			}
			x->num /= y->num;
		}

		lval_del(y);
	}
	lval_del(a);
	return x;
}

lval* builtin_add(lenv* e, lval* a) {
	return builtin_op(e, a, "+");
}
lval* builtin_sub(lenv* e, lval* a) {
	return builtin_op(e, a, "-");
}
lval* builtin_mul(lenv* e, lval* a) {
	return builtin_op(e, a, "*");
}
lval* builtin_div(lenv* e, lval* a) {
	return builtin_op(e, a, "/");
}

lval* builtin_def(lenv* e, lval* a) {
	LASSERT_TYPE("def", a, 0, LVAL_QEXPR);

	// First argument is a symbol list
	lval* syms = a->cell[0];

	// Ensure all elements of a first list are symbols
	for(int i = 0; i < syms->count; i++) {
		LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
	      "Function 'def' cannot define non-symbol. "
	      "Got %s, Expected %s.",
	      ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
	}

	// Check correct nuber of symbols
	LASSERT(a, (syms->count == a->count-1),
      "Function 'def' passed too many arguments for symbols. "
      "Got %i, Expected %i.",
      syms->count, a->count-1);

	// Assign copies of values to symbols
	for (int i = 0; i < syms->count; i++) {
		lenv_put(e, syms->cell[i], a->cell[i+1]);
	}

	lval_del(a);
	return lval_sexpr();
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
	lval* k = lval_sym(name);
	lval* v = lval_fun(func);
	lenv_put(e, k, v);
	lval_del(k);
	lval_del(v);
}

void lenv_add_builtins(lenv* e) {
	// Variables funcions
	lenv_add_builtin(e, "def", builtin_def);

	// List functions
	lenv_add_builtin(e, "list", builtin_list);
	lenv_add_builtin(e, "head", builtin_head);
	lenv_add_builtin(e, "tail", builtin_tail);
	lenv_add_builtin(e, "eval", builtin_eval);
	lenv_add_builtin(e, "join", builtin_join);

	// Mathematical functions
	lenv_add_builtin(e, "+", builtin_add);
	lenv_add_builtin(e, "-", builtin_sub);
	lenv_add_builtin(e, "*", builtin_mul);
	lenv_add_builtin(e, "/", builtin_div);

}

//////////////////////////////////////////////////////////////////////////////
//////////////////////// Evaluation ///////////////////////////////////////////////

lval* lval_eval_sexpr(lenv* e, lval* v) {

	/* Evaluate children */
	for(int i = 0; i < v->count; i++) {
		v->cell[i] = lval_eval(e, v->cell[i]);
	}

	/* Error checking */
	for(int i = 0; i < v->count; i++) {
		if(v->cell[i]->type == LVAL_ERR) {
			return lval_take(v, i);
		}
	}

	/* Empty expression */
	if(v->count == 0) {return v;}

	/* Single expression */
	if(v->count == 1) { return lval_take(v, 0); }

	// Ensure first element is function after eval
	lval* f = lval_pop(v, 0);
	if(f->type != LVAL_FUN) {
		lval* err = lval_err(
	      "S-Expression starts with incorrect type. "
	      "Got %s, Expected %s.",
	      ltype_name(f->type), ltype_name(LVAL_FUN));
	    lval_del(f); lval_del(v);
	    return err;
	}

	// If so,call function to get a result
	lval* result = f->fun(e, v);
	lval_del(f);
	return result;
}

lval* lval_eval(lenv* e, lval* v) {
	if(v->type == LVAL_SYM) {
		lval* x = lenv_get(e, v);
		lval_del(v);
		return x;
	}

	/* Evaluate S-Expression */
	if(v->type == LVAL_SEXPR) {
		return lval_eval_sexpr(e,v);
	}

	/* All other lval types remain the same */
	return v;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////// Reading ///////////////////////////////////////////////


lval* lval_read_num(mpc_ast_t* t){
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ?
		lval_num(x) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {
	/* If symbol or number, return conversion to that type */
	if(strstr(t->tag, "number")) { return lval_read_num(t); }
	if(strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

	/* If root or sexpr, create empty list */
	lval* x = NULL;
	if(strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
	if(strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
	if(strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

	/* fill the list with any valid expressions contained within */
	for(int i = 0; i < t->children_num; i++) {
		if(strcmp(t->children[i]->contents, "(") == 0) { continue; }
		if(strcmp(t->children[i]->contents, ")") == 0) { continue; }
		if(strcmp(t->children[i]->contents, "{") == 0) { continue; }
		if(strcmp(t->children[i]->contents, "}") == 0) { continue; }
		if(strcmp(t->children[i]->tag, "regex") == 0) { continue; }
		x = lval_add(x, lval_read(t->children[i]));
	}

	return x;
}

int main(int argc, char** argv) {
	/* Create parsers*/
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Sexpr  = mpc_new("sexpr");
	mpc_parser_t* Qexpr  = mpc_new("qexpr");
	mpc_parser_t* Expr   = mpc_new("expr");
	mpc_parser_t* Lispy  = mpc_new("lispy");

	mpca_lang(MPCA_LANG_DEFAULT,
	"                                        						\
	number : /-?[0-9]+/ ;                    						\
	symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;						\
	sexpr  : '(' <expr>* ')' ;               						\
	qexpr  : '{' <expr>* '}' ;										\
	expr   : <number> | <symbol> | <sexpr> | <qexpr>; 				\
	lispy  : /^/ <expr>* /$/ ;               						\
	",
	Number, Symbol, Sexpr, Qexpr, Expr, Lispy);


	puts("Lispy version 0.0.0.0.7");
	puts("Press Ctrl+C to Exit\n");

	lenv* e = lenv_new();
	lenv_add_builtins(e);

	while(1) {
		char* input = readline("lispy> ");

		add_history(input);

		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			lval* x = lval_eval(e, lval_read(r.output));
			lval_println(x);
			lval_del(x);
			mpc_ast_delete(r.output);
		} else {
			/* Otherwise, print the error */
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);
	}
	lenv_del(e);

	/* Undefine and delete parsers */
	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
	return 0;
}
