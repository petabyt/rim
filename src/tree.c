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
		tree->buffer = realloc(tree->buffer, tree->of + size + 1000);
		if (tree->buffer == NULL) abort();
		tree->buffer_length = tree->of + size + 1000;
	}
}

// All data written to/from the tree must be 8 byte aligned
// to make sure misaligned reads/writes won't happen.
#ifndef NDEBUG
inline static void check_align(const void *ptr) {
	if ((((uintptr_t)ptr) & 7) != 0) {
		rim_abort("Misaligned access\n");
	}
}
#else
#define check_align(ptr) ;
#endif

inline static unsigned int copy_string(uint8_t *to, const char *str, unsigned int max) {
	check_align(to);
	unsigned int i;
	for (i = 0; str[i] != '\0'; i++) {
		if (i > max) rim_abort("overflow");
		to[i] = str[i];
	}
	to[i] = '\0';
	i++;
	unsigned int aligned_len = (i / 8 + 1) * 8;
	for (; i < aligned_len; i++) {
		if (i > max) rim_abort("overflow");
		to[i] = '\0';
	}
	return aligned_len;
}
inline static unsigned int write_u32(uint8_t *to, uint32_t x) {
	check_align(to);
	((uint32_t *)to)[0] = x;
	return 4;
}
inline static unsigned int read_u32(const uint8_t *from, uint32_t *temp) {
	check_align(from);
	*temp = ((const uint32_t *)from)[0];
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
	unsigned int this_w_of = tree->of;
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
		// If this widget has a parent, increment the number of children it has
		struct WidgetHeader *potential_parent = (struct WidgetHeader *)(tree->buffer + tree->widget_stack[tree->widget_stack_depth - 1]);
		potential_parent->n_children++;
	}


	tree->widget_stack[tree->widget_stack_depth] = this_w_of;
	tree->widget_stack_depth++;
	if (tree->widget_stack_depth == TREE_MAX_DEPTH) {
		rim_abort("Max depth reached");
	}
}

struct WidgetProp *rim_add_prop(struct RimTree *tree, enum RimPropType type) {
	ensure_buffer_size(tree, sizeof(struct WidgetProp));
	if (tree->widget_stack_depth == 0) {
		rim_abort("No widget to add property to\n");
	}
	struct WidgetHeader *parent = (struct WidgetHeader *)(tree->buffer + tree->widget_stack[tree->widget_stack_depth - 1]);
	parent->n_props++;
	struct WidgetProp *prop = (struct WidgetProp *)(tree->buffer + tree->of);
	prop->length = sizeof(struct WidgetProp);
	prop->type = type;
	prop->already_fufilled = 0;
	prop->res0 = 0;
	tree->of += prop->length;
	return prop;
}


void rim_add_prop_string(struct RimTree *tree, enum RimPropType type, const char *value) {
	struct WidgetProp *prop = rim_add_prop(tree, type);
	ensure_buffer_size(tree, sizeof(struct WidgetProp) + strlen(value) + 8);
	unsigned int dat_len = copy_string(prop->data, value, tree->buffer_length - tree->of);
	prop->length += dat_len;
	tree->of += dat_len;
}

void rim_add_prop_u32(struct RimTree *tree, enum RimPropType type, uint32_t val) {
	ensure_buffer_size(tree, sizeof(struct WidgetProp) + 8);
	struct WidgetProp *prop = rim_add_prop(tree, type);
	((uint32_t *)prop->data)[0] = val;
	((uint32_t *)prop->data)[1] = 0;
	prop->length += 8;
	tree->of += 8;
}

void rim_add_prop_data(struct RimTree *tree, enum RimPropType type, void *val, unsigned int length) {
	length = ((length / 8) + 1) * 8; // ensure align by 8
	ensure_buffer_size(tree, sizeof(struct WidgetProp) + length);
	struct WidgetProp *prop = rim_add_prop(tree, type);
	memcpy(prop->data, val, length);
	prop->length += length;
	tree->of += length;
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

int rim_get_prop_string(struct WidgetHeader *h, int type, char **val) {
	struct WidgetProp *p = rim_get_prop(h, type);
	if (p == NULL) return -1;
	(*val) = (char *)p->data;
	return 0;
}

int rim_get_prop_u32(struct WidgetHeader *h, int type, uint32_t *val) {
	struct WidgetProp *p = rim_get_prop(h, type);
	if (p == NULL) return -1;
	memcpy(val, p->data, 4);
	return 0;
}

int rim_mark_prop_fufilled(struct WidgetHeader *h, int type) {
	struct WidgetProp *p = rim_get_prop(h, type);
	if (p == NULL) return -1;
	p->already_fufilled = 1;
	return 0;
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
