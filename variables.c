#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"
#include <editline/readline.h>

#define LASSERT(args, cond, err) \
	if(!(cond)) { lval_del(args); return lval_err(err); }
/* lval struct */
typedef struct lval {
	int type;
	long num;
	char* err;
	char* sym;
	int count;
	struct lval** cell;
} lval ;

/* lval types */
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

/* Create a new number type lval */
lval* lval_num(long x){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
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
lval* lval_err(char* m) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m) + 1);
	strcpy(v->err, m);
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

void lval_del(lval* v) {
	switch(v->type) {
		case LVAL_NUM: break;

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
		case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
		case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
	}
}

/* print an lval and new line */
void lval_println(lval* v) {
	lval_print(v);
   	putchar('\n');
}

lval* builtin_head(lval* a) {
	/* Check error conditions */
	LASSERT(a, a->count == 1,
      "Function 'head' passed too many arguments!");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
      "Function 'head' passed incorrect type!");
    LASSERT(a, a->cell[0]->count != 0,
      "Function 'head' passed {}!");

	/* Otherwise take first argument */
	lval* v = lval_take(a, 0);

	/* Delete all elements thare are not head and return */
	while(v->count > 1) { lval_del(lval_pop(v, 1));}
	return v;
}

lval* builtin_tail(lval* a) {
	/* Error conditions */
	LASSERT(a, a->count == 1,
      "Function 'tail' passed too many arguments!");
    LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
      "Function 'tail' passed incorrect type!");
    LASSERT(a, a->cell[0]->count != 0,
      "Function 'tail' passed {}!");

	/* Take first argument */
	lval* v = lval_take(a, 0);

	/* Delete first element and return */
	lval_del(lval_pop(v, 0));
	return v;
}

lval* builtin_list(lval* a) {
	a->type = LVAL_QEXPR;
	return a;
}

lval* lval_eval(lval* v);


lval* builtin_eval(lval* a) {
	LASSERT(a, a->count == 1,
		"Function 'eval' passed too many arguments!");
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
		"Function 'eval' passed incorrect type!");

	lval* x = lval_take(a, 0);
	x->type = LVAL_SEXPR;
	return lval_eval(x);
}

lval* lval_join(lval* x, lval* y) {
	/* foreach cell in y, add it to x */
	while(y->count) {
		x = lval_add(x, lval_pop(y, 0));
	}

	/* Return y and return x*/
	lval_del(y);
	return x;
}

lval* builtin_join(lval* a) {
	for (int i = 0; i < a->count; i++) {
		LASSERT(a, a->cell[i]-> type == LVAL_QEXPR,
			"Function 'join' passed incorrect type");
	}

	lval* x = lval_pop(a, 0);

	while(a->count) {
		x = lval_join(x,lval_pop(a,0));
	}

	lval_del(a);
	return x;
}

lval* builtin_op(lval* a, char* op) {
	/* ENsure all arguments are numbers */
	for (int i = 0; i < a->count; i++) {
		if(a->cell[i]->type != LVAL_NUM) {
			lval_del(a);
			return lval_err("Cannot operate on a non-number");
		}
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

lval* builtin(lval* a, char* func) {
	if (strcmp("list", func) == 0) { return builtin_list(a);}
	if (strcmp("head", func) == 0) { return builtin_head(a);}
	if (strcmp("tail", func) == 0) { return builtin_tail(a);}
	if (strcmp("join", func) == 0) { return builtin_join(a);}
	if (strcmp("eval", func) == 0) { return builtin_eval(a);}
	if (strstr("+-/*", func)) { return builtin_op(a, func);}
	lval_del(a);
	return lval_err("Unknown function!");

}

lval* lval_eval_sexpr(lval* v) {

	/* Evaluate children */
	for(int i = 0; i < v->count; i++) {
		v->cell[i] = lval_eval(v->cell[i]);
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

	/* Ensure first element is a symbol */
	lval* f  = lval_pop(v,0);
	if(f->type != LVAL_SYM) {
		lval_del(f);
		lval_del(v);
		return lval_err("S-Expression does not start with symbol!");
	}

	/* Call builtin with operator */
	lval* result = builtin(v, f->sym);
	lval_del(f);
	return result;
}

lval* lval_eval(lval* v) {
	/* Evaluate S-Expression */
	if(v->type == LVAL_SEXPR) {
		return lval_eval_sexpr(v);
	}

	/* All other lval types remain the same */
	return v;
}

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
	symbol : \"list\" | \"head\" | \"tail\"  						\
		   | \"join\" | \"eval\" |'+' | '-' | '*' | '/' ;       	\
	sexpr  : '(' <expr>* ')' ;               						\
	qexpr  : '{' <expr>* '}' ;										\
	expr   : <number> | <symbol> | <sexpr> | <qexpr>; 				\
	lispy  : /^/ <expr>* /$/ ;               						\
	",
	Number, Symbol, Sexpr, Qexpr, Expr, Lispy);


	puts("Lispy version 0.0.0.0.7");
	puts("Press Ctrl+C to Exit\n");

	while(1) {
		char* input = readline("lispy> ");

		add_history(input);

		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			lval* x = lval_eval(lval_read(r.output));
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

	/* Undefine and delete parsers */
	mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
	return 0;
}
