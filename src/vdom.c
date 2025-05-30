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

	if (!(rim_widget_get_rules(ctx, h, parent) & RIM_FLAG_INIT_CHILDREN_FIRST)) {
		rc = rim_widget_append(ctx, h, parent);
		if (rc) {
			rim_abort("Couldn't append widget '%s' to '%s'\n", rim_eval_widget_type(h->type), rim_eval_widget_type(parent->type));
		}
	}

	int has_set_after_children = 0;
	int prop_of = of;
	for (size_t i = 0; i < h->n_props; i++) {
		struct PropHeader *p = (struct PropHeader *)(buffer + of);
		of += (int)p->length;
		if (rim_prop_get_rules(ctx, h, p) & RIM_PROP_NUMBER_VALUE) {
			has_set_after_children = 1;
		}
		if (p->already_fulfilled == 0) {
			if (rim_widget_tweak(ctx, h, p, RIM_PROP_ADDED)) {
				printf("Failed to change property\n");
			}
		}
	}

	for (size_t i = 0; i < h->n_children; i++) {
		of += rim_init_tree_widgets(ctx, tree, base + of, h);
	}

	if (has_set_after_children) {
		for (size_t i = 0; i < h->n_props; i++) {
			struct PropHeader *p = (struct PropHeader *)(buffer + prop_of);
			prop_of += (int)p->length;
			if ((rim_prop_get_rules(ctx, h, p) & RIM_PROP_NUMBER_VALUE) && p->already_fulfilled == 0) {
				if (rim_widget_tweak(ctx, h, p, RIM_PROP_ADDED)) {
					printf("Failed to change property\n");
				}
			}
		}
	}

	if (rim_widget_get_rules(ctx, h, parent) & RIM_FLAG_INIT_CHILDREN_FIRST) {
		rc = rim_widget_append(ctx, h, parent);
		if (rc) {
			rim_abort("Couldn't append widget '%s' to '%s'\n", rim_eval_widget_type(h->type), rim_eval_widget_type(parent->type));
		}
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
		struct PropHeader *p = (struct PropHeader *)(buffer + of);
		of += (int)p->length;
	}

	for (size_t i = 0; i < h->n_children; i++) {
		of += rim_destroy_tree_widgets(ctx, tree, base + of, h);
	}

	if (h->os_handle == 0x0) {
		rim_abort("BUG: Widget handle is NULL\n");
	}

	int rc = rim_widget_remove(ctx, h, parent);
	if (rc) {
		rim_abort("Couldn't remove widget '%s' from '%s'\n", rim_eval_widget_type(h->type), rim_eval_widget_type(parent->type));
	}

	h->is_detached = 1;

	rc = rim_widget_destroy(ctx, h);
	if (rc) {
		rim_abort("Couldn't destroy widget %s\n", rim_eval_widget_type(h->type));
	}

	return of;
}

int rim_patch_props(struct RimContext *ctx, struct WidgetHeader *old_h, struct WidgetHeader *new_h, unsigned int *old_of_p, unsigned int *new_of_p, int set_after_children) {
	uint32_t max_n_props = max(old_h->n_props, new_h->n_props);
	int has_set_after_children = 1;
	for (size_t i = 0; i < max_n_props; i++) {
		struct PropHeader *old_p = (struct PropHeader *)(ctx->tree_old->buffer + (*old_of_p));
		struct PropHeader *new_p = (struct PropHeader *)(ctx->tree_new->buffer + (*new_of_p));

		(*new_of_p) += (int)new_p->length;
		(*old_of_p) += (int)old_p->length;

		if ((rim_prop_get_rules(ctx, new_h, new_p) & RIM_PROP_NUMBER_VALUE) && set_after_children == 0) {
			has_set_after_children = 1;
			continue;
		}

		if (i >= old_h->n_props) {
			if (rim_widget_tweak(ctx, new_h, new_p, RIM_PROP_ADDED)) {
				printf("Failed to add property\n");
			}
			continue;
		} else if (i >= new_h->n_props) {
			if (rim_widget_tweak(ctx, new_h, old_p, RIM_PROP_REMOVED)) {
				printf("Failed to remove property\n");
			}
			continue;
		} else {
			if (old_p->length == new_p->length && !memcmp(old_p->data, new_p->data, old_p->length - sizeof(struct PropHeader))) {
				// Same state
			} else if (old_p->type == new_p->type) {
				// If the old property 'last_changed_by' event ID still associates with the current event ID,
				// Then we don't need to fulfill it. Sync it up and it won't be updated again.
				if (old_p->last_changed_by == ctx->current_event_id) {
					new_p->already_fulfilled = 1;
				}
				if (new_p->already_fulfilled == 0) {
					if (rim_widget_tweak(ctx, new_h, new_p, RIM_PROP_CHANGED)) {
						printf("Failed to change property\n");
					}
				}
			} else {
				if (rim_widget_tweak(ctx, new_h, old_p, RIM_PROP_REMOVED)) {
					printf("Failed to remove property\n");
				}
				if (new_p->already_fulfilled == 0) {
					if (rim_widget_tweak(ctx, new_h, new_p, RIM_PROP_ADDED)) {
						printf("Failed to add property\n");
					}
				}
			}
		}
	}

	return has_set_after_children;
}

int rim_patch_tree(struct RimContext *ctx, unsigned int *old_of_p, unsigned int *new_of_p, struct WidgetHeader *old_parent) {
	struct WidgetHeader *old_h = (struct WidgetHeader *)(ctx->tree_old->buffer + (*old_of_p));
	struct WidgetHeader *new_h = (struct WidgetHeader *)(ctx->tree_new->buffer + (*new_of_p));

	// TODO: Fix diff_tree instead of using this overflow hack
	if ((*old_of_p) >= ctx->tree_old->of) {
		printf("TODO\n");
		return 0;
	}
	if ((*new_of_p) >= ctx->tree_new->of) {
		printf("TODO\n");
		return 0;
	}

	if (ctx->tree_old->of == 0) {
		rim_abort("BUG: Tree patcher doesn't init trees\n");
	}

	if (ctx->tree_new->of == 0) {
		rim_abort("BUG: Tree patcher doesn't tear down trees\n");
	}

	if (new_h->is_detached && !old_h->is_detached) {
		// Check if this node has been detached by rim_last_widget_detach.
		// If so then this node is dead.
		(*new_of_p) += rim_get_node_length(new_h); // TODO: Maybe use rim_destroy_tree_widgets, does the same thing
		(*old_of_p) += rim_get_node_length(old_h);
		return 0;
	}
	if (old_h->is_detached && !new_h->is_detached) {
		// Prevent differ from assuming previously detached widget can be re-used
		(*new_of_p) += rim_init_tree_widgets(ctx, ctx->tree_new, (*new_of_p), old_parent);
		(*old_of_p) += rim_get_node_length(old_h);
		return 0;
	}
	if (old_h->type != new_h->type || new_h->invalidate) {
		// Type has changed in this widget so we'll assume the rest of the tree is unusable.
		// So: Destroy widgets in old tree, init widgets in new tree.
		(*old_of_p) += rim_destroy_tree_widgets(ctx, ctx->tree_old, (*old_of_p), old_parent);
		(*new_of_p) += rim_init_tree_widgets(ctx, ctx->tree_new, (*new_of_p), old_parent);
		return 0;
	} else {
		// Same type, copy over handle
		new_h->os_handle = old_h->os_handle;
		// Need to update the unique ID since it was passed to the onclick handler
		if (new_h->unique_id != old_h->unique_id) {
			if (rim_widget_update_onclick(ctx, new_h)) {
				rim_abort("Failed to update onclick on '%s'\n", rim_eval_widget_type(new_h->type));
			}
		}
	}

	if (new_h->os_handle == 0x0) {
		rim_dump_tree(ctx->tree_new);
		rim_abort("BUG: new_h->os_handle is null '%s'\n", rim_eval_widget_type(new_h->type));
	}

	(*new_of_p) += sizeof(struct WidgetHeader);
	(*old_of_p) += sizeof(struct WidgetHeader);

	unsigned int props_old_of_p = (*old_of_p);
	unsigned int props_new_of_p = (*new_of_p);
	int has_set_after_children = rim_patch_props(ctx, old_h, new_h, old_of_p, new_of_p, 0);

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
			// Sad: Since the backend API currently isn't capable of inserting widgets into an old_parent at a specific index
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

	// Don't run prop patcher again unless necessary
	if (has_set_after_children) {
		rim_patch_props(ctx, old_h, new_h, &props_old_of_p, &props_new_of_p, 1);
	}

	return 0;
}

int rim_diff_tree(struct RimContext *ctx) {
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
			int rc = rim_patch_tree(ctx, &old_of, &new_of, NULL);
			if (rc) rim_abort("differ failed\n");
		}
	}

	return 0;
}

int rim_init_tree(struct RimContext *ctx) {
	unsigned int base = 0;
	for (unsigned int i = 0; i < ctx->tree_new->n_root_children; i++) {
		base += rim_init_tree_widgets(ctx, ctx->tree_new, base, NULL);
	}
	return 0;
}

static void fulfill_matching_event_prop(struct RimContext *ctx, struct WidgetHeader *w) {
	if (ctx->last_event.affected_property != RIM_PROP_NONE) {
		struct PropHeader *p = rim_get_prop(w, ctx->last_event.affected_property);
		if (p == NULL) return;
		p->already_fulfilled = 1;
		p->last_changed_by = ctx->current_event_id;
	}
}

int rim_last_widget_event(int lookback) {
	struct RimContext *ctx = rim_get_global_ctx();
	pthread_mutex_lock(&ctx->event_mutex);
	if (ctx->last_event.is_valid) {
		if (ctx->tree_new->widget_stack_depth - lookback < 0) rim_abort("look back underflow\n");
		struct WidgetHeader *match = (struct WidgetHeader *)(ctx->tree_new->buffer + ctx->tree_new->widget_stack[ctx->tree_new->widget_stack_depth - lookback]);
		if (match->unique_id == ctx->last_event.unique_id) {
			fulfill_matching_event_prop(ctx, match);
			ctx->last_event.is_valid = 0;
			sem_post(ctx->event_consumed_signal);
			int evtype = ctx->last_event.type;
			pthread_mutex_unlock(&ctx->event_mutex);
			return evtype;
		}
	}
	pthread_mutex_unlock(&ctx->event_mutex);
	return 0;
}

int rim_last_widget_detach(int lookback) {
	struct RimContext *ctx = rim_get_global_ctx();
	struct WidgetHeader *match = (struct WidgetHeader *)(ctx->tree_new->buffer + ctx->tree_new->widget_stack[ctx->tree_new->widget_stack_depth - lookback]);
	match->is_detached = 1;
	return 0;
}

static void wait_event_consumed(struct RimContext *ctx) {
	// Wait for the previous event to be consumed
	if (ctx->last_event.is_valid) {
		// This does not work for more than two events. I don't care right now because retained-mode UI is single-threaded
		// and shouldn't be trying to run multiple callbacks at a time.
		sem_wait(ctx->event_consumed_signal);
	}
}

static void on_event(struct RimContext *ctx, enum RimWidgetEvent event, int unique_id) {
	ctx->last_event.is_valid = 1;
	ctx->last_event.type = event;
	ctx->last_event.unique_id = unique_id;
	ctx->nop_event_counter = 2;	
}

void rim_on_widget_event(struct RimContext *ctx, enum RimWidgetEvent event, int unique_id) {
	wait_event_consumed(ctx);
	pthread_mutex_lock(&ctx->event_mutex);
	ctx->last_event.affected_property = RIM_PROP_NONE;
	on_event(ctx, event, unique_id);
	pthread_mutex_unlock(&ctx->event_mutex);
	sem_post(ctx->event_signal);
}

void rim_on_widget_event_data(struct RimContext *ctx, enum RimWidgetEvent event, enum RimPropType type, int unique_id, const void *buffer, unsigned int length) {
	wait_event_consumed(ctx);
	pthread_mutex_lock(&ctx->event_mutex);
	if (length > ctx->last_event.data_buf_size) {
		ctx->last_event.data = realloc(ctx->last_event.data, length + 100);
		if (ctx->last_event.data == NULL) abort();
		ctx->last_event.data_buf_size = length + 100;
	}
	ctx->last_event.data_length = length;
	memcpy(ctx->last_event.data, buffer, length);
	ctx->last_event.affected_property = type;
	on_event(ctx, event, unique_id);
	pthread_mutex_unlock(&ctx->event_mutex);
	sem_post(ctx->event_signal);
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
	sem_post(ctx->event_signal);
}

static void init_tree(void *priv) {
	struct RimContext *ctx = (struct RimContext *)priv;
	rim_init_tree(ctx);
	sem_post(ctx->backend_done_signal);
}

static void diff_tree(void *priv) {
	struct RimContext *ctx = (struct RimContext *)priv;

	rim_diff_tree(ctx);

	sem_post(ctx->backend_done_signal);
}

int rim_poll(rim_ctx_t *ctx) {
	if (ctx->last_event.is_valid) {
		rim_abort("Event not consumed by tree builder\n");
	}
	if (ctx->quit_immediately) {
		return 0;
	}

	if (ctx->tree_old->of == 0 && ctx->tree_new->of != 0) {
		// If new tree has gained contents and old tree is empty, init the tree
		rim_backend_run(ctx, init_tree, ctx);
		sem_wait(ctx->backend_done_signal);
	} else if (ctx->tree_old->of != 0 && ctx->tree_new->of != 0) {
		// Only run differ if both trees have contents
		rim_backend_run(ctx, diff_tree, ctx);
		sem_wait(ctx->backend_done_signal);
	} else if (ctx->tree_old->of != 0 && ctx->tree_new->of == 0) {
		// If new tree suddenly has no contents, exit
		// Code that is run after this should close everything down.
		return 0;
	}

	if (ctx->nop_event_counter) {
		ctx->nop_event_counter--;
	} else {
		// Wait on external event
		sem_wait(ctx->event_signal);
		ctx->current_event_id++;
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
