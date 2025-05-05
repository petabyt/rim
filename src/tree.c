// Copyright Daniel C 2025
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "rim_internal.h"

// This is probably not the ideal way to do this
void rim_tree_save_state(void) {
	struct RimContext *ctx = rim_get_global_ctx();
	memcpy(&ctx->tree_saved, ctx->tree_new, sizeof(struct RimTree));
	ctx->tree_saved.buffer = malloc(ctx->tree_saved.buffer_length);
	memcpy(ctx->tree_saved.buffer, ctx->tree_new->buffer, ctx->tree_saved.buffer_length);
}
void rim_tree_restore_state(void) {
	struct RimContext *ctx = rim_get_global_ctx();
	free(ctx->tree_new->buffer);
	memcpy(ctx->tree_new, &ctx->tree_saved, sizeof(struct RimTree));
}

// Ensure 'size' can fit into the space left in the buffer
static void ensure_buffer_size(struct RimTree *tree, unsigned int size) {
	if (tree->buffer_length < (tree->of + size)) {
		tree->buffer = realloc(tree->buffer, size + 1000);
		tree->buffer_length = size + 1000;
	}
}

// All data written to/from the tree must be 8 byte aligned
// to make sure misaligned reads/writes won't happen.
#ifndef NDEBUG
inline static void check_align(const void *ptr) {
	if ((((uintptr_t)ptr) & 0b111) != 0) {
		rim_abort("Misaligned access\n");
	}
}
#else
#define check_align(ptr) ;
#endif

inline static int copy_string(uint8_t *to, const char *str) {
	check_align(to);
	size_t len = strlen(str);
	int aligned_len = (((int)len + 1) / 8 + 1) * 8;
	memset(to, 0, aligned_len);
	strcpy((char *)to, str);
	return aligned_len;
}
inline static int write_u32(uint8_t *from, uint32_t x) {
	check_align(from);
	((uint32_t *)from)[0] = x;
	return 4;
}
inline static uint32_t read_u32(const uint8_t *from, uint32_t *temp) {
	check_align(from);
	*temp = ((uint32_t *)from)[0];
	return 4;
}

const char *rim_eval_widget_type(uint32_t type) {
	switch (type) {
	case RIM_WINDOW: return "window";
	case RIM_LABEL: return "label";
	case RIM_BUTTON: return "button";
	case RIM_PROGRESS_BAR: return "progress_bar";
	case RIM_IMAGE: return "image";
	case RIM_ENTRY: return "entry";
	case RIM_SPINBOX: return "spinbox";
	case RIM_SLIDER: return "slider";
	case RIM_COMBOBOX: return "combobox";
	case RIM_RADIO: return "radio";
	case RIM_DATE_PICKER: return "date_picker";
	case RIM_TABLE: return "table";
	case RIM_TAB: return "tab";
	case RIM_TAB_BAR: return "tab_bar";
	case RIM_HORIZONTAL_BOX: return "layout_static";
	case RIM_VERTICAL_BOX: return "layout_dynamic";
	case RIM_FLEX_BOX: return "layout_flex";
	case RIM_WINDOW_MENU_BAR: return "RIM_WINDOW_MENU_BAR";
	case RIM_WINDOW_MENU: return "RIM_WINDOW_MENU";
	case RIM_WINDOW_MENU_ITEM: return "RIM_WINDOW_MENU_ITEM";
	default: return "???";
	}
}

void rim_reset_tree(struct RimTree *tree) {
	tree->widget_stack_depth = 0;
	tree->of = 0;
	tree->counter = 0;
	tree->n_root_children = 0;
}

struct RimTree *rim_create_tree(void) {
	struct RimTree *tree = (struct RimTree *)malloc(sizeof(struct RimTree));
	tree->buffer = malloc(1000);
	tree->buffer_length = 1000;
	rim_reset_tree(tree);
	return tree;
}

void rim_end_widget(struct RimTree *tree) {
	if (tree->widget_stack_depth == 0) rim_abort("rim_end_widget called too many times");
	tree->widget_stack_depth--;
}

void rim_add_widget(struct RimTree *tree, enum RimWidgetType type, int allowed_children) {
	ensure_buffer_size(tree, sizeof(struct WidgetHeader));
	struct WidgetHeader *h = (struct WidgetHeader *)(tree->buffer + tree->of);
	h->type = type;
	h->n_children = 0;
	h->n_props = 0;
	h->os_handle = 0;
	h->is_detached = 0;
	h->invalidate = 0;
	h->unique_id = tree->counter;
	h->allowed_children = (uint32_t)allowed_children;
	tree->of += sizeof(struct WidgetHeader);

	check_align(&h->os_handle);

	if (tree->widget_stack_depth == 0) {
		tree->n_root_children++;
	}

	tree->counter++;

	if (tree->widget_stack_depth != 0) {
		// Track a potential parent widget (simply a preceding widget)
		// We want to know if it allows children, and if we can add this widget to it.
		// If so, increment n_children. Else we end the preceding widget
		struct WidgetHeader *potential_parent = tree->widget_stack[tree->widget_stack_depth - 1];

		// TODO: Ending widget a widget automatically caused issues
		// allowed_children is potentially not even needed in the first place
#if 0
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
	if (tree->widget_stack_depth == TREE_MAX_DEPTH) {
		rim_abort("Max depth reached");
	}
}

//struct WidgetProp *rim_add_prop(struct RimTree *tree, enum RimPropType type) {
//	
//}

void rim_add_prop_string(struct RimTree *tree, enum RimPropType type, const char *value) {
	if (tree->widget_stack_depth == 0) {
		rim_abort("No widget to add property to\n");
	}
	ensure_buffer_size(tree, sizeof(struct WidgetProp) + strlen(value) + 1); // nitpick: double strlen
	struct WidgetHeader *parent = tree->widget_stack[tree->widget_stack_depth - 1];
	struct WidgetProp *prop = (struct WidgetProp *)(tree->buffer + tree->of);
	prop->length = sizeof(struct WidgetProp);
	prop->type = type;
	prop->already_fufilled = 0;
	prop->length += copy_string(prop->data, value);
	tree->of += (int)prop->length;
	parent->n_props++;
}

void rim_add_prop_u32(struct RimTree *tree, enum RimPropType type, uint32_t val) {
	if (tree->widget_stack_depth == 0) {
		rim_abort("No widget to add property to\n");
	}
	ensure_buffer_size(tree, sizeof(struct WidgetProp) + 4);
	struct WidgetHeader *parent = tree->widget_stack[tree->widget_stack_depth - 1];
	struct WidgetProp *prop = (struct WidgetProp *)(tree->buffer + tree->of);
	prop->length = sizeof(struct WidgetProp) + 8;
	prop->type = type;
	prop->already_fufilled = 0;
	memcpy(prop->data, &val, 4);
	tree->of += (int)prop->length;
	parent->n_props++;
}

void rim_add_prop_data(struct RimTree *tree, enum RimPropType type, void *val, unsigned int length) {
	length = ((length / 8) + 1) * 8; // ensure align by 8
	if (tree->widget_stack_depth == 0) {
		rim_abort("No widget to add property to\n");
	}
	ensure_buffer_size(tree, sizeof(struct WidgetProp) + length);
	struct WidgetHeader *parent = tree->widget_stack[tree->widget_stack_depth - 1];
	struct WidgetProp *prop = (struct WidgetProp *)(tree->buffer + tree->of);
	prop->length = sizeof(struct WidgetProp) + length;
	prop->type = type;
	prop->already_fufilled = 0;
	memcpy(prop->data, val, length);
	tree->of += (int)prop->length;
	parent->n_props++;
}

struct WidgetProp *rim_get_prop(struct WidgetHeader *h, int type) {
	unsigned int of = 0;
	for (size_t i = 0; i < h->n_props; i++) {
		struct WidgetProp *p = (struct WidgetProp *)(h->data + of);
		if ((int)p->type == type) {
			return p;
		}
		of += (int)p->length;
	}
	return NULL;
}

// TODO: rewrite with rim_get_prop
int rim_get_prop_string(struct WidgetHeader *h, int type, char **val) {
	unsigned int of = 0;
	for (size_t i = 0; i < h->n_props; i++) {
		struct WidgetProp *p = (struct WidgetProp *)(h->data + of);
		if ((int)p->type == type) {
			(*val) = (char *)p->data;
			return 0;
		}
		of += (int)p->length;
	}
	return -1;
}

// TODO: rewrite with rim_get_prop
int rim_get_prop_u32(struct WidgetHeader *h, int type, uint32_t *val) {
	unsigned int of = 0;
	for (size_t i = 0; i < h->n_props; i++) {
		struct WidgetProp *p = (struct WidgetProp *)(h->data + of);
		if ((int)p->type == type) {
			memcpy(val, p->data, 4);
			return 0;
		}
		of += (int)p->length;
	}
	return -1;
}

unsigned int rim_get_node_length(struct WidgetHeader *w) {
	unsigned int of = 0;
	for (unsigned int i = 0; i < w->n_props; i++) {
		struct WidgetProp *p = (struct WidgetProp *)(w->data + of);
		of += p->length;
	}
	for (unsigned int i = 0; i < w->n_children; i++) {
		struct WidgetHeader *c = (struct WidgetHeader *)(w->data + of);
		of += rim_get_node_length(c);
	}
	return sizeof(struct WidgetHeader) + of;
}

int rim_get_child_index(struct WidgetHeader *w, struct WidgetHeader *parent) {
	unsigned int of = 0;
	for (size_t i = 0; i < parent->n_props; i++) {
		struct WidgetProp *p = (struct WidgetProp *)(parent->data + of);
		of += p->length;
	}

	unsigned int skip = 0;

	for (unsigned int i = 0; i < parent->n_children; i++) {
		struct WidgetHeader *c = (struct WidgetHeader *)(parent->data + of);
		if (c->is_detached) skip++;
		if (w->unique_id == c->unique_id) {
			return (int)(i - skip);
		}
		of += rim_get_node_length(c);
	}

	return -1;
}

struct WidgetHeader *rim_get_child(struct WidgetHeader *w, int index) {
	unsigned int of = 0;
	for (size_t i = 0; i < w->n_props; i++) {
		struct WidgetProp *p = (struct WidgetProp *)(w->data + of);
		of += p->length;
	}

	unsigned int skip = 0;

	for (unsigned int i = 0; i < w->n_children; i++) {
		struct WidgetHeader *c = (struct WidgetHeader *)(w->data + of);
		if (c->is_detached) skip++;
		if ((int)i == index) {
			return c;
		}
		of += rim_get_node_length(c);
	}
	return NULL;
}

int rim_find_in_tree(struct RimTree *tree, unsigned int *of, uint32_t unique_id) {
	if (tree->of < (int)sizeof(struct WidgetHeader)) abort();

	struct WidgetHeader *h = (struct WidgetHeader *)(tree->buffer + (*of));

	if (h->unique_id == unique_id) return 1;

	(*of) += sizeof(struct WidgetHeader);

	for (size_t i = 0; i < h->n_props; i++) {
		struct WidgetProp *p = (struct WidgetProp *)(tree->buffer + (*of));
		(*of) += (int)p->length;
	}

	for (size_t i = 0; i < h->n_children; i++) {
		if (rim_find_in_tree(tree, of, unique_id)) {
			return 1;
		}
	}

	return 0;
}
