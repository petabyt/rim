// runtime and virtual DOM implementation
#include <stdio.h>

#include <nim.h>
#include <nim_internal.h>

static int on_create_widget(struct NimContext *ctx, struct WidgetHeader *w) {
	printf("Creating a new widget\n");
	return 0;
}
static int on_free_widget(struct NimContext *ctx, struct WidgetHeader *w) {
	printf("Freeing a widget\n");
	return 0;
}
static int on_tweak_widget(struct NimContext *ctx, struct WidgetHeader *w) {
	printf("Tweaking a widget\n");
	return 0;
}
static int on_append_widget(struct NimContext *ctx, struct WidgetHeader *w, struct WidgetHeader *parent) {
	printf("Appending a widget to x\n");
	return 0;
}

static int build_ui(struct NimTree *tree, int state) {
	nim_add_widget(tree, NIM_WINDOW, -1);
	nim_add_prop_text(tree, NIM_PROP_WIN_TITLE, "Title");
		if (state) {
			nim_add_widget(tree, NIM_LAYOUT_DYNAMIC, 0);
				nim_add_widget(tree, NIM_BUTTON, 0);
				nim_add_prop_text(tree, NIM_PROP_TEXT, "widget will be removed");
				nim_end_widget(tree);
			nim_end_widget(tree);

		} else {
			nim_add_widget(tree, NIM_BUTTON, 0);
			nim_add_prop_text(tree, NIM_PROP_TEXT, "Hello World");
			nim_end_widget(tree);
		}
	nim_end_widget(tree);
	return 0;
}

int main(void) {
	struct NimContext ctx;
	ctx.create = on_create_widget;
	ctx.destroy = on_free_widget;
	ctx.tweak = on_tweak_widget;
	ctx.append = on_append_widget;
	ctx.tree_new = nim_create_tree();
	ctx.tree_old = nim_create_tree();
	build_ui(ctx.tree_old, 1);
	build_ui(ctx.tree_new, 0);
	nim_diff_tree(&ctx);

	return 0;
}
