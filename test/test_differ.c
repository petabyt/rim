// runtime and virtual DOM implementation
#include <stdio.h>
#include <assert.h>
#include <rim.h>
#include <im.h>
#include <rim_internal.h>

int rim_backend_create(struct RimContext *ctx, struct WidgetHeader *w) {
	printf("Creating a new widget '%s'\n", rim_eval_widget_type(w->type));
	w->os_handle = 1;
	return 0;
}
int rim_backend_destroy(struct RimContext *ctx, struct WidgetHeader *w) {
	printf("Freeing a widget '%s'\n", rim_eval_widget_type(w->type));
	assert(w->os_handle == 2);
	w->os_handle = 0;
	return 0;
}
int rim_backend_tweak(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetProp *prop, enum RimPropTrigger type) {
	assert(w->os_handle != 0);
	if (type == RIM_PROP_ADDED) {
		printf("Adding prop to widget '%s'\n", rim_eval_widget_type(w->type));
	} else if (type == RIM_PROP_CHANGED) {
		printf("Changing prop in '%s'\n", rim_eval_widget_type(w->type));
	} else if (type == RIM_PROP_REMOVED) {
		printf("Removing prop from '%s'\n", rim_eval_widget_type(w->type));
	}
	return 0;
}
int rim_backend_remove(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent) {
	assert(w->os_handle != 0);
	w->os_handle = 2; // indicates no parent
	int index = rim_get_child_index(w, parent);
	printf("Removing widget '%s' (index %d) from '%s'\n", rim_eval_widget_type(w->type), index, rim_eval_widget_type(parent->type));
	return 0;
}
int rim_backend_append(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent) {
	assert(w->os_handle != 0);
	const char *parent_type = "(root)";
	if (parent != NULL) parent_type = rim_eval_widget_type(parent->type);
	printf("Appending widget '%s' to '%s'\n", rim_eval_widget_type(w->type), parent_type);
	return 0;
}

int rim_backend_run(struct RimContext *ctx, rim_on_run_callback *callback) {
	return 0;
}

int rim_backend_init(struct RimContext *ctx) {
	return 0;
}

void rim_backend_close(struct RimContext *ctx) {}

int rim_backend_update_id(struct RimContext *ctx, struct WidgetHeader *w) {return 1;}

static void build_ui2(int counter) {
	if (im_begin_window("My Window", 500, 500)) {
		char buffer[64];
		sprintf(buffer, "Events: %04d\n", counter);
		im_label(buffer);
		if (counter & 1) {
			im_label("Hello, World");
		}
		if (im_button("Show More")) {}

		im_label("Two labels with the same text!");
		im_label("Two labels with the same text!");
		im_end_window();
	}
}

static void swap_trees(struct RimContext *ctx) {
	struct RimTree *temp = ctx->tree_old;
	ctx->tree_old = ctx->tree_new;
	ctx->tree_new = temp;
	rim_reset_tree(ctx->tree_new);
}

int main(void) {
	struct RimContext *ctx = rim_init();
	int state = 0;
	build_ui2(state);
	rim_init_tree_widgets(ctx, ctx->tree_new, 0, NULL);

	for (int i = 1; i < 3; i++) {
		state++;
		swap_trees(ctx);
		build_ui2(state);
		printf("--- diff %d ---\n", i);
		rim_diff_tree(ctx);
	}

	return 0;
}
