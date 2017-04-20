#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"
#include <editline/readline.h>

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
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

/* Create a new number type lval */
lval* lval_num(long x){
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
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

lval* lval_read_num(mpc_ast_t* t){
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ?
		lval_num(x) : lval_err("invalid number");
}

lval* lval_add(lval* v, lval* x) {
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}

lval* lval_read(mpc_ast_t* t) {
	/* If symbol or number, return conversion to that type */
	if(strstr(t->tag, "number")) { return lval_read_num(t); }
	if(strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

	/* If root or sexpr, create empty list */
	if(strcmp(t->tag, ">") == 0)) { x = lval_sexpr(); }
	if(strcmp(t->tag, "sexpr") == 0)) { x = lval_sexpr(); }

	/* fill the list with any valid expressions contained within */
	for(int i = 0; i < t->children_num; i++) {
		if(strcmp(t->children[i]->contents, "(") == 0) { continue; }
		if(strcmp(t->children[i]->contents, ")") == 0) { continue; }
		if(strcmp(t->children[i]->tag, "regex") == 0) { continue; }
		x = lval_add(x, lval_read(t->children[i]))
	}

	return x;
}
void lval_del(lval* v) {
	switch(v->type) {
		case LVAL_NUM: break;

		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;

		/* Delete all elements in sexpr */
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
void lval_print(lval v) {
	switch(v.type) {
		case LVAL_NUM: printf("%li", v->num); break;
		case LVAL_ERR: printf("Error: %s", v->err); break;
		case LVAL_SYM: printf("%s", v->sym); break;
		case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
}

/* print an lval and new line */
void lval_println(lval* v) {
	lval_print(v);
   	putchar('\n');
}

lval eval_op(lval x, char* op, lval y) {

	/* If err type */
	if(x.type == LVAL_ERR) { return x;}
	if(y.type == LVAL_ERR) { return y;}

	if(strcmp(op, "+") == 0) { return lval_num(x.num + y.num);}
	if(strcmp(op, "-") == 0) { return lval_num(x.num - y.num);}
	if(strcmp(op, "/") == 0) {
		return y.num == 0
		? lval_err(LERR_DIV_ZERO)
		: lval_num(x.num /y.num);
	}
	if(strcmp(op, "*") == 0) { return lval_num(x.num * y.num);}
	if(strcmp(op, "%") == 0) { return lval_num(x.num % y.num);}
	if(strcmp(op, "^") == 0) {
		int i;
		int multiplier = x.num;
		for (i = 2; i <= y.num; i++){
			x.num = multiplier * x.num;
		}
		return lval_num(x.num);
	}
	if(strcmp(op, "min") == 0) {
	       	return x.num < y.num ? lval_num(x.num) : lval_num(y.num);
	}
	if(strcmp(op, "max") == 0) {
	       	return x.num > y.num ? lval_num(x.num) : lval_num(y.num);
	}

	return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t){
	/* If number */
	if(strstr(t->tag, "number")){
		errno = 0;
		long x = strtol(t->contents, NULL, 10);
		return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
	}

	/* Operator is second child */
	char* op = t->children[1]->contents;

	/* x stores thirrd child */
	lval x = eval(t->children[2]);

	/* Iterate remaining children and combine them */
	int i = 3;
	while(strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	return x;
}

int main(int argc, char** argv) {
	/* Create parsers*/
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Sexpr = mpc_new("sexpr");
	mpc_parser_t* Expr= mpc_new("expr");
	mpc_parser_t* Lispy= mpc_new("lispy");


	/* Define them with language */
	mpca_lang(MPCA_LANG_DEFAULT,
	"										\
		number : /-?[0-9]+/ ;							\
		symbol : '+' | '-' | '*' |'/' | '^' | '%' | \"min\" | \"max\" ;		\
		sexpr : '(' <expr>* ')'							\
		expr : <number> | <symbol> | <sexpr> ;					\
		lispy : /^/ <operator> <expr>+ /$/ ;					\
	",
	Number, Symbol, Sexpr, Expr, Lispy);

	puts("Lispy version 0.0.0.0.2");
	puts("Press Ctrl+C to Exit\n");

	while(1) {
		char* input = readline("lispy> ");

		add_history(input);

		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			lval result = eval(r.output);
			lval_println(result);
			mpc_ast_delete(r.output);
		} else {
			/* Otherwise, print the error */
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);
	}

	/* Undefine and delete parsers */
	mpc_cleanup(4, Number, Symbol, Sexpr, Expr, Lispy);
	return 0;
}
