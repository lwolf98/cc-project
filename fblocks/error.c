#include <stdio.h>
#include <stdarg.h>
#include "error.h"
#include "evaluation.h"

void error_print(int line, const char *msg, ...) {
	char msg_buf[512];
	va_list args;
	va_start(args, msg);
	vsnprintf(msg_buf, sizeof(msg_buf), msg, args);
	fprintf(stderr, "ERR: Line: %d:\n\t%s\n\n", line, msg_buf);
	va_end(args);
}

void error_print_type(int line, const char *type, const char *msg, ...) {
	char msg_buf[512];
	va_list args;
	va_start(args, msg);
	vsnprintf(msg_buf, sizeof(msg_buf), msg, args);
	fprintf(stderr, "ERR: Line: %d:\n\t%s, %s\n\n", line, type, msg_buf);
	va_end(args);
}

void error_print_ast(const astnode_t *node, const char *msg, ...) {
	char msg_buf[512];
	va_list args;
	va_start(args, msg);
	vsnprintf(msg_buf, sizeof(msg_buf), msg, args);
	fprintf(stderr, "ERR: Line: %d, AST: [%s]:\n\t%s\n\n", node->line, node_type_to_str(node->val.type), msg_buf);
	va_end(args);
}
