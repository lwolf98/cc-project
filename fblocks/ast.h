#pragma once

#define MAX_CHILDREN 4

struct list;
typedef struct list list_t;
struct astnode;
typedef struct astnode astnode_t;

typedef struct {
	astnode_t *types;
	astnode_t *variables;
	astnode_t *return_types;
	astnode_t *fnct;
} function_t;

typedef struct val {
	int type;
	union {
		char *str;
		int dec;
		int bool;
		astnode_t *fnct;
		list_t *list;
		struct val *variant;
		struct val *ret;
	};
} value_t;

enum ast_type_categories {
	cat_term,
	cat_value,
	cat_special_type,
	cat_else
};

struct astnode {
	int id;
	int line;
	value_t val;
	struct astnode *parent;
	struct astnode *child[MAX_CHILDREN];

	void (*set_child)(struct astnode *, int, struct astnode *);
	void (*print)(struct astnode *);
	value_t (*execute)(struct astnode *);
	char * (*to_str)(struct astnode *);
	int (*category)(struct astnode *);
	int (*index_of)(struct astnode *, struct astnode *child);
};

astnode_t *astnode_new(int type, int line);


/* AST iterator */

// predicate functions (for astnode.next_specific_node)
int ast_of_type(astnode_t *a, value_t val);
int ast_not_of_type(astnode_t *a, value_t val);

typedef struct ast_it {
	astnode_t *root;
	astnode_t *current;
	int child_index;
	int finished;

	astnode_t * (*next)(struct ast_it *);
	astnode_t * (*next_specific_node)(struct ast_it *s, int (*predicate)(astnode_t *, value_t), value_t);
	void (*skip_next_child)(struct ast_it *s);
	void (*reset)(struct ast_it *);
} ast_iterator;

void init_ast_iterator(ast_iterator *it, astnode_t *root);
