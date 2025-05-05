// runtime and virtual DOM implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rim.h"
#include "rim_internal.h"

#define max(a, b) ((a) > (b) ? (a) : (b))

// Initializes a tree with no previous tree
unsigned int rim_init_tree_widgets(struct RimContext *ctx, struct RimTree *tree, unsigned int base, struct WidgetHeader *parent) {
	unsigned int of = 0;
	uint8_t *buffer = tree->buffer + base;

	if (tree->of < (int)sizeof(struct WidgetHeader)) abort();

	struct WidgetHeader *h = (struct WidgetHeader *)(buffer + of);
	of += sizeof(struct WidgetHeader);

	int rc = rim_widget_create(ctx, h);
	if (rc) {
		rim_abort("Couldn't create widget %s\n", rim_eval_widget_type(h->type));
	}

	if (h->os_handle == 0x0) {
		rim_abort("BUG: h->os_handle is null\n");
	}

	rc = rim_widget_append(ctx, h, parent);
	if (rc) {
		rim_abort("Couldn't append widget '%s' to '%s'\n", rim_eval_widget_type(h->type), rim_eval_widget_type(parent->type));
	}

	for (size_t i = 0; i < h->n_props; i++) {
		struct WidgetProp *p = (struct WidgetProp *)(buffer + of);
		// For most widgets this results in a double property set
		if (rim_widget_tweak(ctx, h, p, RIM_PROP_ADDED)) {
			rim_abort("Failed to change property %s %d\n", rim_eval_widget_type(h->type), p->type);
		}
		of += (int)p->length;
	}

	for (size_t i = 0; i < h->n_children; i++) {
		of += rim_init_tree_widgets(ctx, tree, base + of, h);
	}

	return of;
}

// Destroys a node and its children and removes them all from their parents
unsigned int rim_destroy_tree_widgets(struct RimContext *ctx, struct RimTree *tree, unsigned int base, struct WidgetHeader *parent) {
	unsigned int of = 0;
	uint8_t *buffer = tree->buffer + base;

	if (tree->of < (int)sizeof(struct WidgetHeader)) abort();

	struct WidgetHeader *h = (struct WidgetHeader *)(buffer + of);
	if (h->is_detached) {
		return rim_get_node_length(h);
	}
	of += sizeof(struct WidgetHeader);

	for (size_t i = 0; i < h->n_props; i++) {
		struct WidgetProp *p = (struct WidgetProp *)(buffer + of);
		of += (int)p->length;
	}

	for (size_t i = 0; i < h->n_children; i++) {
		of += rim_destroy_tree_widgets(ctx, tree, base + of, h);
	}

	if (h->is_detached) {
		rim_abort("BUG: Widget already detached from parent\n");
	}

	if (h->os_handle == 0x0) {
		rim_abort("BUG: Widget handle is NULL\n");
	}

	int rc = rim_widget_remove(ctx, h, parent);
	if (rc) {
		rim_abort("Couldn't remove widget\n");
	}

	h->is_detached = 1;

	rc = rim_widget_destroy(ctx, h);
	if (rc) {
		rim_abort("Couldn't destroy widget %s\n", rim_eval_widget_type(h->type));
	}

	//h->os_handle = 0;

	return of;
}

// TODO: Rename 'parent' to old_parent to be more clear
static int rim_patch_tree(struct RimContext *ctx, unsigned int *old_of_p, unsigned int *new_of_p, struct WidgetHeader *parent) {
	struct WidgetHeader *old_h = (struct WidgetHeader *)(ctx->tree_old->buffer + (*old_of_p));
	struct WidgetHeader *new_h = (struct WidgetHeader *)(ctx->tree_new->buffer + (*new_of_p));

	// TODO: Fix diff_tree instead of using this overflow hack
	if ((*old_of_p) >= ctx->tree_old->of) {
		return 0;
	}
	if ((*new_of_p) >= ctx->tree_new->of) {
		return 0;
	}

	if (ctx->tree_old->of == 0) {
		rim_abort("Should this function init the tree?\n");
	}

	if (ctx->tree_new->of == 0) {
		// Special handling for when the new tree has nothing, and the old tree didn't
		// TODO: This code isn't doing anything right now
		(*old_of_p) += rim_destroy_tree_widgets(ctx, ctx->tree_old, (*old_of_p), parent);
		return 0;
	}

	if (new_h->is_detached || old_h->is_detached) {
		// Check if this node has been detached by rim_last_widget_detach.
		// If so then this node is dead.
		(*new_of_p) += rim_get_node_length(new_h);
		(*old_of_p) += rim_get_node_length(old_h);
		return 0;
	}
	if (old_h->type != new_h->type || new_h->invalidate) {
		// Type has changed in this widget so we'll assume the rest of the tree is unusable.
		// So: Destroy widgets in old tree, init widgets in new tree.
		(*old_of_p) += rim_destroy_tree_widgets(ctx, ctx->tree_old, (*old_of_p), parent);
		(*new_of_p) += rim_init_tree_widgets(ctx, ctx->tree_new, (*new_of_p), parent);
		return 0;
	} else {
		// Same type, copy over handle
		new_h->os_handle = old_h->os_handle;
		// Need to update the unique ID since it's passed to the onclick handler
		if (new_h->unique_id != old_h->unique_id) {
			if (rim_backend_update_id(ctx, new_h)) {
				rim_abort("Failed to update widget's unique ID\n");
			}
		}
	}

	if (new_h->os_handle == 0x0) {
		rim_abort("BUG: new_h->os_handle is null '%s'\n", rim_eval_widget_type(new_h->type));
	}

	(*new_of_p) += sizeof(struct WidgetHeader);
	(*old_of_p) += sizeof(struct WidgetHeader);

	uint32_t max_n_props = max(old_h->n_props, new_h->n_props);
	for (size_t i = 0; i < max_n_props; i++) {
		struct WidgetProp *old_p = (struct WidgetProp *)(ctx->tree_old->buffer + (*old_of_p));
		struct WidgetProp *new_p = (struct WidgetProp *)(ctx->tree_new->buffer + (*new_of_p));

		if (i >= old_h->n_props) {
			if (rim_widget_tweak(ctx, new_h, new_p, RIM_PROP_ADDED)) {
				rim_abort("Failed to add property\n");
			}
			(*new_of_p) += (int)new_p->length;
			continue;
		} else if (i >= new_h->n_props) {
			if (rim_widget_tweak(ctx, new_h, old_p, RIM_PROP_REMOVED)) {
				rim_abort("Failed to remove property");
			}
			(*old_of_p) += (int)old_p->length;
			continue;
		} else {
			if (old_p->length == new_p->length && !memcmp(old_p, new_p, old_p->length)) {
				// Property has the same state
			} else if (old_p->type == new_p->type) {
				if (new_p->already_fufilled == 0) {
					if (rim_widget_tweak(ctx, new_h, new_p, RIM_PROP_CHANGED)) {
						rim_abort("Failed to change property %d on %s\n", new_p->type, rim_eval_widget_type(new_h->type));
					}
				}
			} else {
				if (rim_widget_tweak(ctx, new_h, old_p, RIM_PROP_REMOVED)) {
					rim_abort("Failed to remove property\n");
				}
				if (rim_widget_tweak(ctx, new_h, new_p, RIM_PROP_ADDED)) {
					rim_abort("Failed to add property\n");
				}
			}
		}

		(*old_of_p) += (int)old_p->length;
		(*new_of_p) += (int)new_p->length;
	}

	uint32_t max_n_child = max(old_h->n_children, new_h->n_children);
	int invalidate_following_siblings = 0;
	for (size_t i = 0; i < max_n_child; i++) {
		if (i >= old_h->n_children) {
			// Child added to tree
			(*new_of_p) += rim_init_tree_widgets(ctx, ctx->tree_new, (*new_of_p), new_h);
		} else if (i >= new_h->n_children) {
			// Child removed from tree
			(*old_of_p) += rim_destroy_tree_widgets(ctx, ctx->tree_old, (*old_of_p), old_h);
		} else {
			// Sad: Since the backend API currently isn't capable of inserting widgets into a parent at a specific index
			// (it can only append), the rest of the tree has to be thrown away if a single widget differs.
			struct WidgetHeader *old_child_h = (struct WidgetHeader *)(ctx->tree_old->buffer + (*old_of_p));
			struct WidgetHeader *new_child_h = (struct WidgetHeader *)(ctx->tree_new->buffer + (*new_of_p));
			if (old_child_h->type != new_child_h->type) {
				invalidate_following_siblings = 1;
			}

			if (invalidate_following_siblings) {
				new_child_h->invalidate = 1;
			}

			rim_patch_tree(ctx, old_of_p, new_of_p, old_h); // Recurse
		}
	}

	return 0;
}

int rim_diff_tree(struct RimContext *ctx) {
	unsigned int old_of = 0;
	unsigned int new_of = 0;
	return rim_patch_tree(ctx, &old_of, &new_of, NULL);
}

int rim_last_widget_event(int lookback) {
	struct RimContext *ctx = rim_get_global_ctx();
	pthread_mutex_lock(&ctx->event_mutex);
	if (ctx->last_event.is_valid) {
		if (ctx->tree_new->widget_stack_depth - lookback < 0) rim_abort("look back underflow");
		struct WidgetHeader *match = ctx->tree_new->widget_stack[ctx->tree_new->widget_stack_depth - lookback];
		if (match->unique_id == ctx->last_event.unique_id) {
			ctx->last_event.is_valid = 0;
			sem_post(&ctx->event_consumed_sig);
			pthread_mutex_unlock(&ctx->event_mutex);
			return ctx->last_event.type;
		}
	}
	pthread_mutex_unlock(&ctx->event_mutex);
	return 0;
}

int rim_last_widget_detach(int lookback) {
	struct RimContext *ctx = rim_get_global_ctx();
	struct WidgetHeader *match = ctx->tree_new->widget_stack[ctx->tree_new->widget_stack_depth - lookback];
	match->is_detached = 1;
	return 0;
}

void rim_on_widget_event(struct RimContext *ctx, enum RimWidgetEvent event, int unique_id) {
	// Wait for the previous event to be consumed
	if (ctx->last_event.is_valid) {
		// TODO: This does not work for more than two events
		sem_wait(&ctx->event_consumed_sig);
	}
	pthread_mutex_lock(&ctx->event_mutex);
	ctx->last_event.is_valid = 1;
	ctx->last_event.type = event;
	ctx->last_event.unique_id = unique_id;
	ctx->nop_event_counter = 2;
	pthread_mutex_unlock(&ctx->event_mutex);
	sem_post(&ctx->event_sig);
}

void rim_on_widget_event_data(struct RimContext *ctx, enum RimWidgetEvent event, int unique_id, const void *buffer, unsigned int length) {
	pthread_mutex_lock(&ctx->event_mutex);
	if (length > ctx->last_event.data_buf_size) {
		ctx->last_event.data = realloc(ctx->last_event.data, length + 100);
		if (ctx->last_event.data == NULL) abort();
		ctx->last_event.data_buf_size = length + 100;
	}
	ctx->last_event.data_length = length;
	memcpy(ctx->last_event.data, buffer, length);
	rim_on_widget_event(ctx, RIM_EVENT_VALUE_CHANGED, unique_id);
	pthread_mutex_unlock(&ctx->event_mutex);
}

void rim_trigger_event(void) {
	struct RimContext *ctx = rim_get_global_ctx();
	if (ctx->last_event.is_valid) {
		return; // No need to create a new event
	}
	pthread_mutex_lock(&ctx->event_mutex);
	ctx->last_event.is_valid = 1;
	ctx->last_event.type = RIM_EVENT_NONE;
	ctx->last_event.unique_id = 0;
	ctx->nop_event_counter = 0;
	pthread_mutex_unlock(&ctx->event_mutex);
	sem_post(&ctx->event_sig);
}

static void init_tree(void *priv) {
	struct RimContext *ctx = (struct RimContext *)priv;
	unsigned int base = 0;
	for (int i = 0; i < ctx->tree_new->n_root_children; i++) {
		base += rim_init_tree_widgets(ctx, ctx->tree_new, base, NULL);
	}
	sem_post(&ctx->run_done_signal);
}

static void diff_tree(void *priv) {
	struct RimContext *ctx = (struct RimContext *)priv;

	int max_root_children = max(ctx->tree_new->n_root_children, ctx->tree_old->n_root_children);

	unsigned int old_of = 0;
	unsigned int new_of = 0;
	int invalidate_following_siblings = 0;
	for (int i = 0; i < max_root_children; i++) {
		struct WidgetHeader *old_h = (struct WidgetHeader *)(ctx->tree_old->buffer + old_of);
		struct WidgetHeader *new_h = (struct WidgetHeader *)(ctx->tree_new->buffer + new_of);
		if (i >= ctx->tree_new->n_root_children) {
			// Window removed
			old_of += rim_destroy_tree_widgets(ctx, ctx->tree_old, old_of, NULL);
		} else if (i >= ctx->tree_old->n_root_children) {
			// Window added
			new_of += rim_init_tree_widgets(ctx, ctx->tree_new, new_of, NULL);
		} else {
			// Handling windows being removed doesn't work well with the fact that they've already been closed down.
			// TODO: Remove this hack
			if (old_h->unique_id != new_h->unique_id) {
				printf("TODO: Support for removing windows at root is not working yet\n");
				ctx->quit_immediately = 1;
				break;
			}
		
			int rc = rim_patch_tree(ctx, &old_of, &new_of, NULL);
			if (rc) rim_abort("differ failed\n");
		}
	}

	sem_post(&ctx->run_done_signal);
}

int rim_poll(rim_ctx_t *ctx) {
	if (ctx->last_event.is_valid) {
		rim_abort("Event not consumed by application\n");
	}
	if (ctx->quit_immediately) {
		rim_backend_close(ctx);
		return 0;
	}

	if (ctx->tree_old->of == 0 && ctx->tree_new->of != 0) {
		// If new tree has gained contents and old tree is empty, init the tree
		rim_backend_run(ctx, init_tree);
		sem_wait(&ctx->run_done_signal);
	} else if (ctx->tree_old->of != 0 && ctx->tree_new->of != 0) {
		// Only run differ if both trees have contents
		rim_backend_run(ctx, diff_tree);
		sem_wait(&ctx->run_done_signal);
	} else if (ctx->tree_old->of != 0 && ctx->tree_new->of == 0) {
		// If new tree suddenly has no contents, close everything down
		rim_backend_close(ctx);
		return 0;
	} else {
		printf("Empty frame\n");
	}

	if (ctx->nop_event_counter) {
		ctx->nop_event_counter--;
	} else {
		// Wait on external event
		sem_wait(&ctx->event_sig);
	}

	// Clear out external events
	if (ctx->last_event.is_valid && ctx->last_event.type == RIM_EVENT_NONE) {
		ctx->last_event.is_valid = 0;
	}

	// Switch trees, reuse old tree as new tree
	struct RimTree *temp = ctx->tree_old;
	ctx->tree_old = ctx->tree_new;
	ctx->tree_new = temp;
	rim_reset_tree(ctx->tree_new);

	return 1;
}
