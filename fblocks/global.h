#pragma once

#include "list.h"
#include "ast.h"
#include "error.h"

enum value_type {
	start_value_types = 10000,
	type_void,
	type_function,
	type_boolean,
	type_decimal,
	type_arrlist,
	type_string,
	type_variant,
	type_return,
	end_value_types = 10099
};

enum special_types {
	start_special_types = 10100,
	stmt_list,
	define,
	_type,
	_id,
	_if,
	_loop,
	_break,
	_return,
	_term,
	term_list,
	type_list,
	def_list,
	f_call,

	lib_fnct,
	lib_f_print,
	lib_f_println,
	lib_f_readln,
	lib_f_list_add,
	lib_f_list_get,
	lib_f_list_set,
	lib_f_list_size,
	lib_f_str_to_dec,
	lib_f_str_to_bool,

	strong_border,
	weak_border,

	error_type,

	end_special_types = 10199
};

enum operators {
	start_operators = 10200,
	plus,
	minus,
	mul,
	divide,
	gt,
	lt,
	ge,
	le,
	eq,
	ne,
	and,
	or,
	xor,
	not,
	lshift,
	rshift,
	end_operators = 10299
};

/* Debug section */
void debug_print(const char *format, ...);
void debug_print_plain(const char *format, ...);
void debug_print_term(const char *msg, const value_t a, const value_t b, const int op, const value_t res);
void debug_traverse_ast(astnode_t *a);
void debug_print_node(astnode_t *node);
void debug_print_stacks(char *msg);
int debug_id();
int debug_line();