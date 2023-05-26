%{
#include <stdio.h>
#include "global.h"
#include "strop.h"
#include "stack.h"

stack_t *vars, *vars_global;

int yylex(void);
extern string buffer;
extern int yylineno;
extern FILE *yyin;

int yydebug = 1;
void yyerror (const char *msg) {
	//printf("Error triggered:\n");
	//fprintf(stderr, "Error in line %d:\n%s\n", yylineno, msg);
	error_print(yylineno, msg);
}
%}

%define parse.error verbose

%union {
	astnode_t *ast;
	char *str;
	int dec;
	int bool;
}

%token arrow key_return start_if key_else start_loop start_list end_ctrl key_break t_run
<dec> val_num type t_plus t_minus t_mul t_divide t_lt t_gt t_le t_ge t_eq t_ne t_and t_or t_xor t_not t_lshift t_rshift t_invalid_op t_lib_fnct
<str> val_string id
<bool> val_boolean

%right t_lshift
%left t_rshift
%left t_or
%left t_and
%left t_eq t_ne t_xor
%left t_lt t_gt t_le t_ge
%left t_plus t_minus
%left t_mul t_divide
%right t_not

%type	<ast>	STATEMENT_LIST STATEMENT EXPRESSION ASSIGN OPERAND TERM ID TYPE F_CALL
				CONTROL_STMT BLOCK F_DEFINITION VAR_DEFINITION DEF_ID
				F_BLOCK F_STATEMENT_LIST F_STATEMENT TERM_LIST TERM_WRAPPER TYPE_LIST DEF_LIST

%%

START:		STATEMENT_LIST			{ $1->print($1); $1->execute($1); dbg_strop(); }

STATEMENT_LIST:	STATEMENT_LIST STATEMENT	{ $$ = astnode_new(stmt_list, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $2); }
			|	/* epsilon */				{ $$ = NULL; }

STATEMENT:		EXPRESSION ';'
			|	CONTROL_STMT

CONTROL_STMT:	start_if TERM ':' BLOCK end_ctrl						{ $$ = astnode_new(_if, yylineno); $$->set_child($$, 0, $2); $$->set_child($$, 1, $4); }
			|	start_if TERM ':' BLOCK ':' key_else BLOCK end_ctrl		{ $$ = astnode_new(_if, yylineno); $$->set_child($$, 0, $2); $$->set_child($$, 1, $4); $$->set_child($$, 2, $7); }
			|	start_loop BLOCK end_ctrl								{ $$ = astnode_new(_loop, yylineno); $$->set_child($$, 0, $2); }

BLOCK:			'{' STATEMENT_LIST '}'		{ $$ = $2; }

 // expression without return value
EXPRESSION:		F_DEFINITION
			|	VAR_DEFINITION
			|	ASSIGN
			|	key_break		{ $$ = astnode_new(_break, yylineno); }
			|	error			{ $$ = astnode_new(error_type, yylineno); }

F_DEFINITION:	TYPE ID ':' '(' TYPE_LIST ')' arrow '(' TYPE_LIST ')'	{ $$ = astnode_new(define, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $2); $$->set_child($$, 2, $5); $$->set_child($$, 3, $9); }

VAR_DEFINITION:	DEF_ID

TYPE_LIST:		TYPE_LIST ',' TYPE	{ $$ = astnode_new(type_list, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	TYPE
			|	/* epsilon */		{ $$ = NULL; }

ASSIGN:			TERM
			|	ID '=' TERM			{ $$ = astnode_new('=', yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	ID '=' F_BLOCK		{ $$ = astnode_new('=', yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	ID '{' DEF_LIST '}' '=' F_BLOCK		{ $$ = astnode_new('=', yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $6); $$->set_child($$, 2, $3); }
			|	'{' DEF_LIST '}' '=' F_CALL			{ $$ = astnode_new('=', yylineno); ; $$->set_child($$, 1, $5); $$->set_child($$, 3, $2); }

F_BLOCK:		'{' F_STATEMENT_LIST '}'	{ $$ = astnode_new(type_function, yylineno); $$->val.fnct = $2; $$->set_child($$, 0, $2); }

F_STATEMENT_LIST:	F_STATEMENT_LIST F_STATEMENT		{ $$ = astnode_new(stmt_list, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $2); }
				|	/* epsilon */						{ $$ = NULL; }

F_STATEMENT:		STATEMENT							
				|	key_return ';'						{ $$ = astnode_new(_return, yylineno); }
				|	key_return TERM ';'					{ $$ = astnode_new(_return, yylineno); $$->set_child($$, 0, $2); }
				|	key_return '{' TERM_LIST '}' ';'	{ $$ = astnode_new(_return, yylineno); $$->set_child($$, 0, $3); }

DEF_LIST:		DEF_ID ',' DEF_LIST		{ $$ = astnode_new(def_list, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	DEF_ID

DEF_ID:		TYPE ID	{ $$ = astnode_new(define, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $2); }

TYPE:		type	{ $$ = astnode_new(_type, yylineno); $$->val.dec = $1; }

ID:			id		{ $$ = astnode_new(_id, yylineno); $$->val.str = $1; }

TERM:			TERM t_plus TERM	{ $$ = astnode_new($2, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	TERM t_minus TERM	{ $$ = astnode_new($2, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	t_minus TERM		{ $$ = astnode_new($1, yylineno); $$->set_child($$, 0, $2); }
			|	TERM t_mul TERM		{ $$ = astnode_new($2, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	TERM t_divide TERM	{ $$ = astnode_new($2, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	TERM t_lt TERM		{ $$ = astnode_new($2, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	TERM t_gt TERM		{ $$ = astnode_new($2, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	TERM t_le TERM		{ $$ = astnode_new($2, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	TERM t_ge TERM		{ $$ = astnode_new($2, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	TERM t_and TERM		{ $$ = astnode_new($2, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	TERM t_or TERM		{ $$ = astnode_new($2, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	TERM t_xor TERM		{ $$ = astnode_new($2, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	TERM t_eq TERM		{ $$ = astnode_new($2, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	TERM t_ne TERM		{ $$ = astnode_new($2, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	t_not TERM			{ $$ = astnode_new($1, yylineno); $$->set_child($$, 0, $2); }
			|	TERM t_lshift TERM	{ $$ = astnode_new($2, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	TERM t_rshift TERM	{ $$ = astnode_new($2, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	OPERAND

OPERAND:		'(' TERM ')'	{ $$ = $2; }
			|	val_num			{ $$ = astnode_new(type_decimal, yylineno); $$->val.dec = $1; }
			|	val_string		{ $$ = astnode_new(type_string, yylineno); $$->val.str = $1; }
			|	val_boolean		{ $$ = astnode_new(type_boolean, yylineno); $$->val.bool = $1; }
			|	start_list TERM_LIST end_ctrl	{ $$ = astnode_new(type_arrlist, yylineno); $$->child[0] = $2; }
			|	start_list end_ctrl	{ $$ = astnode_new(type_arrlist, yylineno); }
			|	ID				{ $$ = $1; }
			|	F_CALL

F_CALL:			ID '(' TERM_LIST ')'			{ $$ = astnode_new(f_call, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	ID '(' ')'						{ $$ = astnode_new(f_call, yylineno); $$->set_child($$, 0, $1); }
			|	t_lib_fnct '(' TERM_LIST ')'	{ $$ = astnode_new(lib_fnct, yylineno); $$->val.dec = $1; $$->set_child($$, 0, $3); }
			|	t_lib_fnct '(' ')'				{ $$ = astnode_new(lib_fnct, yylineno); $$->val.dec = $1; }

TERM_LIST:		TERM_WRAPPER ',' TERM_LIST	{ $$ = astnode_new(term_list, yylineno); $$->set_child($$, 0, $1); $$->set_child($$, 1, $3); }
			|	TERM_WRAPPER

TERM_WRAPPER:	TERM						{ $$ = astnode_new(_term, yylineno); $$->set_child($$, 0, $1); }

%%

int main (int argc, char *argv[]) {
	if (argc > 2) {
		printf("too many arguments\n");
		return 0;
	}
	if (argc == 2) {
		yyin = fopen(argv[1], "r");
	}
	vars = s_new();
	vars_global = s_new();
	init_string(&buffer);

	return yyparse();
}