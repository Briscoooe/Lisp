#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"
#include <editline/readline.h>

/* lval struct */
typedef struct {
	int type;
	long num;
	int err;
} lval;

/* Error types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM};

/* lval types */
enum { LVAL_NUM, LVAL_ERR};

/* Create a new number type lval */
lval lval_num(long x){
	lval v;
	v.type = LVAL_NUM;
	v.num = x;
	return v;
}

/* Create a new error type lval */
lval lval_err(int x) {
	lval v;
	v.type = LVAL_ERR;
	v.err = x;
	return v;
}

/* Print an lval */
void lval_print(lval v) {
	switch(v.type) {
		/* if number */
		case LVAL_NUM: printf("%li", v.num); break;

		/* If error */
		case LVAL_ERR:
			if(v.err == LERR_DIV_ZERO) {
				printf("Error: Division by Zero!");
			}
			if(v.err == LERR_BAD_OP) {
				printf("error: Invalid operator!");
			}
			if(v.err == LERR_BAD_NUM) {
				printf("Error: Invalid number!");
			}
		break;
	}
}

/* print an lval and new line */
void lval_println(lval v) {
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
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr= mpc_new("expr");
	mpc_parser_t* Lispy= mpc_new("lispy");


	/* Define them with language */
	mpca_lang(MPCA_LANG_DEFAULT,
	"										\
		number : /-?[0-9]+/ ;							\
		operator : '+' | '-' | '*' |'/' | '^' | '%' | \"min\" | \"max\" ;		\
		expr : <number> | '(' <operator> <expr>+ ')' ;				\
		lispy : /^/ <operator> <expr>+ /$/ ;					\
	",
	Number, Operator, Expr, Lispy);
	
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
	mpc_cleanup(4, Number, Operator, Expr, Lispy);
	return 0;
}
