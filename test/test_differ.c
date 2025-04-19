// runtime and virtual DOM implementation
#include <stdio.h>
#include <rim.h>
#include <rim_internal.h>

static int on_create_widget(struct RimContext *ctx, struct WidgetHeader *w) {
	printf("Creating a new widget '%s'\n", rim_eval_widget_type(w->type));
	return 0;
}
static int on_free_widget(struct RimContext *ctx, struct WidgetHeader *w) {
	printf("Freeing a widget '%s'\n", rim_eval_widget_type(w->type));
	return 0;
}
static int on_tweak_widget(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetProp *prop, enum RimPropTrigger type) {
	if (type == RIM_PROP_ADDED) {
		printf("Adding prop to widget '%s'\n", rim_eval_widget_type(w->type));
	} else if (type == RIM_PROP_CHANGED) {
		printf("Changing prop in '%s'\n", rim_eval_widget_type(w->type));
	} else if (type == RIM_PROP_REMOVED) {
		printf("Removing prop from '%s'\n", rim_eval_widget_type(w->type));
	}
	return 0;
}
static int on_append_widget(struct RimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent) {
	const char *parent_type = "(root)";
	if (parent != NULL) parent_type = rim_eval_widget_type(parent->type);
	printf("Appending widget '%s' to '%s'\n", rim_eval_widget_type(w->type), parent_type);
	return 0;
}

static int build_ui(struct RimTree *tree, int state) {
	rim_add_widget(tree, RIM_WINDOW, -1);
	rim_add_prop_text(tree, RIM_PROP_WIN_TITLE, "Title");
		if (state) {
			rim_add_widget(tree, RIM_LAYOUT_DYNAMIC, 0);
				rim_add_widget(tree, RIM_BUTTON, 0);
				rim_add_prop_text(tree, RIM_PROP_TEXT, "widget will be removed");
				rim_end_widget(tree);
			rim_end_widget(tree);
		} else {
			rim_add_widget(tree, RIM_BUTTON, 0);
			rim_add_prop_text(tree, RIM_PROP_TEXT, "Hello World");
			rim_end_widget(tree);
		}
	rim_end_widget(tree);
	return 0;
}

int main(void) {
	struct RimContext ctx;
	ctx.create = on_create_widget;
	ctx.destroy = on_free_widget;
	ctx.tweak = on_tweak_widget;
	ctx.append = on_append_widget;
	ctx.tree_new = rim_create_tree();
	ctx.tree_old = rim_create_tree();
	build_ui(ctx.tree_old, 1);
	build_ui(ctx.tree_new, 0);
	rim_init_tree_widgets(&ctx, ctx.tree_old, 0, NULL);
	rim_diff_tree(&ctx);

	return 0;
}
