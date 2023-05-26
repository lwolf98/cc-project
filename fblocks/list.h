#pragma once

#include "ast.h"

typedef struct list {
	value_t *arr;
	int size;

	void (*add)(struct list *s, value_t val);
	value_t (*get)(struct list *s, int i);
	int (*set)(struct list *s, int i, value_t val);
} list_t;

void init_list(list_t *s);
list_t *list_new();

list_t *merge_lists(list_t *a, list_t *b);
void include_list(list_t *main, list_t *to_add);
