#pragma once

#include "ast.h"

enum err_types {
	err_id_already_defined = 1,
	err_id_not_defined,
	err_type_mismatch,
	err_wrong_args_number
};

int set_exit_on_error(int exit);
void error_print(int line, const char *msg, ...);
void error_print_type(int line, const char *type, const char *msg, ...);
void error_print_ast(const astnode_t *node, const char *msg, ...);