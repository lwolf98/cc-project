#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "stack.h"
#include "strop.h"

void s_push(stack_t *s, val_t num);
val_t s_pop(stack_t *s);
val_t s_peek(stack_t *s);
int s_isempty(stack_t *s);
val_t *s_lookup(struct stack_t *s, char *id);
void s_pop_to_index(stack_t *s, int index);
void s_exit_scope(stack_t *s, int scope_type);

stack_t *s_new() {
	stack_t *stack = malloc(sizeof *stack);

	stack->index = -1;
	stack->storage = NULL;
	stack->push = s_push;
	stack->pop = s_pop;
	stack->peek = s_peek;
	stack->isempty = s_isempty;
	stack->lookup = s_lookup;
	stack->pop_to_index = s_pop_to_index;
	stack->exit_scope = s_exit_scope;

	return stack;
}

void s_push(stack_t *s, val_t num) {
	s->index++;
	if (s->storage == NULL)
		s->storage = malloc(1 * sizeof(val_t *));
	else
		s->storage = realloc(s->storage, (s->index+1) * sizeof num);

	s->storage[s->index] = num;
}

val_t s_pop(stack_t *s) {
	if (s->index < 0) {
		val_t empty = { .id = NULL };
		return empty;
	}

	val_t value = s->storage[s->index];
	s->index--;
	s->storage = realloc(s->storage, (s->index+1) * sizeof value);
	return value;
}

val_t s_peek(stack_t *s) {
	if (s->index < 0) {
		val_t empty = { .id = NULL };
		return empty;
	}

	return s->storage[s->index];
}

int s_isempty(stack_t *s) {
	return s->index < 0;
}

val_t *s_lookup(stack_t *s, char *id) {
	val_t *var = NULL;
	for (int i = s->index; i >= 0; i--) {
		var = &s->storage[i];
		if (var->val.type == strong_border)
			return NULL;
		if (var->val.type == weak_border)
			continue;
		if (strcmp(var->id, id) == 0)
			return var;
	}

	return NULL;
}

void s_pop_to_index(stack_t *s, int index) {
	if (index < 0) index = 0;
	while (s->index > index) {
		val_t v = s->pop(s);
		debug_print("Popped %s\n", v.id);
		debug_print("Index = %d\n", s->index);
	}
}

void s_exit_scope(stack_t *s, int scope_type) {
	val_t v;
	//while ((v = s->pop(s)).val.type != scope_type && s->index > 0);
	do {
		v = s->pop(s);
	} while (v.val.type != scope_type && s->index > -1);
}
