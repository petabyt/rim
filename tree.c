#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "nim.h"

inline static int copy_string(uint8_t *to, const char *str) {
	size_t len = strlen(str);
	int aligned_len = (((int)len + 1) / 4 + 1) * 4;
	memset(to, 0, aligned_len);
	strcpy((char *)to, str);
	return aligned_len;
}
inline static int write_u32(uint8_t *from, uint32_t x) {
	((uint32_t *)from)[0] = x;
	return 4;
}
inline static uint32_t read_u32(const uint8_t *from, uint32_t *temp) {
	*temp = ((uint32_t *)from)[0];
	return 4;
}

struct __attribute__((packed)) WidgetProp {
	uint32_t length;
	uint32_t type;
	uint8_t data[];
};

struct __attribute__((packed)) WidgetPropButton {
	uint32_t length;
	uint32_t type;
	uint8_t data[];
};

void nim_end_widget(struct Tree *tree) {
	assert(tree->widget_stack_depth != 0);
	tree->widget_stack_depth--;
}

void nim_add_widget(struct Tree *tree, enum WidgetTypes type, int allowed_children) {
	struct WidgetHeader *h = (struct WidgetHeader *)(tree->buffer + tree->of);
	h->type = type;
	h->n_children = 0;
	h->n_props = 0;
	h->os_handle = 0;
	h->unique_id = 123;
	h->allowed_children = (uint32_t)allowed_children;
	tree->of += sizeof(struct WidgetHeader);

	if (tree->widget_stack_depth) {
		// Track a potential parent widget (simply a preceeding widget)
		// We want to know if it allows children, and if we can add this widget to it.
		// If so, increment n_children. Else we end the preceeding widge
		struct WidgetHeader *potential_parent = tree->widget_stack[tree->widget_stack_depth - 1];
#if 0 // caused problems
		if (potential_parent->allowed_children != 0xffffffff && potential_parent->allowed_children >= potential_parent->n_children) {
			end_widget(tree);
		}
#endif

		// Add to potential parent's sibling
		if (tree->widget_stack_depth) {
			potential_parent = tree->widget_stack[tree->widget_stack_depth - 1];
			potential_parent->n_children++;
		}
	}

	tree->widget_stack[tree->widget_stack_depth] = h;
	tree->widget_stack_depth++;
	if (tree->widget_stack_depth == 5) {
		printf("Max depth reached\n");
		abort();
	}
}

void nim_add_prop_text(struct Tree *tree, enum PropTypes type, const char *value) {
	if (tree->widget_stack_depth == 0) {
		printf("No widget to add property to\n");
		abort();
	}
	struct WidgetHeader *parent = tree->widget_stack[tree->widget_stack_depth - 1];
	struct WidgetProp *prop = (struct WidgetProp *)(tree->buffer + tree->of);
	prop->length = 8;
	prop->type = type;
	prop->length += copy_string(prop->data, value);
	tree->of += prop->length;
	parent->n_props++;
}

#if 0
int nim_get_curr(struct NimBackend *b, int *type) {
	struct WidgetHeader *h = (struct WidgetHeader *)(b->tree->buffer + b->of);
	b->of += sizeof(struct WidgetHeader);
	(*type) = h->type;
	b->iter_prop = 0;
	return 0;
}

int nim_next_prop(struct NimBackend *b, struct NimProp *np) {
	if (b->iter_prop < (int)b->header->n_props) return 0;

	struct WidgetProp *p = (struct WidgetProp *)(b->tree->buffer + b->of);
	np->type = p->type;
	b->of += p->length;

	return 1;
}
#endif

int nim_get_prop(struct WidgetHeader *h, struct NimProp *np, int type) {
	int of = 0;
	for (size_t i = 0; i < h->n_props; i++) {
		struct WidgetProp *p = (struct WidgetProp *)((uint8_t *)h + of);
		if ((int)p->type == type) {
			np->type = p->type;
			np->value = (const char *)p->data;
			return 0;
		}
		of += p->length;
	}
	return -1;
}

int nim_init_tree(struct NimBackend *backend, int base, struct WidgetHeader *parent, int depth) {
	struct Tree *tree = backend->tree;
	int of = 0;
	uint8_t *buffer = tree->buffer + base;

	if (tree->of < (int)sizeof(struct WidgetHeader)) return 0;

	struct WidgetHeader *h = (struct WidgetHeader *)(buffer + of);
	of += sizeof(struct WidgetHeader);

	backend->create(NULL, h);
	backend->append(NULL, h, parent);

	for (size_t i = 0; i < h->n_props; i++) {
		// Assumes string type
		struct WidgetProp *p = (struct WidgetProp *)(buffer + of);
		of += p->length;
	}

	for (size_t i = 0; i < h->n_children; i++) {
		of += nim_init_tree(backend, base + of, h, depth + 1);
	}

	return of;
}

static int diff_tree_apply(struct Tree *tree_old, struct Tree *tree_new, int *old_of_p, int *new_of_p, int ignore) {
	int old_of = *old_of_p;
	int new_of = *new_of_p;

	struct WidgetHeader *old_h = (struct WidgetHeader *)(tree_old->buffer + old_of);
	old_of += sizeof(struct WidgetHeader);
	struct WidgetHeader *new_h = (struct WidgetHeader *)(tree_new->buffer + new_of);
	new_of += sizeof(struct WidgetHeader);

	if (old_h->type != new_h->type) {
		printf("widget %d type changed\n", old_h->unique_id);
		ignore = 1;
	} else {
		printf("widget %d same state\n", old_h->unique_id);
	}

	uint32_t max_n_props = old_h->n_props;
	if (new_h->n_props > max_n_props) max_n_props = new_h->n_props;
	for (size_t i = 0; i < max_n_props; i++) {
		struct WidgetProp *old_p = (struct WidgetProp *)(tree_old->buffer + old_of);
		struct WidgetProp *new_p = (struct WidgetProp *)(tree_new->buffer + new_of);

		if (i >= old_h->n_props) {
			if (ignore == 0) printf("property added\n");
			new_of += new_p->length;
			continue;
		} else if (i >= new_h->n_props) {
			if (ignore == 0) printf("property removed\n");
			old_of += old_p->length;
			continue;
		}

		if (ignore == 0) {
			if (old_p->length == new_p->length && !memcmp(old_p, new_p, old_p->length)) {
				printf("property %d same state\n", (int)i);			
			} else {
				printf("property %d in %d changed\n", (int)i, old_h->unique_id);
			}
		}

		old_of += old_p->length;
		new_of += new_p->length;
	}

	uint32_t max_n_child = old_h->n_children;
	if (new_h->n_children > max_n_child) max_n_child = new_h->n_children;
	(*old_of_p) = old_of;
	(*new_of_p) = new_of;
	for (size_t i = 0; i < max_n_child; i++) {
		if (i >= old_h->n_children) {
			if (ignore == 0) {
				printf("child added to tree\n");
				printf("Unimplemented"); abort();
			}
		} else if (i >= old_h->n_children) {
			if (ignore == 0) {
				printf("child removed\n");
				printf("Unimplemented"); abort();
			}
		} else {
			diff_tree_apply(tree_old, tree_new, old_of_p, new_of_p, ignore);
		}
	}
	
	return 0;
}

static int diff_tree(struct Tree *tree_old, struct Tree *tree_new) {
	int old_of = 0;
	int new_of = 0;
	return diff_tree_apply(tree_old, tree_new, &old_of, &new_of, 0);
}

int nim_libui_start(void);

int main(void) {

	//dump_tree(&tree, 0, 0);
	//diff_tree(&tree, &tree2);

	nim_libui_start();
	
	return 0;
}
