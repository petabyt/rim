// runtime and virtual DOM implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rim.h"
#include "rim_internal.h"

static struct RimContext *global_context = NULL;

int rim_generate_unique_id(struct RimContext *ctx) {
	return 0;
}

// Initializes a tree with no previous tree
int rim_init_tree_widgets(struct RimContext *ctx, struct RimTree *tree, int base, struct WidgetHeader *parent, int depth) {
	int of = 0;
	uint8_t *buffer = tree->buffer + base;

	if (tree->of < (int)sizeof(struct WidgetHeader))
		return 0;

	struct WidgetHeader *h = (struct WidgetHeader *)(buffer + of);
	of += sizeof(struct WidgetHeader);

	int rc = ctx->create(ctx, h);
	if (rc) {
		printf("Couldn't create widget %s\n", rim_eval_widget_type(h->type));
		abort();
	}

	rc = ctx->append(ctx, h, parent);
	if (rc) {
		printf("Couldn't append widget '%s' to '%s'\n", rim_eval_widget_type(h->type), rim_eval_widget_type(parent->type));
		abort();
	}

	for (size_t i = 0; i < h->n_props; i++) {
		struct WidgetProp *p = (struct WidgetProp *)(buffer + of);
		of += (int)p->length;
	}

	for (size_t i = 0; i < h->n_children; i++) {
		of += rim_init_tree_widgets(ctx, tree, base + of, h, depth + 1);
	}

	return of;
}

#define FLAG_DELETE (1 << 0)
#define FLAG_IGNORE (1 << 1)

static int rim_patch_tree(struct RimContext *ctx, int *old_of_p, int *new_of_p, int flag) {
	struct RimTree *tree_old = ctx->tree_old;
	struct RimTree *tree_new = ctx->tree_new;
	int old_of = *old_of_p;
	int new_of = *new_of_p;

	struct WidgetHeader *old_h = (struct WidgetHeader *)(tree_old->buffer + old_of);
	old_of += sizeof(struct WidgetHeader);
	struct WidgetHeader *new_h = (struct WidgetHeader *)(tree_new->buffer + new_of);
	new_of += sizeof(struct WidgetHeader);

	if (old_h->type != new_h->type) {
		// Type has changed in this widget so we'll assume the rest of the tree is unusable
		flag |= FLAG_DELETE;
	} else {
		// Same type, copy over handles
		new_h->unique_id = old_h->unique_id;
		new_h->os_handle = old_h->os_handle;
	}

	// TODO: Should there be a function to free the rest of a node
	// as well as initing the rest of a node?

	// If the type of the node has changed or something else that makes the node unusable, we need to
	// free the old tree, and init the new tree.

	uint32_t max_n_props = old_h->n_props;
	if (new_h->n_props > max_n_props)
		max_n_props = new_h->n_props;
	for (size_t i = 0; i < max_n_props; i++) {
		struct WidgetProp *old_p = (struct WidgetProp *)(tree_old->buffer + old_of);
		struct WidgetProp *new_p = (struct WidgetProp *)(tree_new->buffer + new_of);

		// TODO: Should compare prop types?

		if (flag & FLAG_DELETE) {
			// If deleting tree, we don't care about properties (unless if there's something that needs to be freed)			
		} else {
			if (i >= old_h->n_props) {
				ctx->tweak(ctx, new_h, new_p, NIM_PROP_ADDED);
				new_of += (int)new_p->length;
				continue;
			} else if (i >= new_h->n_props) {
				// This is a naive way of doing this because properties could not be ordered in the same
				// way as in the last tree, leading to a the wrong property being removed.
				ctx->tweak(ctx, new_h, old_p, NIM_PROP_REMOVED);
				old_of += (int)old_p->length;
				continue;
			}

			if (old_p->length == new_p->length && !memcmp(old_p, new_p, old_p->length)) {
				// Property has the same state
			} else {
				ctx->tweak(ctx, new_h, new_p, NIM_PROP_CHANGED);
			}
		}

		old_of += (int)old_p->length;
		new_of += (int)new_p->length;
	}

	// TODO this is broken, need to:
	// diff():
	//   if child added: ctx->create, append to parent
	//   if child removed: delete, remove from parent

	uint32_t max_n_child = old_h->n_children;
	if (new_h->n_children > max_n_child)
		max_n_child = new_h->n_children;
	(*old_of_p) = old_of;
	(*new_of_p) = new_of;
	for (size_t i = 0; i < max_n_child; i++) {
		if (i >= old_h->n_children) {
			if (!(flag & FLAG_IGNORE)) {
				printf("child added to tree\n");
				backend->append
				//				printf("Unimplemented"); abort();
				// backend->append(backend->priv, new_h, )
			}
		} else if (i >= new_h->n_children) {
			if (!(flag & FLAG_IGNORE)) {
				printf("child removed\n");
				//				printf("Unimplemented\n"); abort();
			}
		} else {
			rim_patch_tree(ctx, old_of_p, new_of_p, flag);
		}
	}

	if (flag & FLAG_DELETE) {
		ctx->destroy(ctx->priv, old_h);
	}

	return 0;
}

int rim_diff_tree(struct RimContext *ctx) {
	int old_of = 0;
	int new_of = 0;
	return rim_patch_tree(ctx, &old_of, &new_of, 0);
}

struct RimTree *rim_get_current_tree(void) {
	if (global_context == NULL) {
		rim_abort("global_context null");
	}
	return global_context->tree_new;
}
rim_ctx_t *rim_get_global_ctx(void) {
	return global_context;
}
int rim_last_widget_event(void) {
	return 0;
}

void rim_on_widget_event(struct RimContext *ctx, enum RimWidgetEvent event, int unique_id) {
	printf("TODO: Do something to give state loop event info\n");
	sem_post(&ctx->event_sig);
}

struct RimContext *rim_init(void) {
	struct RimContext *ctx = (struct RimContext *)calloc(1, sizeof(struct RimContext));
	ctx->header = 0;
	ctx->event_counter = 1;

	ctx->tree_new = rim_create_tree();
	ctx->tree_old = rim_create_tree();

	sem_init(&ctx->event_sig, 0, 0);

	global_context = ctx;

	return ctx;
}

static void work_tree(void *priv) {
	struct RimContext *ctx = (struct RimContext *)priv;
	printf("Initializing the tree for the first time\n");
	rim_init_tree_widgets(ctx, ctx->tree_new, 0, NULL, 0);
}

static void diff_tree(void *priv) {
	struct RimContext *ctx = (struct RimContext *)priv;
	printf("Diffing tree\n");
	rim_diff_tree(ctx);
}

int rim_poll(rim_ctx_t *ctx) {
	// If new tree has gained contents and old tree is empty, init the tree
	if (ctx->tree_old->of == 0 && ctx->tree_new->of != 0) {
		ctx->run(ctx, work_tree);
	} else {
		ctx->run(ctx, diff_tree);
		printf("Run tree differ now\n");
	}

	if (ctx->event_counter) {
		ctx->event_counter--;
	} else {
		// sem_post(&ctx->event_sig);
		sem_wait(&ctx->event_sig);
	}

	// Switch trees, reuse old tree as new tree
	struct RimTree *temp = ctx->tree_old;
	ctx->tree_old = ctx->tree_new;
	ctx->tree_new = temp;
	rim_reset_tree(ctx->tree_new);

	return 1;
}
