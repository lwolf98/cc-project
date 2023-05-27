#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "error.h"
#include "evaluation.h"

static int exit_on_error = 1;	// Default behavior, whether to exit the program or not in an error case

int set_exit_on_error(int exit) {
	int prev_value = exit_on_error;
	exit_on_error = exit;
	return prev_value;
}

void error_print(int line, const char *msg, ...) {
	char msg_buf[512];
	va_list args;
	va_start(args, msg);
	vsnprintf(msg_buf, sizeof(msg_buf), msg, args);
	fprintf(stderr, "ERR: Line: %d:\n\t%s\n\n", line, msg_buf);
	va_end(args);
	
	if (exit_on_error)
		exit(EXIT_FAILURE);
}

void error_print_type(int line, const char *type, const char *msg, ...) {
	char msg_buf[512];
	va_list args;
	va_start(args, msg);
	vsnprintf(msg_buf, sizeof(msg_buf), msg, args);
	fprintf(stderr, "ERR: Line: %d:\n\t%s, %s\n\n", line, type, msg_buf);
	va_end(args);
	
	if (exit_on_error)
		exit(EXIT_FAILURE);
}

void error_print_ast(const astnode_t *node, const char *msg, ...) {
	char msg_buf[512];
	va_list args;
	va_start(args, msg);
	vsnprintf(msg_buf, sizeof(msg_buf), msg, args);
	fprintf(stderr, "ERR: Line: %d, AST: [%s]:\n\t%s\n\n", node->line, node_type_to_str(node->val.type), msg_buf);
	va_end(args);

	if (exit_on_error)
		exit(EXIT_FAILURE);
}
