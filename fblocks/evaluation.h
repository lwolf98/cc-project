#pragma once

#include "global.h"

value_t evaluate_term(value_t a, value_t b, int op);
list_t *evaluate_term_list(astnode_t *value_list);

char *data_type_to_str(int type);
char *node_type_to_str(int type);
char *operator_to_str(int op);
char *value_to_str(const value_t *val);
char *lib_f_to_str(int type);

int define_id(int type, char *id);
int set_id(char *id, value_t value);
value_t lookup_id(char *id);
void enter_function();
void exit_function();
void enter_block();
void exit_block();
int assign_fnct_params(astnode_t *id_list, list_t *params);

char *dec_to_str(int val);
char *bool_to_str(int val);
int bool_to_dec(int val);
int dec_to_bool(int val);
int str_to_bool(char *val);
int str_to_dec(char *val);
