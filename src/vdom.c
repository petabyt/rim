// runtime and virtual DOM implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rim.h"
#include "rim_internal.h"

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

	if (h->os_handle == 0x0) {
		rim_abort("BUG: h->os_handle is null %x\n");
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

	h->os_handle = 0;

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
			if (ctx->tweak(ctx, new_h, new_p, RIM_PROP_ADDED)) {
				rim_abort("Failed to add property\n");
			}
			(*new_of_p) += (int)new_p->length;
			continue;
		} else if (i >= new_h->n_props) {
			if (ctx->tweak(ctx, new_h, old_p, RIM_PROP_REMOVED)) {
				rim_abort("Failed to remove property");
			}
			(*old_of_p) += (int)old_p->length;
			continue;
		} else {
			if (old_p->length == new_p->length && !memcmp(old_p, new_p, old_p->length)) {
				// Property has the same state
			} else {
				if (old_p->type == new_p->type) {
					if (ctx->tweak(ctx, new_h, new_p, RIM_PROP_CHANGED)) {
						rim_abort("Failed to change property\n");
					}
				} else {
					if (ctx->tweak(ctx, new_h, new_p, RIM_PROP_ADDED)) {
						rim_abort("Failed to add property\n");
					}
				}
			}
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

int rim_last_widget_event(void) {
	struct RimContext *ctx = rim_get_global_ctx();
	pthread_mutex_lock(&ctx->event_mutex);
	if (ctx->last_event.is_valid) {
		// Checking the last created widget for the unique ID is probably not the best way to check
		struct WidgetHeader *match = ctx->tree_new->widget_stack[ctx->tree_new->widget_stack_depth];
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

void rim_on_widget_event(struct RimContext *ctx, enum RimWidgetEvent event, int unique_id) {
	// Wait for the previous event to be consumed
	// TODO: This does not work for multiple events
	if (ctx->last_event.is_valid) {
		sem_wait(&ctx->event_consumed_sig);
	}
	pthread_mutex_lock(&ctx->event_mutex);
	ctx->last_event.is_valid = 1;
	ctx->last_event.type = event;
	ctx->last_event.unique_id = unique_id;
	ctx->event_counter = 1;
	pthread_mutex_unlock(&ctx->event_mutex);
	sem_post(&ctx->event_sig);
}

void rim_on_widget_event_data(struct RimContext *ctx, enum RimWidgetEvent event, int unique_id, const void *buffer, unsigned int length) {
	pthread_mutex_lock(&ctx->event_mutex);
	if (length > ctx->last_event.data_buf_size) {
		ctx->last_event.data = realloc(ctx->last_event.data, length + 100);
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
	ctx->event_counter = 0;
	pthread_mutex_unlock(&ctx->event_mutex);
	sem_post(&ctx->event_sig);
}

static void work_tree(void *priv) {
	struct RimContext *ctx = (struct RimContext *)priv;
	rim_init_tree_widgets(ctx, ctx->tree_new, 0, NULL);
	sem_post(&ctx->run_done_signal);
}

static void diff_tree(void *priv) {
	struct RimContext *ctx = (struct RimContext *)priv;
	rim_diff_tree(ctx);
	sem_post(&ctx->run_done_signal);
}

int rim_poll(rim_ctx_t *ctx) {
	if (ctx->last_event.is_valid) {
		rim_abort("Event ignored\n");
	}

	//pthread_mutex_unlock(&ctx->event_mutex);

	void usleep(unsigned int us);
	if (ctx->tree_old->of == 0 && ctx->tree_new->of != 0) {
		// If new tree has gained contents and old tree is empty, init the tree
		ctx->run(ctx, work_tree);
		sem_wait(&ctx->run_done_signal);
	} else if (ctx->tree_old->of != 0 && ctx->tree_new->of != 0) {
		// Only run differ if both trees have contents
		ctx->run(ctx, diff_tree);
		sem_wait(&ctx->run_done_signal);
	} else {
		printf("Empty frame\n");
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

	// Clear out external events
	if (ctx->last_event.is_valid && ctx->last_event.type == RIM_EVENT_NONE) {
		ctx->last_event.is_valid = 0;
	}

	// Switch trees, reuse old tree as new tree
	//pthread_mutex_lock(&ctx->event_mutex);
	struct RimTree *temp = ctx->tree_old;
	ctx->tree_old = ctx->tree_new;
	ctx->tree_new = temp;
	rim_reset_tree(ctx->tree_new);

	return 1;
}
