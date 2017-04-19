#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"
#include <editline/readline.h>


long eval_op(long x, char* op, long y){
	if(strcmp(op, "+") == 0) { return x + y;}
	if(strcmp(op, "-") == 0) { return x - y;}
	if(strcmp(op, "*") == 0) { return x * y;}
	if(strcmp(op, "/") == 0) { return x / y;}
	if(strcmp(op, "^") == 0) { 
		int i;
		int multiplier = x;
		for (i = 2; i <= y; i++){
			x = multiplier * x;
		}
		return x;
	}
	if(strcmp(op, "%") == 0) { return x % y; }
	if(strcmp(op, "min") == 0) { 
		if(x < y) {
			return x;
		} else {
			return y;
		}	
	}
	if(strcmp(op, "max") == 0) { 
		if(x > y) {
			return x;
		} else {
			return y;
		}
	}
	return 0;
}

long eval(mpc_ast_t* t){
	/* If number */
	if(strstr(t->tag, "number")){
		return atoi(t->contents);
	}

	/* Operator is second child */
	char* op = t->children[1]->contents;

	/* x stores thirrd child */
	long x = eval(t->children[2]);

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
	
	while(1)
	{
		char* input = readline("lispy> ");

		add_history(input);

		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			long result = eval(r.output);
			printf("%li\n", result);
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
