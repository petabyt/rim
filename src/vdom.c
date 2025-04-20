// runtime and virtual DOM implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rim.h"
#include "rim_internal.h"

static struct RimContext *global_context = NULL;

int rim_get_dpi(void) {
	return 96;
}

// Initializes a tree with no previous tree
int rim_init_tree_widgets(struct RimContext *ctx, struct RimTree *tree, int base, struct WidgetHeader *parent) {
	int of = 0;
	uint8_t *buffer = tree->buffer + base;

	if (tree->of < (int)sizeof(struct WidgetHeader)) abort();

	struct WidgetHeader *h = (struct WidgetHeader *)(buffer + of);
	of += sizeof(struct WidgetHeader);

	int rc = ctx->create(ctx, h);
	if (rc) {
		rim_abort("Couldn't create widget %s\n", rim_eval_widget_type(h->type));
	}

	rc = ctx->append(ctx, h, parent);
	if (rc) {
		rim_abort("Couldn't append widget '%s' to '%s'\n", rim_eval_widget_type(h->type), rim_eval_widget_type(parent->type));
	}

	for (size_t i = 0; i < h->n_props; i++) {
		struct WidgetProp *p = (struct WidgetProp *)(buffer + of);
		of += (int)p->length;
	}

	for (size_t i = 0; i < h->n_children; i++) {
		of += rim_init_tree_widgets(ctx, tree, base + of, h);
	}

	return of;
}

int rim_destroy_tree_widgets(struct RimContext *ctx, struct RimTree *tree, int base, struct WidgetHeader *parent) {
	int of = 0;
	uint8_t *buffer = tree->buffer + base;

	if (tree->of < (int)sizeof(struct WidgetHeader)) abort();

	struct WidgetHeader *h = (struct WidgetHeader *)(buffer + of);
	of += sizeof(struct WidgetHeader);

	for (size_t i = 0; i < h->n_props; i++) {
		struct WidgetProp *p = (struct WidgetProp *)(buffer + of);
		of += (int)p->length;
	}

	for (size_t i = 0; i < h->n_children; i++) {
		of += rim_destroy_tree_widgets(ctx, tree, base + of, parent);
	}

	int rc = ctx->remove(ctx, h, parent);
	if (rc) {
		rim_abort("Couldn't destroy widget\n");
	}

	h->is_detached = 1;

	rc = ctx->destroy(ctx, h);
	if (rc) {
		rim_abort("Couldn't destroy widget %s\n", rim_eval_widget_type(h->type));
	}

	return of;
}

#define max(a, b) ((a) > (b) ? (a) : (b))

static int rim_patch_tree(struct RimContext *ctx, int *old_of_p, int *new_of_p, struct WidgetHeader *parent) {
	struct WidgetHeader *old_h = (struct WidgetHeader *)(ctx->tree_old->buffer + (*old_of_p));
	struct WidgetHeader *new_h = (struct WidgetHeader *)(ctx->tree_new->buffer + (*new_of_p));

	// Special handling for when the new tree has nothing, and the old tree didn't
	if (ctx->tree_new->of == 0) {
		if (ctx->tree_old->of != 0) {
			(*old_of_p) += rim_destroy_tree_widgets(ctx, ctx->tree_old, (*old_of_p), parent);
		}
		return 0;
	}
	if (ctx->tree_old->of == 0) {
		rim_abort("Should this function init the tree?\n");
	}

	if (old_h->type != new_h->type) {
		// Type has changed in this widget so we'll assume the rest of the tree is unusable.
		// So: Destroy widgets in old tree, init widgets in new tree.
		(*old_of_p) += rim_destroy_tree_widgets(ctx, ctx->tree_old, (*old_of_p), parent);
		(*new_of_p) += rim_init_tree_widgets(ctx, ctx->tree_new, (*new_of_p), parent);
		return 0;
	} else {
		// Same type, copy over handles
		new_h->unique_id = old_h->unique_id;
		new_h->os_handle = old_h->os_handle;
	}

	(*new_of_p) += sizeof(struct WidgetHeader);
	(*old_of_p) += sizeof(struct WidgetHeader);

	uint32_t max_n_props = max(old_h->n_props, new_h->n_props);
	for (size_t i = 0; i < max_n_props; i++) {
		struct WidgetProp *old_p = (struct WidgetProp *)(ctx->tree_old->buffer + (*old_of_p));
		struct WidgetProp *new_p = (struct WidgetProp *)(ctx->tree_new->buffer + (*new_of_p));

		// TODO: Rewrite and test the types of both properties
		if (i >= old_h->n_props) {
			ctx->tweak(ctx, new_h, new_p, RIM_PROP_ADDED);
			(*new_of_p) += (int)new_p->length;
			continue;
		} else if (i >= new_h->n_props) {
			// This is a naive way of doing this because properties could not be ordered in the same
			// way as in the last tree, leading to a the wrong property being removed.
			ctx->tweak(ctx, new_h, old_p, RIM_PROP_ADDED);
			(*old_of_p) += (int)old_p->length;
			continue;
		}

		if (old_p->length == new_p->length && !memcmp(old_p, new_p, old_p->length)) {
			// Property has the same state
		} else {
			ctx->tweak(ctx, new_h, new_p, RIM_PROP_CHANGED);
		}

		(*old_of_p) += (int)old_p->length;
		(*new_of_p) += (int)new_p->length;
	}

	uint32_t max_n_child = max(old_h->n_children, new_h->n_children);

	for (size_t i = 0; i < max_n_child; i++) {
		if (i >= old_h->n_children) {
			// Child added to tree
			(*new_of_p) += rim_init_tree_widgets(ctx, ctx->tree_new, (*new_of_p), new_h);
		} else if (i >= new_h->n_children) {
			// Child removed from tree
			(*old_of_p) += rim_destroy_tree_widgets(ctx, ctx->tree_old, (*old_of_p), old_h);
		} else {
			rim_patch_tree(ctx, old_of_p, new_of_p, new_h);
		}
	}

	return 0;
}

int rim_diff_tree(struct RimContext *ctx) {
	int old_of = 0;
	int new_of = 0;
	return rim_patch_tree(ctx, &old_of, &new_of, NULL);
}
rim_ctx_t *rim_get_global_ctx(void) {
	if (global_context == NULL) {
		rim_abort("global_context is NULL\n");
	}
	return global_context;
}
struct RimTree *rim_get_current_tree(void) {
	return rim_get_global_ctx()->tree_new;
}
int rim_last_widget_event(void) {
	struct RimContext *ctx = rim_get_global_ctx();
	if (ctx->event_counter) {
		// Checking the last created widget for the unique ID is a very naive way of doing this
		struct WidgetHeader *match = ctx->tree_new->widget_stack[ctx->tree_new->widget_stack_depth];
		if (match->unique_id == ctx->last_event.unique_id) {
			ctx->event_counter--;
			return ctx->last_event.type;
		}
	}
	return 0;
}

void rim_on_widget_event(struct RimContext *ctx, enum RimWidgetEvent event, int unique_id) {
	ctx->last_event.type = event;
	ctx->last_event.unique_id = unique_id;
	ctx->event_counter += 2;
	sem_post(&ctx->event_sig);
}

struct RimContext *rim_init(void) {
	struct RimContext *ctx = (struct RimContext *)calloc(1, sizeof(struct RimContext));
	ctx->header = 0;
	ctx->event_counter = 1; // 1 event for rim_poll to be called twice at beginning

	ctx->tree_new = rim_create_tree();
	ctx->tree_old = rim_create_tree();

	sem_init(&ctx->event_sig, 0, 0);

	global_context = ctx;

	return ctx;
}

static void work_tree(void *priv) {
	struct RimContext *ctx = (struct RimContext *)priv;
	printf("Initializing the tree for the first time\n");
	rim_init_tree_widgets(ctx, ctx->tree_new, 0, NULL);
	sem_post(&ctx->event_sig);
}

static void diff_tree(void *priv) {
	struct RimContext *ctx = (struct RimContext *)priv;
	printf("Diffing tree\n");
	rim_diff_tree(ctx);
	sem_post(&ctx->event_sig);
}

int rim_poll(rim_ctx_t *ctx) {
	if (ctx->tree_old->of == 0 && ctx->tree_new->of != 0) {
		// If new tree has gained contents and old tree is empty, init the tree
		ctx->run(ctx, work_tree);
		sem_wait(&ctx->event_sig);
	} else if (ctx->tree_old->of != 0 && ctx->tree_new->of != 0) {
		// Only run differ if both trees have contents
		ctx->run(ctx, diff_tree);
		sem_wait(&ctx->event_sig);
	}

	if (ctx->event_counter) {
		ctx->event_counter--;
	} else {
		// Wait on external event
		sem_wait(&ctx->event_sig);
		// For now just close on any window event
		if (ctx->last_event.type == RIM_EVENT_WINDOW_CLOSE) {
			return 0;
		}
	}

	// Switch trees, reuse old tree as new tree
	struct RimTree *temp = ctx->tree_old;
	ctx->tree_old = ctx->tree_new;
	ctx->tree_new = temp;
	rim_reset_tree(ctx->tree_new);

	return 1;
}
