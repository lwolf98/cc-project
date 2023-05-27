#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "global.h"
#include "strop.h"
#include "evaluation.h"
#include "stack.h"

static FILE *dot;
static int ast_id = 0;

void ast_set_child(astnode_t *s, int index, astnode_t *child) {
	s->child[index] = child;

	if (child)
		child->parent = s;
}

char *ast_to_str(const astnode_t *s) {
	string buf;
	init_string(&buf);
	if (debug_id() || debug_line()) {
		buf.append(&buf, "(");
		if (debug_id()) {
			buf.append(&buf, "ID:");
			buf.append(&buf, dec_to_str(s->id));
		}
		if (debug_line()) {
			buf.append(&buf, ",Ln:");
			buf.append(&buf, dec_to_str(s->line));
		}
		buf.append(&buf, ") ");
	}

	buf.append(&buf, node_type_to_str(s->val.type));
	if (s->val.type == type_void)
		return buf.get(&buf);

	char *val = NULL;
	switch (s->val.type) {	
		case _type:			val = data_type_to_str(s->val.dec);
							break;
		case lib_fnct:		val = lib_f_to_str(s->val.dec);
							break;
		case _id:			
		case 'F':			
		case type_string:	val = s->val.str;
							break;
		case type_decimal:	val = dec_to_str(s->val.dec);
							break;
		case type_boolean:	val = bool_to_str(s->val.bool);
							break;
	}

	if (val) buf.append(&buf, ": ")->append(&buf, val);
	
	return buf.get(&buf);
}

int ast_category(astnode_t *s) {
	int type = s->val.type;
	if (type > start_operators && type < end_operators)
		return cat_term;
	if (type > start_value_types && type < end_value_types)
		return cat_value;
	if (type > start_special_types && type < end_special_types)
		return cat_special_type;

	return cat_else;
}

int ast_index_of(astnode_t *s, astnode_t *child) {
	for (int i = 0; i < MAX_CHILDREN; i++)
		if (s->child[i] == child)
			return i;

	return -1;
}

void print_ast_rec(astnode_t *root, int depth) {
	// Create graph node
	fprintf(dot, "	n%d [label=\"%s\"]\n", root->id, root->to_str(root));
	for (int i = 0; i < MAX_CHILDREN; i++) {
		if (root->child[i]) {
			// Create graph edge
			fprintf(dot, "	n%d -> n%d\n", root->id, root->child[i]->id);
			print_ast_rec(root->child[i], depth+1);
		}
	}
}

void ast_print(astnode_t *s) {
	dot = fopen("ast.gv", "w");
	fprintf(dot, "digraph ast {\n");

	print_ast_rec(s, 0);

	fprintf(dot, "}\n");
	fclose(dot);
}

int check_f_call(const astnode_t *node, const char *f_name, int param_size, int expected_param_size, list_t *values, ...) {
	if (param_size != expected_param_size) {
		error_print_ast(node, "Wrong number of arguments on function call %s, expected %d but received %d", f_name, expected_param_size, param_size);
		return 1;
	}
	if (expected_param_size == 0)
		return 0;

	va_list args;
	va_start(args, values);
	for (int i = 0; i < expected_param_size; i++) {
		int arg = va_arg(args, int);
		if (values->arr[i].type != arg && arg != type_variant) {
			error_print_ast(node, "Type mismatch on function call %s in argument %d, expected %s but received %s", 
							f_name, i+1, node_type_to_str(arg), node_type_to_str(values->arr[i].type));

			va_end(args);
			return 1;
		}
	}
	va_end(args);
	return 0;
}

int check_f_params(astnode_t *type_list, astnode_t *id_list) {
	ast_iterator_t type_it, id_it;
	init_ast_iterator(&type_it, type_list);
	init_ast_iterator(&id_it, id_list);
	astnode_t *type_node, *id_node;
	while (1) {
		type_node = type_it.next_specific_node(&type_it, ast_of_type, (value_t) { .type = _type });
		id_node = id_it.next_specific_node(&id_it, ast_of_type, (value_t) { .type = define });
		if (type_node == NULL || id_node == NULL)
			break;

		if (type_node->val.dec != id_node->child[0]->val.dec)
			return 1;
	}

	if (type_node == NULL && id_node == NULL)	return 0;
	else										return 1;
}

value_t ast_execute(astnode_t *s) {
	debug_print_node(s);

	if (s->val.type == define) {
		int type = s->child[0]->val.dec; // get data type (val.type == _type)
		char *id = s->child[1]->val.str; // get name of ID (val.type == _id)
		int err = define_id(type, id);
		debug_print_stacks("DEF_ID");
		debug_print("Define ID: %s(%s)\n", id, data_type_to_str(type));
		if (err) {
			error_print_ast(s, "Could not define ID %s (type: %s), already defined", id, data_type_to_str(type));
			return (value_t) { .type = type_void };
		};
		if (type == type_function) {
			// Building a function node F that is stored as the value of a function ID
			// indices:	(0) type list input params
			//			(1) type list output params
			//			(2) function variables definition list (will be set in the '=' case)
			//			(3) statement list of the function (function body) (will be set in the '=' case)
			astnode_t *f = astnode_new('F', -1);
			f->set_child(f, 0, s->child[2]);
			f->set_child(f, 1, s->child[3]);
			err = set_id(id, (value_t) { .type = type_function, .fnct = f });
			if (err) error_print_ast(s, "Could not assign function value to ID: %s", id);
			debug_print_stacks("SET_FNCT");
		}
		return (value_t) { .type = type_void };
	}

	if (s->val.type == '=') {
		if (s->child[3]) {
			// Multi value assignment

			astnode_t *return_id_list = s->child[3];
			astnode_t *f_call = s->child[1];
			astnode_t *f_id_node = f_call->child[0];
			char *id = f_id_node->val.str;

			return_id_list->execute(return_id_list);
			value_t val = f_call->execute(f_call);
			if (val.type != type_arrlist) {
				error_print_ast(s, "Could not correctly assign return values, function %s did not return an ID list", id);
				return (value_t) { .type = type_void };
			}

			// assign values
			int err = assign_fnct_params(return_id_list, val.list);
			free(val.list);

			if (err == err_type_mismatch)
				error_print_ast(s, "Could not correctly assign return values from function %s, type mismatch in the ID list", id);
			else if (err == err_wrong_args_number)
				error_print_ast(s, "Could not correctly assign return values from function %s, wrong number of variables in the ID list", id);
			else if (err)
				error_print_ast(s, "Could not correctly assign return values from function %s, unspecified error", id);

			return (value_t) { .type = type_void };
		}

		// Single value assignment

		// Preparation and term evaluation
		char *id = s->child[0]->val.str; // get name of ID
		value_t val = s->child[1]->execute(s->child[1]);
		int err = 0;
		if (val.type == type_function) {
			value_t f = lookup_id(id);
			if (f.type != type_function) {
				error_print_ast(s, "Could not assign function to ID %s, type mismatch", id);
				return (value_t) { .type = type_void };
			}
			if (check_f_params(f.fnct->child[0], s->child[2])) {
				error_print_ast(s, "Could not assign function to ID %s, violating type constraints of the function definition", id);
				return (value_t) { .type = type_void };
			}
			f.fnct->set_child(f.fnct, 2, s->child[2]);
			f.fnct->set_child(f.fnct, 3, val.fnct);
			f.fnct->val.str = id;
			val = f;
		}

		// Assignment
		err = set_id(id, val);
		debug_print_stacks("SET_ID");
		
		debug_print("Assign value to ID: %s = %s\n", id, value_to_str(&val));
		if (err) error_print_ast(s, "Could not assign value to ID: %s = %s, ID not defined or type mismatch", id, value_to_str(&val));

		return (value_t) { .type = type_void };
	}

	if (s->val.type == lib_fnct) {
		astnode_t *value_list = s->child[0];
		value_t res = (value_t) { .type = type_void };
		list_t *values = evaluate_term_list(value_list);
		int size = values->size;
		
		char *txt = NULL;
		int num = 0;
		value_t lst;
		char *f_name = lib_f_to_str(s->val.dec);
		switch (s->val.dec) {
			case lib_f_print:	if (check_f_call(s, f_name, size, 1, values, type_variant)) break;
								txt = value_to_str(&values->arr[0]);
								printf("%s", txt);
								break;
			case lib_f_println:	if (check_f_call(s, f_name, size, 1, values, type_variant)) break;
								txt = value_to_str(&values->arr[0]);
								printf("%s\n", txt);
								break;
			case lib_f_readln:	if (check_f_call(s, f_name, size, 0, values)) break;
								printf("> ");
								char local_buf[100];
								string buf;
								init_string(&buf);
								fgets(local_buf, sizeof(local_buf), stdin);
								if (local_buf == NULL) {
									printf("EOF\n");
									exit(1);
								}
								
								local_buf[strcspn(local_buf, "\n")] = '\0';
								buf.append(&buf, local_buf);
								res = (value_t) { .type = type_string, .str = buf.get(&buf) };
								break;
			case lib_f_str_to_dec:	if (check_f_call(s, f_name, size, 1, values, type_string)) break;
									num = str_to_dec(values->arr[0].str);
									res = (value_t) { .type = type_decimal, .dec = num };
									break;
			case lib_f_str_to_bool:	if (check_f_call(s, f_name, size, 1, values, type_string)) break;
									num = str_to_bool(values->arr[0].str);
									res = (value_t) { .type = type_boolean, .bool = num };
									break;
			case lib_f_list_add:	if (check_f_call(s, f_name, size, 2, values, type_arrlist, type_variant)) break;
									lst = values->arr[0];
									lst.list->add(lst.list, values->arr[1]);
									break;
			case lib_f_list_get:	if (check_f_call(s, f_name, size, 2, values, type_arrlist, type_decimal)) break;
									lst = values->arr[0];
									res = lst.list->get(lst.list, values->arr[1].dec);
									break;
			case lib_f_list_set:	if (check_f_call(s, f_name, size, 3, values, type_arrlist, type_decimal, type_variant)) break;
									lst = values->arr[0];
									lst.list->set(lst.list, values->arr[1].dec, values->arr[2]);
									break;
			case lib_f_list_size:	if (check_f_call(s, f_name, size, 1, values, type_arrlist)) break;
									lst = values->arr[0];
									res = (value_t) { .type = type_decimal, .dec = lst.list->size };
									break;

		}

		free(values);
		return res;
	}

	if (s->val.type == f_call) {
		char *id = s->child[0]->val.str;
		astnode_t *value_list = s->child[1];
		value_t f_val = lookup_id(id);
		if (f_val.type == type_void) {
			error_print_ast(s, "Could not call function %s, not defined", id);
			return f_val;
		}

		list_t *params = evaluate_term_list(value_list);
		enter_function();

		// Parameter list: assign values to variables
		if (f_val.fnct->child[2]) {
			f_val.fnct->child[2]->execute(f_val.fnct->child[2]);

			int err = assign_fnct_params(f_val.fnct->child[2], params);
			if (err == err_type_mismatch)
				error_print_ast(s, "Type mismatch in parameter list of function call %s", id);
			else if (err == err_wrong_args_number)
				error_print_ast(s, "Wrong number of arguments in function call %s", id);
			else if (err)
				error_print_ast(s, "Unspecified error in function call %s", id);

			if (err) return (value_t) { .type = type_void };
		}

		value_t op = f_val.fnct->child[3]->execute(f_val.fnct->child[3]);

		debug_print_stacks("BEFORE EXIT");
		exit_function();
		debug_print_stacks("EXIT FNCT");

		free(params);

		if (op.type == _return) {
			if (op.ret) {
				value_t ret = *op.ret;
				free(op.ret);
				return ret;
			}
		}

		return op;
	}

	if (s->val.type == _term) {
		return s->child[0]->execute(s->child[0]);
	}

	if (s->val.type == _return) {
		//case: no return value
		if (s->child[0] == NULL)
			return s->val;

		//case: one or more return values

		// search function ID:
		astnode_t *node = s;
		while (node->val.type != 'F') {
			node = node->parent;
			if (node == NULL) {
				error_print_ast(s, "Error on function return, associated function ID not found");
				return s->val;
			}
		}

		char *id = node->val.str;
		if (id == NULL) {
			error_print_ast(s, "Error on function return, associated function ID not found");
			return s->val;
		}

		// get function definition
		value_t f_val = lookup_id(id);
		if (f_val.type == type_void) {
			error_print_ast(s, "Error on function return, associated function ID %s not defined", id);
			return s->val;
		}

		astnode_t *return_term = s->child[0];
		if (return_term->val.type != term_list) {
			// Single value return
			value_t *val = malloc(sizeof val);
			*val = return_term->execute(return_term);
			s->val.ret = val;
		}
		else {
			// Multi value return
			list_t *return_values = evaluate_term_list(return_term);
			value_t *val = malloc(sizeof val);
			*val = (value_t) { .type = type_arrlist, .list = return_values };
			s->val.ret = val;
		}

		return s->val;
	}

	// this case occurs only when ID is on RHS of '='
	if (s->val.type == _id) {
		char *id = s->val.str;
		value_t val = lookup_id(id);
		if (val.type == type_variant)
			val = *(val.variant);
		
		return val;
	}

	if (s->val.type == _if) {
		value_t predicate = s->child[0]->execute(s->child[0]);
		if (predicate.type != type_boolean) {
			error_print_ast(s, "Could not execute if block, wrong type of predicate (%s instead of %s)", node_type_to_str(predicate.type), node_type_to_str(type_boolean));
			return (value_t) { .type = type_void };
		}

		debug_print_stacks("BEFORE ENTER IF");
		enter_block();
		debug_print_stacks("AFTER ENTER IF");
		value_t val = (value_t) { .type = type_void };
		if (predicate.bool)
			val = s->child[1]->execute(s->child[1]);
		else if (s->child[2])
			val = s->child[2]->execute(s->child[2]);

		debug_print_stacks("BEFORE EXIT IF");
		exit_block();
		debug_print_stacks("AFTER EXIT IF");
		return val;
	}

	if (s->val.type == _break)
		return s->val;

	if (s->val.type == _loop) {
		int running = 1;
		while (running) {
			enter_block();
			debug_print_stacks("AFTER ENTER LOOP");
			value_t op = s->child[0]->execute(s->child[0]);
			if (op.type == _break)
				running = 0;
			exit_block();
			debug_print_stacks("AFTER EXIT LOOP");
		}
		return (value_t) { .type = type_void };
	}

	if (s->category(s) == cat_term) {
		value_t a = s->child[0]->execute(s->child[0]);
		value_t b = (value_t) { .type = type_void };
		if (s->child[1])
			b = s->child[1]->execute(s->child[1]);

		value_t res = evaluate_term(a, b, s->val.type);
		if (res.type == type_void) {
			error_print_ast(s, "Term could not be evaluated, no valid operation found for\n\tfirst operand: %s (%s), second operand: %s (%s), operation=[%s]",
							value_to_str(&a), node_type_to_str(a.type), value_to_str(&b), node_type_to_str(b.type), operator_to_str(s->val.type));
		}

		debug_print_term("Evaluated term", a, b, s->val.type, res);
		return res;
	}

	if (s->category(s) == cat_value) {
		value_t eval_val = s->val;
		if (s->val.type == type_arrlist) {
			list_t *lst = list_new();
			eval_val = (value_t) { .type = type_arrlist, .list = lst };

			// If list has no entries
			if (s->child[0] == NULL)
				return eval_val;

			// Evaluate list entries (if there are any)
			ast_iterator_t it;
			init_ast_iterator(&it, s->child[0]);
			astnode_t *node = NULL;
			while (1) {
				node = it.next_specific_node(&it, ast_of_type, (value_t) { .type = _term });
				if (node == NULL)
					break;

				// Evaluate term of array
				lst->add(lst, node->child[0]->execute(node->child[0]));
			}
			
		}

		return eval_val;
	}

	if (s->val.type == error_type) {
		error_print_ast(s, "Could not execute statement, syntax error (see information above)");
	}

	// Case else (statements)
	for (int i = 0; i < MAX_CHILDREN; i++) {
		if (s->child[i]) {
			value_t op = s->child[i]->execute(s->child[i]);
			if (op.type == _break)
				return op;
			if (op.type == _return)
				return op;
		}
	}

	return (value_t) { .type = type_void };
}

astnode_t *astnode_new(int type, int line) {
	astnode_t *a = calloc(sizeof *a, 1);
	a->id = ast_id++;
	a->line = line;
	a->val.type = type;
	a->parent = NULL;
	a->set_child = ast_set_child;
	a->print = ast_print;
	a->execute = ast_execute;
	a->to_str = ast_to_str;
	a->category = ast_category;
	a->index_of = ast_index_of;
	return a;
}

/* AST iterator */

// predicate functions (for astnode.next_specific_node)
int ast_of_type(astnode_t *a, value_t val)		{ return a->val.type == val.type; }
int ast_not_of_type(astnode_t *a, value_t val)	{ return a->val.type != val.type; }

astnode_t *ast_it_next(ast_iterator_t *s) {
	if (s->finished)		return NULL;
	if (s->current == NULL) {
		s->current = s->root;
		return s->current;
	}

	// case: current node has another child
	for (int i = s->child_index; i < MAX_CHILDREN; i++) {
		astnode_t *child = s->current->child[i];
		if (child != NULL) {
			s->current = child;
			s->child_index = 0;
			return child;
		}
	}

	// case: no further children and has parent
	astnode_t *parent = s->current->parent;
	if (s->current != s->root && parent != NULL) {
		int index = parent->index_of(parent, s->current);
		s->current = parent;
		s->child_index = index + 1;

		return ast_it_next(s);
	}

	// case: root node (no parent) without further children
	s->finished = 1;
	return NULL;
}

astnode_t *ast_it_next_specific_node(ast_iterator_t *s, int (*predicate)(astnode_t *, value_t), value_t val) {
	astnode_t *node = NULL;
	do {
		node = s->next(s);
		if (node == NULL)
			return NULL;

	} while (!predicate(node, val));

	return node;
}

void ast_it_skip_next_child(ast_iterator_t *s) {
	s->child_index++;
}

void ast_it_reset(ast_iterator_t *s) {
	init_ast_iterator(s, s->root);
}

void init_ast_iterator(ast_iterator_t *it, astnode_t *root) {
	it->root = root;
	it->current = NULL;
	it->child_index = 0;
	it->finished = 0;

	it->next = ast_it_next;
	it->next_specific_node = ast_it_next_specific_node;
	it->skip_next_child = ast_it_skip_next_child;
	it->reset = ast_it_reset;;
}
