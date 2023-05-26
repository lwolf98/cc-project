#include <stdio.h>
#include <stdarg.h>
#include "ast.h"
#include "stack.h"
#include "strop.h"
#include "evaluation.h"

extern stack_t *vars_global, *vars;

// Setup console debugging information
static int debugging =			0;	// Turn on/off general debugging messages
static int debug_tree_node =	0;	// Print information about the currently executed AST node
static int debug_stack =		0;	// Print the state of the global and local variable stack

// Flags for additional information in the AST node presentation
// (used in ast_to_str, e. g. for the graphical presentation of the AST)
static int debug_id_flag =		1;	// Add the AST node ID 
static int debug_line_flag =	1;	// Add the corresponding line number to 

int debug_id()		{ return debug_id_flag; }
int debug_line()	{ return debug_line_flag; }

void debug_print_node(astnode_t *node) {
	if (debug_tree_node)
		printf("EXEC NODE: %s\n", node->to_str(node));
}

void debug_print_plain(const char *format, ...) {
	if (!debugging)
		return;

	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

void debug_print(const char *format, ...) {
	if (!debugging)
		return;

	va_list args;
	va_start(args, format);
	printf("DBG: ");
	vprintf(format, args);
	va_end(args);
}

void debug_print_term(const char *msg, const value_t a, const value_t b, const int op, const value_t res) {
	char *val_a = value_to_str(&a);
	char *val_b = value_to_str(&b);
	char *val_res = value_to_str(&res);
	char *str_op = operator_to_str(op);
	debug_print("%s: (%s,%s,%s) => %s\n", msg, val_a, val_b, str_op, val_res);
}

void debug_traverse_ast(astnode_t *a) {
	ast_iterator it;
	init_ast_iterator(&it, a);
	while (!it.finished) {
		astnode_t *node = it.next(&it);
		if (node)
			debug_print("Node: %s\n", node->to_str(node));
	}
}

char *stack_to_str(stack_t *s) {
	string buf;
	init_string(&buf);
	buf.append_char(&buf, '{');
	for (int i = 0; i <= s->index; i++) {
		if (i > 0) buf.append_char(&buf, ',');
		if (i == s->index) buf.append_char(&buf, '[');

		val_t *var = &s->storage[i];
		if (var->val.type == weak_border) buf.append(&buf, "BLOCK");
		else if (var->val.type == strong_border) buf.append(&buf, "FNCT");
		else {
			buf.append(&buf, var->id);
			buf.append_char(&buf, '=');
			char *str_val = value_to_str(&var->val);
			buf.append(&buf, str_val);
		}

		if (i == s->index) buf.append_char(&buf, ']');
	}
	buf.append_char(&buf, '}');
	return buf.get(&buf);
}

void debug_print_stacks(char *msg) {
	if (debug_stack) {
		if (msg) printf("%s:\n", msg);
		printf("Global vars:	%s\n", stack_to_str(vars_global));
		printf("Local vars:	%s\n\n", stack_to_str(vars));
	}
}