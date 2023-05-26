%{
	#include "global.h"
	#include "lang.tab.h"
	#include "strop.h"

	string buffer;
	int dec_val;

	void str_to_operator(char *op, int *res_arr);
	int str_to_type(char *type);
%}

%option noyywrap noinput nounput yylineno
%x STR STROP CMT_ONE CMT_MULTI HEX OCT BIN

TYPE "int"|"bool"|"string"|"function"|"list"|"variant"
ID [a-z_]+
OP [-+*/<>]|">="|"<="|"=="|"!="|"&&"|"||"|"^||"|"!"|"<<"|">>"

DEC [1-9][0-9]*|0
STRING [^\"<]*
BOOL "true"|"false"

BINARY -?(1[01]*|0)
OCTAL -?([1-7][0-7]*|0)
HEXADECIMAL -?([1-9a-fA-F][0-9a-fA-F]*|0)
%%

<INITIAL>"//"	{ BEGIN(CMT_ONE); }
<CMT_ONE>"\n"	{ BEGIN(INITIAL); }

<INITIAL>"/*"	{ BEGIN(CMT_MULTI); }
<CMT_MULTI>"*/"	{ BEGIN(INITIAL); }

<INITIAL>"->"		{ return arrow; }
<INITIAL>"break"	{ return key_break; }
<INITIAL>"return"	{ return key_return; }
<INITIAL>"<if:"		{ return start_if; }
<INITIAL>"else:"	{ return key_else; }
<INITIAL>"<loop:"	{ return start_loop; }
<INITIAL>"<list:"	{ return start_list; }
<INITIAL>":>"		{ return end_ctrl; }
<INITIAL>"run!"		{ return t_run;}

<INITIAL>{TYPE}		{ yylval.dec = str_to_type(yytext); return type; }
<INITIAL>{OP}		{ int op_val[2]; str_to_operator(yytext, &op_val[0]); yylval.dec = op_val[0]; return op_val[1]; }

<INITIAL>{DEC}	{ yylval.dec = atoi(yytext); return val_num; }

<INITIAL>"<HEX:"	{ BEGIN(HEX); }
<HEX>{HEXADECIMAL}	{ dec_val = strtol(yytext, NULL, 16); }
<HEX>">"			{ BEGIN(INITIAL); yylval.dec = dec_val; return val_num; }

<INITIAL>"<OCT:"	{ BEGIN(OCT); }
<OCT>{OCTAL}		{ dec_val = strtol(yytext, NULL, 8); }
<OCT>">"			{ BEGIN(INITIAL); yylval.dec = dec_val; return val_num; }

<INITIAL>"<BIN:"	{ BEGIN(BIN); }
<BIN>{BINARY}		{ dec_val = strtol(yytext, NULL, 2); }
<BIN>">"			{ BEGIN(INITIAL); yylval.dec = dec_val; return val_num; }

<INITIAL>{BOOL}	{ yylval.bool = strcmp(yytext, "true") == 0 ? 1 : 0; return val_boolean; }

<INITIAL>"\""	{ BEGIN(STR); }
<STR>"\"\""		{ buffer.append(&buffer, "\""); }
<STR>"\""		{ BEGIN(INITIAL); yylval.str = buffer.get(&buffer); return val_string; }

<STR>"<"		{ BEGIN(STROP); }
<STROP>">"		{ BEGIN(STR); }

<STROP>"NEW"	{ buffer.append(&buffer, "\n"); }
<STROP>\\{DEC}	{ buffer.append_char(&buffer, (char)atoi(yytext+1)); }
<STROP>.		{ buffer.append(&buffer, yytext); }
<STR>{STRING}	{ buffer.append(&buffer, yytext); }

<INITIAL>"print"		{ yylval.dec = lib_f_print; return t_lib_fnct; }
<INITIAL>"println"		{ yylval.dec = lib_f_println; return t_lib_fnct; }
<INITIAL>"readln"		{ yylval.dec = lib_f_readln; return t_lib_fnct; }
<INITIAL>"list_add"		{ yylval.dec = lib_f_list_add; return t_lib_fnct; }
<INITIAL>"list_get"		{ yylval.dec = lib_f_list_get; return t_lib_fnct; }
<INITIAL>"list_set"		{ yylval.dec = lib_f_list_set; return t_lib_fnct; }
<INITIAL>"list_size"	{ yylval.dec = lib_f_list_size; return t_lib_fnct; }
<INITIAL>"str_to_dec"	{ yylval.dec = lib_f_str_to_dec; return t_lib_fnct; }
<INITIAL>"str_to_bool"	{ yylval.dec = lib_f_str_to_bool; return t_lib_fnct; }
<INITIAL>{ID}			{ buffer.append(&buffer, yytext); yylval.str = buffer.get(&buffer); return id; }

<*>(\ |\n|\t)	{ } // Ignore blanks and newlines

	/* Pass all other single characters as individual	*/
	/* tokens with their ASCII value as token id		*/
<INITIAL>.		{ return *yytext; }
<CMT_ONE>.		{ }
<CMT_MULTI>.	{ }

%%

void str_to_operator(char *op, int *res_arr) {
	if (strcmp(op, "+") == 0)			{ res_arr[0] = plus; res_arr[1] = t_plus; }
	else if (strcmp(op, "-") == 0)		{ res_arr[0] = minus; res_arr[1] = t_minus; }
	else if (strcmp(op, "*") == 0)		{ res_arr[0] = mul; res_arr[1] = t_mul; }
	else if (strcmp(op, "/") == 0)		{ res_arr[0] = divide; res_arr[1] = t_divide; }
	else if (strcmp(op, ">") == 0)		{ res_arr[0] = gt; res_arr[1] = t_gt; }
	else if (strcmp(op, "<") == 0)		{ res_arr[0] = lt; res_arr[1] = t_lt; }
	else if (strcmp(op, ">=") == 0)		{ res_arr[0] = ge; res_arr[1] = t_ge; }
	else if (strcmp(op, "<=") == 0)		{ res_arr[0] = le; res_arr[1] = t_le; }
	else if (strcmp(op, "==") == 0)		{ res_arr[0] = eq; res_arr[1] = t_eq; }
	else if (strcmp(op, "!=") == 0)		{ res_arr[0] = ne; res_arr[1] = t_ne; }
	else if (strcmp(op, "&&") == 0)		{ res_arr[0] = and; res_arr[1] = t_and; }
	else if (strcmp(op, "||") == 0)		{ res_arr[0] = or; res_arr[1] = t_or; }
	else if (strcmp(op, "^||") == 0)	{ res_arr[0] = xor; res_arr[1] = t_xor; }
	else if (strcmp(op, "!") == 0)		{ res_arr[0] = not; res_arr[1] = t_not; }
	else if (strcmp(op, "<<") == 0)		{ res_arr[0] = lshift; res_arr[1] = t_lshift; }
	else if (strcmp(op, ">>") == 0)		{ res_arr[0] = rshift; res_arr[1] = t_rshift; }
	else								{ res_arr[0] = -1; res_arr[1] = t_invalid_op; }
}

int str_to_type(char *type) {
	if (strcmp(type, "string") == 0) return type_string;
	else if (strcmp(type, "int") == 0) return type_decimal;
	else if (strcmp(type, "bool") == 0) return type_boolean;
	else if (strcmp(type, "function") == 0) return type_function;
	else if (strcmp(type, "list") == 0) return type_arrlist;
	else if (strcmp(type, "variant") == 0) return type_variant;
	else return -1;
}
