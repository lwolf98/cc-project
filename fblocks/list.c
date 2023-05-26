#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include "global.h"

void list_add(list_t *s, value_t val) {
	s->arr = realloc(s->arr, (s->size+1)*sizeof(value_t));
	s->size++;
	s->set(s, s->size-1, val);
}

value_t list_get(list_t *s, int i) {
	if (i > s->size-1) { return (value_t) { .type = type_void }; }
	value_t val = s->arr[i];
	return val;
}

int list_set(list_t *s, int i, value_t val) {
	if (i > s->size-1) { return 1; }
	s->arr[i] = val;
	return 0;
}

void init_list(list_t *s) {
	s->arr = NULL;
	s->size = 0;
	s->add = list_add;
	s->get = list_get;
	s->set = list_set;
}

list_t *list_new() {
	list_t *lst = malloc(sizeof(list_t));
	init_list(lst);
	return lst;
}

list_t *merge_lists(list_t *a, list_t *b) {
	list_t *res = list_new();
	for (int i = 0; i < a->size; i++)
		res->add(res, a->get(a, i));
	for (int i = 0; i < b->size; i++)
		res->add(res, b->get(b, i));

	return res;
}

void include_list(list_t *main, list_t *to_add) {
	for (int i = 0; i < to_add->size; i++)
		main->add(main, to_add->get(to_add, i));
}
