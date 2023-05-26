#pragma once

#include "global.h"

typedef struct {
	char *id;
	value_t val;
} val_t;

typedef struct stack_t {
	val_t *storage;
	int index;

	void (*push)(struct stack_t *, val_t);
	val_t (*pop)(struct stack_t *);
	val_t (*peek)(struct stack_t *);
	int (*isempty)(struct stack_t *);
	val_t *(*lookup)(struct stack_t *, char *id);
	void (*pop_to_index)(struct stack_t *, int index);
	void (*exit_scope)(struct stack_t *, int scope_type);
} stack_t;

stack_t *s_new();