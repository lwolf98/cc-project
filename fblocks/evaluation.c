#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "evaluation.h"
#include "strop.h"
#include "stack.h"
#include "list.h"

extern stack_t *vars, *vars_global;


/* term evaluation*/

typedef struct {
	int type_a;
	int type_b;
	int type_op;
	value_t (*exec)(value_t a, value_t b);
} operation_t;

value_t op_plus_str(value_t a, value_t b) {
	string res;
	init_string(&res);
	res.append(&res, a.str);
	res.append(&res, b.str);
	return (value_t) { type_string, .str = res.get(&res) };
}
value_t op_eq_str(value_t a, value_t b) { return (value_t) { type_boolean, .bool= strcmp(a.str, b.str) == 0 }; }
value_t op_ne_str(value_t a, value_t b) { return (value_t) { type_boolean, .bool= strcmp(a.str, b.str) != 0 }; }

value_t op_plus_dec(value_t a, value_t b)	{ return (value_t) { type_decimal, .dec=a.dec+b.dec }; }
value_t op_minus(value_t a, value_t b)		{ return (value_t) { type_decimal, .dec=a.dec-b.dec }; }
value_t op_unary_minus(value_t a, value_t b)	{ return (value_t) { type_decimal, .dec= -a.dec }; }
value_t op_mul(value_t a, value_t b)		{ return (value_t) { type_decimal, .dec=a.dec*b.dec }; }
value_t op_divide(value_t a, value_t b)		{ return (value_t) { type_decimal, .dec=a.dec/b.dec }; }
value_t op_gt(value_t a, value_t b)			{ return (value_t) { type_boolean, .bool=a.dec>b.dec }; }
value_t op_lt(value_t a, value_t b)			{ return (value_t) { type_boolean, .bool=a.dec<b.dec }; }
value_t op_ge(value_t a, value_t b)			{ return (value_t) { type_boolean, .bool=a.dec>=b.dec }; }
value_t op_le(value_t a, value_t b)			{ return (value_t) { type_boolean, .bool=a.dec<=b.dec }; }
value_t op_eq_dec(value_t a, value_t b)		{ return (value_t) { type_boolean, .bool=a.dec==b.dec }; }
value_t op_ne_dec(value_t a, value_t b)		{ return (value_t) { type_boolean, .bool=a.dec!=b.dec }; }

value_t op_and(value_t a, value_t b) { return (value_t) { type_boolean, .bool=a.bool && b.bool }; }
value_t op_or(value_t a, value_t b) { return (value_t) { type_boolean, .bool=a.bool || b.bool }; }
value_t op_xor(value_t a, value_t b) { return (value_t) { type_boolean, .bool=a.bool != b.bool }; }
value_t op_not(value_t a, value_t b) { return (value_t) { type_boolean, .bool= a.bool ? 0 : 1 }; }
value_t op_eq_bool(value_t a, value_t b) { return (value_t) { type_boolean, .bool=a.bool==b.bool }; }

value_t op_plus_list(value_t a, value_t b) {
	return (value_t) { .type = type_arrlist, .list = merge_lists(a.list, b.list) };
}
value_t op_lshift_list(value_t a, value_t b) { include_list(a.list, b.list); return a; }
value_t op_rshift_list(value_t a, value_t b) { include_list(b.list, a.list); return b; }
value_t op_eq_list(value_t a, value_t b)	{
	value_t res = { .type = type_boolean, .bool = 0 };
	if (a.list->size != b.list->size)
		return res;

	for (int i = 0; i < a.list->size; i++) {
		value_t item_a = a.list->arr[i];
		value_t item_b = b.list->arr[i];
		if (item_a.type != item_b.type)
			return res;

		switch (item_a.type) {
			case type_boolean:	if (op_eq_bool(item_a, item_b).bool) break;	else return res;
			case type_decimal:	if (op_eq_dec(item_a, item_b).bool) break;	else return res;
			case type_string:	if (op_eq_str(item_a, item_b).bool) break;	else return res;
			case type_arrlist:	if (op_eq_list(item_a, item_b).bool) break;	else return res;
			default: return res;
		}
	}

	res.bool = 1;
	return res;
}

operation_t ops[] = {
	{type_string, type_string, plus, op_plus_str},
	{type_string, type_string, eq, op_eq_str},
	{type_string, type_string, ne, op_ne_str},

	{type_decimal, type_decimal, plus, op_plus_dec},
	{type_decimal, type_decimal, minus, op_minus},
	{type_decimal, type_void, minus, op_unary_minus},
	{type_decimal, type_decimal, mul, op_mul},
	{type_decimal, type_decimal, divide, op_divide},

	{type_decimal, type_decimal, gt, op_gt},
	{type_decimal, type_decimal, lt, op_lt},
	{type_decimal, type_decimal, ge, op_ge},
	{type_decimal, type_decimal, le, op_le},
	{type_decimal, type_decimal, eq, op_eq_dec},
	{type_decimal, type_decimal, ne, op_ne_dec},

	{type_boolean, type_boolean, and, op_and},
	{type_boolean, type_boolean, or, op_or},
	{type_boolean, type_boolean, xor, op_xor},
	{type_boolean, type_void, not, op_not},
	{type_boolean, type_boolean, eq, op_eq_bool},
	{type_boolean, type_boolean, ne, op_xor},

	{type_arrlist, type_arrlist, plus, op_plus_list},
	{type_arrlist, type_arrlist, lshift, op_lshift_list},
	{type_arrlist, type_arrlist, rshift, op_rshift_list},
	{type_arrlist, type_arrlist, eq, op_eq_list}
};

operation_t *get_operation(int type_a, int type_b, int type_op) {
	int size_ops = sizeof(ops) / sizeof(ops[0]);
	for (int i = 0; i < size_ops; i++)
		if (ops[i].type_a == type_a && ops[i].type_b == type_b && ops[i].type_op == type_op)
			return &ops[i];

	return NULL;
}

void convert_types(value_t *main, value_t *to_convert) {
	if (main->type == to_convert->type)
		return;

	int convert_from = to_convert->type;
	switch (main->type) {
		case type_string:	to_convert->str = value_to_str(to_convert);
							to_convert->type = type_string;
							return;

		case type_decimal:	to_convert->type = type_decimal;
							if (convert_from == type_boolean)
								to_convert->dec = to_convert->bool;

							return;

		case type_boolean:	// nothing to convert
							return;
	}
}

value_t evaluate_term(value_t a, value_t b, int op) {
	operation_t *operation = get_operation(a.type, b.type, op);
	if (operation) return operation->exec(a, b);
	
	// if operation not found: try implicit type conversion
	if (a.type > b.type)	convert_types(&a, &b);
	else					convert_types(&b, &a);

	operation = get_operation(a.type, b.type, op);
	if (operation) return operation->exec(a, b);

	// if still not found: return void
	return (value_t) { .type = type_void };
}

list_t *evaluate_term_list(astnode_t *value_list) {
	ast_iterator it_value;
	init_ast_iterator(&it_value, value_list);
	astnode_t *node;
	list_t *values = list_new();

	while (1) {
		node = it_value.next_specific_node(&it_value, ast_of_type, (value_t) { .type = _term });
		if (node == NULL)
			break;

		value_t value = node->execute(node);
		it_value.skip_next_child(&it_value);
		values->add(values, value);
	}

	for (int i = 0; i < values->size; i++)
		debug_print("value index %d (%d)\n", i, values->get(values, i).type);

	return values;
}


/* type conversion */

int no_of_digits(int i) {
	if (i < 0)
		return no_of_digits(i * -1) + 1;

	return i == 0 ? 1 : log10(i) + 1;
}

char *dec_to_str(int val) {
	int str_size = no_of_digits(val) + 1;

	char *str = malloc(str_size * sizeof(char));
	sprintf(str, "%d", val);
	str[str_size-1] = '\0';
	return str;
}

char *bool_to_str(int val) {
	return val ? "true" : "false";
}

int bool_to_dec(int val) {
	return val;
}

int dec_to_bool(int val) {
	return val ? 1 : 0;
}

int str_to_bool(char *val) {
	return strcmp(val, "true") == 0 ? 1 : 0;
}

int str_to_dec(char *val) {
	return atoi(val);
}


/* to string */

char *operator_to_str(int op) {
	if (op == plus) return "+";
	else if (op == minus) return "-";
	else if (op == mul) return "*";
	else if (op == divide) return "/";
	else if (op == plus) return "+";
	else if (op == gt) return ">";
	else if (op == lt) return "<";
	else if (op == ge) return ">=";
	else if (op == le) return "<=";
	else if (op == eq) return "==";
	else if (op == ne) return "!=";
	else if (op == and) return "&&";
	else if (op == or) return "||";
	else if (op == xor) return "^||";
	else if (op == not) return "!";
	else if (op == lshift) return "<<";
	else if (op == rshift) return ">>";
	else return "NOOP";
}

char *data_type_to_str(int type) {
	switch (type) {
		case type_string: return "string";
		case type_decimal: return "int";
		case type_boolean: return "bool";
		case type_function: return "function";
		case type_arrlist: return "list";
		case type_variant: return "variant";
		case type_void: return "void";
	}

	return "invalid type";
}

char *node_type_to_str(int type) {
	char *res = operator_to_str(type);
	if (strcmp(res, "NOOP") != 0)
		return res;

	switch (type) {
		case type_void: return "T_VOID";
		case type_function: return "T_FNCT";
		case type_boolean: return "T_BOOL";
		case type_decimal: return "T_DEC";
		case type_string: return "T_STR";
		case type_arrlist: return "T_LIST";
		case type_variant: return "T_VARIANT";
		case stmt_list: return "STATEMENT_LIST";
		case type_list: return "TYPE_LIST";
		case term_list: return "TERM_LIST";
		case _term: return "TERM";
		case def_list: return "DEF_LIST";
		case define: return "DEFINE";
		case _type: return "TYPE";
		case _id: return "ID";
		case _if: return "IF";
		case _loop: return "LOOP";
		case _break: return "BREAK";
		case f_call: return "F_CALL";
		case lib_fnct: return "LIB_FNCT";
		case _return: return "RETURN";
		case error_type: return "ERROR";
	}

	if (type >= 0 && type < 256) {
		string buf;
		init_string(&buf);
		buf.append_char(&buf, type);
		return buf.get(&buf);
	}

	return "INVALID_TYPE";
}

char *value_to_str(const value_t *val) {
	char *val_str = val->str;
	if (val->type == type_decimal)		val_str = dec_to_str(val->dec);
	else if (val->type == type_boolean)	val_str = bool_to_str(val->bool);
	else if (val->type == type_arrlist) {
		list_t *lst = val->list;
		if (lst == NULL)
			return "NO_DEF";
		
		string buf;
		init_string(&buf);
		buf.append_char(&buf, '[');
		char *item;
		for (int i = 0; i < lst->size; i++) {
			value_t val = lst->get(lst, i);
			item = value_to_str(&val);
			if (i == 0)	{ buf.append(&buf, item); }
			else		{ buf.append_char(&buf, ','); buf.append(&buf, item); }

			if (val.type == type_decimal)
				free(item);
		}
		buf.append_char(&buf, ']');
		val_str = buf.get(&buf);
	}
	else if (val->type == type_variant) {
		if (val->variant == NULL)
			return "NO_DEF";

		val_str = value_to_str(val->variant);
	}
	else if (val->type == type_function)
		val_str = val->fnct ? "F_DEF" : "NO_DEF";
	else if (val->type == type_void)
		val_str = "VOID";

	return val_str;
}

char *lib_f_to_str(int type) {
	switch (type) {
		case lib_f_print: return "print";
		case lib_f_println: return "println";
		case lib_f_readln: return "readln";
		case lib_f_list_add: return "list_add";
		case lib_f_list_get: return "list_get";
		case lib_f_list_set: return "list_set";
		case lib_f_list_size: return "list_size";
		case lib_f_str_to_dec: return "str_to_dec";
		case lib_f_str_to_bool: return "str_to_bool";
		default: return "no_lib_funct";
	}
}


/* ID / stack handling */

int has_local_scope() {
	return vars->index >= 0;
}

val_t *lookup_var_by_id(char *id) {
	val_t *var = NULL;

	// lookup in local scope
	if (has_local_scope()) {
		var = vars->lookup(vars, id);
		if (var != NULL)
			return var;
	}

	// lookup in global scope
	var = vars_global->lookup(vars_global, id);
	return var;
}

value_t lookup_id(char *id) {
	val_t *var = lookup_var_by_id(id);
	if (var)	return var->val;
	else		return (value_t) { .type = type_void };
}

int define_id(int type, char *id) {
	stack_t *s = has_local_scope() ? vars : vars_global;
	val_t *var = s->lookup(s, id);
	if (var != NULL)
		return err_id_already_defined;

	string id_copy;
	init_string(&id_copy);
	id_copy.append(&id_copy, id);
	s->push(s, (val_t) { .id = id_copy.get(&id_copy), .val = { .type = type } });
	return 0;
}

int set_id(char *id, value_t value) {
	val_t *var = lookup_var_by_id(id);
	if (var == NULL)
		return err_id_not_defined;

	if (var->val.type == type_variant) {
		value_t *ref_val = NULL;
		if (var->val.variant)	ref_val = var->val.variant;
		else 					ref_val = malloc(sizeof(*ref_val));
		
		*ref_val = value;
		value = (value_t) { .type = type_variant, .variant = ref_val };
	}
	else if (var->val.type != value.type)
		return err_type_mismatch;

	var->val = value;
	return 0;
}

void enter_function() {
	vars->push(vars, (val_t) { .val = (value_t) { .type = strong_border } });
}

void exit_function() {
	vars->exit_scope(vars, strong_border);
}

void enter_block() {
	vars->push(vars, (val_t) { .val = (value_t) { .type = weak_border } });
}

void exit_block() {
	vars->exit_scope(vars, weak_border);
}

int assign_fnct_params(astnode_t *id_list, list_t *params) {
	int params_size = params->size;
	ast_iterator it_id;
	init_ast_iterator(&it_id, id_list);
	astnode_t *id_node;
	int index = 0;

	while (1) {
		id_node = it_id.next_specific_node(&it_id, ast_of_type, (value_t) { .type = define });
		if (id_node == NULL)
			break;
		if (index >= params_size) {
			//Error in assign fnct params: too few arguments
			return err_wrong_args_number;
		}

		char *id = id_node->child[1]->val.str;
		value_t value = params->arr[index++];

		//DEBUG:
		debug_print("Assign: %s = %s\n", id_node->child[1]->to_str(id_node->child[1]), value_to_str(&value));
		
		int err = set_id(id, value);
		if (err) return err;
	}

	if (index < params_size) {
		//Error in assign fnct params: too many arguments
		return err_wrong_args_number;
	}

	return 0;
}
