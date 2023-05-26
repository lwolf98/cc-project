#pragma once

typedef struct string {
	char *value;
	int size;

	struct string * (*append)(struct string *, char *);
	struct string * (*append_char)(struct string *, char);
	char * (*get)(struct string *);
} string;

void init_string(string *str);
void dbg_strop();