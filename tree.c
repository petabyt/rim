// Copyright Daniel C 2025
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "nim.h"
#include "nim_internal.h"

#define CONFIG_ALIGNMENT_CHECKS

// All data written to/from the tree must be 32 bit aligned. This is so that
// 4 ldrb/strb don't have to be done.
inline static int copy_string(uint8_t *to, const char *str) {
#ifdef CONFIG_ALIGNMENT_CHECKS
	assert((((uintptr_t)to) & 0b11) == 0);
#endif
	size_t len = strlen(str);
	int aligned_len = (((int)len + 1) / 4 + 1) * 4;
	memset(to, 0, aligned_len);
	strcpy((char *)to, str);
	return aligned_len;
}
inline static int write_u32(uint8_t *from, uint32_t x) {
#ifdef CONFIG_ALIGNMENT_CHECKS
	assert((((uintptr_t)from) & 0b11) == 0);
#endif
	((uint32_t *)from)[0] = x;
	return 4;
}
inline static uint32_t read_u32(const uint8_t *from, uint32_t *temp) {
#ifdef CONFIG_ALIGNMENT_CHECKS
	assert((((uintptr_t)from) & 0b11) == 0);
#endif
	*temp = ((uint32_t *)from)[0];
	return 4;
}

int nim_abort(const char *reason) {
	puts(reason);
	abort();
}

struct NimTree *nim_create_tree(void) {
	struct NimTree *tree = (struct NimTree *)malloc(sizeof(struct NimTree));
	tree->widget_stack_depth = 0;
	tree->buffer = malloc(1000);
	tree->buffer_length = 1000;
	tree->of = 0;
	return tree;
}

void nim_end_widget(struct NimTree *tree) {
	assert(tree->widget_stack_depth != 0);
	tree->widget_stack_depth--;
}

void nim_add_widget(struct NimTree *tree, enum NimWidgetType type, int allowed_children) {
	struct WidgetHeader *h = (struct WidgetHeader *)(tree->buffer + tree->of);
	h->type = type;
	h->n_children = 0;
	h->n_props = 0;
	h->os_handle = 0;
//	h->unique_id = nim_generate_unique_id(ctx);
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

void nim_add_prop_text(struct NimTree *tree, enum NimPropType type, const char *value) {
	if (tree->widget_stack_depth == 0) {
		printf("No widget to add property to\n");
		abort();
	}
	struct WidgetHeader *parent = tree->widget_stack[tree->widget_stack_depth - 1];
	struct WidgetProp *prop = (struct WidgetProp *)(tree->buffer + tree->of);
	prop->length = 8;
	prop->type = type;
	prop->length += copy_string(prop->data, value);
	tree->of += (int)prop->length;
	parent->n_props++;
}

int nim_get_prop(struct WidgetHeader *h, struct NimProp *np, int type) {
	int of = 0;
	for (size_t i = 0; i < h->n_props; i++) {
		struct WidgetProp *p = (struct WidgetProp *)(h->data + of);
		if ((int)p->type == type) {
			np->type = (int)p->type;
			np->value = (const char *)p->data;
			return 0;
		}
		of += (int)p->length;
	}
	return -1;
}

struct NimContext *nim_init(void) {
	struct NimContext *ctx = (struct NimContext *)calloc(1, sizeof(struct NimContext));
	ctx->of = 0;
	ctx->header = 0;

	ctx->tree_new = nim_create_tree();
	ctx->tree_old = nim_create_tree();

	sem_init(&ctx->event_sig, 0, 0);

	return ctx;
}